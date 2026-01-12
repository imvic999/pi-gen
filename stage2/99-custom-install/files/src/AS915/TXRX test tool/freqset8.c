#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
//#include <fcntl.h>
#include <sched.h>
#include <gpiod.h>
//#include <poll.h>

#include "rffc2071a_gateway_8.h"

#define CHIP_NAME "/dev/gpiochip4"

#define CLK_GPIO    22
#define ENABLE_GPIO 6
#define DATA_GPIO   5
#define RX_LPF_GPIO 21
#define TX_LPF_GPIO 27
#define LED_GPIO    4

#define FSK_NUM     4
#define MAX_FSK_CHAN_NUM 8
#define MAX_FSK_IDD_NUM 7
struct gpiod_chip *chip = NULL;
struct gpiod_line *clk_line = NULL;
struct gpiod_line *enx_line = NULL;
struct gpiod_line *dat_line = NULL;
struct gpiod_line *rx_lpf_line = NULL;
struct gpiod_line *tx_lpf_line = NULL;
struct gpiod_line *led_line = NULL;
uint8_t g_fsk_num = FSK_NUM;
uint8_t customIDD = 5;

const uint8_t MIXER_TX = 1;
const uint8_t MIXER_RX = 2;

void set_realtime_priority()
{
    struct sched_param sched;
    sched.sched_priority = 99;
    if (sched_setscheduler(0, SCHED_FIFO, &sched) == -1) {
	perror("Failed to set real-time priority");
	exit(1);
    } else {
	printf("Real-time priority set successfully.\n");
    }
}

void gpiod_init()
{
#if 1
    if (!(chip = gpiod_chip_open(CHIP_NAME))) {
	printf("gpiod_chip_open(%s) failure!!\n", CHIP_NAME);
	exit(1);
    }
#else
    if (!(chip = gpiod_chip_open_by_name("gpiochip0"))) {
	printf("gpiod_chip_open(%s) failure!!\n", "gpiochip0");
	exit(1);
    }
#endif
    if (!(clk_line = gpiod_chip_get_line(chip, CLK_GPIO))) {
	printf("CLK_GPIO: gpiod_chip_get_line(%s, %d) failure!!\n", CHIP_NAME, CLK_GPIO);
	exit(1);
    }
    if(gpiod_line_request_output(clk_line, "clk", 0) < 0) {
        printf("Failed to request output on clock line\r\n");
        exit(1);
    }

    if (!(enx_line = gpiod_chip_get_line(chip, ENABLE_GPIO))) {
	printf("ENABLE_GPIO: gpiod_chip_get_line(%s, %d) failure!!\n", CHIP_NAME, ENABLE_GPIO);
	exit(1);
    }
    if(gpiod_line_request_output(enx_line, "enx", 1) < 0) {
        printf("Failed to request output on enable line\r\n");
        exit(1);
    }

    if (!(dat_line = gpiod_chip_get_line(chip, DATA_GPIO))) {
	printf("DATA_GPIO: gpiod_chip_get_line(%s, %d) failure!!\n", CHIP_NAME, DATA_GPIO);
	exit(1);
    }
    if(gpiod_line_request_output(dat_line, "dat", 0) < 0) {
        printf("Failed to request output on data line\r\n");
        exit(1);
    }

    if (!(led_line = gpiod_chip_get_line(chip, LED_GPIO))) {
	printf("DATA_GPIO: gpiod_chip_get_line(%s, %d) failure!!\n", CHIP_NAME, LED_GPIO);
	exit(1);
    }

    if(gpiod_line_request_output(led_line, "led", 0) < 0) {
        printf("Failed to request output on LED line\r\n");
        exit(1);
    }

     if (!(tx_lpf_line = gpiod_chip_get_line(chip, TX_LPF_GPIO))) {
	printf("DATA_GPIO: gpiod_chip_get_line(%s, %d) failure!!\n", CHIP_NAME, TX_LPF_GPIO);
	exit(1);
    }
    
    //Vic@20251128 TX LPF high by default for 253 MHz
    if(gpiod_line_request_output(tx_lpf_line, "txlpf", 1) < 0) {
        printf("Failed to request output on LPF line\r\n");
        exit(1);
    }

    if (!(rx_lpf_line = gpiod_chip_get_line(chip, RX_LPF_GPIO))) {
	printf("DATA_GPIO: gpiod_chip_get_line(%s, %d) failure!!\n", CHIP_NAME, RX_LPF_GPIO);
	exit(1);
    }
    
    //Vic@20251128 RX LPF low by default for 10 MHz
    if(gpiod_line_request_output(rx_lpf_line, "rxlpf", 0) < 0) {
        printf("Failed to request output on LPF line\r\n");
        exit(1);
    }
}

