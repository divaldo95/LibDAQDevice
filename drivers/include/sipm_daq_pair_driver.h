#ifndef sipm_daq_pair_driver_H
#define sipm_daq_pair_driver_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>

#include "TTree.h"
#include "TFile.h"
#include "TROOT.h"

#include "memory_access.h"

#define sipm_daq_bram_size (128*1024)

#define sipm_daq_max_samples_single_buff (sipm_daq_bram_size/2)
#define sipm_daq_max_samples_double_buff (sipm_daq_bram_size/4)

#define daq_logic_mmap_length 4096
#define sipm_daq_mmap_length (sipm_daq_bram_size)

//sipm_daq_pair sampling controller  registers 
//all registers divided to two 16bit word, low word -> ch0, high word -> ch1
#define sipm_daq_control_reg (0*4)
#define sipm_daq_settings_reg (1*4)
#define sipm_daq_event_size_reg (2*4)  //16bit
#define sipm_daq_event_count_reg (3*4) //16bit
#define sipm_daq_trigger_delay_reg (4*4) //16bit
#define sipm_daq_delay_line_length_reg (10*4) //16bit
#define sipm_daq_status_0_reg (12*4)
#define sipm_daq_status_1_reg (13*4)


//sipm daq pair trigger control registers
#define sipm_daq_trigger_threshold_reg (5*4)  //10bit  divided
#define sipm_daq_trigger_holdoff_reg (6*4)    //16bit divided
#define sipm_daq_trigger_out_mux_sel_reg (7*4) 
#define sipm_daq_trigger_in_mux_sel_reg (8*4) //3 bit divided
#define sipm_daq_trigger_manual_trig_reg (9*4) //bit0 manual_trig(pulse) | bit1 global manual trig(pulse, implemetend only on master daq pair)




 



class sipm_daq_pair_driver
{
	private:
	bool initialized;
	memory_access daq_logic;
	int xdma_fd;
	uint32_t ch0_bram_base_addr;
	uint32_t ch1_bram_base_addr;
	
	bool ch1_implemented;
	uint8_t daq_id;
	
	uint16_t *ch0_sample_buffer = NULL;
	uint16_t *ch1_sample_buffer = NULL;
	
	bool continous_mode;
	bool double_buffered_mode;
	
	uint32_t status_reg_shadow_reg;
	

	

	
	
	public:
	
   enum trig_out_sel
   {
      CH0_re = 0,
      CH1_re,
      CH0_AND_CH1,
	  CH0_OR_CH1,
   };
   
   enum trig_in_sel
   {
      own_ch_re = 0,
      int_trig_0,
	  int_trig_1,
	  int_trig_2,
	  int_trig_3,
	  int_trig_4,
	  ext_trig_re,
	  ext_trig_fe,
	  global_manual_trig,
	  own_manual_trig
   };
	
	sipm_daq_pair_driver();
	~sipm_daq_pair_driver();
	
	int init(const char *xdma_dev_file, const char *XDMA_bypass_dev,uint8_t daq_id, uint32_t daq_base_addr, uint32_t ch0_bram_base_addr, uint32_t ch1_bram_base_addr = 0);
	
	void write_daq_reg_32(uint32_t reg_addr, uint32_t data);
	uint32_t read_daq_reg_32(uint32_t reg_addr);
	void write_daq_reg_16(uint32_t reg_addr, uint16_t data, uint8_t upper_word);
	uint16_t read_daq_reg_16(uint32_t reg_addr, uint8_t upper_word);
	
	void start_sampling(); //both channel
	void start_sampling(uint8_t ch);
	void stop_sampling(); //both channel
	void stop_sampling(uint8_t ch);
	void reset_buf_sel(uint8_t ch);
	void reset_buf_sel();
	void reset_overwrite_err_flag();
	void reset_overwrite_err_flag(uint8_t ch);
	void reset_valid_data_in_L_buff_flag();
	void reset_valid_data_in_L_buff_flag(uint8_t ch);
	void reset_valid_data_in_H_buff_flag();
	void reset_valid_data_in_H_buff_flag(uint8_t ch);
	
	void gen_global_manual_trigger();
	void gen_manual_trigger();
	
	int set_mode(bool double_buffered, bool continous);
	void update_status_reg_shadow();
	uint8_t get_daq_state_from_shadow_reg(uint8_t ch);//0 idle | 1 wait trigger | 2 wait trigger delay | 3 sampling
	uint8_t get_daq_last_buffer_from_shadow_reg(uint8_t ch); // return actual read safe buffer id 0 or 1
	void set_event_size_reg(uint16_t event_size); //samples/event
	void set_event_count_reg(uint16_t event_size); //number of events
	
	void set_trigger_delay_reg(uint8_t ch, uint16_t trig_delay); // n*sample time
	void set_trigger_delay_reg(uint16_t trig_delay); // n*sample time
	void set_delay_line_reg(uint8_t ch, uint8_t delay); // n*sample time  0-63
	void set_delay_line_reg(uint8_t delay); // n*sample time  0-63
	void set_trigger_threshold_reg(uint8_t ch, uint16_t threshold); // 0-1023 
	void set_trigger_threshold_reg(uint16_t threshold); // 0-1023 
	void set_trigger_holdoff_reg(uint8_t ch, uint16_t trig_holdoff); // n*sample time
	void set_trigger_holdoff_reg(uint16_t trig_holdoff); // n*sample time
	void set_daq_pair_trig_output_mux(uint8_t ch, sipm_daq_pair_driver::trig_out_sel selected);
	void select_trig_source(uint8_t ch, sipm_daq_pair_driver::trig_in_sel selected);
	
	
	ssize_t dma_read_to_buffer(uint8_t ch, uint64_t size, uint8_t read_buffer_id = 0);
	void get_data_8bit(uint8_t ch, uint32_t size, uint8_t *out_buf);
	int  get_data_with_header(uint8_t ch, uint32_t size, uint8_t *out_buf, uint32_t header);
	void get_data_uint16(uint8_t ch, uint32_t length, uint16_t *out_buf);  //single buffer mode only!!!
	uint16_t* get_data_pointer_uint16(uint8_t ch);
	
};























#endif