#include "calibration_module.h"

calibration_module::calibration_module()
{
	initialized = false;
	
}


calibration_module::~calibration_module()
{
	if(initialized)
	{
		
	}
}


int calibration_module::init( const char *XDMA_bypass_dev, uint32_t calib_sel_gpio_base_addr, TVP7002 adc_[3], sipm_daq_pair_driver daq_pair_[5], double ADC_FS_voltage_)
{
	if(initialized)
		return -1;
	
	if(adc_ == NULL)
		return -1;
	adc = adc_;
	
	if(daq_pair_ == NULL)
		return -1;
	daq_pair = daq_pair_;
	
	if(mem_calib_vsel_gpio.init_mmap(XDMA_bypass_dev, calib_sel_gpio_base_addr, 4096) < 0)
	{
		printf("mem_calib_vsel_gpio memory access init failed\n");
		return -2;
	}
	
	//adc[2].set_input_mux(0,2);
	
	
	DAQ_event_count = 1;
	DAQ_event_samples = 32768;
	ADC_FS_voltage = ADC_FS_voltage_;
	
	init_daq_pairs();
	
	initialized = true;
	
	return 0;
}


void calibration_module::init_daq_pairs()
{
	for(int i=0;i<5;i++)
	{
		daq_pair[i].stop_sampling();
		usleep(10);
	}
	
	
	for(int i=0;i<5;i++)
	{
		daq_pair[i].set_mode(false, false); 
		daq_pair[i].reset_buf_sel();
		daq_pair[i].reset_overwrite_err_flag();
		
		daq_pair[i].set_event_size_reg(DAQ_event_samples);
		daq_pair[i].set_event_count_reg(DAQ_event_count);
		
		for(int ch =0;ch<2;ch++)
		{
			daq_pair[i].set_trigger_delay_reg(ch,0);
			daq_pair[i].set_delay_line_reg(ch,1);
			daq_pair[i].set_trigger_threshold_reg(ch,100);
			daq_pair[i].set_trigger_holdoff_reg(ch,0);
		}
		daq_pair[i].set_daq_pair_trig_output_mux(0, sipm_daq_pair_driver::CH0_AND_CH1);  //CH0_AND_CH1
		
		daq_pair[i].select_trig_source(0, sipm_daq_pair_driver::global_manual_trig); 
		daq_pair[i].select_trig_source(1, sipm_daq_pair_driver::global_manual_trig);
	}
	
}


void calibration_module::select_calibration_voltage(uint8_t sel)  //0-> 0.879V 1->0.559V 2->0.298 3->0.249  (nominal 877.7 558.5 297.5   ) 
{
	mem_calib_vsel_gpio.write_32(0, sel); 
	usleep(500000);
}


int calibration_module::measure()
{
	
	for(int i=0;i<5;i++)
	{
		daq_pair[i].stop_sampling();
		usleep(1);
	}
	usleep(100);
	
	for(int i=0;i<5;i++)
	{
		daq_pair[i].start_sampling();
		usleep(1);
	}
	usleep(100);
	
	daq_pair[0].gen_global_manual_trigger();
	usleep(10000);
	
	for(int i=0;i<5;i++)
	{
		daq_pair[i].update_status_reg_shadow();
		uint8_t state_u = daq_pair[i].get_daq_state_from_shadow_reg(0);
		uint8_t state_d = 0;
		if(i != 4)
		{
			state_d = daq_pair[i].get_daq_state_from_shadow_reg(1);
		}
		
		if(state_u != 0)
		{
			printf("daq_pair%d u won't stop in time error!\n", i);
			return -1;
		}
		if(i != 4)
		{
			if(state_d != 0)
			{
				printf("daq_pair%d d won't stop in time error!\n", i);
				return -1;
			}
		}
	}
	
	for(int i=0;i<5;i++)
	{
		uint32_t dma_cpy_bytes = DAQ_event_count * DAQ_event_samples * 2;
					
		daq_pair[i].dma_read_to_buffer(0, dma_cpy_bytes, 0);
		waveform_buffers[i*2+0] = daq_pair[i].get_data_pointer_uint16(0);
		
		if(i != 4)
		{
			daq_pair[i].dma_read_to_buffer(1, dma_cpy_bytes, 0);
			waveform_buffers[i*2+1] = daq_pair[i].get_data_pointer_uint16(1);
		}
	}
	
	return 0;
}

void calibration_module::dump_waveform(uint16_t ch)
{
	uint32_t samples = DAQ_event_samples;
	printf("\n");
	for(uint32_t i=0;i<samples; i++)
	{
		printf("%d ", waveform_buffers[ch][i]);
	}
	
	printf("\n");
}


