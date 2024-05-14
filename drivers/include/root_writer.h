#ifndef root_writer_H
#define root_writer_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>

#include "TNtupleD.h"
#include "TTree.h"
#include "TFile.h"
#include "TROOT.h"
#include "TBrowser.h"
#include <TObject.h>



class root_writer
{
	private:
	bool opened;
	TFile *root_file_p;
	TTree *step_Tree;
	uint32_t waveform_max_length;
	uint64_t write_cnt;
	
	uint16_t *waveform_buff;
	
	
	public:
	
	root_writer();
	~root_writer();
	
	int mkdir_p(const char *path);
	void close_file(); 
	int open_file(char *out_file_path, uint32_t waveform_length);
	bool is_open();
	void create_new_voltage_step_tree(uint32_t step_index);
	int save_step_metadata(uint32_t meas_id, float voltage, float temp_before, float temp_after, uint64_t end_timestamp);
	int save_metadata(float voltage_U, float voltage_D, char *module_id_str, uint32_t SIPM_index,
								   float temp_U_before, float temp_D_before, float temp_pcb_before, 
								   float temp_U_after, float temp_D_after, float temp_pcb_after, 
								   float ADC_gain,
								   uint32_t adapter_type, uint32_t trig_threshold, char *preamp_board_id_str);
	int save_waveforms(uint16_t *waveform_U, uint16_t *waveform_D);
	//int save_waveform_pair(uint16_t *waveform_U, uint16_t *waveform_D);
	int save_waveforms(uint16_t *waveform, uint32_t N_pair);
	
};





















#endif //TVP7002_H