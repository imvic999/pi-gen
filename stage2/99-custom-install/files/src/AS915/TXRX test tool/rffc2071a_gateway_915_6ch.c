#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include "rffc2071a_gateway_6.h"

#define RaspberryPi

#ifdef RaspberryPi
#include <gpiod.h>

#define FSK_IDD     5
#define FSK_TC      1

#define DATA_GPIO   5

extern struct gpiod_chip *chip;
extern struct gpiod_line *clk_line;
extern struct gpiod_line *enx_line;
extern struct gpiod_line *dat_line;
extern uint8_t g_fsk_num;

#else //RaspberryPi

#include "zinwell.h"
#include "stm32wlxx_hal.h"
#include "sys_app.h"
#include "LmHandler_mbwrapper.h"

uint8_t g_fsk_num = FSK_NUM;

#endif //RaspberryPi

// RFFC2071A REGISTER MAP
#define RFFC_REG_ADDR_LF 0x00
#define RFFC_REG_ADDR_VCO_CTRL 0x03
#define RFFC_REG_ADDR_CT_CAL1 0x04
#define RFFC_REG_ADDR_CT_CAL2 0x05
#define RFFC_REG_ADDR_PLL_CAL1 0x06
#define RFFC_REG_ADDR_PLL_CAL2 0x07
#define RFFC_REG_ADDR_VCO_AUTO 0x08
#define RFFC_REG_ADDR_PLL_CTRL 0x09
#define RFFC_REG_ADDR_MIX_CONT 0x0B
#define RFFC_REG_ADDR_P1_FREQ1 0x0C
#define RFFC_REG_ADDR_P1_FREQ2 0x0D
#define RFFC_REG_ADDR_P1_FREQ3 0x0E
#define RFFC_REG_ADDR_P2_FREQ1 0x0F
#define RFFC_REG_ADDR_P2_FREQ2 0x10
#define RFFC_REG_ADDR_P2_FREQ3 0x11
#define RFFC_REG_ADDR_SDI_CTRL 0x15
#define RFFC_REG_ADDR_TEST 0x1E

// Reg name : LF(DEF : 0xBEFA)
#define RFFC_READ_LF_LFACT(data_byte) (data_byte >> 15)
#define RFFC_WRITE_LF_LFACT(data_byte, value) ((data_byte & 0x7FFF) | ((value & 0x01) << 15))
#define RFFC_READ_LF_P2CPDEF(data_byte) ((data_byte & 0x7E00) >> 9)
#define RFFC_WRITE_LF_P2CPDEF(data_byte, value) ((data_byte & (~0x7E00)) | ((value & 0x3F) << 9))
#define RFFC_READ_LF_P1CPDEF(data_byte) ((data_byte & 0x01F8) >> 3)
#define RFFC_WRITE_LF_P1CPDEF(data_byte, value) ((data_byte & (~0x01F8)) | ((value & 0x3F) << 3))
#define RFFC_READ_LF_PLLCPL(data_byte) (data_byte & 0x0007)
#define RFFC_WRITE_LF_PLLCPL(data_byte, value) ((data_byte & (~0x0007)) | (value & 0x07))

// Reg name : VCO_CTRL(DEF : 0x2D02)
#define RFFC_READ_VCO_CTRL_XTVCO(data_byte) (data_byte >> 15)
#define RFFC_WRITE_VCO_CTRL_XTVCO(data_byte, value) ((data_byte & 0x7FFF) | ((value & 0x01) << 15))
#define RFFC_READ_VCO_CTRL_CTAVG(data_byte) ((data_byte & 0x6000) >> 13)
#define RFFC_WRITE_VCO_CTRL_CTAVG(data_byte, value) ((data_byte & (~0x6000)) | ((value & 0x03) << 13))
#define RFFC_READ_VCO_CTRL_CTPOL(data_byte) ((data_byte & 0x1000) >> 12)
#define RFFC_WRITE_VCO_CTRL_CTPOL(data_byte, value) ((data_byte & (~0x1000)) | ((value & 0x01) << 12))
#define RFFC_READ_VCO_CTRL_CLKPL(data_byte) ((data_byte & 0x0800) >> 11)
#define RFFC_WRITE_VCO_CTRL_CLKPL(data_byte, value) ((data_byte & (~0x0800)) | ((value & 0x01) << 11))
#define RFFC_READ_VCO_CTRL_KVAVG(data_byte) ((data_byte & 0x0600) >> 9)
#define RFFC_WRITE_VCO_CTRL_KVAVG(data_byte, value) ((data_byte & (~0x0600)) | ((value & 0x03) << 9))
#define RFFC_READ_VCO_CTRL_KVRNG(data_byte) ((data_byte & 0x0100) >> 8)
#define RFFC_WRITE_VCO_CTRL_KVRNG(data_byte, value) ((data_byte & (~0x0100)) | ((value & 0x01) << 8))
#define RFFC_READ_VCO_CTRL_KVPOL(data_byte) ((data_byte & 0x0080) >> 7)
#define RFFC_WRITE_VCO_CTRL_KVPOL(data_byte, value) ((data_byte & (~0x0080)) | ((value & 0x01) << 7))
#define RFFC_READ_VCO_CTRL_XOI1(data_byte) ((data_byte & 0x0040) >> 6)
#define RFFC_WRITE_VCO_CTRL_XOI1(data_byte, value) ((data_byte & (~0x0040)) | ((value & 0x01) << 6))
#define RFFC_READ_VCO_CTRL_XOI2(data_byte) ((data_byte & 0x0020) >> 5)
#define RFFC_WRITE_VCO_CTRL_XOI2(data_byte, value) ((data_byte & (~0x0020)) | ((value & 0x01) << 5))
#define RFFC_READ_VCO_CTRL_XOI3(data_byte) ((data_byte & 0x0010) >> 4)
#define RFFC_WRITE_VCO_CTRL_XOI3(data_byte, value) ((data_byte & (~0x0010)) | ((value & 0x01) << 4))
#define RFFC_READ_VCO_CTRL_REFST(data_byte) ((data_byte & 0x0008) >> 3)
#define RFFC_WRITE_VCO_CTRL_REFST(data_byte, value) ((data_byte & (~0x0008)) | ((value & 0x01) << 3))
#define RFFC_READ_VCO_CTRL_ICPUP(data_byte) ((data_byte & 0x0006) >> 1)
#define RFFC_WRITE_VCO_CTRL_ICPUP(data_byte, value) ((data_byte & (~0x0006)) | ((value & 0x03) << 1))

// Reg name : CT_CAL1(DEF : 0xACBF)
#define RFFC_READ_CT_CAL1_P1CTGAIN(data_byte) (data_byte >> 13)
#define RFFC_WRITE_CT_CAL1_P1CTGAIN(data_byte, value) ((data_byte & 0x1FFF) | ((value & 0x07) << 13))
#define RFFC_READ_CT_CAL1_P1CTV(data_byte) ((data_byte & 0x1F00) >> 8)
#define RFFC_WRITE_CT_CAL1_P1CTV(data_byte, value) ((data_byte & (~0x1F00)) | ((value & 0x1F) << 8))
#define RFFC_READ_CT_CAL1_P1CT(data_byte) ((data_byte & 0x0080) >> 7)
#define RFFC_WRITE_CT_CAL1_P1CT(data_byte, value) ((data_byte & (~0x0080)) | ((value & 0x01) << 7))
#define RFFC_READ_CT_CAL1_P1CTDEF(data_byte) (data_byte & 0x007F)
#define RFFC_WRITE_CT_CAL1_P1CTDEF(data_byte, value) ((data_byte & (~0x007F)) | (value & 0x7F))

