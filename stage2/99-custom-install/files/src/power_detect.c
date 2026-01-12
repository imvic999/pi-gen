#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <stdint.h>

#define I2C_BUS     "/dev/i2c-1"
uint8_t crc8(uint8_t init_value, uint8_t* data, size_t len);

////////// RT9466 //////////
#define RT9467_ADDR 0x53  // RT9467 I2C address

#define CHG_CTRL2    0x02
#define Control7_reg 0x07

#define VBAT       0x40 //0100
#define IBAT       0x90 //1001
#define TEMP_JC    0xc0 //1100
#define ADC_START  0x01//0001

#define CHG_STAT   0x42
#define ADC_DATA_H 0x44
#define ADC_DATA_L 0x45
#define CHG_FAULT  0x51

////////// RT7292 //////////
#define RT7202_ADDR 0x1B       // RT7202 I2C 地址

typedef struct __RT9466__ {
    bool firstTime, lowVoltage, ledBlinking, inFastCharge;
    int  notifyMCUPowerOff;
    int  vBAT_limit, vBAT_Poweroff_limit, valueIBAT;
    int  interval;
    int  fd_i2c, fd_uart;
} RT9466;
uint32_t i2c_failured_count = 0;
uint32_t loop_count = 0;
time_t t_s = 0, t_c = 0;
bool startByService = false;
bool debugMode = false;

#define NC         "\033[0m"
#define GreenColor "\033[1;32m"
#define RedColor   "\033[31m"
#define CyanColor  "\033[36m"

////////// RT9466 //////////
int read_register(int fd, uint8_t reg_addr, uint8_t *value)
{
    int bytes_rw;

    bytes_rw = write(fd, &reg_addr, 1);
    if (bytes_rw != 1) {
        if (!startByService) printf("%s%s: Failed to write register address=0x%x, bytes_written=%d%s\n", RedColor, __func__, reg_addr, bytes_rw, NC);
	if (errno == ENXIO) {
	    if (!startByService) printf("- Error ENXIO: No such device or address. (Did you check i2cdetect?)\n");
	} else if (errno == EIO) {
	    if (!startByService) printf("- Error EIO: I/O error. (Might be a NACK or bus error)\n");
	} else {
	    if (!startByService) printf("- Check your hardware connections or power supply.\n");
	}
i2c_failured_count++;
        return -1;
    }

    //usleep(300);
    bytes_rw = read(fd, value, 1);
    if (bytes_rw != 1) {
        if (!startByService) printf("%s%s: Failed to read register value of address=0x%x, bytes_read=%d%s\n", RedColor, __func__, reg_addr, bytes_rw, NC);
	if (bytes_rw == -1) {
	     if (!startByService) printf("Common causes: Check power, connections, or if the slave is responsive.\n");
	     if (errno == EIO) {
		 if (!startByService) printf("- Error EIO: I/O error. This is often a NACK received during the read phase.\n");
	     }
	 }
i2c_failured_count++;
        return -1;
    }

    return 0;
}

int write_register(int fd, uint8_t reg_addr, uint8_t value)
{
    uint8_t buf[2];

    buf[0] = reg_addr;
    buf[1] = value;

    if (write(fd, buf, 2) != 2) {
        if (!startByService) printf("%s%s: Failed to write register address=0x%x, value=0x%x%s\n", RedColor, __func__, reg_addr, value, NC);
i2c_failured_count++;
        return -1;
    }

    return 0;
}

