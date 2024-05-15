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

    DLL_EXPORT_TYP VISIBLE void DAQ_Dev_SetCallback(DAQDevice *dev, DAQCommon::MeasurementStatusChangedCallbackFunc callback) 
    {
        dev -> SetDataReceivedCallback(callback);
    }

    DLL_EXPORT_TYP VISIBLE void DAQ_Dev_GetVersion(DAQDevice *dev, int *swMajor, int *swMinor, int *fpgaMajor, int *fpgaMinor) 
    {
        dev -> GetVersion(swMajor, swMinor, fpgaMajor, fpgaMinor);
    }

    DLL_EXPORT_TYP VISIBLE void DAQ_Dev_OpenNewRootFile(DAQDevice *dev, uint8_t channel, char *fname) 
    {
        dev -> OpenNewRootFile(channel, fname);
    }

    DLL_EXPORT_TYP VISIBLE void DAQ_Dev_CloseRootFile(DAQDevice *dev, uint8_t channel) 
    {
        dev -> CloseRootFile(channel);
    }

    DLL_EXPORT_TYP VISIBLE void DAQ_Dev_CloseAllRootFile(DAQDevice *dev) 
    {
        dev -> CloseAllRootFile();
    }

    DLL_EXPORT_TYP VISIBLE void DAQ_Dev_WriteStepMetadata(DAQDevice *dev, uint8_t channel, double voltage, double tempBefore, double tempAfter)
    {
        dev -> WriteStepMetadata(channel, voltage, tempBefore, tempAfter);
    }

    DLL_EXPORT_TYP VISIBLE void DAQ_Dev_StartNewStep(DAQDevice *dev)
    {
        dev -> StartNewStep();
    }

    DLL_EXPORT_TYP VISIBLE void DAQ_Dev_SetChannelStateBits(DAQDevice *dev, uint8_t activeChannels)
    {
        dev -> SetChannelStateBits(activeChannels);
    }

    DLL_EXPORT_TYP VISIBLE void DAQ_Dev_GetMeasurementState(DAQDevice *dev, bool *measurementRunning, bool *daqPairIsRunning, uint32_t *daqPairEventCount)
    {
        dev -> GetMeasurementState(measurementRunning, daqPairIsRunning, daqPairEventCount);
    }

    DLL_EXPORT_TYP VISIBLE void DAQ_Dev_SetWaveformSaveState(DAQDevice *dev, bool state)
    {
        dev -> SetWaveformSaveState(state);
    }

    DLL_EXPORT_TYP VISIBLE void DAQ_Dev_SetWaveformSendState(DAQDevice *dev, bool state)
    {
        dev -> SetWaveformSendState(state);
    }

    DLL_EXPORT_TYP VISIBLE void DAQ_Dev_SetEventLength(DAQDevice *dev, uint8_t length)
    {
        dev -> SetEventLength(length);
    }
}