#include "sipm_daq_pair_driver.h"

sipm_daq_pair_driver::sipm_daq_pair_driver()
{
	initialized = false;
	ch0_sample_buffer = NULL;
	ch1_sample_buffer = NULL;
	
	continous_mode = false;
	double_buffered_mode = false;
}


sipm_daq_pair_driver::~sipm_daq_pair_driver()
{
	if(initialized)
	{
		free(ch0_sample_buffer);
		free(ch1_sample_buffer);
	}
}


int sipm_daq_pair_driver::init(const char *xdma_dev_file, const char *XDMA_bypass_dev,uint8_t daq_id, uint32_t daq_base_addr, uint32_t ch0_bram_base_addr, uint32_t ch1_bram_base_addr)
{
	if(initialized)
		return -1;
	
	this->ch0_bram_base_addr = ch0_bram_base_addr;
	this->ch1_bram_base_addr = ch1_bram_base_addr;
	this->daq_id = daq_id;
	
	if(daq_logic.init_mmap(XDMA_bypass_dev, daq_base_addr, daq_logic_mmap_length) < 0)
	{
		printf("daq_logic memory access init failed");
		initialized = false;
		return -2;
	}
	
	xdma_fd = open(xdma_dev_file, O_RDWR);
	if (xdma_fd < 0) 
	{
        printf("unable to open device %s\n", xdma_dev_file);
		return -3;
    }
	printf("dev:%s opened\n", xdma_dev_file);
	
	if(posix_memalign((void **)&ch0_sample_buffer, 4096 /*alignment */ ,sipm_daq_mmap_length) != 0) 
	{
		printf("posix_memalign failed (ch0)\n");
		return -3;
	}
	
	if(ch1_bram_base_addr != 0)
	{
		ch1_implemented = true;
		
		if(posix_memalign((void **)&ch1_sample_buffer, 4096 /*alignment */ ,sipm_daq_mmap_length) != 0) 
		{
			printf("posix_memalign failed(ch1)\n");
			return -4;
		}
	}
	else
	{
		ch1_implemented = false;
	}
	
	
	
	initialized = true;
	
	return 0;
}

void sipm_daq_pair_driver::write_daq_reg_32(uint32_t reg_addr, uint32_t data)
{
	daq_logic.write_32(reg_addr, data);
}

uint32_t sipm_daq_pair_driver::read_daq_reg_32(uint32_t reg_addr)
{
	return daq_logic.read_32(reg_addr);
}

void sipm_daq_pair_driver::write_daq_reg_16(uint32_t reg_addr, uint16_t data, uint8_t upper_word)
{
	if(upper_word != 0)
		reg_addr += 2;
	daq_logic.write_16(reg_addr, data);
	
	//printf("write:%x read:%x\n",data,  daq_logic.read_16(reg_addr));
}

uint16_t sipm_daq_pair_driver::read_daq_reg_16(uint32_t reg_addr, uint8_t upper_word)
{
	if(upper_word != 0)
		reg_addr += 2;
	return daq_logic.read_16(reg_addr);
}

void sipm_daq_pair_driver::start_sampling() //both channel
{
	write_daq_reg_32(sipm_daq_control_reg,0);
	//usleep(1);
	write_daq_reg_32(sipm_daq_control_reg,1<<(16+0) | 1<<0);
	//usleep(1);
	write_daq_reg_32(sipm_daq_control_reg,0);
}

void sipm_daq_pair_driver::start_sampling(uint8_t ch)
{
	write_daq_reg_16(sipm_daq_control_reg,0, ch);
	usleep(1);
	write_daq_reg_16(sipm_daq_control_reg,1<<0, ch);
	usleep(1);
	write_daq_reg_16(sipm_daq_control_reg,0, ch);
}

void sipm_daq_pair_driver::stop_sampling() //both channel
{
	write_daq_reg_32(sipm_daq_control_reg,0);
	usleep(1);
	write_daq_reg_32(sipm_daq_control_reg,1<<(16+1) | 1<<1);
	usleep(1);
	write_daq_reg_32(sipm_daq_control_reg,0);
}