// Reg name : CT_CAL2(DEF : 0xACBF)
#define RFFC_READ_CT_CAL2_P2CTGAIN(data_byte) (data_byte >> 13)
#define RFFC_WRITE_CT_CAL2_P2CTGAIN(data_byte, value) ((data_byte & 0x1FFF) | ((value & 0x07) << 13))
#define RFFC_READ_CT_CAL2_P2CTV(data_byte) ((data_byte & 0x1F00) >> 8)
#define RFFC_WRITE_CT_CAL2_P2CTV(data_byte, value) ((data_byte & (~0x1F00)) | ((value & 0x1F) << 8))
#define RFFC_READ_CT_CAL2_P2CT(data_byte) ((data_byte & 0x0080) >> 7)
#define RFFC_WRITE_CT_CAL2_P2CT(data_byte, value) ((data_byte & (~0x0080)) | ((value & 0x01) << 7))
#define RFFC_READ_CT_CAL2_P2CTDEF(data_byte) (data_byte & 0x007F)
#define RFFC_WRITE_CT_CAL2_P2CTDEF(data_byte, value) ((data_byte & (~0x007F)) | (value & 0x7F))

// Reg name : PLL_CAL1(DEF : 0x0028)
#define RFFC_READ_PLL_CAL1_P1KV(data_byte) (data_byte >> 15)
#define RFFC_WRITE_PLL_CAL1_P1KV(data_byte, value) ((data_byte & 0x7FFF) | ((value & 0x01) << 15))
#define RFFC_READ_PLL_CAL1_P1DN(data_byte) ((data_byte & 0x7FC0) >> 6)
#define RFFC_WRITE_PLL_CAL1_P1DN(data_byte, value) ((data_byte & (~0x7FC0)) | ((value & 0x01FF) << 6))
#define RFFC_READ_PLL_CAL1_P1KVGAIN(data_byte) ((data_byte & 0x0038) >> 3)
#define RFFC_WRITE_PLL_CAL1_P1KVGAIN(data_byte, value) ((data_byte & (~0x0038)) | ((value & 0x07) << 3))
#define RFFC_READ_PLL_CAL1_P1SGN(data_byte) ((data_byte & 0x0004) >> 2)
#define RFFC_WRITE_PLL_CAL1_P1SGN(data_byte, value) ((data_byte & (~0x0004)) | ((value & 0x01) << 2))

// Reg name : PLL_CAL2(DEF : 0x0028)
#define RFFC_READ_PLL_CAL2_P2KV(data_byte) (data_byte >> 15)
#define RFFC_WRITE_PLL_CAL2_P2KV(data_byte, value) ((data_byte & 0x7FFF) | ((value & 0x01) << 15))
#define RFFC_READ_PLL_CAL2_P2DN(data_byte) ((data_byte & 0x7FC0) >> 6)
#define RFFC_WRITE_PLL_CAL2_P2DN(data_byte, value) ((data_byte & (~0x7FC0)) | ((value & 0x01FF) << 6))
#define RFFC_READ_PLL_CAL2_P2KVGAIN(data_byte) ((data_byte & 0x0038) >> 3)
#define RFFC_WRITE_PLL_CAL2_P2KVGAIN(data_byte, value) ((data_byte & (~0x0038)) | ((value & 0x07) << 3))
#define RFFC_READ_PLL_CAL2_P2SGN(data_byte) ((data_byte & 0x0004) >> 2)
#define RFFC_WRITE_PLL_CAL2_P2SGN(data_byte, value) ((data_byte & (~0x0004)) | ((value & 0x01) << 2))

// Reg name : VCO_AUTO(DEF : 0xFF00)
#define RFFC_READ_VCO_AUTO_AUTO(data_byte) (data_byte >> 15)
#define RFFC_WRITE_VCO_AUTO_AUTO(data_byte, value) ((data_byte & 0x7FFF) | ((value & 0x01) << 15))
#define RFFC_READ_VCO_AUTO_CTMAX(data_byte) ((data_byte & 0x7F00) >> 8)
#define RFFC_WRITE_VCO_AUTO_CTMAX(data_byte, value) ((data_byte & (~0x7F00)) | ((value & 0x7F) << 8))
#define RFFC_READ_VCO_AUTO_CTMIN(data_byte) ((data_byte & 0x00FE) >> 1)
#define RFFC_WRITE_VCO_AUTO_CTMIN(data_byte, value) ((data_byte & (~0x00FE)) | ((value & 0x7F) << 1))

// Reg name : PLL_CTRL(DEF : 0x8220)
#define RFFC_READ_PLL_CTRL_DIVBY(data_byte) (data_byte >> 15)
#define RFFC_WRITE_PLL_CTRL_DIVBY(data_byte, value) ((data_byte & 0x7FFF) | ((value & 0x01) << 15))
#define RFFC_READ_PLL_CTRL_CLKDIV(data_byte) ((data_byte & 0x7000) >> 12)
#define RFFC_WRITE_PLL_CTRL_CLKDIV(data_byte, value) ((data_byte & (~0x7000)) | ((value & 0x07) << 12))
#define RFFC_READ_PLL_CTRL_PLLST(data_byte) ((data_byte & 0x0800) >> 11)
#define RFFC_WRITE_PLL_CTRL_PLLST(data_byte, value) ((data_byte & (~0x0800)) | ((value & 0x01) << 11))
#define RFFC_READ_PLL_CTRL_TVCO(data_byte) ((data_byte & 0x07C0) >> 6)
#define RFFC_WRITE_PLL_CTRL_TVCO(data_byte, value) ((data_byte & (~0x07C0)) | ((value & 0x1F) << 6))
#define RFFC_READ_PLL_CTRL_LDEN(data_byte) ((data_byte & 0x0020) >> 5)
#define RFFC_WRITE_PLL_CTRL_LDEN(data_byte, value) ((data_byte & (~0x0020)) | ((value & 0x01) << 5))
#define RFFC_READ_PLL_CTRL_LDLEV(data_byte) ((data_byte & 0x0010) >> 4)
#define RFFC_WRITE_PLL_CTRL_LDLEV(data_byte, value) ((data_byte & (~0x0010)) | ((value & 0x01) << 4))
#define RFFC_READ_PLL_CTRL_RELOK(data_byte) ((data_byte & 0x0008) >> 3)
#define RFFC_WRITE_PLL_CTRL_RELOK(data_byte, value) ((data_byte & (~0x0008)) | ((value & 0x01) << 3))
#define RFFC_READ_PLL_CTRL_ALOI(data_byte) ((data_byte & 0x0004) >> 2)
#define RFFC_WRITE_PLL_CTRL_ALOI(data_byte, value) ((data_byte & (~0x0004)) | ((value & 0x01) << 2))
#define RFFC_READ_PLL_CTRL_PLLDY(data_byte) (data_byte & 0x0003)
#define RFFC_WRITE_PLL_CTRL_PLLDY(data_byte, value) ((data_byte & (~0x0003)) | (value & 0x03))

// Reg name : MIX_CONT(DEF : 0x4800)
#define RFFC_READ_MIX_CONT_FULLD(data_byte) (data_byte >> 15)
#define RFFC_WRITE_MIX_CONT_FULLD(data_byte, value) ((data_byte & 0x7FFF) | ((value & 0x01) << 15))
#define RFFC_READ_MIX_CONT_P1MIXIDD(data_byte) ((data_byte & 0x7000) >> 12)
#define RFFC_WRITE_MIX_CONT_P1MIXIDD(data_byte, value) ((data_byte & (~0x7000)) | ((value & 0x07) << 12))
#define RFFC_READ_MIX_CONT_P2MIXIDD(data_byte) ((data_byte & 0x0E00) >> 9)
#define RFFC_WRITE_MIX_CONT_P2MIXIDD(data_byte, value) ((data_byte & (~0x0E00)) | ((value & 0x07) << 9))

