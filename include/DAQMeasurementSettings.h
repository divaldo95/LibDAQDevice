#ifndef __DAQ_MEASUREMENT_SETTINGS_H__
#define __DAQ_MEASUREMENT_SETTINGS_H__

#include <inttypes.h>

/**
 *
 *  @struct  DAQMeasurementSettings
 *  @author David Baranyai | Based on Balazs Gyongyosi's work
 *	@date   2024.05
 *  @brief  DAQ handling class
 */

struct DAQMeasurementSettings
{
  uint32_t DAQ_event_count;
  uint32_t event_samples;
  uint32_t event_count;       
  bool save_waveform;
  bool send_waveform;
  bool save_integral;
  bool send_integral;
  bool ch_enabled[8];
  float ADC_gain;
  float ADC_FS_voltage;
};

#endif /* End of __DAQ_MEASUREMENT_SETTINGS_H__ */