void sipm_daq_pair_driver::stop_sampling(uint8_t ch)
{
	write_daq_reg_16(sipm_daq_control_reg,0, ch);
	usleep(1);
	write_daq_reg_16(sipm_daq_control_reg,1<<1, ch);
	usleep(1);
	write_daq_reg_16(sipm_daq_control_reg,0, ch);
}

void sipm_daq_pair_driver::reset_buf_sel() //both channel
{
	write_daq_reg_32(sipm_daq_control_reg,1<<(16+2) | 1<<2);
	usleep(1);
	write_daq_reg_32(sipm_daq_control_reg,0);
}

void sipm_daq_pair_driver::reset_buf_sel(uint8_t ch)
{
	write_daq_reg_16(sipm_daq_control_reg,1<<2, ch);
	usleep(1);
	write_daq_reg_16(sipm_daq_control_reg,0, ch);
}

void sipm_daq_pair_driver::reset_overwrite_err_flag() //both channel
{
	write_daq_reg_32(sipm_daq_control_reg,1<<(16+3) | 1<<3);
	usleep(1);
	write_daq_reg_32(sipm_daq_control_reg,0);
}

void sipm_daq_pair_driver::reset_overwrite_err_flag(uint8_t ch)
{
	write_daq_reg_16(sipm_daq_control_reg,1<<3, ch);
	usleep(1);
	write_daq_reg_16(sipm_daq_control_reg,0, ch);
}

void sipm_daq_pair_driver::reset_valid_data_in_L_buff_flag() //both channel
{
	write_daq_reg_32(sipm_daq_control_reg,1<<(16+4) | 1<<4);
	usleep(1);
	write_daq_reg_32(sipm_daq_control_reg,0);
}

void sipm_daq_pair_driver::reset_valid_data_in_L_buff_flag(uint8_t ch)
{
	write_daq_reg_16(sipm_daq_control_reg,1<<4, ch);
	usleep(1);
	write_daq_reg_16(sipm_daq_control_reg,0, ch);
}

void sipm_daq_pair_driver::reset_valid_data_in_H_buff_flag() //both channel
{
	write_daq_reg_32(sipm_daq_control_reg,1<<(16+5) | 1<<5);
	usleep(1);
	write_daq_reg_32(sipm_daq_control_reg,0);
}

void sipm_daq_pair_driver::reset_valid_data_in_H_buff_flag(uint8_t ch)
{
	write_daq_reg_16(sipm_daq_control_reg,1<<5, ch);
	usleep(1);
	write_daq_reg_16(sipm_daq_control_reg,0, ch);
}

void sipm_daq_pair_driver::gen_global_manual_trigger()
{
	write_daq_reg_32(sipm_daq_trigger_manual_trig_reg,1<<1);
	usleep(1);
	write_daq_reg_32(sipm_daq_trigger_manual_trig_reg,0);
}

void sipm_daq_pair_driver::gen_manual_trigger()
{
	write_daq_reg_32(sipm_daq_trigger_manual_trig_reg,1<<0);
	usleep(1);
	write_daq_reg_32(sipm_daq_trigger_manual_trig_reg,0);
}

int sipm_daq_pair_driver::set_mode(bool double_buffered, bool continous)
{
	if(continous and (not double_buffered))
	{
		printf("continous mode without double buffering not supported!!!!\n");
		continous_mode = false;
		double_buffered_mode = false;
		return -1;
	}
	
	continous_mode = continous;
	double_buffered_mode = double_buffered;
	
	uint32_t reg_val = 0;
	if(continous_mode)
		reg_val |= 1<<0;
	if(double_buffered_mode)
		reg_val |= 1<<1;
	write_daq_reg_32(sipm_daq_settings_reg, reg_val<<16 | reg_val);
	usleep(1);
	
	return 0;
}

void sipm_daq_pair_driver::update_status_reg_shadow()  //0 idle | 1 wait trigger | 2 wait trigger delay | 3 sampling
{
	uint8_t status;
	
	status_reg_shadow_reg = read_daq_reg_32(sipm_daq_status_0_reg);
}

