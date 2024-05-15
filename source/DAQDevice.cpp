#include "DAQDevice.h"

DAQDevice::DAQDevice()
{
    g_stopRequested = false;
    DAQ_meas_settings.DAQ_event_count = 256; // 256;
    DAQ_meas_settings.event_samples = 64;
    DAQ_meas_settings.event_count = 1000000;
    DAQ_meas_settings.event_count = 1000000;
    DAQ_meas_settings.save_waveform = false;
    DAQ_meas_settings.send_waveform = true;
    DAQ_meas_settings.save_integral = false;
    DAQ_meas_settings.send_integral = false;
    for (int i = 0; i < 8; i++)
        DAQ_meas_settings.ch_enabled[i] = false;

    DAQ_meas_settings.ADC_gain = 1.0;
    DAQ_meas_settings.ADC_FS_voltage = 1.0;
}

DAQDevice::~DAQDevice()
{
    for (int i = 0; i < 4; i++)
    {
        root_files[i].close_file();
    }
    printf("files closed\n");
}

void DAQDevice::SendWaveform(uint16_t *waveform, uint8_t ch)
{
    const int tmp_buffer_len = 1024 * 2 + 4;
    uint8_t tmp_buffer[tmp_buffer_len];

    uint32_t len = DAQ_meas_settings.event_samples * 2;
    if (len > tmp_buffer_len)
        len = tmp_buffer_len;
    // uint32_t header = daq_pair + (DAQ_meas_settings.module%4)*4;  //first byte daq pair id, secound byte ch select  //U
    uint32_t header = ch / 2;
    if (ch % 2)
        header |= 1 << 8; // D
    *(uint32_t *)tmp_buffer = header;
    memcpy(tmp_buffer + 4, waveform, len);

    sendto(udp_sockfd, tmp_buffer, len + 4, 0, (struct sockaddr *)&udp_dest_addr, sizeof(udp_dest_addr));

    /*
    header |= 1<<8;  //D
    *(uint32_t*)tmp_buffer = header;
    memcpy(tmp_buffer+4, waveform_D, len);
    sendto(udp_sockfd, tmp_buffer, len+4, 0, (struct sockaddr*) &udp_dest_addr, sizeof(udp_dest_addr));
    */
}

int DAQDevice::Init()
{
    umask(0);

    if (init_network_interface(TCP_PORT, UDP_PORT))
    {
        printf("network interface init failed\n");
        return -1;
    }

    if (i2c.init(XDMA_bypass_dev, iic_base_addr, 0xE4 >> 1) < 0)
    {
        printf("i2c driver init failed\n");
        return -2;
    }

    for (int i = 0; i < 3; i++)
    {
        if (adc[i].init(&i2c, i, XDMA_bypass_dev, i == 0) < 0) // reset all ADC before adc0 init
        {
            printf("ADC%d init failed\n", i);
            return -3;
        }

        for (int j = 0; j < 3; j++)
        {
            adc[i].set_coarse_gain(j, DAQ_meas_settings.ADC_gain); // 1.0 volt regen!!!!!
            adc[i].set_fine_gain(j, 1.0);
            adc[i].set_coarse_offset(j, 0);
            adc[i].set_fine_offset(j, 0);
            adc[i].set_input_mux(j, 2);
        }

        printf("ADC GAIN set to:%.3f FS_voltage:%.3f\n", DAQ_meas_settings.ADC_gain, 1.0 / DAQ_meas_settings.ADC_gain);
    }

    for (int i = 0; i < 5; i++)
    {
        if (daq_pair[i].init(XDMA_c2h_dev, XDMA_bypass_dev, i, daq_pair_base_addr_table[i], ch0_bram_base_addr_table[i], ch1_bram_base_addr_table[i]) < 0)
        {
            printf("daq pair%d init failed\n", i);
            return -4;
        }
    }

    if (calib_mod.init(XDMA_bypass_dev, calib_sel_gpio_base_addr, adc, daq_pair, DAQ_meas_settings.ADC_FS_voltage) < 0)
    {
        printf("calibration module init failed\n");
        return -5;
    }

    calib_mod.run_calibration();

    for (int i = 0; i < 5; i++)
    {
        daq_pair[i].stop_sampling();
        usleep(10);
    }

    for (int i = 0; i < 4; i++)
    {
        daq_pair[i].set_mode(true, false); // double buffer enabled
        daq_pair[i].reset_buf_sel();
        daq_pair[i].reset_overwrite_err_flag();

        daq_pair[i].set_event_size_reg(DAQ_meas_settings.event_samples);
        daq_pair[i].set_event_count_reg(DAQ_meas_settings.DAQ_event_count);

        for (int ch = 0; ch < 2; ch++)
        {
            // CMD_SET_EVENT_LENGTH overwrite trigger delay settings!!!!!
            daq_pair[i].set_trigger_delay_reg(ch, 30);      // trigger signal to pulse kb 210ns 26clk
            daq_pair[i].set_delay_line_reg(ch, 1);          // daq logic + ADC delay kb 25clk ha delay_line=1 trig delay=0
            daq_pair[i].set_trigger_threshold_reg(ch, 500); // fixed trigger treshold not used!!!
            daq_pair[i].set_trigger_holdoff_reg(ch, 0);
        }
        daq_pair[i].set_daq_pair_trig_output_mux(0, sipm_daq_pair_driver::CH0_AND_CH1); // CH0_AND_CH1
        sipm_daq_pair_driver::trig_in_sel trig_in_sel;
        if (i == 0)
            trig_in_sel = sipm_daq_pair_driver::int_trig_0;
        else if (i == 1)
            trig_in_sel = sipm_daq_pair_driver::int_trig_1;
        else if (i == 2)
            trig_in_sel = sipm_daq_pair_driver::int_trig_2;
        else if (i == 3)
            trig_in_sel = sipm_daq_pair_driver::int_trig_3;

        daq_pair[i].select_trig_source(0, trig_in_sel); // own_trigger out  sipm_daq_pair_driver::own_ch_re
        daq_pair[i].select_trig_source(1, trig_in_sel); // own_trigger out
    }

    set_external_trigger();

    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            adc[i].set_input_mux(j, 2); // set input mux to ext input
        }
    }

    return 0;
}