// Reg name : P1_FREQ1(DEF : 0x1A94)
#define RFFC_READ_P1_FREQ1_P1N(data_byte) (data_byte >> 7)
#define RFFC_WRITE_P1_FREQ1_P1N(data_byte, value) ((data_byte & 0x007F) | ((value & 0x01FF) << 7))
#define RFFC_READ_P1_FREQ1_P1LODIV(data_byte) ((data_byte & 0x0070) >> 4)
#define RFFC_WRITE_P1_FREQ1_P1LODIV(data_byte, value) ((data_byte & (~0x0070)) | ((value & 0x07) << 4))
#define RFFC_READ_P1_FREQ1_P1PRESC(data_byte) ((data_byte & 0x000C) >> 2)
#define RFFC_WRITE_P1_FREQ1_P1PRESC(data_byte, value) ((data_byte & (~0x000C)) | ((value & 0x03) << 2))
#define RFFC_READ_P1_FREQ1_P1VCOSEL(data_byte) (data_byte & 0x0003)
#define RFFC_WRITE_P1_FREQ1_P1VCOSEL(data_byte, value) ((data_byte & (~0x0003)) | (value & 0x03))

// Reg name : P1_FREQ2(DEF : 0xD89D)
// total 2 byte

// Reg name : P1_FREQ3(DEF : 0x8900)
#define RFFC_READ_P1_FREQ3_P1NLSB(data_byte) (data_byte >> 8)
#define RFFC_WRITE_P1_FREQ3_P1NLSB(data_byte, value) ((data_byte & 0x00FF) | ((value & 0xFF) << 8))

// Reg name : P2_FREQ1(DEF : 0x1E84)
#define RFFC_READ_P2_FREQ1_P2N(data_byte) (data_byte >> 7)
#define RFFC_WRITE_P2_FREQ1_P2N(data_byte, value) ((data_byte & 0x007F) | ((value & 0x01FF) << 7))
#define RFFC_READ_P2_FREQ1_P2LODIV(data_byte) ((data_byte & 0x0070) >> 4)
#define RFFC_WRITE_P2_FREQ1_P2LODIV(data_byte, value) ((data_byte & (~0x0070)) | ((value & 0x07) << 4))
#define RFFC_READ_P2_FREQ1_P2PRESC(data_byte) ((data_byte & 0x000C) >> 2)
#define RFFC_WRITE_P2_FREQ1_P2PRESC(data_byte, value) ((data_byte & (~0x000C)) | ((value & 0x03) << 2))
#define RFFC_READ_P2_FREQ1_P2VCOSEL(data_byte) (data_byte & 0x0003)
#define RFFC_WRITE_P2_FREQ1_P2VCOSEL(data_byte, value) ((data_byte & (~0x0003)) | (value & 0x03))

// Reg name : P2_FREQ2(DEF : 0x89D8)
// total 2 byte

// Reg name : P2_FREQ3(DEF : 0x9D00)
#define RFFC_READ_P2_FREQ3_P2NLSB(data_byte) (data_byte >> 8)
#define RFFC_WRITE_P2_FREQ3_P2NLSB(data_byte, value) ((data_byte & 0x00FF) | ((value & 0xFF) << 8))

// Reg name : SDI_CTRL(DEF : 0x0000)
#define RFFC_READ_SDI_CTRL_SIPIN(data_byte) (data_byte >> 15)
#define RFFC_WRITE_SDI_CTRL_SIPIN(data_byte, value) ((data_byte & 0x7FFF) | ((value & 0x01) << 15))
#define RFFC_READ_SDI_CTRL_ENBL(data_byte) ((data_byte & 0x4000) >> 14)
#define RFFC_WRITE_SDI_CTRL_ENBL(data_byte, value) ((data_byte & (~0x4000)) | ((value & 0x01) << 14))
#define RFFC_READ_SDI_CTRL_MODE(data_byte) ((data_byte & 0x2000) >> 13)
#define RFFC_WRITE_SDI_CTRL_MODE(data_byte, value) ((data_byte & (~0x2000)) | ((value & 0x01) << 13))
#define RFFC_READ_SDI_CTRL_4WIRE(data_byte) ((data_byte & 0x1000) >> 12)
#define RFFC_WRITE_SDI_CTRL_4WIRE(data_byte, value) ((data_byte & (~0x1000)) | ((value & 0x01) << 12))
#define RFFC_READ_SDI_CTRL_ADDR(data_byte) ((data_byte & 0x0800) >> 11)
#define RFFC_WRITE_SDI_CTRL_ADDR(data_byte, value) ((data_byte & (~0x0800)) | ((value & 0x01) << 11))
#define RFFC_READ_SDI_CTRL_RESET(data_byte) ((data_byte & 0x0002) >> 1)
#define RFFC_WRITE_SDI_CTRL_RESET(data_byte, value) ((data_byte & (~0x0002)) | ((value & 0x01) << 1))

// Reg name : TEST(DEF : 0x0005)
#define RFFC_READ_TEST_TEN(data_byte) (data_byte >> 15)
#define RFFC_WRITE_TEST_TEN(data_byte, value) ((data_byte & 0x7FFF) | ((value & 0x01) << 15))
#define RFFC_READ_TEST_TMUX(data_byte) ((data_byte & 0x7000) >> 12)
#define RFFC_WRITE_TEST_TMUX(data_byte, value) ((data_byte & (~0x7000)) | ((value & 0x07) << 12))
#define RFFC_READ_TEST_TSEL(data_byte) ((data_byte & 0x0C00) >> 10)
#define RFFC_WRITE_TEST_TSEL(data_byte, value) ((data_byte & (~0x0C00)) | ((value & 0x03) << 10))
#define RFFC_READ_TEST_LFSR(data_byte) ((data_byte & 0x0200) >> 9)
#define RFFC_WRITE_TEST_LFSR(data_byte, value) ((data_byte & (~0x0200)) | ((value & 0x01) << 9))
#define RFFC_READ_TEST_LFSRP(data_byte) ((data_byte & 0x0100) >> 8)
#define RFFC_WRITE_TEST_LFSRP(data_byte, value) ((data_byte & (~0x0100)) | ((value & 0x01) << 8))
#define RFFC_READ_TEST_LFSRGATETIME(data_byte) ((data_byte & 0x00F0) >> 4)
#define RFFC_WRITE_TEST_LFSRGATETIME(data_byte, value) ((data_byte & (~0x00F0)) | ((value & 0x0F) << 4))
#define RFFC_READ_TEST_LFSRT(data_byte) ((data_byte & 0x0008) >> 3)
#define RFFC_WRITE_TEST_LFSRT(data_byte, value) ((data_byte & (~0x0008)) | ((value & 0x01) << 3))
#define RFFC_READ_TEST_RGBYP(data_byte) ((data_byte & 0x0004) >> 2)
#define RFFC_WRITE_TEST_RGBYP(data_byte, value) ((data_byte & (~0x0004)) | ((value & 0x01) << 2))
#define RFFC_READ_TEST_RCBYP(data_byte) ((data_byte & 0x0002) >> 1)
#define RFFC_WRITE_TEST_RCBYP(data_byte, value) ((data_byte & (~0x0002)) | ((value & 0x01) << 1))