double calibration_module::calc_waveform_avg(uint16_t ch)
{
	uint32_t samples = DAQ_event_samples;
	double avg = 0;
	for(uint32_t i=0;i<samples;i++)
	{
		avg += waveform_buffers[ch][i];
	}
	
	avg /= samples;
	
	return avg;
}

double calibration_module::measure_and_calc_avg(uint8_t ch)
{
	if(ch > 8)
		return -1000;
	
	const int runs = 10;
	
	double avg = 0;
	
	for(int i =0;i < runs;i++)
	{
		measure();
		//dump_waveform(ch);
		avg += calc_waveform_avg(ch);
		usleep(1000);
	}
	avg /= runs;
		
	return avg;
}

int calibration_module::set_offset(uint8_t ch, int value)
{
	uint8_t adc_i = ch/3;
	uint8_t ch_i = ch%3;
	return adc[adc_i].set_coarse_offset(ch_i, value);
}

int calibration_module::set_fine_offset(uint8_t ch, int value)
{
	uint8_t adc_i = ch/3;
	uint8_t ch_i = ch%3;
	return adc[adc_i].set_fine_offset(ch_i, value);
}

int calibration_module::set_coarse_gain(uint8_t ch, float gain)
{
	uint8_t adc_i = ch/3;
	uint8_t ch_i = ch%3;
	return adc[adc_i].set_coarse_gain(ch_i, gain);
}

int calibration_module::set_fine_gain(uint8_t ch, double gain)
{
	uint8_t adc_i = ch/3;
	uint8_t ch_i = ch%3;
	return adc[adc_i].set_fine_gain(ch_i, gain);
}

int calibration_module::set_input_mux(uint8_t ch, uint8_t value)
{
	uint8_t adc_i = ch/3;
	uint8_t ch_i = ch%3;
	return adc[adc_i].set_input_mux(ch_i, value);
}


void calibration_module::run_two_point_calibration(uint8_t ch, double *offset_o, double *gain_err_o)
{
	//set_fine_gain(ch, 1.0);
	set_input_mux(ch, 1);  
	//set_offset(ch, 0);
	usleep(100000);
	
	double vin_A = 0.249;
	double vin_B = 0.879;
	
	select_calibration_voltage(3);  //0.249V
	double code_A = measure_and_calc_avg(ch);
	
	select_calibration_voltage(0);  //0.879V
	double code_B = measure_and_calc_avg(ch);
	
	double slope = (code_B - code_A) / (vin_B - vin_A);
	
	// ym = slope*(Vin-vin_A)+ code_A
	double offset = (slope * (-1.0*vin_A)) + code_A;    // vin=0 to ym equation
	
	double ideal_slope = (1023 - 0) / (ADC_FS_voltage - 0);
	double gain_err = (slope - ideal_slope) / ideal_slope ;
	
	printf("ch:%d code_A:%.1lf code_B:%.1lf\n", ch, code_A, code_B);
	printf("ch:%d slope:%.4lf count/V  offset:%.1lf gain_error:%.2lf percent\n",ch, slope, offset, gain_err*100.0);
	
	*offset_o = offset;
	
	*gain_err_o = gain_err;
		
}

int calibration_module::run_calibration()
{
	
	for(int ch = 0;ch<9;ch++)
	{
		set_fine_gain(ch, 1.0);
		set_offset(ch, 0);
	}
	
	for(int ch = 0;ch<9;ch++)
	{
		run_two_point_calibration(ch, &offests[ch], &gain_errors[ch]);
	}
	
	for(int ch = 0;ch<9;ch++)
	{
		//set_fine_offset(ch, (int)(offests[ch]*-1.0));
		set_fine_gain(ch, 1.0 + gain_errors[ch]*-1.0);
	}
	
	printf("after gain cal\n");
	for(int ch = 0;ch<9;ch++)
	{
		run_two_point_calibration(ch, &offests[ch], &gain_errors[ch]);
	}
	
	for(int ch = 0;ch<9;ch++)
	{
		set_fine_offset(ch, (int)(offests[ch]*-1.0));
		//set_fine_gain(ch, 1.0 + gain_errors[ch]*-1.0);
	}
	
	printf("after offset cal\n");
	for(int ch = 0;ch<9;ch++)
	{
		run_two_point_calibration(ch, &offests[ch], &gain_errors[ch]);
	}
	
	printf("test calibration resoult!\n");
	for(int i=0;i<4;i++)
	{
		select_calibration_voltage(i);
		printf("calib voltage sel:%d\n", i);
		
		for(int ch = 0;ch<9;ch++)
		{
			double avg = measure_and_calc_avg(ch);
			avg = avg * (ADC_FS_voltage/1023);
			printf("ch:%d avg:%.3lfV\n", ch, avg);
		}
	}
	
	return 0;
}