int read_adc_register(int fd, uint8_t getType, uint8_t* adc_data_h, uint8_t* adc_data_l)
{
    uint8_t value;

    value = getType | ADC_START;
    if (write_register(fd, 0x11, value) != 0) return 1;

    usleep(500000);

    if (read_register(fd, 0x55, &value) != 0) {//check ADC_DONEI (0x55, bit0) = 1 
        if (!startByService) printf("%sread ADC_DONEI failured!!%s\n", RedColor, NC);
        return -1;
    } else if ((value & 0x01) != 1) {
        if (read_register(fd, 0x42, &value) != 0) {//check ADC_STAT (0x42, bit0) = 0
	    if (!startByService) printf("%sread ADC_STAT failured!!%s\n", RedColor, NC);
	    return -1;
        } else if ((value & 0x01) != 0) {
            if (!startByService) printf("%sADC_DONEI (0x55, bit0) = 0 and ADC_STAT (0x42, bit0) = 1%s\n", RedColor, NC);
	    return -1;
        }
    }

    if (read_register(fd, ADC_DATA_H, adc_data_h) != 0) {
        if (!startByService) printf("%sread ADC_DATA_H failure!!%s\n", RedColor, NC);
	return -1;
    }

    if (read_register(fd, ADC_DATA_L, adc_data_l) != 0) {
        if (!startByService) printf("%sread ADC_DATA_H failure!!%s\n", RedColor, NC);
	return -1;
    }

    return 0;
}

int getVBAT(RT9466 *rt9466)
{
    uint8_t adc_data_h, adc_data_l;
    int     n = -1;

    if (read_adc_register(rt9466->fd_i2c, VBAT, &adc_data_h, &adc_data_l) == 0) {
        n = (adc_data_h * 256 + adc_data_l) * 5;
	rt9466->lowVoltage = (n < rt9466->vBAT_limit) ? true : false;
        if (n < rt9466->vBAT_Poweroff_limit) rt9466->notifyMCUPowerOff = 1;
	time_t t = time(NULL);
	if (t_s == 0) t_s = t;
	t_c = t;
	if (!startByService || (loop_count%30) == 0) printf("VBAT=0x%02x%02x : %dmV - %lu(%lu)%s, ledBlinking=%d\n", adc_data_h, adc_data_l, n, t, (t_c - t_s), (rt9466->lowVoltage ? " low voltage" : ""), rt9466->ledBlinking);
    } else {
	if (!startByService) printf("%sgetVBAT failed!!%s\n", RedColor, NC);
    }

    return n;
}

float getIBAT(RT9466 *rt9466)
{
    uint8_t ichg, adc_data_h, adc_data_l;
    float   r = -1.0;

    if (read_register(rt9466->fd_i2c, Control7_reg, &ichg) == 0 &&
	read_adc_register(rt9466->fd_i2c, IBAT, &adc_data_h, &adc_data_l) == 0) {
	r = (float)((adc_data_h * 256 + adc_data_l) * 50);
	rt9466->valueIBAT =  r;

	ichg =  ichg >> 2;
	//printf("n=%d, ichg=0x%x\t", (int)r, ichg);

	if (ichg < 4) r = r * 0.57; //ICHG[5:0] setting < 450mA
	else if (ichg <8) r = r * 0.63; //ICHG[5:0] setting 500mA ~ 850mA
	//else //ICHG[5:0] setting >= 900mA
	 
	if (!startByService) printf("IBAT=0x%02x%02x  : %.3fmA\n", adc_data_h, adc_data_l, r);
    }

    return r;
}

int getTEMP_JC(RT9466* rt9466)
{
    uint8_t adc_data_h, adc_data_l;
    int     n = -1;

    if (read_adc_register(rt9466->fd_i2c, TEMP_JC, &adc_data_h, &adc_data_l) == 0) {
	n = (adc_data_h * 256 + adc_data_l) * 2 - 40;
	if (!startByService) printf("TEMP_JC=0x%02x%02x : %d°C\n", adc_data_h, adc_data_l, n);
    }

    return n;
}