int DAQDevice::Start()
{
    // if it is running
    int retVal = -10;
    if (!g_thread.joinable())
    {
        retVal = StartMeasurement();
        g_stopRequested.store(false);
        g_thread = std::thread(&DAQDevice::ThreadFunction, this);
        MeasurementStateChangedEvent(MeasurementStatus::Running, "");
    }
    return retVal;
}

int DAQDevice::Stop()
{
    int retVal = -10;
    if (g_thread.joinable())
    {
        g_stopRequested.store(true);
        g_thread.join();
        close_socket(tcp_sockfd);
        retVal = StopMeasurement();
    }
    // If it is not in running state do not update state and send event
    if (status == MeasurementStatus::Running)
    {
        MeasurementStateChangedEvent(MeasurementStatus::Stopped, "");
    }
    return retVal;
}

void DAQDevice::SetDataReceivedCallback(DAQCommon::MeasurementStatusChangedCallbackFunc callback)
{
    statusChangeCallback = callback;
}

int DAQDevice::GetVersion(int *swMajor, int *swMinor, int *fpgaMajor, int *fpgaMinor)
{
    *swMajor = SOFTWARE_MAJOR_VERSION;
    *swMinor = SOFTWARE_MINOR_VERSION;
    *fpgaMajor = FPGA_MAJOR_VERSION;
    *fpgaMinor = FPGA_MINOR_VERSION;
    return 0;
}

int DAQDevice::OpenNewRootFile(uint8_t channel, const char *fname)
{
    char out_file_path[PATH_MAX];
    if (DAQ_loop_enabled)
    {
        // stop measurement first
        return -1;
    }
    else if (root_files[channel].is_open())
    {
        // close root file first
        return -2;
    }
    else
    {
        measurement_ID[channel] = 0;
        snprintf(out_file_path, PATH_MAX, "%s/%s", out_data_folder_path, fname);
        printf("channel:%hhu new root file path:%s\n", channel, out_file_path);
        if (root_files[channel].open_file(out_file_path, DAQ_meas_settings.event_samples) < 0)
        {
            printf("root file:%s open failed!\n", out_file_path);
            return -3;
        }
    }
    return 0;
}

int DAQDevice::CloseRootFile(uint8_t channel)
{
    if (!root_files[channel].is_open())
    {
        // root file is not opened
        return -1;
    }
    root_files[channel].close_file();
    return 0;
}

int DAQDevice::CloseAllRootFile()
{
    for (int i = 0; i < 8; i++)
    {
        if (root_files[i].is_open())
        {
            root_files[i].close_file();
        }
    }
    return 0;
}

int DAQDevice::WriteStepMetadata(uint8_t channel, double voltage, double tempBefore, double tempAfter)
{
    if (!DAQ_meas_settings.ch_enabled[channel])
    {
        // channel not enabled
        return -1;
    }
    if (!root_files[channel].is_open())
    {
        // root file not opened
        return -2;
    }
    uint64_t timestamp = DAQCommon::get_sec();
    root_files[channel].save_step_metadata(measurement_ID[channel], voltage, tempBefore, tempAfter, timestamp);
    return 0;
}

int DAQDevice::StartNewStep()
{
    if (DAQ_loop_enabled)
    {
        // measurement already running
        return -1;
    }
    for (int i = 0; i < 8; i++)
    {
        if (root_files[i].is_open())
        {
            root_files[i].create_new_voltage_step_tree(measurement_ID[i]);
            measurement_ID[i]++;
        }
    }
    return 0;
}

