using System;
using System.Runtime.InteropServices;
using LibDAQ.Classes;

public enum MeasurementStatus
{
    Stopped,
    Running,
    Finished
}

internal class Program
{
    

    // Define the callback function
    
    private static void Main(string[] args)
    {
        int swMajor, swMinor, fpgaMajor, fpgaMinor;

        DAQDevice dev;
        try
        {
            HVPSU hvpsu = new HVPSU("/dev/ttyUSB1", 115200, 10000);
            PSoCCommunicator pulser = new PSoCCommunicator("/dev/ttyUSB0", 115200, 10000);

            dev = new DAQDevice();
            dev.GetVersion(out swMajor, out swMinor, out fpgaMajor, out fpgaMinor);
            Console.WriteLine($"DAQ software version: {swMajor}.{swMinor}");
            Console.WriteLine($"FPGA software version: {fpgaMajor}.{fpgaMinor}");

            hvpsu.Start();
            pulser.Start();

            //For safety
            hvpsu.DisableAllOutput();
            pulser.SetMode(0, 0, 0, 0, MeasurementMode.MeasurementModes.Off, [0, 0, 0, 0]);

            dev.Init();
            dev.SetChannelStateBits(1); //enable first channel
            dev.Start();

            pulser.SetMode(0, 0, 0, 0, MeasurementMode.MeasurementModes.SPS, [0, 0, 0, 0]);
            hvpsu.SetOutputVoltage(0, 39.0f);

            bool finished = dev.WaitForMeasurementFinish(TimeSpan.FromSeconds(60));
            if (finished)
            {
                Console.WriteLine("Finished");
            }
            else 
            {
                Console.WriteLine("Timout");
                dev.Stop();
            }

            hvpsu.DisableAllOutput();
            pulser.SetMode(0, 0, 0, 0, MeasurementMode.MeasurementModes.Off, [0, 0, 0, 0]);
        }
        catch (System.Exception ex)
        {
            
            Console.WriteLine($"Exception: {ex.Message}");
        }
    }
}