int getCHG_STAT(RT9466 *rt9466)
{
    uint8_t value;

    if (read_register(rt9466->fd_i2c, CHG_STAT, &value) == 0)
    {
	uint8_t s;

	if (!startByService) printf("CHG_STAT(0x42) : 0x%02x\n", value);
	if (!startByService) printf("\tCharger status: ");
	s = (value & 0xc0) >> 6;
	if (s == 0) if (!startByService) printf("Ready\n");
	else if (s == 1) if (!startByService) printf("Charge in progress\n");
	else if (s == 2) if (!startByService) printf("Charge done\n");
	else if (!startByService) printf("Fault\n");

	s = (value & 0x20) >> 5;
        rt9466->inFastCharge = (s ? true : false);
	if (!startByService) printf("Battery voltage level for operation mode: Charger operate in %s\n", (s) ? "fast-charge level" : "pre-charge");

	s = (value & 0x10) >> 4;
	if (!startByService) printf("\tBattery voltage level for operation mode: Charger %s in trickle level\n",
	       (s) ? "operates" : "does not operate");

	s = (value & 0x08) >> 3;
	if (!startByService) printf("\tBoost mode status: %s in boost mode\n", (s) ? "" : "Does not");

	s = (value & 0x04) >> 2;
	if (!startByService) printf("\tBoost mode VBUS OVP status: %s\n", (s) ? "occurs" : "does not occur");

	s = (value & 0x01);
	if (!startByService) printf("\tADC status: is %s\n", (s) ? "under conversion" : "idle");
	
	return value;
    } else if (!startByService) printf("get CHG_STAT failure!!\n");

    return -1;
}

int getCHG_CTRL8(RT9466 *rt9466)
{
    uint8_t value;

    if (read_register(rt9466->fd_i2c, 0x08, &value) == 0)
    {
	printf("CHG_CTRL8(0x08) : VOREG=0x%02x\n", value);
	printf("\tVPREC: 0b%04b\n", (value & 0xf0) >> 4);
	printf("\tIPREC: 0b%04b\n", (value & 0x0f));
    } else printf("get CHG_CTRL8 failure!!\n");

    return -1;
}

int getCHG_CTRL4(RT9466 *rt9466)
{
    uint8_t value;

    if (read_register(rt9466->fd_i2c, 0x08, &value) == 0)
    {
	printf("CHG_CTRL4(0x04) : VOREG=0x%02x\n", value);
    } else printf("get CHG_CTRL4 failure!!\n");

    return -1;
}

int getCHG_FAULT(RT9466 *rt9466)
{
    uint8_t value;

    if (read_register(rt9466->fd_i2c, CHG_FAULT, &value) == 0)
    {
	if (!startByService) printf("CHG_FAULT(0x51) : 0x%02x\n", value);
        if (!startByService) printf("\tVBUS %s OVP\n", (value & 0x80) ? "is" : "is not");
        if (!startByService) printf("\tBaterry %s OVP\n", (value & 0x40) ? "is" : "is not");
        if (!startByService) printf("\tSystem %s OVP\n", (value & 0x20) ? "is" : "is not");
        if (!startByService) printf("\tSystem %s UVP\n", (value & 0x10) ? "is" : "is not");

	return value;
    }

    return -1;
}

int getCHG_NTC(RT9466 *rt9466)
{
    uint8_t value;

    if (read_register(rt9466->fd_i2c, 0x43, &value) == 0)
    {
	if (!startByService) printf("CHG_NTC(BAT NTC fault status)(0x43): 0x%02x\n", value);
	value = (value & 0x70) >> 4;
	if (value == 0) if (!startByService) printf("\t000: Normal\n");
	else if (value == 2) if (!startByService) printf("\t010: Warm\n");
	else if (value == 3) if (!startByService) printf("\t011: Cool\n");
	else if (value == 5) if (!startByService) printf("\t101: Cold\n");
	else if (value == 6) if (!startByService) printf("\t110: Hot\n");

	return value;
    }

    return -1;
}

int getTS_STATC(RT9466 *rt9466)
{
    uint8_t value;

    if (read_register(rt9466->fd_i2c, 0x52, &value) == 0)
    {
	if (!startByService) printf("CHG_TS_NTC(0x52): 0x%02x\n", value);
	if (!startByService) printf("\tTS_BAT_HOT %s\n", (value & 0x80) ? "Temperature is hot" : "Normal temperature");
	if (!startByService) printf("\tTS_BAT_WARM %s\n", (value & 0x40) ? "Temperature is warm" : "Normal termperature");
	if (!startByService) printf("\tTS_BAT_COOL %s\n", (value & 0x20) ? "Temperature is cool" : "Normal termperature");
	printf("\tTS_BAT_COLD %s\n", (value & 0x10) ? "Temperature is cold" : "Normal termperature");

	return value;
    }

    return -1;
}