enum PrescType
{
    PRESC_2 = 1,
    PRESC_4 = 2,
};

enum LoDivType
{
    LO_DIV_1 = 0,
    LO_DIV_2 = 1,
    LO_DIV_4 = 2,
    LO_DIV_8 = 3,
    LO_DIV_16 = 4,
    LO_DIV_32 = 5,
};

#define RFFC_MULTI_SLICE_ADDR1 0    // ADD1(A5) : 0, ADD2(A6) : 0     <=== TX
#define RFFC_MULTI_SLICE_ADDR2 2    // ADD1(A5) : 0, ADD2(A6) : 1     <=== RX
#define RFFC_MULTI_SLICE_ADDR3 1    // ADD1(A5) : 1, ADD2(A6) : 0
#define RFFC_MULTI_SLICE_ADDR4 3    // ADD1(A5) : 1, ADD2(A6) : 1

static uint8_t curr_mult_slice_addr = RFFC_MULTI_SLICE_ADDR1;

// *****************************

// default register settings(reference integrated_synthesizer_mixer_register_map_programming_guide.pdf).
static uint16_t lf_byte = 0xBEFA;
static uint16_t vco_ctrl_byte = 0x2D02;
static uint16_t ct_cal1_byte = 0xACBF;
static uint16_t ct_cal2_byte = 0xACBF;
static uint16_t pll_cal1_byte = 0x0028;
static uint16_t pll_cal2_byte = 0x0028;
static uint16_t vco_auto_byte = 0xFF00;
static uint16_t pll_ctrl_byte = 0x8220;
static uint16_t mix_cont_byte = 0x4800;
static uint16_t p1_freq1_byte = 0x1A94;
static uint16_t p1_freq2_byte = 0xD89D;
static uint16_t p1_freq3_byte = 0x8900;
static uint16_t p2_freq1_byte = 0x1E84;
static uint16_t p2_freq2_byte = 0x89D8;
static uint16_t p2_freq3_byte = 0x9D00;
static uint16_t sdi_ctrl_byte = 0x0000;
static uint16_t test_byte = 0x0005;

// For Extend variable(request by other)
uint8_t g_fsk_idd = FSK_IDD;
uint8_t g_fsk_tc = FSK_TC;

// *****************************


// *****************************

#define DUAL_CHANNEL
// *****************************

#define FSK_CHANNEL_6

#define CHIP_FREQ 915.7     // MHz
#ifdef DUAL_CHANNEL
#define TX_FREQ 918.2     // MHz
#endif

// *****************************


// ******* Cmd Operator ********

#ifdef RaspberryPi

static void gpio_delay()
{
    struct timespec ts = { 0, 30 };
    nanosleep(&ts, NULL);
}

static void setGpioCmd(char* buf, int size)
{
    gpiod_line_release(dat_line);
    gpiod_line_request_output(dat_line, "dat", 0);

    gpiod_line_set_value(clk_line, 1);
    gpio_delay();
    gpiod_line_set_value(enx_line, 0);
    for (int i = 0; i < size; i++)
    {
        gpiod_line_set_value(clk_line, 0);
        gpiod_line_set_value(dat_line, buf[i]);
        gpio_delay();
	    gpiod_line_set_value(clk_line, 1);
        gpio_delay();
    }
    gpiod_line_set_value(enx_line, 1);
    gpiod_line_set_value(clk_line, 0);
    gpiod_line_release(dat_line);
    gpio_delay();
    gpiod_line_set_value(clk_line, 1);
    gpio_delay();
    gpiod_line_set_value(clk_line, 0);
}

static void getGpioCmd(uint8_t *cmd, int size, uint16_t *data)
{
    gpiod_line_release(dat_line);
    gpiod_line_request_output(dat_line, "dat", 0);

    gpiod_line_set_value(clk_line, 1);
    gpio_delay();
    gpiod_line_set_value(enx_line, 0);
    for (int i = 0; i < 9; i++)
    {
        gpiod_line_set_value(clk_line, 0);
        gpiod_line_set_value(dat_line, cmd[i]);
        gpio_delay();
	    gpiod_line_set_value(clk_line, 1);
        gpio_delay();
    }
    gpiod_line_set_value(clk_line, 0);
    gpiod_line_release(dat_line);
    gpio_delay();
    
    gpiod_line_request_input(dat_line, "dat");
    
    gpiod_line_set_value(clk_line, 1);
    gpio_delay();
    gpiod_line_set_value(clk_line, 0);
    gpio_delay();
    gpiod_line_set_value(clk_line, 1);
    gpio_delay();

    uint16_t read_data = 0;
    for (int i = 0; i < 16; i++) { //Read 16 bits
        gpiod_line_set_value(clk_line, 0);
	    int bit = gpiod_line_get_value(dat_line);
	    read_data = read_data | (bit << (15 - i));
        gpio_delay();
        if(i == 15) {
            gpiod_line_set_value(enx_line, 1);
        }
        gpiod_line_set_value(clk_line, 1);
        gpio_delay();
    }
    gpiod_line_set_value(clk_line, 0);
    *data = read_data;
    gpiod_line_release(dat_line);
    gpio_delay();
    gpiod_line_set_value(clk_line, 1);
    gpio_delay();
    gpiod_line_set_value(clk_line, 0);
}


#else
static void setGpioEnbable(int enbable)
{
    if (enbable) HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);       // NSS set low
    else HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);       // NSS set high
}

static void setGpioClock(int clock)
{
    if (clock) HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);         // clock up
    else HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);         // clock down
}

static void setGpioData(int data)
{
    if (data) HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);   // data 1
    else HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);      // data 0
}

static uint16_t getGpioData()
{
    GPIO_PinState res = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_5);
    if (res == GPIO_PIN_RESET) return 0;
    else return 1;
}

static void setGpioCmd(char* buf, int size)
{
    // clock(low), enable(high), data(no set)

    setGpioClock(true);     // clock(low t0 => [t1/2] => high t0)
    // clock(high t0)

    setGpioEnbable(true);   // enable(high => [t1/2] => low)
    // clock(high t1/2), enable(low)

    setGpioClock(false);    // clock(high t1/2 => [t1/2] => high t1 & low t0)
    // clock(low t0)

    setGpioEnbable(true);   // extra interval
    // clock(low t1/2)

    int i;
    for (i = 0; i < size; i++)
    {
        // clock(low t1/2 or high t1/2)
        setGpioClock(false);    // clock(high t1/2 => [t1/2] => low t0)
        // clock(low t0)

        setGpioData(buf[i]);    // data(data no t0 => [t1/2] => data yes t0)
        // clock(low t0 => low t1/2), data(data t0)

        setGpioClock(true);     // clock(low t1/2 => [t1/2] => low t1 & high t0)
        // clock(high t0, data(data t1/2)

        if (i != (size - 1))
            setGpioEnbable(true);   // extra interval
        else 
            setGpioEnbable(false);   // extra interval
        // clock(high t1/2, data(data t1)
    }

    setGpioClock(false);    // clock(high t1/2 => [t1/2] => low t0)
    // clock(low t0)

    setGpioData(0);         // data return to default(0)
    // clock(low t1/2)

    setGpioClock(true);     // clock(low t1/2 => [t1/2] => low t1 & high t0)
    // clock(high t0)

    setGpioEnbable(false);   // extra interval
    // clock(high t1/2)

    setGpioClock(false);    // clock(high t1/2 => [t1/2] => high t1 & low t0)
    // clock(low t0)

    // time interval between 3 byte.
    HAL_Delay(20);
}

