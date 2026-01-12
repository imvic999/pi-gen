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
#define RT9467_ADDR 0x53       // RT9467 I2C 地址

#define CHG_CTRL2    0x02
#define Control3_reg 0x03
//IAICR[7:2]

#define Control7_reg 0x07
//IAICR[7:2]

#define CHG_ADC 0x11
#define VBUS_DIV5 0x10 //0001
#define VBUS_DIV2 0x20 //0010
#define VSYS      0x30 //0011
#define VBAT      0x40 //0100
#define TS_BAT    0x60 //0110
#define IBUS      0x80 //1000
#define IBAT      0x90 //1001
#define RGEN      0xb0 //1011
#define TEMP_JC   0xc0 //1100

#define ADC_START 0x01//0001

#define CHG_STAT  0x42

#define ADC_DATA_H 0x44
#define ADC_DATA_L 0x45

#define CHG_FAULT  0x51


int read_register(int fd, uint8_t reg_addr, uint8_t *value)
{
    if (write(fd, &reg_addr, 1) != 1) {
        printf("%s: Failed to write register address=0x%x\n", __func__, reg_addr);
        return -1;
    }

    if (read(fd, value, 1) != 1) {
        printf("%s: Failed to read register value of address=0x%x\n", __func__, reg_addr);
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
        printf("%s: Failed to write register address=0x%x, value=0x%x\n", __func__, reg_addr, value);
        return -1;
    }

    return 0;
}

int read_adc_register(int fd, uint8_t getType, uint8_t* adc_data_h, uint8_t* adc_data_l)
{
    uint8_t value;

    value = getType | ADC_START;
    write_register(fd, 0x11, value);

    usleep(500000);

    if (read_register(fd, 0x55, &value) != 0) {//check ADC_DONEI (0x55, bit0) = 1 
        printf("read ADC_DONEI failured!!\n");
        return -1;
    } else if ((value & 0x01) != 1) {
        if (read_register(fd, 0x42, &value) != 0) {//check ADC_STAT (0x42, bit0) = 0
	    printf("read ADC_STAT failured!!\n");
	    return -1;
        } else if ((value & 0x01) != 0) {
            printf("ADC_DONEI (0x55, bit0) = 0 and ADC_STAT (0x42, bit0) = 1\n");
	    return -1;
        }
    }

    if (read_register(fd, ADC_DATA_H, adc_data_h) != 0) {
        printf("read ADC_DATA_H failure!!\n");
	return -1;
    }

    if (read_register(fd, ADC_DATA_L, adc_data_l) != 0) {
        printf("read ADC_DATA_H failure!!\n");
	return -1;
    }

    return 0;
}

int getVBUS_DIV5(int fd)
{
    uint8_t adc_data_h, adc_data_l;
    int     n = -1;

    if (read_adc_register(fd, VBUS_DIV5, &adc_data_h, &adc_data_l) == 0) {
	n = (adc_data_h * 256 + adc_data_l) * 25;
	printf("VBUS_DIV5=0x%02x%02x : %dmV\n", adc_data_h, adc_data_l, n);
    }

    return n;
}

int getVBUS_DIV2(int fd)
{
    uint8_t adc_data_h, adc_data_l;
    int     n = -1;

    if (read_adc_register(fd, VBUS_DIV2, &adc_data_h, &adc_data_l) == 0) {
	n = (adc_data_h * 256 + adc_data_l) * 10;
	printf("VBUS_DIV2=0x%02x%02x : %dmV\n", adc_data_h, adc_data_l, n);
    }

    return n;
}

int getVBAT(int fd)
{
    uint8_t adc_data_h, adc_data_l;
    int     n = -1;

    if (read_adc_register(fd, VBAT, &adc_data_h, &adc_data_l) == 0) {
	n = (adc_data_h * 256 + adc_data_l) * 5;
	printf("VBAT=0x%02x%02x : %dmV\n", adc_data_h, adc_data_l, n);
    }

    return n;
}

int getVSYS(int fd)
{
    uint8_t adc_data_h, adc_data_l;
    int     n = -1;

    if (read_adc_register(fd, VSYS, &adc_data_h, &adc_data_l) == 0) {
	n = (adc_data_h * 256 + adc_data_l) * 5;
	printf("VSYS=0x%02x%02x : %dmV\n", adc_data_h, adc_data_l, n);
    }

    return n;
}

float getTS_BAT(int fd)
{
    uint8_t adc_data_h, adc_data_l;
    float   r = -1.0;

    if (read_adc_register(fd, TS_BAT, &adc_data_h, &adc_data_l) == 0) {
	int n = adc_data_h * 256 + adc_data_l;
	r = n * 0.25;
	printf("TS_BAT=0x%02x%02x : %.3f%\n", adc_data_h, adc_data_l, r);
    }

    return r;
}