int getCHG_STATC(RT9466 *rt9466)
{
    uint8_t value;

    if (read_register(rt9466->fd_i2c, 0x50, &value) == 0)
    {
	if (!startByService) printf("CHG_STATC(0x50): 0x%02x\n", value);
	if (!startByService) printf("\tPWR_RDY %s\n", (value & 0x80) ? "Input power is good" : "Input power is bad");
	if (!startByService) printf("\tCHG_MIVR %s\n", (value & 0x40) ? "MIVR loop is active" : "MIVR loop is not active");
	if (!startByService) printf("\tCHG_AICR %s\n", (value & 0x20) ? "AICR loop is active" : "AICR is not active");
	if (!startByService) printf("\tCHG_TREG Thermal regulation loop is%s active\n", (value & 0x10) ? "" : " not");

	return value;
    }

    return -1;
}

////////// RT7202 //////////
#define DD printf("%s%d\n", __FILE__, __LINE__);
int read_register2(int fd, uint8_t reg_addr, uint8_t len, uint8_t *value)
{
    uint8_t buf[10];

    buf[0] = reg_addr;
    buf[1] = 0;
    buf[2] = crc8(0x00, buf, 2);
    if (debugMode) {
	printf("write: 0x%x 0x%x 0x%x\n", buf[0], buf[1], buf[2]);
    }

    if (write(fd, buf, 3) != 3) {
        printf("%s: Failed to write register address=0x%x\n", __func__, reg_addr);
        return -1;
    }


    buf[0] = buf[1] = buf[2] =0x0;
    if (read(fd, value, len + 1) != (len + 1)) {
        printf("%s: Failed to read register value of address=0x%x\n", __func__, reg_addr);
        return -1;
    }

    if (debugMode)
    {
	int i;
	printf("read reg_addr=0x%x\n", reg_addr);
	for (i = 0; i < len + 1; i++) printf("value[%d]=0x%x\n", i, value[i]);
    }

    uint8_t crc = crc8(0x00, value, len);
    if (debugMode) {
	printf("%s%d: crc=0x%x, value[%d]=0x%x\n", __FILE__, __LINE__, crc, len, value[len]);
}

    if (crc != value[len]) {
        printf("%s%d: crc=0x%x, value[%d]=0x%x\n", __FILE__, __LINE__, crc, len, value[len]);
    }

    return 0;
}

int write_register2(int fd, uint8_t reg_addr, uint8_t len, uint8_t *value)
{
    uint8_t buf[10];
    int i;

    buf[0] = reg_addr;
    buf[1] = len;
    for (i = 0; i < len; i++)
        buf[2 + i] = value[i];

    buf[2 + i] = crc8(0x00, buf, len + 2);

    if (debugMode) {
	printf("write(len=%d):", len);
	for (i = 0; i < (len + 3); i++)
	    printf(" 0x%x", buf[i]);
	printf("\n");
    }

    if (write(fd, buf, len + 3) != (len + 3)) {
        printf("%s: Failed to write register address=0x%x\n", __func__, reg_addr);
        return -1;
    }

    return 0;
}

int get_profile(int fd)
{
    printf("\nget Profile Selection(0x50):\n");
    uint8_t received_data[2];
    int data;
    if (read_register2(fd, 0x50, (uint8_t)2, received_data) == 0) {
        data = received_data[0]  | received_data[1] << 8;
printf("received_data[0]=0x%x, received_data[1]=0x%x, data=0x%x\n", received_data[0], received_data[1], data);
        printf("\tProfile Selection (Object Position): ");
        if (data == 0) printf("N/A\n");
        else if (data == 1) printf("1st PD profile/QC 5V/TypeC\n");
        else if (data == 2) printf("2nd PD profile/QC 9V\n");
        else if (data == 3) printf("3rd PD profile/QC 12V\n");
        else if (data == 4) printf("4th PD profile\n");
        else if (data == 5) printf("5th PD profile\n");
        else if (data == 6) printf("6th PD profile\n");
        else if (data == 7) printf("7th PD profile\n");
    } else {
        printf("%s: Failed to read register address=0x%x\n", __func__, 0x50);
        return -1;
    }

    return data;
}

