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
#define RT7202_ADDR 0x1B       // RT7202 I2C 地址

#define FWCodeVersion (uint8_t)0x00
#define ProtocolMode  (uint8_t)0x02

uint8_t crc8(uint8_t init_value, uint8_t* data, size_t len);
bool debugMode = false;

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

int write_register(int fd, uint8_t reg_addr, uint8_t len, uint8_t *value)
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

int main(int argc, char* argv[])
{
    int      i, fd, n;
    uint8_t  addr, adc_data_h, adc_data_l;
    uint8_t  profile_selection=0;
    bool read51 = true;

    for (i = 1; i < argc; i++) {
        if (!strcasecmp(argv[i], "debug")) debugMode = true;
	else if (!strncasecmp(argv[i], "profile_selection=", 18)) profile_selection = atoi(argv[i] + 18);
        else if (!strcasecmp(argv[i], "skip51")) read51 = false;
    }
if (profile_selection > 3) profile_selection = 3;
printf("read51=%d, profile_selection=%d\n", read51, profile_selection);

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
        printf("Firmware Code Version(0x00):");
        for (i = 0; i < 8; i++) printf(" 0x%02x", fw_code[i]);
        printf("\n");
    }

    uint8_t received_data[32];
#if 1
    printf("\nProtocol Mode(0x02):");
    if (read_register2(fd, ProtocolMode, (uint8_t)2, received_data) == 0) {
	uint16_t data = received_data[0]  | received_data[1] << 8;
        if (data == 0) printf(" No Protocol\n");
	else if (data == 1) printf("Type C\n");
	else if (data == 2) printf("PD\n");
	else if (data == 3) printf("QC 2.0\n");
	else if (data == 4 || data == 5) printf("Reserv\n");
	else if (data == 6) printf("DCP\n");
	else printf("Unknown protocol data=0x%x\n", data);
    }

    printf("\nStatus Change(0x10):\n");
    if (read_register2(fd, 0x10, (uint8_t)2, received_data) == 0) {
	uint16_t data = received_data[0]  | received_data[1] << 8;
	printf("\tInsert Status: %s\n", ((data & 0x01) ? "Changed" : "No Change"));
	printf("\tPD Status: %s\n", ((data & 0x02) ? "Changed" : "No Change"));
	printf("\tPower Fault: %s\n", ((data & 0x04) ? "Occurred" : "No"));
    }

    printf("\nEC/IO Config(0x56):");
    if (read_register2(fd, 0x56, (uint8_t)2, received_data) == 0) {
	uint16_t data = received_data[0]  | received_data[1] << 8;
	printf(" %s\n", ((data & 0x01) ? "EC" : "IO"));
    }

    printf("\nVBUS Voltage and IBUS Current(0x30):\n");
    if (read_register2(fd, 0x30, (uint8_t)4, received_data) == 0) {
	uint16_t data = received_data[0]  | received_data[1] << 8;
	printf("\tIBUS Current: 0x%x:%dmA\n", data, data);
	data = received_data[2]  | received_data[3] << 8;
	printf("\tVBUS Voltage: 0x%x:%dmV\n", data, data);
    }

    printf("\nSource Caabilities(0x21):\n");
    if (read_register2(fd, 0x21, (uint8_t)32, received_data) == 0) {
	uint16_t data = received_data[0]  | received_data[1] << 8;
	printf("\tSource Capabilities(port Partner): 0x%04x\n", data);
    }

    printf("\nPD Status(0x12):\n");
    if (read_register2(fd, 0x12, (uint8_t)2, received_data) == 0) {
	uint16_t data = received_data[0]  | received_data[1] << 8;
	printf("\tDevice Power Role: %s\n", ((data & 0x01) ? "Source" : "Sink"));
	printf("\tDevice Data Role: %s\n", ((data & 0x02) ? "DFP" : "UFP"));
	printf("\tDevice Vconn Role: %s\n", ((data & 0x04) ? "Vconn" : "No Vconn"));
	printf("\tCC Direction: %s\n", ((data & 0x08) ? "CC2 Communication" : "CC1 Communication"));

	uint8_t t = (data & 0x30) >> 4;
	printf("\tCC Advertisement Level: ");
	if (t == 0) printf("N/A\n");
	else if (t == 1) printf("Default USB Power\n");
	else if (t == 2) printf("1.5A\n");
	else printf("3A\n");

	t = (data & 0xc0) >> 6;
	printf("\tPD Sepc Version: ");
	if (t == 0) printf("N/A\n");
	else if (t == 1) printf("PD 1.0\n");
	else if (t == 2) printf("PD 2.0\n");
	else printf("PD 3.0\n");

	t = (data & 0x100) >> 8;
	printf("\tExplicit Contract: %s\n", (t ? "Yes" : "No"));

	t = (data & 0x200) >> 9;
	printf("\tInsertion Detect: %s\n", (t ? "Yes" : "No"));

	t = (data & 0x400) >> 10;
	printf("\tDesired PDO: The RT7202KLA Sink %s the Desired PDO\n", (t ? "has found" : "did not find"));

	t = (data & 0x800) >> 11;
	printf("\tInvalid Object Position: The Object Position issued by the EC is%s within the Source Device Position \n", (t ? " not" : ""));

	t = (data & 0x1000) >> 12;
	printf("\tReceived PS_RDY Message: %s\n", (t ? "Yes" : "No"));

	t = (data & 0x2000) >> 13;
	printf("\tReceived Reject Message: %s\n", (t ? "Yes" : "No"));

	t = (data & 0x4000) >> 14;
	printf("\tPD Status: The currently requested voltage is %s the previous\n", (t ? "different from" : "the same as"));

	t = (data & 0x8000) >> 15;
	printf("\tReceive Source_Capabilities Message: %s\n", (t ? "Yes" : "No"));
    }

    printf("\nPower Fault(0x13): ");
    if (read_register2(fd, 0x13, (uint8_t)2, received_data) == 0) {
	printf("0x%02x%02x\n", received_data[1], received_data[0]);
	printf("\t%sOVP Occurred\n", ((received_data[0] & 1) ? "" : "No "));
	printf("\t%sUVP Occurred\n", ((received_data[0] & 2) ? "" : "No "));
	printf("\t%sOCP Occurred\n", ((received_data[0] & 4) ? "" : "No "));
	printf("\t%sOTP Occurred\n", ((received_data[0] & 8) ? "" : "No "));
    }

    printf("\nPDO Operating Voltage(0x20):\n");
    if (read_register2(fd, 0x20, (uint8_t)2, received_data) == 0) {
	uint16_t data = received_data[0]  | received_data[1] << 8;
	printf("\tOperating Voltage of the PDO: %dmV\n", data);
    }

    printf("\nOVP Config(0x52)\n");
    //OVP Config
    if (read_register2(fd, 0x52, (uint8_t)4, received_data) == 0) {
	printf("\t OVP En: %s\n", ((received_data[0] & 0x01) ? "Enable" : "Disable"));
	printf("\t OVP Level: %d%\n", received_data[1]);
	printf("\t OVP DBTime: %dms\n", received_data[2]);
    }

    printf("\nUVP Config(0x53):\n");
    if (read_register2(fd, 0x53, (uint8_t)4, received_data) == 0) {
	printf("\t UVP En: %s\n", ((received_data[0] & 0x01) ? "Enable" : "Disable"));
	printf("\t UVP Level: %d%\n", received_data[1]);
	printf("\t UVP DBTime: %dms\n", received_data[2]);
    }

if (read51) {
    printf("\nSystem Power(0x51):\n");
    if (read_register2(fd, 0x51, (uint8_t)2, received_data) == 0) {
	uint16_t data = received_data[0]  | received_data[1] << 8;
	printf("\tSystem Power Limit: 0x%02x:%dW\n", data, data);
    }
}

if (profile_selection > 0) {
    printf("\nProfile Selection(0x50) for Write:\n");
    received_data[0] = profile_selection; received_data[1] = 0;
    if (write_register(fd, 0x50, (uint8_t)2, received_data) == 0) {
    printf("write 0x50 success!!\n");
    sleep(1);
    }
}
#endif

    printf("\nProfile Selection(0x50):\n");
    if (read_register2(fd, 0x50, (uint8_t)2, received_data) == 0) {
	uint16_t data = received_data[0]  | received_data[1] << 8;
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
    }

    close(fd);
    return 0;
}