uint8_t sipm_daq_pair_driver::get_daq_state_from_shadow_reg(uint8_t ch)//0 idle | 1 wait trigger | 2 wait trigger delay | 3 sampling
{
	if(ch > 1)
	{
		printf("ch number only 0 or 1 !!!\n");
		return 0;
	}
	
	uint16_t reg_val;
	if(ch) //ch 1
	{
		reg_val = (uint16_t)(status_reg_shadow_reg >> 16);
	}
	else
	{
		reg_val = (uint16_t)status_reg_shadow_reg;
	}
	
	return reg_val & 0x07;
}

uint8_t sipm_daq_pair_driver::get_daq_last_buffer_from_shadow_reg(uint8_t ch)// return actual read safe buffer id 0 or 1
{
	if(ch > 1)
	{
		printf("ch number only 0 or 1 !!!\n");
		return 0;
	}
	
	uint16_t reg_val;
	if(ch) //ch 1
	{
		reg_val = (uint16_t)(status_reg_shadow_reg >> 16);
	}
	else
	{
		reg_val = (uint16_t)status_reg_shadow_reg;
	}
	
	return !!(reg_val & (1<<5));
}

void sipm_daq_pair_driver::set_event_size_reg(uint16_t event_size) //samples/event
{
	write_daq_reg_32(sipm_daq_event_size_reg, event_size<<16 | event_size); 
}
void sipm_daq_pair_driver::set_event_count_reg(uint16_t event_count) //number of events
{
	if(event_count > 1)
		event_count ++;
	write_daq_reg_32(sipm_daq_event_count_reg, event_count<<16 | event_count); 
}



void sipm_daq_pair_driver::set_trigger_delay_reg(uint8_t ch, uint16_t trig_delay) // n*sample time
{
	write_daq_reg_16(sipm_daq_trigger_delay_reg, trig_delay, ch); 
}

void sipm_daq_pair_driver::set_trigger_delay_reg(uint16_t trig_delay) // n*sample time
{
	set_trigger_delay_reg(0, trig_delay);
	set_trigger_delay_reg(1, trig_delay);
}

void sipm_daq_pair_driver::set_delay_line_reg(uint8_t ch, uint8_t delay) // n*sample time  0-63
{
	if(delay > 63)
		delay = 63;
	write_daq_reg_16(sipm_daq_delay_line_length_reg, delay, ch); 
}

void sipm_daq_pair_driver::set_delay_line_reg(uint8_t delay) // n*sample time  0-63
{
	set_delay_line_reg(0, delay);
	set_delay_line_reg(1, delay);
}

void sipm_daq_pair_driver::set_trigger_threshold_reg(uint8_t ch, uint16_t threshold) // 0-1023 
{
	write_daq_reg_16(sipm_daq_trigger_threshold_reg, threshold, ch); 
}

void sipm_daq_pair_driver::set_trigger_threshold_reg(uint16_t threshold) // 0-1023 
{
	set_trigger_threshold_reg(0, threshold);
	set_trigger_threshold_reg(1, threshold);
}

void sipm_daq_pair_driver::set_trigger_holdoff_reg(uint8_t ch, uint16_t trig_holdoff) // n*sample time
{
	write_daq_reg_16(sipm_daq_trigger_holdoff_reg, trig_holdoff, ch); 
}

void sipm_daq_pair_driver::set_trigger_holdoff_reg(uint16_t trig_holdoff) // n*sample time
{
	set_trigger_holdoff_reg(0, trig_holdoff);
	set_trigger_holdoff_reg(1, trig_holdoff);
}

