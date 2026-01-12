#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdbool.h>

#define I2C_BUS "/dev/i2c-1"   // Raspberry Pi 5 上的 I²C bus (通常是 i2c-1)
#define BQ27Z746_ADDR 0x55     // BQ27Z746 I2C 地址: 7-bit, 8-bit: 0xAA or 0xAB for write or read

// 常用暫存器位址 (Standard Commands)
#define REG_VOLT 0x08 // Voltage (2 bytes)
#define REG_SOC  0x2c // RelativeStateOfCharge (2 bytes)
#define REG_CAP  0x10 // RemainingCapacity (2 bytes)

int read_register3(int fd, uint8_t reg_addr, uint8_t len, uint8_t *value)
{
    if (write(fd, &reg_addr, 1) != 1) {
        printf("%s: Failed to write register address=0x%x\n", __func__, reg_addr);
        return -1;
    }

    if (read(fd, value, len) != len) {
        printf("%s: Failed to read register value of address=0x%x\n", __func__, reg_addr);
        return -1;
    }
int i;
for (i = 0; i < len + 1; i++) printf("value[%d]=0x%x\n", i, value[i]);

    return 0;
}


int main(int argc, char* argv[])
{
    int fd;

    // 開啟 I2C 裝置
    if ((fd = open(I2C_BUS, O_RDWR)) < 0) {
        perror("Failed to open i2c bus");
        exit(1);
    }

    // 設定 I2C slave address
    if (ioctl(fd, I2C_SLAVE, BQ27Z746_ADDR) < 0) {
        perror("Failed to acquire bus access and/or talk to slave");
        exit(1);
    }

    uint8_t  reg, data[2];
    uint16_t voltage, soc, capacity;

    while (true)
    {
	// --- 讀取電壓 (0x08) ---
	reg = REG_VOLT;
	if (read_register3(fd, reg, 2, data) == 0) {
	    voltage = (data[1] << 8) | data[0]; // 讀取 2 位元組 (Little-endian)
	    printf("電壓: %d mV\n", voltage);
        } else {
	    printf("read voltage failure!!\n");
	}

	// --- 讀取電量百分比 (0x2C) ---
	reg = REG_SOC;
	if (read_register3(fd, reg, 2, data) == 0) {
	    soc = (data[1] << 8) | data[0];
	    printf("電量: %d %%\n", data);
	} else {
	    printf("read soc failure!!\n");
	}

	// --- 讀取剩餘容量 (0x10) ---
	reg = REG_CAP;
	if (read_register3(fd, reg, 2, data) == 0) {
	    capacity = (data[1] << 8) | data[0];
	    printf("剩餘容量: %d mAh\n");
	} else {
	    printf("read capacity failure!!\n");
	}

	sleep(2); // 每兩秒讀取一次
    }

    close(fd);
    return 0;
}
