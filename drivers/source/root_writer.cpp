#include "root_writer.h"

root_writer::root_writer()
{
	opened = false;
	root_file_p = NULL;
	waveform_max_length = 0;
}


root_writer::~root_writer()
{
	close_file();
}

void root_writer::close_file()
{
	if(opened && root_file_p != NULL)
	{
		root_file_p->cd();
		
		if(step_Tree != NULL)
			step_Tree->Write("",TObject::kOverwrite);
		
		root_file_p->Close();
		delete root_file_p;
		root_file_p = NULL;
		
		free(waveform_buff);
		
		opened = false;
	}
}

bool root_writer::is_open()
{
	return opened;
}


int root_writer::open_file(char *out_file_path, uint32_t waveform_length)
{
	if(opened)
	{
		printf("root file already opened\n");
		return -1;
	}
	
	waveform_max_length = waveform_length;
	write_cnt = 0;
	
	if(posix_memalign((void **)&waveform_buff, 4096 /*alignment */ ,waveform_max_length*2) != 0) 
	{
		printf("posix_memalign failed (waveform_buff)\n");
		return -10;
	}
	
	if(mkdir_p(out_file_path) >= 0)
	{
		root_file_p = new TFile(out_file_path,"RECREATE");
		if(root_file_p != NULL && !root_file_p->IsZombie())
		{
			opened = true;
		}
		else
		{
			opened = false;
			printf("root file open error\n");
			return -2;
		}
	}
	
	
	
	step_Tree = NULL;
	
	opened = true;
	return 0;
}

void root_writer::create_new_voltage_step_tree(uint32_t step_index)
{
	if(!opened)
		return;
	
	if(step_Tree != NULL)
	{
		root_file_p->cd();
		step_Tree->Write("",TObject::kOverwrite);
		delete step_Tree;
		step_Tree = NULL;
	}
	char branch_format_string[100];
	snprintf(branch_format_string, 100, "step:%d", step_index);
	step_Tree =  new TTree(branch_format_string, branch_format_string);
	snprintf(branch_format_string, 100, "waveform[%d]/s", waveform_max_length);
	step_Tree->Branch("waveform", "", branch_format_string);
	
	step_Tree->SetBranchAddress("waveform", waveform_buff);
}