void set_low_pass_filter(uint8_t mixer, uint8_t highLow)
{
    if(mixer != MIXER_TX && mixer != MIXER_RX) {
        printf("set_low_pass_filter: mixer should be 1 or 2\r\n");
        return;
    }

    if(highLow != (uint8_t)0 && highLow != (uint8_t)1) {
        printf("set_low_pass_filter: highLow should be 0 (low) or 1 (high)\r\n");
        return;
    }

    if(gpiod_line_set_value(mixer == MIXER_TX ? tx_lpf_line : rx_lpf_line, highLow) != 0) {
        printf("Failed to set low pass filter line (%d, %d)\r\n", mixer, highLow);
    }
    
}
    
    /*
void toggle_led()
{
    gpiod_line_set_value(led_line, 0);
    usleep(5 * 1000);
    gpiod_line_set_value(led_line, 1);
}
*/

void printUsage(){
    printf("usage: freqset6 -chn=0~%d -idd=0~7(default:5)\n"
	    	"       0:  TX(%.2f), RX(%.2f) [Default]\n"
            "       1:  TX(%.2f), RX(%.2f)\n"
            "       2:  TX(%.2f), RX(%.2f)\n"
            "       3:  TX(%.2f), RX(%.2f)\n"
            "       4:  TX(%.2f), RX(%.2f)\n"
            "       5:  TX(%.2f), RX(%.2f)\n"
            "       6:  TX(%.2f), RX(%.2f)\n"
            "       7:  TX(%.2f), RX(%.2f)\n",
            MAX_FSK_CHAN_NUM - 1,
            FSK_CH8_TX_1, FSK_CH8_RX_1,
            FSK_CH8_TX_2, FSK_CH8_RX_2,
            FSK_CH8_TX_3, FSK_CH8_RX_3,
            FSK_CH8_TX_4, FSK_CH8_RX_4,
            FSK_CH8_TX_5, FSK_CH8_RX_5,
            FSK_CH8_TX_6, FSK_CH8_RX_6,
            FSK_CH8_TX_7, FSK_CH8_RX_7,
            FSK_CH8_TX_8, FSK_CH8_RX_8
        );
}


int main(int argc, char *argv[])
{
    int  addr = -1;

    set_realtime_priority();
    printf("Running real-time sysfs bit-bang...\n");

    if(argc == 1) {
        g_fsk_num = 0;
    }

    for (int i = 1; i < argc; i++)
    {
	    if (!strcmp(argv[i], "-h")) {
	        printUsage();                
	        exit(0);
	    } else if (!strncmp(argv[i], "-chn=", 5)) {
	        g_fsk_num = atoi(argv[i] + 5);
	        if (g_fsk_num < 0) g_fsk_num = 0;
	        else if (g_fsk_num >= MAX_FSK_CHAN_NUM) g_fsk_num = MAX_FSK_CHAN_NUM - 1;
        } else if (!strncmp(argv[i], "-idd=", 5)) {
            customIDD = atoi(argv[i] + 5);
            if (customIDD < 0 || customIDD > MAX_FSK_IDD_NUM) customIDD = 5;
        }
    }

    gpiod_init();

    rffc2071a_gw_init(g_fsk_num, customIDD);

    if(g_fsk_num == 2 || g_fsk_num == 3 || g_fsk_num == 6 || g_fsk_num == 7 ) {
        set_low_pass_filter(MIXER_TX, 0); //
    }
    else {
        set_low_pass_filter(MIXER_TX, 1); 
    }

    if(g_fsk_num < 4) {
        // FSK_CH8 1~4 10 MHz
        set_low_pass_filter(MIXER_RX, 0);
    }
    else {
        // FSK_CH8 5~8 204 MHz
        set_low_pass_filter(MIXER_RX, 1);
    }
    printf("1lock channel %d\r\n", g_fsk_num);fflush(stdout);
    
    gpiod_line_release(clk_line);
    gpiod_line_release(enx_line);
    gpiod_line_release(dat_line);
    gpiod_line_release(led_line);
    gpiod_line_release(tx_lpf_line);
    gpiod_line_release(rx_lpf_line);
    gpiod_chip_close(chip);
    return 0;

}
