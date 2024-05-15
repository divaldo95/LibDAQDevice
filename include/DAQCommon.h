#ifndef __DAQ_COMMON_H__
#define __DAQ_COMMON_H__

#include <string>
#include <new>
#include <memory>
#include <filesystem>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <limits.h>    
#include <sys/stat.h>  

#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "Enums.h"

/**
 *
 *  @namespace  DAQCommon
 *  @author David Baranyai
 *	@date   2024.05
 *  @brief  Common functions, typedefs, etc, for DAQ readout and settings.
 */

namespace DAQCommon
{
    // Define the callback function signature
    using MeasurementStatusChangedCallbackFunc = void(*)(MeasurementStatus, const char*);
    using EventCountChangedCallbackFunc = void(*)(uint32_t);

    uint64_t get_millis();
    uint64_t get_sec();
    uint64_t get_micros();
    int mkdir_p(const char *path);
    int set_socket_timeout(int sockfd, uint32_t time_in_ms);
    void close_socket(int sockfd);

}


#endif /* End of __DAQ_COMMON_H__ */