void sipm_daq_pair_driver::set_daq_pair_trig_output_mux(uint8_t ch, sipm_daq_pair_driver::trig_out_sel selected)
{
	uint16_t sel_bits;
	uint32_t reg_val = read_daq_reg_32(sipm_daq_trigger_out_mux_sel_reg);
	switch(selected)
	{
		case CH0_re: 
			sel_bits = 0;
			break;
		case CH1_re:
			sel_bits = 1;
			break;
		case CH0_AND_CH1:
			sel_bits = 2;
			break;
		case CH0_OR_CH1:
			sel_bits = 3;
			break;
	}
	
	reg_val &= ~0x07;
	reg_val |= sel_bits;
	write_daq_reg_32(sipm_daq_trigger_out_mux_sel_reg, reg_val);
}

void sipm_daq_pair_driver::select_trig_source(uint8_t ch, sipm_daq_pair_driver::trig_in_sel selected)
{
	uint16_t sel_bits;
	uint32_t out_mux_reg_val = read_daq_reg_32(sipm_daq_trigger_out_mux_sel_reg);
	bool update_out_mux = false;
	uint32_t in_mux_reg_val;
	switch(selected)
	{
		case own_ch_re: 
			in_mux_reg_val = 0;
			break;
		case int_trig_0:
			in_mux_reg_val = 1;
			break;
		case int_trig_1: 
			in_mux_reg_val = 2;
			break;
		case int_trig_2:
			in_mux_reg_val = 3;
			break;
		case int_trig_3:
			in_mux_reg_val = 4;
			break;
		case int_trig_4:
			in_mux_reg_val = 5;
			break;
		case ext_trig_re:
			in_mux_reg_val = 6;
			out_mux_reg_val &= ~(1<<3);
			update_out_mux = true;
			break;
		case ext_trig_fe:
			in_mux_reg_val = 6;
			out_mux_reg_val |= (1<<3);
			update_out_mux = true;
			break;
		case global_manual_trig:
			in_mux_reg_val = 7;
			out_mux_reg_val |= (1<<4);
			update_out_mux = true;
			break;
		case own_manual_trig:
			in_mux_reg_val = 7;
			out_mux_reg_val &= ~(1<<4);
			update_out_mux = true;
			break;
	}
	
	
	write_daq_reg_16(sipm_daq_trigger_in_mux_sel_reg, in_mux_reg_val, ch);
	printf("write:%x read:%x\n",in_mux_reg_val,  daq_logic.read_32(sipm_daq_trigger_in_mux_sel_reg));
	if(update_out_mux)
	{
		write_daq_reg_32(sipm_daq_trigger_out_mux_sel_reg, out_mux_reg_val);
		//printf("out mux reg write:%x read:%x\n",out_mux_reg_val,  daq_logic.read_32(sipm_daq_trigger_out_mux_sel_reg));
	}
}


#define RW_MAX_SIZE	0x7ffff000
ssize_t sipm_daq_pair_driver::dma_read_to_buffer(uint8_t ch, uint64_t size, uint8_t read_buffer_id)
{
	ssize_t rc;
	uint64_t count = 0;
	char *buf;
	off_t offset;
	if(ch == 0)
	{
		buf = (char*)ch0_sample_buffer;
		offset = ch0_bram_base_addr;
	}
	else
	{
		buf = (char*)ch1_sample_buffer;
		offset = ch1_bram_base_addr;
	}
	
	if(read_buffer_id)
	{
		offset += (sipm_daq_max_samples_double_buff*2);
	}
	int loop = 0;
	const char *fname = "DMA_C2H_0";

	while (count < size)
	{
		uint64_t bytes = size - count;

		if (bytes > RW_MAX_SIZE)
			bytes = RW_MAX_SIZE;

		//if (offset)
		{
			rc = lseek(xdma_fd, offset, SEEK_SET);
			if (rc != offset) 
			{
				printf("%s, seek off 0x%lx != 0x%lx.\n", fname, rc, offset);
				return -1;
			}
		}

		// read data from file into memory buffer 
		rc = read(xdma_fd, buf, bytes);
		if (rc < 0)
		{
			printf("%s, read 0x%lx @ 0x%lx failed %ld.\n", fname, bytes, offset, rc);
			return -2;
		}

		count += rc;
		if (rc != bytes) 
		{
			printf("%s, read underflow 0x%lx/0x%lx @ 0x%lx.\n", fname, rc, bytes, offset);
			break;
		}

		buf += bytes;
		offset += bytes;
		loop++;
	}

	if (count != size && loop)
		printf("%s, read underflow 0x%lx/0x%lx.\n",fname, count, size);
	return count;
}