static void getGpioCmd(char* buf, int size, uint16_t* data)
{
    // init read value
    *data = 0;

    // clock(low), enable(high), data(no set).

    setGpioClock(true);     // clock(low t0 => [t1/2] => high t0)
    // clock(high t0)

    setGpioEnbable(true);   // enable(high => [t1/2] => low)
    // clock(high t1/2), enable(low)

    setGpioClock(false);    // clock(high t1/2 => [t1/2] => high t1 & low t0)
    // clock(low t0)

    setGpioEnbable(true);   // extra interval
    // clock(low t1/2)

    int i;
    for (i = 0; i < size; i++)
    {
        // clock(low t1/2 or high t1/2)
        setGpioClock(false);    // clock(high t1/2 => [t1/2] => low t0)
        // clock(low t0)

        setGpioData(buf[i]);    // data(data no t0 => [t1/2] => data yes t0)
        // clock(low t0 => low t1/2), data(data t0)

        setGpioClock(true);     // clock(low t1/2 => [t1/2] => low t1 & high t0)
        // clock(high t0, data(data t1/2)

        setGpioEnbable(true);   // extra interval
        // clock(high t1/2, data(data t1)
    }

    // wait time
    HAL_Delay(10);
    setGpioData(1);     // default data to high
    HAL_Delay(5);
    // clock(high t0)

    // 1.5 clock
    setGpioClock(false);    // clock(high t0 => low t0)
    // clock(low t0)
    setGpioEnbable(true);   // extra interval
    // clock(low t1/2)
    setGpioClock(true);    // clock(low t1/2 => [t1/2] => low t1 & high t0)
    // clock(high t0)
    setGpioEnbable(true);   // extra interval
    // clock(high t1/2)
    setGpioClock(false);    // clock(high t1/2 => [t1/2] => high t1 & low t0)
    // clock(low t0)
    setGpioEnbable(true);   // extra interval
    // clock(low t1/2)
    setGpioClock(true);    // clock(low t1/2 => [t1/2] => low t1 & high t0)
    // clock(high t0)

    for (i = 0; i < 16; i++) {
        setGpioEnbable(true);   // extra interval
        // clock(high t1/2)

        setGpioClock(false);    // clock(high t0 => low t0)
        // clock(low t0)

        uint16_t value = getGpioData();  // clock(low t0 => [t1/2] => low t1/2)
        *data = *data | (value << (15 - i));
        // clock(low t1/2)

        setGpioClock(true);    // clock(low t1/2 => [t1/2] => high t0)
        // clock(high t0)
    }

    setGpioEnbable(true);   // extra interval
    // clock(high t1/2)
    setGpioClock(false);    // clock(high t0 => low t0)
    // clock(low t0)
    setGpioEnbable(false);   // disable(low t0 => [t1/2] => low t1/2)
    // clock(low t1/2)
    setGpioClock(true);    // clock(low t1/2 => [t1/2] => high t0)
    // clock(high t0)
    setGpioData(0);         // data return to default(0)
    // clock(high t1/2)
    setGpioClock(false);    // clock(high t1/2 => [t1/2] => high t1 & low t0)
    // clock(low t0)

    // time interval between 3 byte.
    HAL_Delay(20);
}
#endif

// data send handle.
void rffc2071a_send(const uint8_t register_addr, const uint16_t data)
{
    uint8_t i, j;
    uint8_t address = register_addr | ((curr_mult_slice_addr & 0x03) << 5);
    char cmd[25];
    cmd[0] = 0;

    j = 1;
    for (i = 8; i > 0; i--, j++) {
        if (((address & (1 << (i - 1))) >> (i - 1)) > 0)
            cmd[j] = 1;
        else
            cmd[j] = 0;
    }
    j = 9;
    for (i = 16; i > 0; i--, j++) {
        if (((data & (1 << (i - 1))) >> (i - 1)) > 0)
            cmd[j] = 1;
        else
            cmd[j] = 0;
    }

    setGpioCmd(cmd, 25);
}

// data recv handle.
uint16_t rffc2071a_recv(const uint8_t register_addr, const uint8_t multi_slice_addr)
{
    uint8_t i, j;
    uint8_t address = register_addr | ((multi_slice_addr & 0x03) << 5);
    char cmd[9];
    cmd[0] = 0;
    uint16_t data;

    j = 1;
    for (i = 8; i > 0; i--, j++) {
        if (((address & (1 << (i - 1))) >> (i - 1)) > 0)
            cmd[j] = 1;
        else
            cmd[j] = 0;
    }
    cmd[1] = 1;     // R

    getGpioCmd(cmd, 9, &data);

    return data;
}

// *****************************


// ***** Parameter handle ******

static double get_clock_generator_lo(const double source_freq, const double target_freq, const bool large_lo)
{
    double lo = 0;

    if (large_lo) lo = source_freq + target_freq;
    else {
        if (source_freq > target_freq) lo = source_freq - target_freq;
        else lo = target_freq - source_freq;
        if (lo < 85.0) lo = source_freq + target_freq;
    }

    return lo;
}

static uint8_t get_fbkdiv(const double lo)
{
    int n_lo = log2(5400.0 / lo);
    uint8_t lodiv = (uint8_t)pow(2, n_lo);
    double fvco = lodiv * lo;

    if (fvco > 3300.0) return PRESC_4;
    else return PRESC_2;
}

static void get_freq_parameter(const double lo, uint8_t* lodiv, uint8_t* n, uint16_t* nummsb, uint8_t* numlsb)
{
    int n_lo = log2(5400.0 / lo);
    uint8_t lodiv_value = (uint8_t)pow(2, n_lo);
    double fvco = lodiv_value * lo;
    uint8_t fbkdiv;
    if (fvco > 3300.0) fbkdiv = 4;
    else fbkdiv = 2;
    double n_div = fvco / (fbkdiv * 26.0);

    // transform value to enum
    switch (lodiv_value) {
    case 1:
        *lodiv = LO_DIV_1;
        break;
    case 2:
        *lodiv = LO_DIV_2;
        break;
    case 4:
        *lodiv = LO_DIV_4;
        break;
    case 8:
        *lodiv = LO_DIV_8;
        break;
    case 16:
        *lodiv = LO_DIV_16;
        break;
    case 32:
        *lodiv = LO_DIV_32;
        break;
    }
    *n = (uint8_t)n_div;
    *nummsb = (65536.0 * (n_div - *n));
    *numlsb = (256.0 * ((65536.0 * (n_div - *n)) - *nummsb));
}

// *****************************


// ******* Init Process ********

static void init_register_rerord()
{
    lf_byte = 0xBEFA;
    vco_ctrl_byte = 0x2D02;
    ct_cal1_byte = 0xACBF;
    ct_cal2_byte = 0xACBF;
    pll_cal1_byte = 0x0028;
    pll_cal2_byte = 0x0028;
    vco_auto_byte = 0xFF00;
    pll_ctrl_byte = 0x8220;
    mix_cont_byte = 0x4800;
    p1_freq1_byte = 0x1A94;
    p1_freq2_byte = 0xD89D;
    p1_freq3_byte = 0x8900;
    p2_freq1_byte = 0x1E84;
    p2_freq2_byte = 0x89D8;
    p2_freq3_byte = 0x9D00;
    sdi_ctrl_byte = 0x0000;
    test_byte = 0x0005;
}