int set_profile(int fd, uint8_t profile_selection)
{
    printf("\nset Profile Selection(0x50) for Write:\n");
    uint8_t data[2];
    data[0] = profile_selection; data[1] = 0;
    if (write_register2(fd, 0x50, (uint8_t)2, data) == 0) {
	printf("%swrite profile_selection=0x%x(%s) to 0x50 success!!%s\n", GreenColor, profile_selection, (profile_selection == 2 ? "9V" : "5V"), NC);
    } else {
        printf("%s: Failed to read register address=0x%x\n", __func__, 0x50);
        return -1;
    }

    return 0;
}

#define SleepInterval 1000
int check_profile(RT9466 *rt9466)
{
    int profile;

    profile = get_profile(rt9466->fd_i2c);
    usleep(SleepInterval);

    if (rt9466->inFastCharge) {
        if (rt9466->valueIBAT >= 1000) {//set profile to 9V
	    if (profile !=  2) {
		set_profile(rt9466->fd_i2c, (uint8_t)2);
		usleep(SleepInterval);
		get_profile(rt9466->fd_i2c);
	    }
        } else {
	    if (profile !=  1) {
		set_profile(rt9466->fd_i2c, (uint8_t)1);
		usleep(SleepInterval);
		get_profile(rt9466->fd_i2c);
	    }
        }
#if 0
    } else if (profile != 1) {//set profile to 5V
	set_profile(rt9466->fd_i2c, (uint8_t)1);
	get_profile(rt9466->fd_i2c);
#endif
    }
}

void* check_VBAT(void* arg)
{
    RT9466 *rt9466 = (RT9466*)arg;

    if (rt9466->interval < 0) rt9466->interval = 60;

    while (true) {
	if (!startByService) printf("%si2c_failured_count=%d, loop_count=%d%s\n", CyanColor, i2c_failured_count, loop_count++, NC);

        if (ioctl(rt9466->fd_i2c, I2C_SLAVE, RT9467_ADDR) < 0) {
	    perror("Failed to acquire bus access and/or talk to slave");
	    exit(1);
        }
	getVBAT(rt9466);
	usleep(SleepInterval);
	getIBAT(rt9466);
	usleep(SleepInterval);
	getTEMP_JC(rt9466);
	usleep(SleepInterval);
	getCHG_STAT(rt9466);
	usleep(SleepInterval);
	getCHG_CTRL8(rt9466);
	usleep(SleepInterval);
	getCHG_CTRL4(rt9466);
	usleep(SleepInterval);
	getCHG_FAULT(rt9466);
	usleep(SleepInterval);
	getCHG_NTC(rt9466);
	usleep(SleepInterval);
	getTS_STATC(rt9466);
	usleep(SleepInterval);
	getCHG_STATC(rt9466);
	usleep(SleepInterval);
uint8_t value;
	if (read_register(rt9466->fd_i2c, 0x02, &value) == 0) {
	    if (!startByService) printf("register 0x02 : 0x%x\n", value);
	} else {
	    if (!startByService) printf("read_register 0x02 failed\n");
	}
      
        sleep(1);
	if (ioctl(rt9466->fd_i2c, I2C_SLAVE, RT7202_ADDR) < 0) {
	    perror("Failed to acquire bus access and/or talk to slave");
	    exit(1);
	}
	usleep(SleepInterval);
	check_profile(rt9466);
	if (!startByService) printf("\n");
	if (ioctl(rt9466->fd_i2c, I2C_SLAVE, RT9467_ADDR) < 0) {
	    perror("Failed to acquire bus access and/or talk to slave");
	    exit(1);
	}
	sleep(rt9466->interval - 5);
    }

    return NULL;
}

