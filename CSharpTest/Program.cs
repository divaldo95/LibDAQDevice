using System;
using System.Runtime.InteropServices;

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
        Console.WriteLine("Hello, World!");

        DAQDevice dev;
        try
        {
            dev = new DAQDevice();
            dev.Init();
            dev.Start();
            Thread.Sleep(3000);
            dev.Stop();
        }
        catch (System.Exception ex)
        {
            
            Console.WriteLine($"Exception: {ex.Message}");
        }
    }
}