float getIBUS(int fd)
{
    uint8_t iaicr, adc_data_h, adc_data_l;
    float   r = -1.0;

    if (read_register(fd, Control3_reg, &iaicr) == 0 &&
	read_adc_register(fd, IBUS, &adc_data_h, &adc_data_l) == 0) {
	r = (float)((adc_data_h * 256 + adc_data_l) * 50);
	iaicr = iaicr >> 2;

	if (iaicr < 6) r = r * 0.67; //IAICR[5:0] setting < 400mA

	printf("IBUS=0x%02x%02x : %.3fmA\n", adc_data_h, adc_data_l, r);
    }

    return r;
}

float getIBAT(int fd)
{
    uint8_t ichg, adc_data_h, adc_data_l;
    float   r = -1.0;

    if (read_register(fd, Control7_reg, &ichg) == 0 &&
	read_adc_register(fd, IBAT, &adc_data_h, &adc_data_l) == 0) {
	r = (float)((adc_data_h * 256 + adc_data_l) * 50);

	ichg =  ichg >> 2;
	//printf("n=%d, ichg=0x%x\t", (int)r, ichg);

	if (ichg < 4) r = r * 0.57; //ICHG[5:0] setting < 450mA
	else if (ichg <8) r = r * 0.63; //ICHG[5:0] setting 500mA ~ 850mA
	//else //ICHG[5:0] setting >= 900mA
	 
	printf("IBAT=0x%02x%02x  : %.3fmA\n", adc_data_h, adc_data_l, r);
    }

    return r;
}

int getTEMP_JC(int fd)
{
    uint8_t adc_data_h, adc_data_l;
    int     n = -1;

    if (read_adc_register(fd, TEMP_JC, &adc_data_h, &adc_data_l) == 0) {
	n = (adc_data_h * 256 + adc_data_l) * 2 - 40;
	printf("TEMP_JC=0x%02x%02x : %d°C\n", adc_data_h, adc_data_l, n);
    }

    return n;
}

int getCHG_STAT(int fd)
{
    uint8_t value;

    if (read_register(fd, CHG_STAT, &value) == 0)
    {
	uint8_t s;

	printf("CHG_STAT : 0x%02x\n", value);
	printf("\tCharger status: ");
	s = (value & 0xc0) >> 6;
	if (s == 0) printf("Ready\n");
	else if (s == 1) printf("Charge in progress\n");
	else if (s == 2) printf("Charge done\n");
	else printf("Fault\n");

	s = (value & 0x20) >> 5;
	printf("\tBattery voltage level for operation mode: Charger operate in %s\n", (s) ? "fast-charge level" : "pre-charge");

	s = (value & 0x10) >> 4;
	printf("\tBattery voltage level for operation mode: Charger %s in trickle level\n",
	       (s) ? "operates" : "does not operate");

	s = (value & 0x08) >> 3;
	printf("\tBoost mode status: %s in boost mode\n", (s) ? "" : "Does not");

	s = (value & 0x04) >> 2;
	printf("\tBoost mode VBUS OVP status: %s\n", (s) ? "occurs" : "does not occur");

	s = (value & 0x01);
	printf("\tADC status: is %s\n", (s) ? "under conversion" : "idle");
	
	return value;
    } else printf("get CHG_STAT failure!!\n");

    return -1;
}

int getCHG_FAULT(int fd)
{
    uint8_t value;

    if (read_register(fd, CHG_FAULT, &value) == 0)
    {
	printf("CHG_FAULT : 0x%02x\n", value);
        printf("\tVBUS %s OVP\n", (value & 0x80) ? "is" : "is not");
        printf("\tBaterry %s OVP\n", (value & 0x40) ? "is" : "is not");
        printf("\tSystem %s OVP\n", (value & 0x20) ? "is" : "is not");
        printf("\tSystem %s UVP\n", (value & 0x10) ? "is" : "is not");

	return value;
    } else printf("get CHG_FAULT failure!!\n");

    return -1;
}

int getCHG_NTC(int fd)
{
    uint8_t value;

    if (read_register(fd, 0x43, &value) == 0)
    {
	printf("CHG_NTC(BAT NTC fault status): 0x%02x\n", value);
	value = (value & 0x70) >> 4;
	if (value == 0) printf("\t000: Normal\n");
	else if (value == 2) printf("\t010: Warm\n");
	else if (value == 3) printf("\t011: Cool\n");
	else if (value == 5) printf("\t101: Cold\n");
	else if (value == 6) printf("\t110: Hot\n");

	return value;
    }

    return -1;
}