int main_loop(RT9466* rt9466, bool debug)
{
    int     i = 0, uart_fd = rt9466->fd_uart;
    int     rx_bytes, tx_bytes;
    uint8_t recv[256], tx_buf[2];
    uint8_t waitAck = 0, waitResponse = 0;
    char   *p, *q;
    time_t  t1, t2;

    recv[0] = '\0';
    i = 0;
    // Receive loop (non-blocking, reads up to 256 bytes if available)
    while (1) {
        rx_bytes = read(uart_fd, recv + i, 255 - i);

	if (rx_bytes < 0) {
	    if (errno != EAGAIN && errno != EWOULDBLOCK) {
		printf("UART RX error\n");
		break;
	    }
        } else if (rx_bytes > 0) {
	    if (debug) {
		for (int j = 0; j < rx_bytes; j++) printf("%c", recv[i + j]);
	    }
	    i += rx_bytes;
	    recv[i] = '\0';
        }

	if (i) {
	    p = recv;
	    //skip debug message && process receive cmd/ack
	    while (*p) {
	        if ((q = strchr(p, '\n'))) {
		    *q = '\0';
		    if (strlen(p) == 2) {
			if (!strcmp(p, "m1")) {//mcu notify CM5 power off
			    printf("\t\033[1;31;40mreceive m1 power off cmd!!\033[0m\n");
			    tx_buf[0] = 'p'; tx_buf[1] = '1';
			    if ((tx_bytes = write(uart_fd, tx_buf, 2)) != 2) {
				printf("%ssend p1 to mcu%s\n", GreenColor, tx_bytes, NC);
			    }
			} else if (!strcmp(p, "m0")) {
                            printf("%smcu disable 5V power%s\n", GreenColor, NC);
			} else if (!strcmp(p, "m3")) {
			    rt9466->ledBlinking = true;
                            printf("%smcu enable led blinking%s\n", GreenColor, NC);
			} else if (!strcmp(p, "m4")) {
			    rt9466->firstTime = false;
			    rt9466->ledBlinking = false;
                            printf("%smcu disable led blinking%s\n", GreenColor, NC);
			}
		    }
		    p = q + 1;
		}
	    }
	    strcpy(recv, p);
	    i = strlen(recv);
	}

	if (rt9466->notifyMCUPowerOff) {
	    t2 = time(NULL);
	    if (!waitResponse || (t2 - t1) > 2) {
		tx_buf[0] = 'p'; tx_buf[1] = '0';
		if ((tx_bytes = write(uart_fd, tx_buf, 2)) == 2) {
		    waitResponse = '0';
		    t1 = time(NULL);
		    t2 = 0;
                    printf("%swrite p0 & waitResponse m0%s\n", GreenColor, NC);
		} else {
		    printf("%swrite p0 failure!!%s\n", GreenColor, NC);
                }
	    }
	} else if (rt9466->lowVoltage && !rt9466->ledBlinking) {//notify mcu start led blinking
	    t2 = time(NULL);
	    if (!waitResponse || (t2 - t1) > 2) {
		tx_buf[0] = 'p'; tx_buf[1] = '3';
		if ((tx_bytes = write(uart_fd, tx_buf, 2)) == 2) {
		    waitResponse = '3';
		    t1 = time(NULL);
		    t2 = 0;
                    printf("write p3 & waitResponse m3\n", GreenColor, NC);
		} else {
		    printf("%swrite p3 failure!!%s\n", GreenColor, NC);
                }
	    }
	} else if (rt9466->firstTime || (!rt9466->lowVoltage && rt9466->ledBlinking)) {//notify mcu stop led blinkg
	    t2 = time(NULL);
	    if (!waitResponse || (t2 - t1) > 2) {
		tx_buf[0] = 'p'; tx_buf[1] = '4';
		if ((tx_bytes = write(uart_fd, tx_buf, 2)) != 0) {
		    waitResponse = '4';
		    t1 = time(NULL);
		    t2 = 0;
                    printf("%swrite p4 & waitResponse m4%s\n", GreenColor, NC);
		}
	    }
	}
usleep(100);
    }

    // Cleanup
    close(uart_fd);
    return 0;
}