int DAQDevice::StartMeasurement()
{
    bool err = false;
    if (DAQ_loop_enabled)
    {
        // measurement already running
        return -1;
    }

    if ((DAQ_meas_settings.save_waveform || DAQ_meas_settings.save_integral))
    {
        for (int i = 0; i < 8; i++)
        {
            if (DAQ_meas_settings.ch_enabled[i] && !root_files[i].is_open())
            {
                // first open file if save is enabled
                return -2;
            }
        }
    }

    for (int i = 0; i < 4; i++)
    {
        DAQ_pair_event_cnt[i] = 0;
        if (DAQ_meas_settings.ch_enabled[i * 2 + 0] || DAQ_meas_settings.ch_enabled[i * 2 + 1])
        {
            DAQ_pair_is_run[i] = true;
            highest_active_daq_pair = i;
        }
        else
        {
            DAQ_pair_is_run[i] = false;
        }
    }
    //std::cout << "Enabling loop..." << std::endl;
    DAQ_loop_enabled = true;
    first_meas_start = true;
    //std::cout << "Loop enabled" << std::endl;
    return 0;
}

int DAQDevice::StopMeasurement()
{
    if (!DAQ_loop_enabled)
    {
        // Measurement is not running
        return -1;
    }
    
    for (int i = 0; i < 5; i++)
    {
        daq_pair[i].stop_sampling();
        if (i < 4)
            DAQ_pair_is_run[i] = false;
    }
    DAQ_loop_enabled = false;
    return 0;
}

int DAQDevice::SetChannelStateBits(uint8_t activeChannels)
{
    if (DAQ_loop_enabled)
    {
        // measurement is running
        return -1;
    }
    for (int i = 0; i < 8; i++)
    {
        DAQ_meas_settings.ch_enabled[i] = !!(activeChannels & 1 << i);
    }
    return 0;
}

int DAQDevice::GetMeasurementState(bool *measurementRunning, bool *daqPairIsRunning, uint32_t *daqPairEventCount)
{
    *measurementRunning = DAQ_loop_enabled;
    std::memcpy(daqPairIsRunning, DAQ_pair_is_run, 4 * sizeof(uint32_t));
    std::memcpy(daqPairEventCount, DAQ_pair_event_cnt, 4 * sizeof(uint32_t));
    return 0;
}

int DAQDevice::SetWaveformSaveState(bool saveState)
{
    if (DAQ_loop_enabled)
    {
        //measurement is running
        return -1;
    }
    DAQ_meas_settings.save_waveform = saveState;
    return 0;
}

int DAQDevice::SetWaveformSendState(bool sendState)
{
    if (DAQ_loop_enabled)
    {
        //measurement is running
        return -1;
    }
    DAQ_meas_settings.send_waveform = sendState;
    return 0;
}

int DAQDevice::SetEventLength(uint8_t length)
{
    bool opened_file = false;
    for (int i = 0; i < 8; i++)
    {
        if (root_files[i].is_open())
            opened_file = true;
        break;
    }
    if (!DAQ_loop_enabled && !opened_file)
    {
        DAQ_meas_settings.event_samples = length;
        DAQ_meas_settings.DAQ_event_count = sipm_daq_max_samples_double_buff / length;

        for (int i = 0; i < 4; i++)
        {
            daq_pair[i].set_event_size_reg(DAQ_meas_settings.event_samples);
            daq_pair[i].set_event_count_reg(DAQ_meas_settings.DAQ_event_count);
            daq_pair[i].set_trigger_delay_reg(30); // trigger signal to pulse kb 210ns 26clk
            daq_pair[i].set_delay_line_reg(1);     // daq logic + ADC delay kb 25clk ha delay_line=1 trig delay=0
        }
    }
    else
    {
        if (opened_file && DAQ_loop_enabled)
            // Root files opened and measurement enabled
            return -1;
        else if (opened_file)
            // Root files opened
            return -2;
        else
            // Measurement enabled
            return -3;
    }
    return 0;
}

