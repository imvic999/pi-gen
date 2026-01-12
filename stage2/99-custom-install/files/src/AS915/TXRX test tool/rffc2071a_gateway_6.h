/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __RFFC2071_H__
#define __RFFC2071_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FSK_CHANNEL_6

#define FSK_CH8_TX_1 257.1
#define FSK_CH8_TX_2 257.26
#define FSK_CH8_TX_3 833.1
#define FSK_CH8_TX_4 833.26
#define FSK_CH8_TX_5 257.1
#define FSK_CH8_TX_6 257.26
#define FSK_CH8_TX_7 833.1
#define FSK_CH8_TX_8 833.26

#define FSK_CH8_RX_1 10.1
#define FSK_CH8_RX_2 10.26
#define FSK_CH8_RX_3 10.1
#define FSK_CH8_RX_4 10.26
#define FSK_CH8_RX_5 204.1
#define FSK_CH8_RX_6 204.26
#define FSK_CH8_RX_7 204.1
#define FSK_CH8_RX_8 204.26

#define FSK_CH6_TX_1 253.1
#define FSK_CH6_TX_2 253.26
#define FSK_CH6_TX_3 253.42
#define FSK_CH6_TX_4 253.58
#define FSK_CH6_TX_5 253.74
#define FSK_CH6_TX_6 253.9

#define FSK_CH6_RX_1 10.1
#define FSK_CH6_RX_2 10.26
#define FSK_CH6_RX_3 10.42
#define FSK_CH6_RX_4 10.58
#define FSK_CH6_RX_5 10.74
#define FSK_CH6_RX_6 10.9

/**
* init rffc2071a device
*   parameter : 
*       stream : 
*           true : open mixer1(TX)
*           false : open mixer2(RX)
*       large_lo : 
*           true : use larger LO
*           false : use smaller LO
*       fsk_num : FSK number(0 ~ x)
*/
bool rffc2071a_init(const bool stream, const bool large_lo, const uint8_t fsk_num);
/**
* init rffc2071a device
*   parameter :
*       stream :
*           true : open mixer1(TX)
*           false : open mixer2(RX)
*       large_lo :
*           true : use larger LO
*           false : use smaller LO
*       chip_freq_mhz : Chip Frequency(MHz)
*       target_freq_mhz : Fout Frequency(MHz)
*/
bool rffc2071a_init2(const bool stream, const bool large_lo, const double chip_freq_mhz, const double target_freq_mhz);
/**
* switch rffc2071a Fout frequency
*   parameter :
*       stream :
*           true : open mixer1(TX)
*           false : open mixer2(RX)
*       large_lo :
*           true : use larger LO
*           false : use smaller LO
*       fsk_num : FSK number(0 ~ x)
*/
void rffc2071a_fsk_changed(const bool stream, const bool large_lo, const uint8_t fsk_num);
/**
* switch rffc2071a Fout frequency
*   parameter :
*       stream :
*           true : open mixer1(TX)
*           false : open mixer2(RX)
*       large_lo :
*           true : use larger LO
*           false : use smaller LO
*       chip_freq_mhz : Chip Frequency(MHz)
*       target_freq_mhz : Fout Frequency(MHz)
*/
void rffc2071a_fsk_changed2(const bool stream, const bool large_lo, const double chip_freq_mhz, const double target_freq_mhz);
/**
* reset rffc2071a device
*/
void rffc2071a_reset();
/**
* send value to rffc2071a register
*   parameter : 
*       register_addr : register address(ex : VCO_CTRL register address = 0x03)
*       data : value to register.
*/
void rffc2071a_send(const uint8_t register_addr, const uint16_t data);
/**
* get value from rffc2071a register
*   parameter :
*       register_addr : register address(ex : VCO_CTRL register address = 0x03)
*   return : 
*       register value
*/
uint16_t rffc2071a_recv(const uint8_t register_addr, const uint8_t multi_slice_addr);
/**
* init rffc2071a gateway(mult rffc2071a)
*   parameter :
*       fsk_num : FSK number(0 ~ x)
*/
void rffc2071a_gw_init(const uint8_t fsk_num, const uint8_t customIDD);
/**
* init rffc2071a gateway(mult rffc2071a)
*   parameter :
*       us_freq_mhz : rffc2071a Chip1(TX) Fout Frequency(MHz)
*       ds_freq_mhz : rffc2071a Chip2(RX) Fout Frequency(MHz)
*/
void rffc2071a_gw_init2(const double us_freq_mhz, const double ds_freq_mhz);
/**
* switch rffc2071a gateway(mult rffc2071a) Fout frequency
*   parameter :
*       fsk_num : FSK number(0 ~ x)
*/
void rffc2071a_fsk_gw_changed(const uint8_t fsk_num);
/**
* switch rffc2071a gateway(mult rffc2071a) Fout frequency
*   parameter :
*       us_freq_mhz : rffc2071a Chip1(TX) Fout Frequency(MHz)
*       ds_freq_mhz : rffc2071a Chip2(RX) Fout Frequency(MHz)
*/
void rffc2071a_fsk_gw_changed2(const double us_freq_mhz, const double ds_freq_mhz);

void disableMultiSlice();

#ifdef __cplusplus
}
#endif

#endif /* __USART_H__ */