void sipm_daq_pair_driver::get_data_8bit(uint8_t ch, uint32_t size, uint8_t *out_buf)
{
	uint16_t *in_buf;
	if(ch == 0)
	{
		in_buf = ch0_sample_buffer;
	}
	else
	{
		in_buf = ch1_sample_buffer;
	}
	for(int i=0;i<size;i++)
	{
		out_buf[i] = (uint8_t)(in_buf[i]>>2);	
	}
}

int sipm_daq_pair_driver::get_data_with_header(uint8_t ch, uint32_t size, uint8_t *out_buf, uint32_t header)
{
	uint16_t *in_buf;
	if(ch == 0)
	{
		in_buf = ch0_sample_buffer;
	}
	else
	{
		in_buf = ch1_sample_buffer;
	}
	
	memcpy(out_buf+4, in_buf, size);
	((uint32_t*)out_buf)[0] = header;
	
	return size + 4;
}

//legacy function do not use!!!
void sipm_daq_pair_driver::get_data_uint16(uint8_t ch, uint32_t length, uint16_t *out_buf)
{
	uint16_t *in_buf;
	if(ch == 0)
	{
		in_buf = ch0_sample_buffer;
	}
	else
	{
		in_buf = ch1_sample_buffer;
	}
	
	memcpy(out_buf, in_buf, length*2);
}

uint16_t* sipm_daq_pair_driver::get_data_pointer_uint16(uint8_t ch)
{
	uint16_t *in_buf;
	if(ch == 0)
	{
		in_buf = ch0_sample_buffer;
	}
	else
	{
		in_buf = ch1_sample_buffer;
	}

	return in_buf;
}









