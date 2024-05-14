#include "DAQCommon.h"

uint64_t DAQCommon::get_millis()
{
    struct timespec te;
    clock_gettime(CLOCK_MONOTONIC, &te);
    uint64_t milliseconds = te.tv_sec*1000LL + te.tv_nsec/1000000; // calculate milliseconds
    return milliseconds;
}

uint64_t DAQCommon::get_sec() 
{
    struct timespec te;
    clock_gettime(CLOCK_MONOTONIC, &te);
    uint64_t sec = te.tv_sec;
    return sec;
}

uint64_t DAQCommon::get_micros() 
{
    struct timespec te;
    clock_gettime(CLOCK_MONOTONIC, &te);
    uint64_t milliseconds = te.tv_sec*1000000LL + te.tv_nsec/1000; 
    return milliseconds;
}


int DAQCommon::mkdir_p(const char *path)
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

int DAQCommon::set_socket_timeout(int sockfd, uint32_t time_in_ms)
{
    struct timeval timeout; 
    timeout.tv_sec = time_in_ms / 1000;
    timeout.tv_usec = (time_in_ms % 1000)*1000;
    
    if (setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
    {
        printf("setsockopt failed\n");
        return -1;
    }

    if (setsockopt (sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
    {
        printf("setsockopt failed\n");
        return -2;
    }
    
    return 0;
}

void DAQCommon::close_socket(int sockfd)
{
    shutdown(sockfd, SHUT_RDWR);
    close(sockfd);
}