int root_writer::mkdir_p(const char *path)
{
    /* Adapted from http://stackoverflow.com/a/2336245/119527 */
    const size_t len = strlen(path);
    char _path[PATH_MAX];
    char *p; 

    errno = 0;

    /* Copy string so its mutable */
    if (len > sizeof(_path)-1) {
        errno = ENAMETOOLONG;
        return -1; 
    }   
    strcpy(_path, path);

    /* Iterate the string */
    for (p = _path + 1; *p; p++) {
        if (*p == '/') {
            /* Temporarily truncate */
            *p = '\0';

            if (mkdir(_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0) { //
                if (errno != EEXIST)
                    return -1; 
            }

            *p = '/';
        }
    }   
/*
    if (mkdir(_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0) {
        if (errno != EEXIST)
            return -1; 
    }   
*/
    return 0;
}


//after new voltage step
int root_writer::save_step_metadata(uint32_t meas_id, float voltage, float temp_before, float temp_after, uint64_t end_timestamp)
{
	if(!opened)
		return -1;
	
	root_file_p->cd();
	
	TTree *tree = NULL;
	tree = (TTree*)gDirectory->Get("step_metadata");
	if(tree == NULL)
	{
		tree =  new TTree("step_metadata", "step_metadata");
		
		tree->Branch("meas_id", &meas_id,"meas_id/i");
		tree->Branch("voltage", &voltage,"voltage/F");
		tree->Branch("temp_before", &temp_before,"temp_before/F");
		tree->Branch("temp_after", &temp_after,"temp_after/F");
		tree->Branch("end_timestamp", &end_timestamp,"end_timestamp/g");
	}
	else
	{
		tree->SetBranchAddress("meas_id", &meas_id);
		tree->SetBranchAddress("voltage", &voltage);
		tree->SetBranchAddress("temp_before", &temp_before);
		tree->SetBranchAddress("temp_after", &temp_after);
		tree->SetBranchAddress("end_timestamp", &end_timestamp);
	}
	tree->Fill();
	tree->Write("", TObject::kOverwrite);
	delete tree;
	
	return 0;
}

//once after file create (or close?)
int root_writer::save_metadata(float voltage_U, float voltage_D, char *module_id_str, uint32_t SIPM_index,
								float temp_U_before, float temp_D_before, float temp_pcb_before, 
								float temp_U_after, float temp_D_after, float temp_pcb_after, 
								float ADC_gain,
								uint32_t adapter_type, uint32_t trig_threshold, char *preamp_board_id_str)
{
	root_file_p->cd();
	
	TTree *tree = NULL;
	tree = (TTree*)gDirectory->Get("metadata");
	if(tree == NULL)
	{
		tree =  new TTree("metadata", "metadata");
		
		tree->Branch("voltage_U", &voltage_U,"voltage_U/F");
		tree->Branch("voltage_D", &voltage_D,"voltage_D/F");
		tree->Branch("module_id_str", module_id_str,"module_id_str/C");
		tree->Branch("SIPM_index", &SIPM_index,"sipm_index/i");
		
		tree->Branch("temp_U_before", &temp_U_before,"temp_U_before/F");
		tree->Branch("temp_D_before", &temp_D_before,"temp_D_before/F");
		tree->Branch("temp_pcb_before", &temp_pcb_before,"temp_pcb_before/F");
		
		tree->Branch("temp_U_after", &temp_U_after,"temp_U_after/F");
		tree->Branch("temp_D_after", &temp_D_after,"temp_D_after/F");
		tree->Branch("temp_pcb_after", &temp_pcb_after,"temp_pcb_after/F");
		
		tree->Branch("ADC_gain", &ADC_gain,"ADC_gain/F");
		
		tree->Branch("adapter_type", &adapter_type,"adapter_type/i");
		tree->Branch("trig_threshold", &trig_threshold,"trig_threshold/i");
		tree->Branch("preamp_board_id_str", preamp_board_id_str,"preamp_board_id_str/C");
	}
	else
	{
		tree->SetBranchAddress("voltage_U", &voltage_U);
		tree->SetBranchAddress("voltage_D", &voltage_D);
		tree->SetBranchAddress("module_id_str", &module_id_str);
		tree->SetBranchAddress("SIPM_index", &SIPM_index);
		
		tree->SetBranchAddress("temp_U_before", &temp_U_before);
		tree->SetBranchAddress("temp_D_before", &temp_D_before);
		tree->SetBranchAddress("temp_pcb_before", &temp_pcb_before);
		
		tree->SetBranchAddress("temp_U_after", &temp_U_after);
		tree->SetBranchAddress("temp_D_after", &temp_D_after);
		tree->SetBranchAddress("temp_pcb_after", &temp_pcb_after);
		
		tree->SetBranchAddress("ADC_gain", &ADC_gain);
		
		tree->SetBranchAddress("adapter_type", &adapter_type);
		tree->SetBranchAddress("trig_threshold", &trig_threshold);
		tree->SetBranchAddress("preamp_board_id_str", preamp_board_id_str);
	}
	tree->Fill();
	tree->Write("",TObject::kOverwrite);
	delete tree;
	
	return 0;
}
/*
int root_writer::save_waveform_pair(uint16_t *waveform_U, uint16_t *waveform_D)
{
	//root_file_p->cd();
	
	Tree->SetBranchAddress("waveform_U", waveform_U);
	Tree->SetBranchAddress("waveform_D", waveform_D);
	Tree->Fill();
	
	return 0; 
}
*/

int root_writer::save_waveforms(uint16_t *waveform, uint32_t N_pair)
{
	if(!opened)
		return -1;
	
	root_file_p->cd();
  
	for(uint32_t i=0;i<N_pair;i++)
	{
		memcpy(waveform_buff, waveform, waveform_max_length*2);
		waveform += waveform_max_length;
		step_Tree->Fill();
	}
	
	if(write_cnt == 10)
	{
		step_Tree->OptimizeBaskets(1024*1024*128*2, 1.5); //  "d"
	}
	
	write_cnt++;
	
	return 0; 
}