static void rffc2071a_set_up_dev_oper()
{
    /*
        process reference [RFFC2071A/integrated_synthesizer_mixer_programming_guide.pdf], page 6
    */

    /* 1. init REGISTER */

    // P2_FREQ1 : p2vcosel = 0
    p2_freq1_byte = RFFC_WRITE_P2_FREQ1_P2VCOSEL(p2_freq1_byte, 0);
    rffc2071a_send(RFFC_REG_ADDR_P2_FREQ1, p2_freq1_byte);

    // VCO_AUTO : ctmin = 0, ctmax = 127
    vco_auto_byte = RFFC_WRITE_VCO_AUTO_CTMIN(vco_auto_byte, 0);
    vco_auto_byte = RFFC_WRITE_VCO_AUTO_CTMAX(vco_auto_byte, 127);
    rffc2071a_send(RFFC_REG_ADDR_VCO_AUTO, vco_auto_byte);

    // CT_CAL1 : p1ctv = 12
    ct_cal1_byte = RFFC_WRITE_CT_CAL1_P1CTV(ct_cal1_byte, 12);
    rffc2071a_send(RFFC_REG_ADDR_CT_CAL1, ct_cal1_byte);

    // CT_CAL2 : p2ctv = 12
    ct_cal2_byte = RFFC_WRITE_CT_CAL2_P2CTV(ct_cal2_byte, 12);
    rffc2071a_send(RFFC_REG_ADDR_CT_CAL2, ct_cal2_byte);

    // TEST : rgbyp = 1
    test_byte = RFFC_WRITE_TEST_RGBYP(test_byte, 1);
    rffc2071a_send(RFFC_REG_ADDR_TEST, test_byte);

    /* 
        A SERIES(ex : RFFC2071A) : VCO TEMPERATURE COMPENSATION 
        
        reference [RFFC2071A/integrated_synthesizer_mixer_programming_guide.pdf], page 12
    */

    // Set icpup = 3 in VCO_CTRL register, charge pump up enable
    vco_ctrl_byte = RFFC_WRITE_VCO_CTRL_ICPUP(vco_ctrl_byte, (g_fsk_tc ? 3 : 1));
    rffc2071a_send(RFFC_REG_ADDR_VCO_CTRL, vco_ctrl_byte);

    // Set ldlev = 1 in PLL_CTRL register, sets wide lock detect range
    pll_ctrl_byte = RFFC_WRITE_PLL_CTRL_LDLEV(pll_ctrl_byte, (g_fsk_tc ? 1 : 0));
    rffc2071a_send(RFFC_REG_ADDR_PLL_CTRL, pll_ctrl_byte);

    /* 2. set chip with 3-wire bus or not */

    // SDI_CTRL : sipin = 1
    sdi_ctrl_byte = RFFC_WRITE_SDI_CTRL_SIPIN(sdi_ctrl_byte, 1);
    rffc2071a_send(RFFC_REG_ADDR_SDI_CTRL, sdi_ctrl_byte);

    /* 3. Enable MultSlice mode or not */
    // SDI_CTRL : addr = 1
    sdi_ctrl_byte = RFFC_WRITE_SDI_CTRL_ADDR(sdi_ctrl_byte, 1);
    rffc2071a_send(RFFC_REG_ADDR_SDI_CTRL, sdi_ctrl_byte);

}

static void rffc2071a_set_up_addition_feature()
{
    /*
        process reference [RFFC2071A/integrated_synthesizer_mixer_programming_guide.pdf], page 7
    */

    /* 1. general purpose outputs or not */
    //  Default(Disable six general purpose outputs)

    /* 2. Lock output signal or not */
    //  Default(dont lock general purpose outputs)

    /* 3. 4-wire programming or not */
    //  Default(request dont need 4-wire)

    /* 4. Frequency modulator or not */
    //  Default(dont need change VCO freq by programming or GPOs)
}

static void rffc2071a_set_oper_freq(const bool tx, const bool large_lo, const double source_freq, const double target_freq)
{
    /*
        process reference [RFFC2071A/integrated_synthesizer_mixer_programming_guide.pdf], page 8
    */

    // get LO
    double lo = get_clock_generator_lo(source_freq, target_freq, large_lo);

    /* 1. Auto VCO select or not */
    //  Default(select auto VCO select)

    /* 2. Auto CT_cal or not */
    //  Default(select auto CT_cal)

    /* 3. Is fVCO > 3.2GHz or not, yse : presc to 4 & LF:pllcpl to 3, no : presc to 2 */

    // Get fbkdiv
    uint8_t fbkdiv = get_fbkdiv(lo);

    if (tx) {
        // P1_FREQ1:p1presc
        p1_freq1_byte = RFFC_WRITE_P1_FREQ1_P1PRESC(p1_freq1_byte, fbkdiv);
        rffc2071a_send(RFFC_REG_ADDR_P1_FREQ1, p1_freq1_byte);
    }
    else {
        // P2_FREQ1:p2presc
        p2_freq1_byte = RFFC_WRITE_P2_FREQ1_P2PRESC(p2_freq1_byte, fbkdiv);
        rffc2071a_send(RFFC_REG_ADDR_P2_FREQ1, p2_freq1_byte);
    }

    if (fbkdiv == PRESC_4) {
        lf_byte = RFFC_WRITE_LF_PLLCPL(lf_byte, 3);
        rffc2071a_send(RFFC_REG_ADDR_LF, lf_byte);
    }
    else {
        // LF:PLLCPL return to default value
        lf_byte = RFFC_WRITE_LF_PLLCPL(lf_byte, 2);
        rffc2071a_send(RFFC_REG_ADDR_LF, lf_byte);
    }

    /* 4. Program */

    // P1_FREQn.p1n, p1lodiv, p1nmsb, p1nlsb
    // P2_FREQn.p2n, p2lodiv, p2nmsb, p2nlsb
    uint8_t lodiv;
    uint8_t n;
    uint16_t nummsb;
    uint8_t numlsb;

    // get frequency parameter
    get_freq_parameter(lo, &lodiv, &n, &nummsb, &numlsb);

    if (tx) {
        // P1_FREQ1
        p1_freq1_byte = RFFC_WRITE_P1_FREQ1_P1LODIV(p1_freq1_byte, lodiv);
        p1_freq1_byte = RFFC_WRITE_P1_FREQ1_P1N(p1_freq1_byte, n);
        rffc2071a_send(RFFC_REG_ADDR_P1_FREQ1, p1_freq1_byte);

        // P1_FREQ2
        rffc2071a_send(RFFC_REG_ADDR_P1_FREQ2, nummsb);
        p1_freq2_byte = nummsb;

        // P1_FREQ3
        p1_freq3_byte = RFFC_WRITE_P1_FREQ3_P1NLSB(p1_freq3_byte, numlsb);
        rffc2071a_send(RFFC_REG_ADDR_P1_FREQ3, p1_freq3_byte);
    }
    else {
        // P2_FREQ1
        p2_freq1_byte = RFFC_WRITE_P2_FREQ1_P2LODIV(p2_freq1_byte, lodiv);
        p2_freq1_byte = RFFC_WRITE_P2_FREQ1_P2N(p2_freq1_byte, n);
        rffc2071a_send(RFFC_REG_ADDR_P2_FREQ1, p2_freq1_byte);

        // P2_FREQ2
        rffc2071a_send(RFFC_REG_ADDR_P2_FREQ2, nummsb);
        p2_freq2_byte = nummsb;

        // P2_FREQ3
        p2_freq3_byte = RFFC_WRITE_P2_FREQ3_P2NLSB(p2_freq3_byte, numlsb);
        rffc2071a_send(RFFC_REG_ADDR_P2_FREQ3, p2_freq3_byte);
    }
}

