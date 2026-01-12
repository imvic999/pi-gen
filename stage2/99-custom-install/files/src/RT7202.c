#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <string.h>

#define I2C_BUS "/dev/i2c-1"   // Raspberry Pi 5 上的 I²C bus (通常是 i2c-1)
#define RT7202_ADDR 0x1B       // RT7202 I2C 地址

#define FWCodeVersion (uint8_t)0x00
#define ProtocolMode  (uint8_t)0x02

uint8_t crc8(uint8_t init_value, uint8_t* data, size_t len);

int read_register2(int fd, uint8_t reg_addr, uint8_t len, uint8_t *value)
{
    uint8_t buf[10];

    buf[0] = reg_addr;
    buf[1] = 0;
    buf[2] = crc8(0x00, buf, 2);
    if (write(fd, buf, 3) != 3) {
        printf("%s: Failed to write register address=0x%x\n", __func__, reg_addr);
        return -1;
    }

    buf[0] = buf[1] = buf[2] =0x0;
    if (read(fd, value, len + 1) != (len + 1)) {
        printf("%s: Failed to read register value of address=0x%x\n", __func__, reg_addr);
        return -1;
    }
int i;
for (i = 0; i < len + 1; i++) printf("value[%d]=0x%x\n", i, value[i]);

    uint8_t crc = crc8(0x00, value, len);
printf("%s%d: crc=0x%x, value[%d]=0x%x\n", __FILE__, __LINE__, crc, len, value[len]);
    if (crc != value[len]) {
        printf("%s%d: crc=0x%x, value[%d]=0x%x\n", crc, len, value[len]);
    }

    return 0;
}

int main(int argc, char* argv[])
{
    int      i, fd, n;
    uint8_t  addr, adc_data_h, adc_data_l;

    // 開啟 I2C 裝置
    if ((fd = open(I2C_BUS, O_RDWR)) < 0) {
        perror("Failed to open i2c bus");
        exit(1);
    }

    // 設定 I2C slave address
    if (ioctl(fd, I2C_SLAVE, RT7202_ADDR) < 0) {
        perror("Failed to acquire bus access and/or talk to slave");
        exit(1);
    }

    uint8_t fw_code[9];
    if (read_register2(fd, FWCodeVersion, (uint8_t)8, fw_code) == 0) {
        ;
    }

    uint8_t received_data[3] = { 0, 0, 0 };
    if (read_register2(fd, ProtocolMode, (uint8_t)2, received_data) == 0) {
	uint16_t data = received_data[0]  | received_data[1] << 8;
        if (data == 0) printf("No Protocol\n");
	else if (data == 1) printf("Type C\n");
	else if (data == 2) printf("PD\n");
	else if (data == 3) printf("QC 2.0\n");
	else if (data == 4 || data == 5) printf("Reserv\n");
	else if (data == 6) printf("DCP\n");
	else printf("Unknown protocol data=0x%x\n", data);
    }

    close(fd);
    return 0;
}
