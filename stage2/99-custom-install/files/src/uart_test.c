#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h> 
#include <termios.h>
#include <string.h>

int main()
{
    // Open the UART device (UART0 on Pi 5 GPIO)
    int uart_fd = open("/dev/ttyAMA4", O_RDWR | O_NOCTTY | O_NDELAY);
    if (uart_fd == -1) {
        perror("Error opening UART");
	printf("test failure!!\n");
        return 1;
    }

    // Configure UART settings
    struct termios options;
    tcgetattr(uart_fd, &options);
    cfsetispeed(&options, B115200);  // Input baud rate
    cfsetospeed(&options, B115200);  // Output baud rate
    options.c_cflag = (options.c_cflag & ~CSIZE) | CS8;  // 8 data bits
    options.c_cflag &= ~PARENB;  // No parity
    options.c_cflag &= ~CSTOPB;  // 1 stop bit
    options.c_cflag |= CLOCAL | CREAD;  // Enable receiver, ignore modem lines
    options.c_iflag = IGNPAR;  // Ignore parity errors
    options.c_oflag = 0;
    options.c_lflag = 0;
    tcflush(uart_fd, TCIFLUSH);
    if (tcsetattr(uart_fd, TCSANOW, &options) != 0) {
        perror("Error configuring UART");
        close(uart_fd);
	printf("test failure!!\n");
        return 1;
    }

#define ReadMCUVersion
    // Transmit example data
#ifndef ReadMCUVersion
    const char *tx_data = "ph";
#else
    const char *tx_data = "vh";
#endif
    int loop_times = 0, tx_bytes = write(uart_fd, tx_data, strlen(tx_data));
    if (tx_bytes < 0) {
        perror("UART TX error");
	printf("test failure!!\n");
    } else {
        printf("Sent %d bytes: %s\n", tx_bytes, tx_data);
    }

    // Receive loop (non-blocking, reads up to 256 bytes if available)
    unsigned char rx_buffer[256];
    while (loop_times < 100) {
        int rx_bytes = read(uart_fd, rx_buffer, sizeof(rx_buffer) - 1);
        if (rx_bytes > 2) {
            printf("Received %d bytes: %s\n", rx_bytes, rx_buffer);
#ifndef ReadMCUVersion
	    if (rx_buffer[0] == 'm' && rx_buffer[1] == 'h') break;
#else
	    if (rx_buffer[0] == 'v' && rx_buffer[1] == 'n') break;
#endif
        } else if (rx_bytes < 0) {
	    if (errno != EAGAIN && errno != EWOULDBLOCK) {
		perror("UART RX error");
		loop_times = -1;
		break;
	    }
        }
        usleep(100000);  // Sleep 100ms to avoid busy-waiting
	loop_times++;
    }

    if (-1 < loop_times && loop_times < 3) printf("received %c%c%c%c%c!!\ntest success!!\n", rx_buffer[0], rx_buffer[1], rx_buffer[2], rx_buffer[3], rx_buffer[4]);
    else printf("test failure!!\n");
    // Cleanup
    close(uart_fd);
    return 0;
}
