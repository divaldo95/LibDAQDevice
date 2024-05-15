using System;
using System.Diagnostics;
using System.Threading.Channels;

namespace LibDAQ.Classes
{
	public class HVPSU : SerialPortHandler
	{
        public HVPSU(string Port, int Baud, int Timeout) : base(Port, Baud, Timeout)
        {
        }

        public static string GetFormattedVoltage(float Voltage)
        {
            return Voltage.ToString("0.000").Replace(',', '.');
        }

        public static double StringToDouble(string str)
        {
            return double.Parse(str.Replace('.', ','));
        }

        public void DisableAllOutput()
        {
            WriteCommand("disable_all_output,0");
        }

        public void SetOutputVoltage(uint Channel, float Voltage)
        {
            if (Channel > 7)
            {
                throw new Exception("Channel could not be higher than 7");
            }

            if (Voltage > 46.0)
            {
                throw new Exception("Voltage could not be higher than 46V");
            }

            WriteCommand("set_hv_voltage," + Channel.ToString() + "," + GetFormattedVoltage(Voltage));
        }


        public void GetVoltageAndCurrent(uint Channel, out double Voltage, out double Current)
        {
            string data = "";
            if (Channel > 7)
            {
                throw new Exception("Channel could not be higher than 7");
            }
            Voltage = -1000.0;
            Current = -1000.0;
            WriteCommand("get_hv_voltage_current," + Channel.ToString());

            data = LastLine.Remove(0, LastLine.IndexOf('*') + 1); //remove everything until first *
            //lastline = lastline.Remove(lastline.IndexOf('*'), 1); //remove last *
            string[] splitted = data.Split(',');

            if (splitted.Length != 2)
            {
                throw new KeyNotFoundException("Voltage or Current not found in received string");
            }

            Voltage = StringToDouble(splitted[0]);
            Current = StringToDouble(splitted[1]);
        }
    }
}
