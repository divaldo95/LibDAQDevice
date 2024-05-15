using System;
namespace LibDAQ.Classes
{
	public static class MeasurementMode
	{
        public enum MeasurementModes
        {
            IV,
            SPS,
            DualSPS,
            QuadSPS,
            SPSVoltageMeasurement,
            DMMResistanceMeasurement,
            Off
        }

        public static char GetMeasurementChar(MeasurementModes m)
        {
            char ret = 'o';
            switch (m)
            {
                case MeasurementModes.IV:
                    ret = 'i';
                    break;
                case MeasurementModes.SPS:
                    ret = 's';
                    break;
                case MeasurementModes.DualSPS:
                    ret = 'd';
                    break;
                case MeasurementModes.QuadSPS:
                    ret = 'q';
                    break;
                case MeasurementModes.SPSVoltageMeasurement:
                    ret = 'v';
                    break;
                case MeasurementModes.DMMResistanceMeasurement:
                    ret = 'r';
                    break;
                case MeasurementModes.Off:
                    ret = 'o';
                    break;
                default:
                    break;
            }
            return ret;
        }
    }
}