static void rffc2071a_set_loop_filter_cali_mode(const bool large_lo)
{
    /*
        process reference [RFFC2071A/integrated_synthesizer_mixer_programming_guide.pdf], page 10
    */

    /* 1. Enable loop filter cal or not */

    // PLL_CAL1 : p1kv = 0, 
    pll_cal1_byte = RFFC_WRITE_PLL_CAL1_P1KV(pll_cal1_byte, 0);
    rffc2071a_send(RFFC_REG_ADDR_PLL_CAL1, pll_cal1_byte);

    // PLL_CAL2 : p2kv = 0
    pll_cal2_byte = RFFC_WRITE_PLL_CAL2_P2KV(pll_cal2_byte, 0);
    if (large_lo == false) pll_cal2_byte = RFFC_WRITE_PLL_CAL2_P2SGN(pll_cal2_byte, 1);   // PLL_CAL2 : p2sgn = 1
    rffc2071a_send(RFFC_REG_ADDR_PLL_CAL2, pll_cal2_byte);
}

static void rffc2071a_enable_device(bool tx)
{
    /*
        process reference [RFFC2071A/integrated_synthesizer_mixer_programming_guide.pdf], page 11
    */

    // 1. SDI_CTRL : sipin = 1
    sdi_ctrl_byte = RFFC_WRITE_SDI_CTRL_SIPIN(sdi_ctrl_byte, 1);

    // 2. set MODE & ENBL pin
    sdi_ctrl_byte = RFFC_WRITE_SDI_CTRL_ENBL(sdi_ctrl_byte, 1);
    if (tx) sdi_ctrl_byte = RFFC_WRITE_SDI_CTRL_MODE(sdi_ctrl_byte, 0); // 0 : Mixer1(TX)
    else sdi_ctrl_byte = RFFC_WRITE_SDI_CTRL_MODE(sdi_ctrl_byte, 1); // 1 : Mixer2(RX)

    rffc2071a_send(RFFC_REG_ADDR_SDI_CTRL, sdi_ctrl_byte);
}

// *****************************


// ******** FSK Handle *********

static bool check_fsk_number(const uint8_t fsk_num)
{
    // check parameter
#ifdef FSK_CHANNEL_6
    if (fsk_num >= 6) return false;
#else
    if (fsk_num >= 8) return false;
#endif
    else return true;
}

static double get_target_freq(const bool tx, const uint8_t fsk_num)
{
    double freq = 0;

#if defined(FSK_CHANNEL_6)
    switch (fsk_num) {
    case 0:
        if (tx) freq = FSK_CH6_TX_1;
        else freq = FSK_CH6_RX_1;
        break;
    case 1:
        if (tx) freq = FSK_CH6_TX_2;
        else freq = FSK_CH6_RX_2;
        break;
    case 2:
        if (tx) freq = FSK_CH6_TX_3;
        else freq = FSK_CH6_RX_3;
        break;
    case 3:
        if (tx) freq = FSK_CH6_TX_4;
        else freq = FSK_CH6_RX_4;
        break;
    case 4:
        if (tx) freq = FSK_CH6_TX_5;
        else freq = FSK_CH6_RX_5;
        break;
    case 5:
        if (tx) freq = FSK_CH6_TX_6;
        else freq = FSK_CH6_RX_6;
        break;
    }
#else
    switch (fsk_num) {
    case 0:
        if (tx) freq = FSK_CH8_TX_1;
        else freq = FSK_CH8_RX_1;
        break;
    case 1:
        if (tx) freq = FSK_CH8_TX_2;
        else freq = FSK_CH8_RX_2;
        break;
    case 2:
        if (tx) freq = FSK_CH8_TX_3;
        else freq = FSK_CH8_RX_3;
        break;
    case 3:
        if (tx) freq = FSK_CH8_TX_4;
        else freq = FSK_CH8_RX_4;
        break;
    case 4:
        if (tx) freq = FSK_CH8_TX_5;
        else freq = FSK_CH8_RX_5;
        break;
    case 5:
        if (tx) freq = FSK_CH8_TX_6;
        else freq = FSK_CH8_RX_6;
        break;
    case 6:
        if (tx) freq = FSK_CH8_TX_7;
        else freq = FSK_CH8_RX_7;
        break;
    case 7:
        if (tx) freq = FSK_CH8_TX_8;
        else freq = FSK_CH8_RX_8;
        break;
    }
#endif

    return freq;
}

// *****************************
void printAllReg(bool tx){
    uint16_t data;
    printf("=============================================\n");
    for (int i = 0; i < 0x1F; i++) {
        data = rffc2071a_recv(i, tx ? RFFC_MULTI_SLICE_ADDR1 : RFFC_MULTI_SLICE_ADDR2);
        printf("%s_Reg: 0x%02X, Data: 0x%04X\n", (tx?"TX":"RX"), i, data);
        usleep(100);
    }
    printf("=============================================\n");
}

// ******* API interface *******

bool rffc2071a_init(const bool tx, const bool large_lo, const uint8_t fsk_num)
{
    // set A5 & A6 to assign rffc chip
    if (tx) curr_mult_slice_addr = RFFC_MULTI_SLICE_ADDR1;
    else curr_mult_slice_addr = RFFC_MULTI_SLICE_ADDR2;

    /* 4. Enable duplex operation or not */
    //  Default(No Need duplex because T/R Switch)

    /* 5. Program MIX_CONT : p1mixidd & p2mixidd */

    // p1mixidd & p2mixidd set lowest value
    mix_cont_byte = RFFC_WRITE_MIX_CONT_P1MIXIDD(mix_cont_byte, g_fsk_idd);
    mix_cont_byte = RFFC_WRITE_MIX_CONT_P2MIXIDD(mix_cont_byte, g_fsk_idd);
    rffc2071a_send(RFFC_REG_ADDR_MIX_CONT, mix_cont_byte);

    /*
        this process reference [integrated_synthesizer_mixer_programming_guide.pdf]
    */

#ifdef RaspberryPi
    usleep(100 * 1000);
#else
    HAL_Delay(100);
#endif

    // check parameter
    if (check_fsk_number(fsk_num) == false) {
#ifdef RaspberryPi
        printf("[%s:%d] fsk_num parameter error\n", __FUNCTION__, __LINE__);
#else
        APP_LOG(0, VLEVEL_ALWAYS, "[%s:%d] fsk_num parameter error\n", __FUNCTION__, __LINE__);
#endif
        return false;
    }

    // get target frequency
    double target_freq = get_target_freq(tx, fsk_num);

    /*
        process reference [RFFC2071A/integrated_synthesizer_mixer_programming_guide.pdf], page 5
    */
    // handle [Set-up additional features]
    rffc2071a_set_up_addition_feature();

    // handle [Set operating frequencies]
#ifdef DUAL_CHANNEL
    if(tx)
        rffc2071a_set_oper_freq(tx, large_lo, TX_FREQ, target_freq);
    else
        rffc2071a_set_oper_freq(tx, large_lo, CHIP_FREQ, target_freq);
#else
    rffc2071a_set_oper_freq(tx, large_lo, CHIP_FREQ, target_freq);
#endif   

    // handle [Set loop filter calibration mode]
    rffc2071a_set_loop_filter_cali_mode(large_lo);

    // handle [Enable Device]
    rffc2071a_enable_device(tx);

    return true;
}