int DAQDevice::PollLoop()
{
    static uint32_t rr_cnt = 0;
    static uint32_t test_cnt = 0;
    static uint32_t send_cnt[4] = {0, 0, 0, 0};

    /*
    if (!DAQ_loop_enabled)
    {
        rr_cnt++;
        if (rr_cnt % 1000)
        {
            test_cnt++;
            test_cnt = test_cnt % 3;
            if (test_cnt == 0)
            {
                MeasurementStateChangedEvent(MeasurementStatus::Running, "Hello1");
            }
            else if (test_cnt == 1)
            {
                MeasurementStateChangedEvent(MeasurementStatus::Stopped, "Hello2");
            }
            if (test_cnt == 2)
            {
                MeasurementStateChangedEvent(MeasurementStatus::Finished, "Hello3");
            }
        }
        return 0;
    }
    */

    if (DAQ_pair_is_run[rr_cnt])
    {
        daq_pair[rr_cnt].update_status_reg_shadow();
        uint8_t state_u = daq_pair[rr_cnt].get_daq_state_from_shadow_reg(0);
        uint8_t state_d = daq_pair[rr_cnt].get_daq_state_from_shadow_reg(1);

        if (state_u == 0 && state_d == 0)
        {
            if (first_meas_start) // new measurement started -> no valid data
            {
                daq_pair[rr_cnt].start_sampling();
                if (rr_cnt == highest_active_daq_pair)
                    first_meas_start = false;
            }
            else
            {
                daq_pair[rr_cnt].start_sampling(); // able to start before dma read if doube buffering enabled

                uint8_t read_buffer_id = daq_pair[rr_cnt].get_daq_last_buffer_from_shadow_reg(0);

                uint32_t dma_cpy_bytes = DAQ_meas_settings.DAQ_event_count * DAQ_meas_settings.event_samples * 2;

                if (DAQ_meas_settings.ch_enabled[rr_cnt * 2 + 0])
                    daq_pair[rr_cnt].dma_read_to_buffer(0, dma_cpy_bytes, read_buffer_id);
                if (DAQ_meas_settings.ch_enabled[rr_cnt * 2 + 1])
                    daq_pair[rr_cnt].dma_read_to_buffer(1, dma_cpy_bytes, read_buffer_id);

                uint16_t *waveform_U = daq_pair[rr_cnt].get_data_pointer_uint16(0);
                uint16_t *waveform_D = daq_pair[rr_cnt].get_data_pointer_uint16(1);

                if (DAQ_meas_settings.save_waveform)
                {
                    if (DAQ_meas_settings.ch_enabled[rr_cnt * 2 + 0])
                        root_files[rr_cnt * 2 + 0].save_waveforms(waveform_U, DAQ_meas_settings.DAQ_event_count);

                    if (DAQ_meas_settings.ch_enabled[rr_cnt * 2 + 1])
                        root_files[rr_cnt * 2 + 1].save_waveforms(waveform_D, DAQ_meas_settings.DAQ_event_count);
                }

                if (DAQ_meas_settings.send_waveform)
                {
                    if (send_cnt[rr_cnt] % 10 == 0)
                    {
                        if (DAQ_meas_settings.ch_enabled[rr_cnt * 2 + 0])
                            DAQ_send_waveform(waveform_U, rr_cnt * 2 + 0);

                        if (DAQ_meas_settings.ch_enabled[rr_cnt * 2 + 1])
                            DAQ_send_waveform(waveform_D, rr_cnt * 2 + 1);
                    }
                    send_cnt[rr_cnt]++;
                }

                DAQ_pair_event_cnt[rr_cnt] += DAQ_meas_settings.DAQ_event_count;

                if (read_buffer_id != daq_pair[rr_cnt].get_daq_last_buffer_from_shadow_reg(1))
                {
                    printf("daq_pair%hhu doube buffer id mismatch error!\n", rr_cnt);
                }

                if (rr_cnt == 0)
                {
                    if (DAQ_pair_event_cnt[rr_cnt] % 2 == 0)
                    {
                        // printf("DAQ_pair event counts:%d %d %d %d\n", DAQ_pair_event_cnt[0],DAQ_pair_event_cnt[1],DAQ_pair_event_cnt[2],DAQ_pair_event_cnt[3]);
                    }
                }

                if (DAQ_pair_event_cnt[rr_cnt] >= DAQ_meas_settings.event_count) // if DAQ_meas_settings.event_count reached -> stop
                {
                    DAQ_pair_is_run[rr_cnt] = false;
                }

                // printf("triggered ch:%d\n", rr_cnt);
            }
        }
        else if ((state_u != 0 && state_d == 0) || (state_d != 0 && state_u == 0))
        {
            usleep(1);
            daq_pair[rr_cnt].start_sampling(); // both ch
            printf("trigger mismatch!!\n");
        }
    }

    rr_cnt++;
    rr_cnt %= 4;

    if (!(DAQ_pair_is_run[0] || DAQ_pair_is_run[1] || DAQ_pair_is_run[2] || DAQ_pair_is_run[3])) // stop if all daq pair completed
    {
        DAQ_loop_enabled = false;
        printf("Measurement completed event counts:%d %d %d %d\n", DAQ_pair_event_cnt[0], DAQ_pair_event_cnt[1], DAQ_pair_event_cnt[2], DAQ_pair_event_cnt[3]);
        MeasurementStateChangedEvent(MeasurementStatus::Finished, "");
    }

    return 0;
}

void DAQDevice::MeasurementStateChangedEvent(MeasurementStatus status, std::string message)
{
    this->status = status;
    if (statusChangeCallback != nullptr)
    {
        statusChangeCallback(status, message.c_str());
    }
}

void DAQDevice::set_external_trigger()
{
    for (int i = 0; i < 4; i++)
    {
        daq_pair[i].set_daq_pair_trig_output_mux(0, sipm_daq_pair_driver::CH0_AND_CH1); // not used

        daq_pair[i].select_trig_source(0, sipm_daq_pair_driver::ext_trig_re);
        daq_pair[i].select_trig_source(1, sipm_daq_pair_driver::ext_trig_re);
    }
}