int init(RT9466 *rt9466)
{
    if ((rt9466->fd_i2c = open(I2C_BUS, O_RDWR)) < 0) {
        printf("Failed to open i2c bus\n");
        return 1;
    }

    if (ioctl(rt9466->fd_i2c, I2C_SLAVE, RT9467_ADDR) < 0) {
        printf("Failed to acquire bus access and/or talk to slave\n");
        return 1;
    }

#if 0
    if ((rt9466->fd_i2c_7202 = open(I2C_BUS, O_RDWR)) < 0) {
        printf("Failed to open i2c bus\n");
        return 1;
    }

    if (ioctl(rt9466->fd_i2c_7202, I2C_SLAVE, RT9467_ADDR) < 0) {
        printf("Failed to acquire bus access and/or talk to slave\n");
        return 1;
    }
#endif

#if 0//
    //if (write_register(rt9466->fd_i2c, CHG_CTRL2, 0x13) != 0)
    if (write_register(rt9466->fd_i2c, CHG_CTRL2, 0x1b) != 0)
    {
        printf("write_register: CHG_CTRL2 failed!!\n");
        return 1;
    }

    if (write_register(rt9466->fd_i2c, 0x03, 0xfe) != 0) {
        printf("write_register: 0x03 failed!!\n");
        return 1;
    }
#endif

    // Open the UART device (UART0 on Pi 5 GPIO)
    rt9466->fd_uart = open("/dev/ttyAMA4", O_RDWR | O_NOCTTY | O_NDELAY);
    if (rt9466->fd_uart == -1) {
        printf("Error opening UART\n");
        return 1;
    }

    // Configure UART settings
    struct termios options;
    tcgetattr(rt9466->fd_uart, &options);
    cfsetispeed(&options, B115200);  // Input baud rate
    cfsetospeed(&options, B115200);  // Output baud rate
    options.c_cflag = (options.c_cflag & ~CSIZE) | CS8;  // 8 data bits
    options.c_cflag &= ~PARENB;  // No parity
    options.c_cflag &= ~CSTOPB;  // 1 stop bit
    options.c_cflag |= CLOCAL | CREAD;  // Enable receiver, ignore modem lines
    options.c_iflag = IGNPAR;  // Ignore parity errors
    options.c_oflag = 0;
    options.c_lflag = 0;
    tcflush(rt9466->fd_uart, TCIFLUSH);
    if (tcsetattr(rt9466->fd_uart, TCSANOW, &options) != 0) {
        printf("Error configuring UART\n");
        return 1;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    bool   tx_enable = false;
    int    i, lower;
    RT9466 arg;

    printf("Version V1.1\nBuiltTime: 2025/12/22 11:40\n");

    arg.firstTime = true;
    arg.lowVoltage = false;
    arg.ledBlinking  = false;
    arg.inFastCharge  = false;
    arg.valueIBAT  = 0;
    arg.notifyMCUPowerOff = 0;
    arg.vBAT_limit = 3200;
    arg.vBAT_Poweroff_limit = 2900;
    arg.interval   = 60;

    for (i = 1; i < argc; i++) {
	if (!strcasecmp(argv[i], "debug")) debugMode = true;
	else if (!strcasecmp(argv[i], "startByService"))
	    startByService = true;
	else if (!strncasecmp(argv[i], "vBAT_limit=", 11)) {
	    arg.vBAT_limit = atoi(argv[i] + 11);
	} else if (!strncasecmp(argv[i], "vBAT_poweroff_limit=", 20)) {
	    arg.vBAT_Poweroff_limit = atoi(argv[i] + 20);
	} else if (!strncasecmp(argv[i], "interval=", 9)) {
	    arg.interval = atoi(argv[i] + 9);
        } else {
	    printf("unknown parameter=%s\n", argv[i]);
	}
    }

    pthread_t thread;
    int       result;

    if (init(&arg) != 0) {
        printf("init failed!!\n");
        return 1;
    }

    result = pthread_create(&thread, NULL, check_VBAT, &arg);
    if (result != 0) {
        printf("pthread_create failed!!\n");
	return 1;
    }

    main_loop(&arg, debugMode);

    // Wait for the thread to finish
    result = pthread_join(thread, NULL);

    if (arg.fd_i2c) close(arg.fd_i2c);

    //if (arg.fd_i2c_7202) close(arg.fd_i2c_7202);

    if (arg.fd_uart) {
	close(arg.fd_uart);
    }

    if (result != 0) {
        printf("pthread_join failed\n");
        return 1;
    }

    return 0;
}