bool rffc2071a_init2(const bool tx, const bool large_lo, const double chip_freq_mhz, const double target_freq_mhz)
{
    /*
        this process reference [integrated_synthesizer_mixer_programming_guide.pdf]
    */

#ifdef RaspberryPi
    usleep(100 * 1000);
#else
    HAL_Delay(100);
#endif

    // check parameter
    if (chip_freq_mhz <= 0) {
#ifdef RaspberryPi
        printf("[%s:%d] target_freq_mhz <= 0\n", __FUNCTION__, __LINE__);
#else
        APP_LOG(0, VLEVEL_ALWAYS, "[%s:%d] chip_freq_mhz <= 0\n", __FUNCTION__, __LINE__);
#endif
        return false;
    }
    if (target_freq_mhz <= 0) {
#ifdef RaspberryPi
        printf("[%s:%d] target_freq_mhz <= 0\n", __FUNCTION__, __LINE__);
#else
        APP_LOG(0, VLEVEL_ALWAYS, "[%s:%d] target_freq_mhz <= 0\n", __FUNCTION__, __LINE__);
#endif
        return false;
    }

    /*
        process reference [RFFC2071A/integrated_synthesizer_mixer_programming_guide.pdf], page 5
    */

    // handle [Set-up device operator]
    rffc2071a_set_up_dev_oper(tx);

    // handle [Set-up additional features]
    rffc2071a_set_up_addition_feature();

    // handle [Set operating frequencies]
    rffc2071a_set_oper_freq(tx, large_lo, chip_freq_mhz, target_freq_mhz);

    // handle [Set loop filter calibration mode]
    rffc2071a_set_loop_filter_cali_mode(large_lo);

    // handle [Enable Device]
    rffc2071a_enable_device(tx);

    return true;
}

void rffc2071a_fsk_changed(const bool tx, const bool large_lo, const uint8_t fsk_num)
{
    // get target frequency
    double target_freq = get_target_freq(tx, fsk_num);

    // set A5 & A6 to assign rffc chip
    if (tx) curr_mult_slice_addr = RFFC_MULTI_SLICE_ADDR1;
    else curr_mult_slice_addr = RFFC_MULTI_SLICE_ADDR2;

    // set freq
    rffc2071a_set_oper_freq(tx, large_lo, CHIP_FREQ, target_freq);
    
    // set freq sgn
    if (tx == false) {
        if (large_lo) pll_cal2_byte = RFFC_WRITE_PLL_CAL2_P2SGN(pll_cal2_byte, 0);
        else pll_cal2_byte = RFFC_WRITE_PLL_CAL2_P2SGN(pll_cal2_byte, 1);
        
        rffc2071a_send(RFFC_REG_ADDR_PLL_CAL2, pll_cal2_byte);
    }

    // Mode select
    if (tx) sdi_ctrl_byte = RFFC_WRITE_SDI_CTRL_MODE(sdi_ctrl_byte, 0); // 0 : Mixer1(TX)
    else sdi_ctrl_byte = RFFC_WRITE_SDI_CTRL_MODE(sdi_ctrl_byte, 1); // 1 : Mixer2(RX)

    rffc2071a_send(RFFC_REG_ADDR_SDI_CTRL, sdi_ctrl_byte);

    // relock PLL
    uint16_t data = RFFC_WRITE_PLL_CTRL_RELOK(pll_ctrl_byte, 1);
    rffc2071a_send(RFFC_REG_ADDR_PLL_CTRL, data);
}

void rffc2071a_fsk_changed2(const bool tx, const bool large_lo, const double chip_freq_mhz, const double target_freq_mhz)
{
    // set A5 & A6 to assign rffc chip
    if (tx) curr_mult_slice_addr = RFFC_MULTI_SLICE_ADDR1;
    else curr_mult_slice_addr = RFFC_MULTI_SLICE_ADDR2;

    // set freq
    rffc2071a_set_oper_freq(tx, large_lo, chip_freq_mhz, target_freq_mhz);

    // set freq sgn
    if (tx == false) {
        if (large_lo) pll_cal2_byte = RFFC_WRITE_PLL_CAL2_P2SGN(pll_cal2_byte, 0);
        else pll_cal2_byte = RFFC_WRITE_PLL_CAL2_P2SGN(pll_cal2_byte, 1);

        rffc2071a_send(RFFC_REG_ADDR_PLL_CAL2, pll_cal2_byte);
    }

    // Mode select
    if (tx) sdi_ctrl_byte = RFFC_WRITE_SDI_CTRL_MODE(sdi_ctrl_byte, 0); // 0 : Mixer1(TX)
    else sdi_ctrl_byte = RFFC_WRITE_SDI_CTRL_MODE(sdi_ctrl_byte, 1); // 1 : Mixer2(RX)

    rffc2071a_send(RFFC_REG_ADDR_SDI_CTRL, sdi_ctrl_byte);

    // relock PLL
    uint16_t data = RFFC_WRITE_PLL_CTRL_RELOK(pll_ctrl_byte, 1);
    rffc2071a_send(RFFC_REG_ADDR_PLL_CTRL, data);
}

void rffc2071a_reset()
{
    uint16_t byte;
    byte = RFFC_WRITE_SDI_CTRL_RESET(0x0000, 1);    // Reset Pin
    curr_mult_slice_addr = RFFC_MULTI_SLICE_ADDR2;
    rffc2071a_send(RFFC_REG_ADDR_SDI_CTRL, byte);
    curr_mult_slice_addr = RFFC_MULTI_SLICE_ADDR1;
    rffc2071a_send(RFFC_REG_ADDR_SDI_CTRL, byte);
}

void rffc2071a_gw_init(const uint8_t fsk_num, const uint8_t customIDD)
{
    g_fsk_idd = customIDD;
    rffc2071a_reset();
    printf("setup ============================\n");
    rffc2071a_set_up_dev_oper();
    // init RFFC chip 1
    rffc2071a_init(true, false, fsk_num);
    // init RFFC chip 2
    rffc2071a_init(false, false, fsk_num);
    //printAllReg(true);
    //printAllReg(false);
}

void rffc2071a_gw_init2(const double us_freq_mhz, const double ds_freq_mhz)
{
    // init RFFC chip 1
    rffc2071a_init2(true, false, CHIP_FREQ, us_freq_mhz);
    // init RFFC chip 2
    rffc2071a_init2(false, false, CHIP_FREQ, ds_freq_mhz);
}

void rffc2071a_fsk_gw_changed(const uint8_t fsk_num)
{
    // change RFFC chip 1
    rffc2071a_fsk_changed(true, false, fsk_num);
    // change RFFC chip 2
    rffc2071a_fsk_changed(false, false, fsk_num);
}

void rffc2071a_fsk_gw_changed2(const double us_freq_mhz, const double ds_freq_mhz)
{
    // change RFFC chip 1
    rffc2071a_fsk_changed2(true, false, CHIP_FREQ, us_freq_mhz);
    // change RFFC chip 2
    rffc2071a_fsk_changed2(false, false, CHIP_FREQ, ds_freq_mhz);
}

// *****************************

void disableMultiSlice()
{
    // disable multi slice mode
    sdi_ctrl_byte = RFFC_WRITE_SDI_CTRL_ADDR(sdi_ctrl_byte, 0);
    curr_mult_slice_addr = RFFC_MULTI_SLICE_ADDR2;
    rffc2071a_send(RFFC_REG_ADDR_SDI_CTRL, sdi_ctrl_byte);
    curr_mult_slice_addr = RFFC_MULTI_SLICE_ADDR1;
    rffc2071a_send(RFFC_REG_ADDR_SDI_CTRL, sdi_ctrl_byte);
}