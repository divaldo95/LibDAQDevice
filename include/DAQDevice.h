#ifndef __DAQ_DEVICE_H__
#define __DAQ_DEVICE_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>    
#include <sys/stat.h>  
#include <signal.h>
#include <thread>
#include <atomic>

#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "DAQCommon.h"

#include "TVP7002.h"
#include "i2c_driver.h"
#include "memory_access.h"
#include "root_writer.h"
#include "calibration_module.h"
#include "sipm_daq_pair_driver.h"
#include "DAQMeasurementSettings.h"
#include "Enums.h"

/**
 *
 *  @class  DAQDevice
 *  @author David Baranyai | Based on Balazs Gyongyosi's work
 *	@date   2024.05
 *  @brief  DAQ handling class
 */

#define sampling_logic_base             0x000A1000
#define bram_start_addr 	            0x00080000
#define iic_base_addr 		            0x00A50000
#define calib_sel_gpio_base_addr 		0x00A60000

#define cmd_buffer_size 512
#define UDP_packet_max_size 1024

class DAQDevice
{
//-------------------Private variables-------------------
private:
    char out_data_folder_path[128] = "/home/user/meas_data/";

    char *XDMA_bypass_dev = (char*)"/dev/xdma0_bypass";
    char *XDMA_c2h_dev    = (char*)"/dev/xdma0_c2h_0";

    uint32_t daq_pair_base_addr_table[5] = {0x00A00000, 0x00A10000, 0x00A20000, 0x00A30000, 0x00A40000};
    uint32_t ch0_bram_base_addr_table[5] = {0x00000000, 0x00200000, 0x00400000, 0x00600000, 0x00800000};
    uint32_t ch1_bram_base_addr_table[5] = {0x00100000, 0x00300000, 0x00500000, 0x00700000, 0x00000000};

    //global variables
    i2c_driver i2c;
    TVP7002 adc[3];
    sipm_daq_pair_driver daq_pair[5];
    root_writer root_files[8];
    calibration_module calib_mod;
    bool first_meas_start = false;
    uint32_t highest_active_daq_pair;
    uint32_t measurement_ID[8] = {0,0,0,0,0,0,0,0};

    DAQMeasurementSettings DAQ_meas_settings;
    bool DAQ_loop_enabled = false;
    bool DAQ_pair_is_run[4] = {false, false, false, false};
    uint32_t DAQ_pair_event_cnt[4] = {0,0,0,0};

    //network global variables
    int tcp_sockfd = -1;
    int udp_sockfd = -1;
    struct sockaddr_in udp_dest_addr;

    char cmd_buffer[cmd_buffer_size+2];

    int TCP_PORT = 50000;
	int UDP_PORT = 50001;

    MeasurementStatus status = MeasurementStatus::Stopped;

    std::atomic<bool> g_stopRequested;
    std::thread g_thread;
    DAQCommon::CallbackFunc g_callback = nullptr;

//-------------------Public functions-------------------
public:
    DAQDevice();
    ~DAQDevice();

    void SendWaveform(uint16_t *waveform, uint8_t ch);

    int Init();

    void Start();
    void Stop();
    void SetDataReceivedCallback(DAQCommon::CallbackFunc callback);

//-------------------Private functions-------------------
private:
    void set_external_trigger();
    int init_network_interface(int tcp_in_port, int udp_dest_port);
    void close_socket(int sockfd);
    void command_poll_loop();
    void DAQ_send_waveform(uint16_t *waveform, uint8_t ch);

    int CMDInterpreter(const char* cmd, int in_socketfd);
    void ThreadFunction();
    int PollLoop();

    void MeasurementStateChangedEvent(MeasurementStatus status, std::string message);
};




#endif /* End of __DAQ_DEVICE_H__ */