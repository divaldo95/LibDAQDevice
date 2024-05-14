#ifndef calibration_module_H
#define calibration_module_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>

#include "memory_access.h"
#include "TVP7002.h"
#include "sipm_daq_pair_driver.h"




//axi gpio registers
#define AXI_GPIO1_DATA_reg 0x00
#define AXI_GPIO1_TRI_reg  0x04



class calibration_module
{
	private:
	bool initialized;
	
	uint32_t DAQ_event_count;
	uint32_t DAQ_event_samples;
	double ADC_FS_voltage;

	TVP7002 *adc;
	sipm_daq_pair_driver *daq_pair;
	
	memory_access mem_calib_vsel_gpio;
	
	uint16_t *waveform_buffers[9];
	
	double offests[9];
	double gain_errors[9];
	
	
	
	public:
	
	calibration_module();
	~calibration_module();
	
	int init( const char *XDMA_bypass_dev, uint32_t calib_sel_gpio_base_addr, TVP7002 adc_[3], sipm_daq_pair_driver daq_pair_[5], double ADC_FS_voltage_);
	
	void init_daq_pairs();
	
	void select_calibration_voltage(uint8_t sel);
	
	
	int measure();
	void dump_waveform(uint16_t ch);
	double calc_waveform_avg(uint16_t ch);
	double measure_and_calc_avg(uint8_t ch);
	
	int set_offset(uint8_t ch, int value);
	int set_fine_offset(uint8_t ch, int value);
	int set_coarse_gain(uint8_t ch, float gain);
	int set_fine_gain(uint8_t ch, double gain);
	int set_input_mux(uint8_t ch, uint8_t value);
	void run_two_point_calibration(uint8_t ch, double *offset_o, double *gain_err_o);
	int run_calibration();

	
};





















#endif //calibration_module_H