/*





void sipm_daq_pair_driver::calc_integrals(bool debug)
{
	int sum = 0;
	int event_index = 0;
	int n_events = sps_daq_buffer_nsamples/sampling_window_size;
	int event_size = sampling_window_size;
	uint8_t start_pos = int_start;
	uint8_t end_pos = int_end;
	
	
	for(int event_c=0; event_c<n_events; event_c++)
	{
		event_index = event_c * event_size;
		sum = 0;
		for(int point_c=int_start;point_c<int_end; point_c++)
		{
			sum += ((int)sample_buffer[event_index + point_c]) - int_bias;
		}
		
		integral_buffer[event_c] = sum;
		if(debug)
		{	
			if(event_c < 3)
			{
				printf("event:%d ",event_c);
				for(int i=0;i<event_size; i++)
				{
					printf("%d ", sample_buffer[event_index + i]);
				}
				printf("\n");
			}
		}	
	}
}

int sipm_daq_pair_driver::start_measure()
{
   const int n_events = sps_daq_buffer_nsamples/sampling_window_size;
	
	printf("measurement started\n");

	start_sampling_and_wait_end();
	uint32_t ret = dma_read_to_buffer((char*)sample_buffer, sps_daq_bram_mmap_length, bram_phys_base_addr);
	if(ret != sps_daq_bram_mmap_length)
	{
		printf("DMA transfer error ret != size\n");
		return -4;
	}
	printf("copy ok\n");

    printf("measurement end\n");	
	return n_events;
}




int sipm_daq_pair_driver::write_integrals_to_txt_file(FILE *out_fp)
{
	int n_events = sps_daq_buffer_nsamples/sampling_window_size;
	for(int i=0;i<n_events;i++)
	{
		fprintf(out_fp, "%d\n", integral_buffer[i]);	
	}
	return 0;
}
*/
/*
int sipm_daq_pair_driver::write_integrals_to_root_file_old(char *ttree_name)
{
	static bool first = true;
	int32_t integral;
	int n_events = sps_daq_buffer_nsamples/sampling_window_size;
	TTree *tree = NULL;
	if(first)
	{
		tree =  new TTree(ttree_name, ttree_name);
		tree->Branch("integral", &integral,"integral/I");
		first = false;
	}
	else
	{
		tree = (TTree*)gDirectory->Get(ttree_name);
		tree->SetBranchAddress("integral", &integral);
	}
	
	
	
	for(int i=0;i<n_events;i++)
	{
		integral = integral_buffer[i];
		tree->Fill();
	}
	tree->Write("",TObject::kOverwrite);
	return 0;
}
*/
/*
int sipm_daq_pair_driver::write_integrals_to_root_file(char *ttree_name)
{
	static TTree *tree = NULL;
	int32_t integral;
	int n_events = sps_daq_buffer_nsamples/sampling_window_size;
	
	if(tree == NULL)
	{
		tree =  new TTree(ttree_name, ttree_name);
		tree->Branch("integral", &integral,"integral/I");
	}
	else
	{
		tree->SetBranchAddress("integral", &integral);
	}
	
	
	
	for(int i=0;i<n_events;i++)
	{
		integral = integral_buffer[i];
		tree->Fill();
	}
	tree->Write("",TObject::kOverwrite);
	return 0;
}


int sipm_daq_pair_driver::measure_and_save_integrals(uint32_t nevent, char *ttree_name)
{
	int event_cnt = 0;
	int32_t integral;
	int n_events = sps_daq_buffer_nsamples/sampling_window_size;
	TTree *tree = NULL;
	tree =  new TTree(ttree_name, ttree_name);
	tree->Branch("integral", &integral,"integral/I");
	
	while(event_cnt < nevent)
	{
		event_cnt += start_measure();
		calc_integrals(true);
		
		for(int i=0;i<n_events;i++)
		{
			integral = integral_buffer[i];
			tree->Fill();
		}
	}
	tree->Write();
	
	delete tree;
	
	return 0;
}

int sipm_daq_pair_driver::save_metadata(uint32_t meas_id, float voltage, float temp_before, float temp_after)
{
	TTree *tree = NULL;
	tree = (TTree*)gDirectory->Get("metadata");
	if(tree == NULL)
	{
		tree =  new TTree("metadata", "metadata");
		tree->Branch("meas_id", &meas_id,"meas_id/i");
		tree->Branch("voltage", &voltage,"voltage/F");
		tree->Branch("temp_before", &temp_before,"temp_before/F");
		tree->Branch("temp_after", &temp_after,"temp_after/F");
	}
	else
	{
		tree->SetBranchAddress("meas_id", &meas_id);
		tree->SetBranchAddress("voltage", &voltage);
		tree->SetBranchAddress("temp_before", &temp_before);
		tree->SetBranchAddress("temp_after", &temp_after);
	}
	tree->Fill();
	tree->Write("",TObject::kOverwrite);
	delete tree;
	
	return 0;
}

int sipm_daq_pair_driver::measure_and_save_waveforms(uint32_t nevent, char *ttree_name)
{
	int event_cnt = 0;
	uint16_t *waveform_ptr = new uint16_t[sampling_window_size];  //waveform_ptr[sampling_window_size];
	int n_events = sps_daq_buffer_nsamples/sampling_window_size;
	TTree *tree = NULL;
	tree =  new TTree(ttree_name, ttree_name);
	char branch_format_string[100];
	snprintf(branch_format_string, 100, "waveform[%d]/s", sampling_window_size);
	tree->Branch("waveforms", waveform_ptr, branch_format_string);
	
	while(event_cnt < nevent)
	{
		event_cnt += start_measure();
		
		uint32_t event_start_index = 0;
		for(int i=0;i<n_events;i++)
		{
			memcpy(waveform_ptr, &sample_buffer[event_start_index],sampling_window_size*2); 
			//waveform_ptr = &sample_buffer[event_start_index];
			
			event_start_index += sampling_window_size;
			tree->Fill();
		}
	}
	tree->Write();
	
	delete tree;
	
	return 0;
}

*/