int DAQDevice::init_network_interface(int tcp_in_port, int udp_dest_port)
{
    printf("TCP_IN_PORT:%d UDP_dest_PORT:%d\n", tcp_in_port, udp_dest_port);

    struct sockaddr_in serv_addr;
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(tcp_in_port);

    tcp_sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0); // non block added!!

    if (tcp_sockfd < 0)
    {
        printf("ERROR opening tcp command socket\n");
        return -1;
    }
    if (bind(tcp_sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("ERROR on binding tcp command socket\n");
        return -2;
    }
    listen(tcp_sockfd, 1);
    // set_socket_timeout(tcp_sockfd, 1);

    // udp out socket
    if ((udp_sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        printf("ERROR opening UDP out SOCKET\n");
        return -3;
    }

    memset((char *)&udp_dest_addr, 0, sizeof(udp_dest_addr));
    udp_dest_addr.sin_family = AF_INET;
    udp_dest_addr.sin_port = htons(udp_dest_port);

    return 0;
}

void DAQDevice::close_socket(int sockfd)
{
    shutdown(sockfd, SHUT_RDWR);
    close(sockfd);
}

void DAQDevice::command_poll_loop()
{
    static uint32_t char_counter = 0;
    static int newsockfd = -1;
    static struct sockaddr_in cli_addr;

    if (newsockfd < 0)
    {
        unsigned int clilen = sizeof(cli_addr);
        // newsockfd = accept4(tcp_sockfd, (struct sockaddr *) &cli_addr, &clilen, SOCK_NONBLOCK);
        newsockfd = accept(tcp_sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0)
        {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
            {
                // printf("accept timeout\n");
            }
            else
            {
                printf("ERROR on accept\n");
            }

            newsockfd = -1;
            return; // no incomming connection
        }
        else // new connection
        {
            printf("client %s connected\n", inet_ntoa(cli_addr.sin_addr));
            if (inet_aton(inet_ntoa(cli_addr.sin_addr), &udp_dest_addr.sin_addr) == 0)
            {
                printf("inet_aton() failed\n");
                return;
            }

            char_counter = 0;
            DAQCommon::set_socket_timeout(newsockfd, 1); // kell??
        }
    }

    // tcp connected read for commands

    while (1)
    {
        char c;
        int n = read(newsockfd, &c, 1);
        if (n == 1)
        {
            if (c != '\0' && c != '\n')
            {
                if (c != '\r')
                {
                    if (char_counter < cmd_buffer_size)
                    {
                        cmd_buffer[char_counter] = c;
                        char_counter++;
                    }
                }
            }
            else
            {
                if (char_counter < cmd_buffer_size)
                {
                    cmd_buffer[char_counter] = c;
                    char_counter++;
                }
                cmd_buffer[char_counter] = '\0';

                printf("received cmd:%s", cmd_buffer);

                char_counter = 0;

                CMDInterpreter(cmd_buffer, newsockfd);
                return;
            }
        }
        else if (n < 0)
        {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
            {
                // printf("read timeout\n");
                return;
            }
            else
            {
                // close_socket(newsockfd);  //kell????
                printf("receive failed!\n");
                newsockfd = -1;
                return;
            }
        }
        else if (n == 0)
        {
            close_socket(newsockfd);
            printf("scoket closed\n");
            newsockfd = -1;
            return;
        }
    }
}

void DAQDevice::DAQ_send_waveform(uint16_t *waveform, uint8_t ch)
{
    const int tmp_buffer_len = 1024 * 2 + 4;
    uint8_t tmp_buffer[tmp_buffer_len];

    uint32_t len = DAQ_meas_settings.event_samples * 2;
    if (len > tmp_buffer_len)
        len = tmp_buffer_len;
    // uint32_t header = daq_pair + (DAQ_meas_settings.module%4)*4;  //first byte daq pair id, secound byte ch select  //U
    uint32_t header = ch / 2;
    if (ch % 2)
        header |= 1 << 8; // D
    *(uint32_t *)tmp_buffer = header;
    memcpy(tmp_buffer + 4, waveform, len);

    sendto(udp_sockfd, tmp_buffer, len + 4, 0, (struct sockaddr *)&udp_dest_addr, sizeof(udp_dest_addr));

    /*
    header |= 1<<8;  //D
    *(uint32_t*)tmp_buffer = header;
    memcpy(tmp_buffer+4, waveform_D, len);
    sendto(udp_sockfd, tmp_buffer, len+4, 0, (struct sockaddr*) &udp_dest_addr, sizeof(udp_dest_addr));
    */
}

int DAQDevice::CMDInterpreter(const char *cmd, int in_socketfd)
{
    int cmd_len = strlen(cmd);
    char resp_buff[256] = {""};
    // printf("cmd_len:%d\n", cmd_len);

    if ((strstr(cmd, "CMD_GET_VERSION") == cmd) && (cmd_len == 16))
    {
        snprintf(resp_buff, 256, "CMD_GET_VERSION OK $ sw_ver:%.2f FPGA_ver:%.2f$\n", 1.0, 1.0); // TODO add version handling!
    }

    else if ((strstr(cmd, "CMD_OPEN_NEW_ROOT_FILE:") == cmd) && (cmd_len > 24)) // format CMD_OPEN_NEW_ROOT_FILE:channel (0-7) | filename with path from data dir
    {
        char fname_par[PATH_MAX];
        char out_file_path[PATH_MAX];
        uint8_t channel = 0;
        int ret = 0;

        if (sscanf(&cmd[23], "%hhu %s", &channel, fname_par) == 2 && strlen(fname_par) > 5 && channel < 8)
        {
            if (DAQ_loop_enabled)
            {
                snprintf(resp_buff, 256, "CMD_OPEN_NEW_ROOT_FILE ERROR code:-2 (stop first)\n");
            }
            else
            {
                if (root_files[channel].is_open())
                {
                    snprintf(resp_buff, 256, "CMD_OPEN_NEW_ROOT_FILE ERROR code:-3 (already open, close first)\n");
                }
                else
                {
                    measurement_ID[channel] = 0;
                    snprintf(out_file_path, PATH_MAX, "%s/%s", out_data_folder_path, fname_par);
                    printf("channel:%hhu new root file path:%s\n", channel, out_file_path);
                    if ((ret += root_files[channel].open_file(out_file_path, DAQ_meas_settings.event_samples)) < 0)
                    {
                        printf("root file:%s open failed!\n", out_file_path);
                        int retVal = snprintf(resp_buff, 256, "CMD_OPEN_NEW_ROOT_FILE ERROR code:-4 (open file) %s\n", out_file_path);
                        if (retVal < 0)
                        {
                            printf("Response string truncated\n");
                        }
                    }
                    else
                    {
                        snprintf(resp_buff, 256, "CMD_OPEN_NEW_ROOT_FILE:%hhu %s OK\n", channel, fname_par);
                    }
                }
            }
        }
        else
        {
            snprintf(resp_buff, 256, "CMD_OPEN_NEW_ROOT_FILE ERROR code:-1 (wrong parameter)\n");
        }
    }

    else if ((strstr(cmd, "CMD_CLOSE_ROOT_FILE:") == cmd) && (cmd_len > 21)) // format CMD_CLOSE_ROOT_FILE:1   channel (0-7)
    {
        uint8_t channel = 0;

        if (sscanf(&cmd[20], "%hhu", &channel) == 1 && channel < 8)
        {
            if (root_files[channel].is_open())
            {
                root_files[channel].close_file();
                snprintf(resp_buff, 256, "CMD_CLOSE_ROOT_FILE:%hhu OK\n", channel);
            }
            else
            {
                snprintf(resp_buff, 256, "CMD_CLOSE_ROOT_FILE: ERROR code:-2 (already closed)\n");
            }
        }
        else
        {
            snprintf(resp_buff, 256, "CMD_CLOSE_ROOT_FILE: ERROR code:-1 (wrong parameter)\n");
        }
    }

    else if ((strstr(cmd, "CMD_CLOSE_ALL_ROOT_FILE") == cmd) && (cmd_len > 22)) // format CMD_CLOSE_ALL_ROOT_FILE
    {
        for (int i = 0; i < 8; i++)
        {
            if (root_files[i].is_open())
            {
                root_files[i].close_file();
            }
        }

        snprintf(resp_buff, 256, "CMD_CLOSE_ALL_ROOT_FILE OK\n");
    }

    else if ((strstr(cmd, "CMD_WRITE_STEP_METADATA:") == cmd) && (cmd_len > 25)) // format CMD_WRITE_STEP_METADATA:channel(0-7) voltage(float) temp_before(float) temp_after(float)
    {
        uint8_t channel = 0;
        float voltage, temp_before, temp_after;

        if (sscanf(&cmd[24], "%hhu %f %f %f", &channel, &voltage, &temp_before, &temp_after) == 4 && channel < 7)
        {
            if (root_files[channel].is_open() && DAQ_meas_settings.ch_enabled[channel])
            {
                uint64_t timestamp = DAQCommon::get_sec();
                root_files[channel].save_step_metadata(measurement_ID[channel], voltage, temp_before, temp_after, timestamp);
                snprintf(resp_buff, 256, "CMD_WRITE_METADATA:%hhu %f %f %f OK\n", channel, voltage, temp_before, temp_after);
            }
            else
            {
                snprintf(resp_buff, 256, "CMD_WRITE_METADATA:%hhu ERROR code:-2 (open root file first!)\n", channel);
            }
        }
        else
        {
            snprintf(resp_buff, 256, "CMD_WRITE_METADATA: ERROR code:-1 (wrong parameter)\n");
        }
    }

    else if ((strstr(cmd, "CMD_START_NEW_STEP") == cmd) && (cmd_len == 19)) // format CMD_START_NEW_STEP  //create new tree for another SPS voltage step measurmenet data (all daq ch)
    {
        if (DAQ_loop_enabled)
        {
            snprintf(resp_buff, 256, "CMD_START_NEW_STEP ERROR code:-1 (already started)\n");
        }
        else
        {
            for (int i = 0; i < 8; i++)
            {
                if (root_files[i].is_open())
                {
                    root_files[i].create_new_voltage_step_tree(measurement_ID[i]);
                    measurement_ID[i]++;
                }
            }
        }
        snprintf(resp_buff, 256, "CMD_START_NEW_STEP OK (step:%d)\n", measurement_ID[0] - 1);
    }

    else if ((strstr(cmd, "CMD_START_MEASURE") == cmd) && (cmd_len == 18)) // format CMD_START_MEASURE
    {
        bool err = false;
        if (DAQ_loop_enabled)
        {
            snprintf(resp_buff, 256, "CMD_START_MEASURE ERROR code:-1 (already started)\n");
        }
        else if ((DAQ_meas_settings.save_waveform || DAQ_meas_settings.save_integral))
        {
            for (int i = 0; i < 8; i++)
            {
                if (DAQ_meas_settings.ch_enabled[i] && !root_files[i].is_open())
                {
                    snprintf(resp_buff, 256, "CMD_START_MEASURE ERROR code:-2 (first)ch:%d (first open file if save enabled)\n", i);
                    err = true;
                    break;
                }
            }
        }

        if (!err) // ok
        {
            for (int i = 0; i < 4; i++)
            {
                DAQ_pair_event_cnt[i] = 0;
                if (DAQ_meas_settings.ch_enabled[i * 2 + 0] || DAQ_meas_settings.ch_enabled[i * 2 + 1])
                {
                    DAQ_pair_is_run[i] = true;
                    highest_active_daq_pair = i;
                }
                else
                {
                    DAQ_pair_is_run[i] = false;
                }
            }
            DAQ_loop_enabled = true;
            first_meas_start = true;

            snprintf(resp_buff, 256, "CMD_START_MEASURE OK\n");
            printf("meas started!");
        }
    }

    else if ((strstr(cmd, "CMD_SET_CH_STATE_BITS:") == cmd) && (cmd_len >= 23)) // format CMD_SET_CH_STATE_BITS:0xFF
    {
        uint8_t active_channels = 0;
        if (sscanf(&cmd[22], "%hhx", &active_channels) == 1)
        {
            if (DAQ_loop_enabled)
            {
                snprintf(resp_buff, 256, "CMD_SET_CH_STATE_BITS ERROR code:-1 (stop first)\n");
            }
            else
            {
                for (int i = 0; i < 8; i++)
                {
                    DAQ_meas_settings.ch_enabled[i] = !!(active_channels & 1 << i);
                }
                snprintf(resp_buff, 256, "CMD_SET_CH_STATE_BITS: %hhx OK\n", active_channels);
            }
        }
        else
        {
            snprintf(resp_buff, 256, "CMD_SET_CH_STATE_BITS ERROR code:-10 (invalid parameter)\n");
        }
    }

    else if ((strstr(cmd, "CMD_STOP_MEASURE") == cmd) && (cmd_len == 17)) // format CMD_STOP_MEASURE
    {
        if (DAQ_loop_enabled)
        {
            for (int i = 0; i < 5; i++)
            {
                daq_pair[i].stop_sampling();
                if (i < 4)
                    DAQ_pair_is_run[i] = false;
            }
        }
        DAQ_loop_enabled = false;
        snprintf(resp_buff, 256, "CMD_STOP_MEASURE OK\n");
    }

    else if ((strstr(cmd, "CMD_GET_MEASUREMENT_STATE") == cmd) && (cmd_len == 26)) // format CMD_GET_MEASUREMENT_STATE
    {
        snprintf(resp_buff, 256, "CMD_GET_MEASUREMENT_STATE OK $state:%hhu %d %d %d %d %d %d %d %d$ OK\n", !!DAQ_loop_enabled, DAQ_pair_event_cnt[0], DAQ_pair_event_cnt[0], DAQ_pair_event_cnt[1], DAQ_pair_event_cnt[1], DAQ_pair_event_cnt[2], DAQ_pair_event_cnt[2], DAQ_pair_event_cnt[3], DAQ_pair_event_cnt[3]);
    }

    else if ((strstr(cmd, "CMD_SET_WAVEFORM_SAVE_STATE:") == cmd) && (cmd_len == 30)) // format CMD_SET_WAVEFORM_SAVE_STATE:state 0 or 1 1->enabled
    {
        uint8_t state;
        if (sscanf(cmd, "CMD_SET_WAVEFORM_SAVE_STATE:%hhu", &state) == 1 && state <= 1)
        {
            if (!DAQ_loop_enabled)
            {
                DAQ_meas_settings.save_waveform = !!state;
                snprintf(resp_buff, 256, "CMD_SET_WAVEFORM_SAVE_STATE:%hhu OK\n", state);
            }
            else
            {
                snprintf(resp_buff, 256, "CMD_SET_WAVEFORM_SAVE_STATE:%hhu ERROR code:-1 (stop first)\n", state);
            }
        }
        else
        {
            snprintf(resp_buff, 256, "CMD_SET_WAVEFORM_SAVE_STATE ERROR code:-10 (invalid parameter)\n");
        }
    }

    else if ((strstr(cmd, "CMD_SET_WAVEFORM_SEND_STATE:") == cmd) && (cmd_len == 30)) // format CMD_SET_WAVEFORM_SEND_STATE:state 0 or 1 1->enabled
    {
        uint8_t state;
        if (sscanf(cmd, "CMD_SET_WAVEFORM_SEND_STATE:%hhu", &state) == 1 && state <= 1)
        {
            if (!DAQ_loop_enabled)
            {
                DAQ_meas_settings.send_waveform = !!state;
                snprintf(resp_buff, 256, "CMD_SET_WAVEFORM_SEND_STATE:%hhu OK\n", state);
            }
            else
            {
                snprintf(resp_buff, 256, "CMD_SET_WAVEFORM_SEND_STATE:%hhu ERROR code:-1 (stop first)\n", state);
            }
        }
        else
        {
            snprintf(resp_buff, 256, "CMD_SET_WAVEFORM_SEND_STATE ERROR code:-10 (invalid parameter)\n");
        }
    }

    else if ((strstr(cmd, "CMD_SET_EVENT_LENGTH:") == cmd) && (cmd_len > 22) && (cmd_len <= 25)) // format CMD_SET_EVENT_LENGTH:length  2-128
    {
        uint8_t length;
        if (sscanf(cmd, "CMD_SET_EVENT_LENGTH:%hhu", &length) == 1 && length >= 2 && length <= 128)
        {
            bool opened_file = false;
            for (int i = 0; i < 8; i++)
            {
                if (root_files[i].is_open())
                    opened_file = true;
                break;
            }
            if (!DAQ_loop_enabled && !opened_file)
            {
                DAQ_meas_settings.event_samples = length;
                DAQ_meas_settings.DAQ_event_count = sipm_daq_max_samples_double_buff / length;

                for (int i = 0; i < 4; i++)
                {
                    daq_pair[i].set_event_size_reg(DAQ_meas_settings.event_samples);
                    daq_pair[i].set_event_count_reg(DAQ_meas_settings.DAQ_event_count);
                    daq_pair[i].set_trigger_delay_reg(30); // trigger signal to pulse kb 210ns 26clk
                    daq_pair[i].set_delay_line_reg(1);     // daq logic + ADC delay kb 25clk ha delay_line=1 trig delay=0
                }

                snprintf(resp_buff, 256, "CMD_SET_EVENT_LENGTH:%hhu OK\n", length);
            }
            else
            {
                if (opened_file && DAQ_loop_enabled)
                    snprintf(resp_buff, 256, "CMD_SET_EVENT_LENGTH:%hhu ERROR code:-1 (stop first and close root files)\n", length);
                else if (opened_file)
                    snprintf(resp_buff, 256, "CMD_SET_EVENT_LENGTH:%hhu ERROR code:-2 (first close root files)\n", length);
                else
                    snprintf(resp_buff, 256, "CMD_SET_EVENT_LENGTH:%hhu ERROR code:-3 (stop first)\n", length);
            }
        }
        else
        {
            snprintf(resp_buff, 256, "CMD_SET_EVENT_LENGTH ERROR code:-10 (invalid parameter)\n");
        }
    }
    /*	 CMD_SET_EVENT_LENGTH handle this
        else if((strstr(cmd, "CMD_SET_EVENT_COUNT:") == cmd) && (cmd_len > 21 ))    //format CMD_SET_EVENT_COUNT:count (count >=1)
        {
            uint32_t count;
            if(sscanf(cmd, "CMD_SET_EVENT_COUNT:%u", &count) == 1 && count >= 1)
            {
                if(!DAQ_loop_enabled)
                {
                    DAQ_meas_settings.event_count = count;
                    snprintf(resp_buff, 256, "CMD_SET_EVENT_COUNT:%u OK\n", count);
                }
                else
                {
                    snprintf(resp_buff, 256, "CMD_SET_EVENT_COUNT:%u ERROR code:-1 (stop first)\n", count);
                }
            }
            else
            {
                snprintf(resp_buff, 256, "CMD_SET_EVENT_COUNT ERROR code:-10 (invalid parameter)\n");
            }
        }
    */

    else
    {
        int i = 0;
        char tmp_buff[200];
        while (*cmd != '\0' && *cmd != '\n' && *cmd != '\r' && i < 199)
        {
            tmp_buff[i] = *cmd;
            i++;
            cmd++;
        }
        tmp_buff[i] = '\0';

        snprintf(resp_buff, 256, "INVALID CMD:%s ERROR\n", tmp_buff); // invalid command
    }

    if (strlen(resp_buff) != 0)
    {
        printf("%s", resp_buff);

        int n = write(in_socketfd, resp_buff, strlen(resp_buff)); //+1  //a lezáró 0 nem kell, a fogadó kód rakja majd a string végére
        if (n != strlen(resp_buff))
        {
            printf("ERROR writing to socket\n");
            return -1;
        }
        else
        {
            return 0;
        }
    }
    else
        return 0;
}

void DAQDevice::ThreadFunction()
{
    while (!g_stopRequested.load())
    {
        if (DAQ_loop_enabled)
        {
            PollLoop();
        }
        command_poll_loop();
        // std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