int getTS_STATC(int fd)
{
    uint8_t value;

    if (read_register(fd, 0x52, &value) == 0)
    {
	printf("CHG_TS_NTC: 0x%02x\n", value);
	printf("\tTS_BAT_HOT %s\n", (value & 0x80) ? "Temperature is hot" : "Normal temperature");
	printf("\tTS_BAT_WARM %s\n", (value & 0x40) ? "Temperature is warm" : "Normal termperature");
	printf("\tTS_BAT_COOL %s\n", (value & 0x20) ? "Temperature is cool" : "Normal termperature");
	printf("\tTS_BAT_COLD %s\n", (value & 0x20) ? "Temperature is cold" : "Normal termperature");

	return value;
    }

    return -1;
}

int dump_registers(int fd)
{
    int     i = 0;
    uint8_t value;

    for (i = 0; i < 0x100; i++) {
        if ((i % 16) == 0) printf("\n0x%02x: ", i);
	read_register(fd, (uint8_t)i, &value);
	printf("%02x ", value);
    }
}

void usage()
{
    printf("RT9466 [-read=reg_addr1] [-read=reg_addrn]\n"
           "       [-getVBUS_DIV5] [-getVBUS_DIV2]\n"
           "       [-getVBAT] [-getVSYS] [-getTS_BAT]\n"
           "       [-getIBUS] [-getIBAT] [-getTEMP_JC]\n"
           "       [-dump]\n");
}

int main(int argc, char* argv[])
{
    int      i, fd;
    uint8_t  addr, val;
    bool     init_Ctrl02_03 = false;


    for (i = 1; i < argc; i++) {
        if (!strcasecmp(argv[i], "-h")) {
	    usage();
	    exit(0);
	} else if (!strcasecmp(argv[i], "-i")) {
	    init_Ctrl02_03 = true;
	}
    }
    // 開啟 I2C 裝置
    if ((fd = open(I2C_BUS, O_RDWR)) < 0) {
        perror("Failed to open i2c bus");
        exit(1);
    }

    // 設定 I2C slave address
    if (ioctl(fd, I2C_SLAVE, RT9467_ADDR) < 0) {
        perror("Failed to acquire bus access and/or talk to slave");
        exit(1);
    }

    if (init_Ctrl02_03) {
        printf("initialized register CHG_CTRL2=0x1b, CHG_CTRL3=0xfe\n");
	//write_register(fd, CHG_CTRL2, 0x13);
	write_register(fd, CHG_CTRL2, 0x1b);
	write_register(fd, 0x03, 0xfe);
    }
read_register(fd, CHG_CTRL2, &val);
printf("CHG_CTRL2: 0x%02x\n", val);
read_register(fd, 0x03, &val);
printf("Control3: 0x%02x\n", val);

    for (i = 1; i < argc; i++) {
        if (!strncasecmp(argv[i], "-read=", 6)) {
	    if (!strncasecmp(argv[i] + 6, "0x", 2)) {
	        addr = (uint8_t)strtol(argv[i] + 8, NULL, 16);
	    } else {
	        addr = atoi(argv[i] + 6);
	    }
	    if (read_register(fd, addr, &val) == 0) {
	        printf("read_register: addr=0x%02x, value=0x%02x\n", addr, val); 
	    }
	} else if (!strcasecmp(argv[i], "-getVBUS_DIV5")) {
	    getVBUS_DIV5(fd);
	} else if (!strcasecmp(argv[i], "-getVBUS_DIV2")) {
	    getVBUS_DIV2(fd);
	} else if (!strcasecmp(argv[i], "-getVBAT")) {
	    getVBAT(fd);
	} else if (!strcasecmp(argv[i], "-getVSYS")) {
	    getVSYS(fd);
	} else if (!strcasecmp(argv[i], "-getTS_BAT")) {
	    //getTS_BAT(fd);
	} else if (!strcasecmp(argv[i], "-getIBUS")) {
	    getIBUS(fd);
	} else if (!strcasecmp(argv[i], "-getIBAT")) {
	    getIBAT(fd);
	} else if (!strcasecmp(argv[i], "-getTEMP_JC")) {
            getTEMP_JC(fd);
	} else if (!strcasecmp(argv[i], "-dump")) {
	    dump_registers(fd);
	} else if (!strcasecmp(argv[i], "-getAll")) {
	    getVBUS_DIV5(fd);
	    getVBUS_DIV2(fd);
	    getVBAT(fd);
	    getVSYS(fd);
	    getTS_BAT(fd);
	    getIBUS(fd);
	    getIBAT(fd);
            getTEMP_JC(fd);
	    getCHG_STAT(fd);
	    getCHG_FAULT(fd);
	    getCHG_NTC(fd);
	    getTS_STATC(fd);
	} else {
	    printf("unknown argument=%s\n", argv[i]);
	}
    }

    close(fd);
    return 0;
}
