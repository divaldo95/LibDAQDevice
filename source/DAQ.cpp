#include <iostream>

#include "DAQDevice.h"

/*
 *   Description:
 *   Define a shared library export for any platform.
 */
#ifdef _WIN32
# define DLL_EXPORT_TYP __declspec(dllexport)
#else
# define DLL_EXPORT_TYP
#endif

#define VISIBLE __attribute__((visibility("default")))

extern "C"
{
    DLL_EXPORT_TYP VISIBLE DAQDevice *DAQ_Dev_Create()
    {
        return new DAQDevice();
    }

    DLL_EXPORT_TYP VISIBLE void DAQ_Dev_Delete(DAQDevice *dev)
    {
        if (dev)
        {
            delete dev;
            dev = nullptr;
        }
    }

    DLL_EXPORT_TYP VISIBLE int DAQ_Dev_Init(DAQDevice *dev)
    {
        return dev -> Init();
    }

    DLL_EXPORT_TYP VISIBLE void DAQ_Dev_Start(DAQDevice *dev)
    {
        dev -> Start();
    }

    DLL_EXPORT_TYP VISIBLE void DAQ_Dev_Stop(DAQDevice *dev)
    {
        dev -> Stop();
    }

    DLL_EXPORT_TYP VISIBLE void DAQ_Dev_SetCallback(DAQDevice *dev, DAQCommon::CallbackFunc callback) 
    {
        dev -> SetDataReceivedCallback(callback);
    }
}