#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>

#define BT_UART "/dev/ttyHSL0"  // UART Bluetooth Interface

#define HCI_CMD_HDR_SIZE 3
#define HCI_EVENT_HDR_SIZE 2
#define HCI_ACL_HDR_SIZE 4
#define L2CAP_HDR_SIZE 4

#define HCI_CREATE_CONN_OPCODE 0x0405
#define HCI_EVENT_CONN_COMPLETE 0x03
#define HCI_ACL_DATA_PACKET 0x02

#define L2CAP_CID_SIGNALING 0x0001
#define L2CAP_ECHO_REQ 0x08
#define L2CAP_ECHO_RESP 0x09

#define ECHO_PAYLOAD_SIZE 600  // Echo request payload size

uint8_t target_bdaddr[6];

/**
 * Convert a Bluetooth address string "BC:9A:78:56:34:12" into uint8_t[6].
 */
int parse_bdaddr(const char *str, uint8_t *bdaddr) {
    if (sscanf(str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
               &bdaddr[5], &bdaddr[4], &bdaddr[3], &bdaddr[2], &bdaddr[1], &bdaddr[0]) != 6) {
        fprintf(stderr, "Invalid Bluetooth address format!\n");
        return -1;
    }
    return 0;
}

// Open and configure UART
int open_uart() {
    int fd = open(BT_UART, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        perror("Failed to open Bluetooth UART device");
        exit(EXIT_FAILURE);
    }

    fcntl(fd, F_SETFL, O_NONBLOCK);  // Set the file descriptor to non-blocking mode

    struct termios tty;
    memset(&tty, 0, sizeof(tty));
    if (tcgetattr(fd, &tty) != 0) {
        perror("Error getting termios attributes");
        exit(EXIT_FAILURE);
    }

    cfsetospeed(&tty, B115200);
    cfsetispeed(&tty, B115200);

    tty.c_cflag = (CLOCAL | CREAD | CS8);
    tty.c_iflag = 0;
    tty.c_oflag = 0;
    tty.c_lflag = 0;

    tcflush(fd, TCIFLUSH);
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        perror("Error setting termios attributes");
        exit(EXIT_FAILURE);
    }

    return fd;
}

int send_hci_cmd(int fd, uint16_t opcode, uint8_t *params, uint8_t param_len) {        
    uint8_t packet[HCI_CMD_HDR_SIZE + param_len];    
    
    packet[0] = opcode & 0xFF;
    packet[1] = (opcode >> 8) & 0xFF;
    packet[2] = param_len;
    memcpy(&packet[3], params, param_len);
    
    return write(fd, packet, sizeof(packet)) < 0 ? -1 : 0;
}

int read_hci_event(int fd, uint8_t *buf, int size, int timeout_ms) {
    int len;
    clock_t start = clock();

    while (1) {
        len = read(fd, buf, size);        
        if (len > 0) return len;

        if (((clock() - start) * 1000 / CLOCKS_PER_SEC) >= timeout_ms) return -1;
        usleep(1000);  // Avoid busy looping
    }
}

int send_l2cap_echo(int fd, uint16_t handle) {
    uint8_t packet[HCI_ACL_HDR_SIZE + L2CAP_HDR_SIZE + ECHO_PAYLOAD_SIZE];
    uint16_t pb_bc_flag = 0x2000;
    uint16_t acl_handle = handle | pb_bc_flag;
    uint16_t l2cap_length = ECHO_PAYLOAD_SIZE;

    packet[0] = acl_handle & 0xFF;
    packet[1] = (acl_handle >> 8) & 0xFF;
    packet[2] = (l2cap_length + L2CAP_HDR_SIZE) & 0xFF;
    packet[3] = ((l2cap_length + L2CAP_HDR_SIZE) >> 8) & 0xFF;

    packet[4] = l2cap_length & 0xFF;
    packet[5] = (l2cap_length >> 8) & 0xFF;
    packet[6] = L2CAP_CID_SIGNALING & 0xFF;
    packet[7] = (L2CAP_CID_SIGNALING >> 8) & 0xFF;

    packet[8] = L2CAP_ECHO_REQ;
    packet[9] = 0x01;
    packet[10] = ECHO_PAYLOAD_SIZE & 0xFF;
    packet[11] = (ECHO_PAYLOAD_SIZE >> 8) & 0xFF;

    memset(&packet[12], 0xAA, ECHO_PAYLOAD_SIZE);  // Fill payload with 0xAA

    return write(fd, packet, sizeof(packet)) < 0 ? -1 : 0;
}

int wait_l2cap_echo_response(int fd, int timeout_ms) {
    uint8_t buf[256];

    while (1) {
        int len = read_hci_event(fd, buf, sizeof(buf), timeout_ms);
        if (len < 0) return -1;

        if (buf[1] == HCI_ACL_DATA_PACKET && buf[6] == L2CAP_ECHO_RESP) {
            printf("Received L2CAP Echo Response!\n");
            return 0;
        }
    }

    return -1;
}

int establish_connection(int fd) {
    uint8_t params[13] = {
        target_bdaddr[0], target_bdaddr[1], target_bdaddr[2],
        target_bdaddr[3], target_bdaddr[4], target_bdaddr[5],
        0x18, 0xCC, 0x01, 0x00, 0x00, 0x00, 0x01
    };

    while (1) {
        printf("Attempting to establish connection ...\n");

        if (send_hci_cmd(fd, HCI_CREATE_CONN_OPCODE, params, sizeof(params)) < 0) {
            printf("Failed to send connection request. Retrying...\n");
            usleep(2000000);  // 2 seconds delay
            continue;
        }
        
        uint8_t buf[256];
        uint16_t handle = 0;        
        time_t start = time(NULL);
                
        while ((time(NULL) - start) < 5) {  // 5-second timeout
            int len = read_hci_event(fd, buf, sizeof(buf), 3000);
            if (len < 0) continue;
            
            if (buf[1] == HCI_EVENT_CONN_COMPLETE) {
                handle = buf[3] | (buf[4] << 8);
                printf("Connection established! Handle: 0x%04X\n", handle);
                return handle;
            }
        }

        printf("Connection timeout. Retrying...\n");
        usleep(1000000);
    }
}

void l2cap_echo_loop(int fd, uint16_t handle) {
    while (1) {
        printf("Sending L2CAP Echo Request...\n");

        if (send_l2cap_echo(fd, handle) < 0) {
            printf("Failed to send L2CAP Echo Request. Retrying in 1s...\n");
            usleep(1000000);
            continue;
        }

        if (wait_l2cap_echo_response(fd, 1000) < 0) {
            printf("No response received. Retrying...\n");
            usleep(1000000);
            continue;
        }

        printf("L2CAP Echo successful. Sending another in 1s...\n");
        usleep(1000000);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <BD_ADDR>\n", argv[0]);
        return 1;
    }

    if (parse_bdaddr(argv[1], target_bdaddr) < 0) return 1;

    int fd = open_uart();
    if (fd < 0) return 1;

    uint16_t handle = establish_connection(fd);
    l2cap_echo_loop(fd, handle);

    close(fd);
    return 0;
}
