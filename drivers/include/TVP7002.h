#ifndef TVP7002_H
#define TVP7002_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>


#include "i2c_driver.h"
#include "memory_access.h"

#define tvp7002_i2c_addr (0xB8>>1)

//tvp 7002 registers
#define tvp7002_h_pll_msb 0x01
#define tvp7002_sync_on_green_threshold 0x10
#define tvp7002_sync_control_1 0x0E
#define tvp7002_h_pll_and_clamp_control 0x0F
#define tvp7002_misc_control_1 0x16
#define tvp7002_misc_control_2 0x17
#define tvp7002_misc_control_3 0x18
#define tvp7002_input_mux_sel_1 0x19
#define tvp7002_inp_mux_sel_2  0x1A
#define tvp7002_b_g_coarse_gain  0x1B
#define tvp7002_r_coarse_gain  0x1C
#define tvp7002_r_coarse_offset  0x20
#define tvp7002_g_coarse_offset  0x1F
#define tvp7002_b_coarse_offset  0x1E
#define tvp7002_alc_enable 0x26
#define tvp7002_sync_bypass 0x36
#define tvp7002_power_control 0x2B
#define tvp7002_red_coarse_gain 0x1C
#define tvp7002_ADC_setup 0x2C
#define tvp7002_b_fine_gain 0x08
#define tvp7002_g_fine_gain 0x09
#define tvp7002_r_fine_gain 0x0A

#define tvp7002_b_fine_offset_MSB 0x0B
#define tvp7002_g_fine_offset_MSB 0x0C
#define tvp7002_r_fine_offset_MSB 0x0D
#define tvp7002_fine_offset_LSB   0x1D

//axi gpio registers
#define AXI_GPIO1_DATA_reg 0x00
#define AXI_GPIO1_TRI_reg  0x04



class TVP7002
{
	private:
	bool initialized;
	i2c_driver *i2c;
	uint8_t bus;
	uint8_t tvp_addr;
	//memory_access mem_gpio;
	
	uint8_t bg_coarse_gain_reg_val;
	uint8_t fine_offset_LSB_reg_val;
	uint8_t input_mux_reg_val;
	
	public:
	
	TVP7002();
	~TVP7002();
	
	int init(i2c_driver *i2c_, uint8_t bus, const char *XDMA_bypass_dev, bool reset_first);
	void hard_reset();
	
	int set_default_config();
	
	int set_coarse_gain(uint8_t channel, float gain);
	int set_coarse_offset(uint8_t channel, int value);
	int set_fine_offset(uint8_t channel, int value);
	int set_input_mux(uint8_t channel, uint8_t input);
	int set_fine_gain(uint8_t channel, double gain);
	
	
	
	
};





















#endif //TVP7002_H