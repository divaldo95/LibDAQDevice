using System.Runtime.InteropServices;

public class DAQDevice : IDisposable
{
    const string LibraryName = "DAQ";
    private bool disposedValue;
    private IntPtr daqDev;

    // Define the delegate for the callback function
    public delegate void CallbackDelegate(MeasurementStatus status, string message);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr DAQ_Dev_Create();

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr DAQ_Dev_Delete(IntPtr daqDev);
    
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void DAQ_Dev_SetCallback(IntPtr daqDev, CallbackDelegate callback);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int DAQ_Dev_Start(IntPtr daqDev);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int DAQ_Dev_Stop(IntPtr daqDev);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int DAQ_Dev_Init(IntPtr daqDev);
    
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int DAQ_Dev_GetVersion(IntPtr daqDev, out int swMajor, out int swMinor, out int fpgaMajor, out int fpgaMinor);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int DAQ_Dev_OpenNewRootFile(IntPtr daqDev, byte channel, string fileName);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int DAQ_Dev_CloseRootFile(IntPtr daqDev, byte channel);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int DAQ_Dev_CloseAllRootFile(IntPtr daqDev);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int DAQ_Dev_WriteStepMetadata(IntPtr daqDev, byte channel, double voltage, double tempBefore, double tempAfter);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int DAQ_Dev_StartNewStep(IntPtr daqDev);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int DAQ_Dev_SetChannelStateBits(IntPtr daqDev, byte activeChannels);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int DAQ_Dev_GetMeasurementState(IntPtr daqDev, out bool measurementRunning, IntPtr daqPairIsRunning, IntPtr daqPairEventCount);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int DAQ_Dev_SetWaveformSaveState(IntPtr daqDev, bool state);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int DAQ_Dev_SetWaveformSendState(IntPtr daqDev, bool state);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int DAQ_Dev_SetEventLength(IntPtr daqDev, byte length);

    protected virtual void Dispose(bool disposing)
    {
        if (!disposedValue)
        {
            if (disposing)
            {
                // TODO: dispose managed state (managed objects)
                Stop();
                DAQ_Dev_Delete(daqDev);
            }

            // TODO: free unmanaged resources (unmanaged objects) and override finalizer
            // TODO: set large fields to null
            disposedValue = true;
        }
    }

    // // TODO: override finalizer only if 'Dispose(bool disposing)' has code to free unmanaged resources
    // ~DAQDevice()
    // {
    //     // Do not change this code. Put cleanup code in 'Dispose(bool disposing)' method
    //     Dispose(disposing: false);
    // }

    public void Dispose()
    {
        // Do not change this code. Put cleanup code in 'Dispose(bool disposing)' method
        Dispose(disposing: true);
        GC.SuppressFinalize(this);
    }

    public DAQDevice()
    {
        daqDev = DAQ_Dev_Create();
        SetCallback(CallbackFunction);
    }

    public bool WaitForMeasurementFinish(TimeSpan timeout)
    {
        Console.WriteLine("Waiting for measurement to finish with a timeout of " + timeout.TotalSeconds + " seconds...");
        return measurementFinishedEvent.WaitOne(timeout); // Wait until the event is signaled or the timeout expires
    }

    public void Init()
    {
        int retVal = DAQ_Dev_Init(daqDev);
        switch (retVal)
        {
            case -1:
                throw new ApplicationException("Failed to initialize network interface");
            case -2:
                throw new ApplicationException("Failed to initialize I2C interface");
            case -3:
                throw new ApplicationException("Failed to initialize ADCs");
            case -4:
                throw new ApplicationException("Failed to initialize DAQ pairs");
            case -5:
                throw new ApplicationException("Failed to initialize calibration module");
            default:
                break;
        } 
    }

    public void Start()
    {
        int retVal = DAQ_Dev_Start(daqDev);
        switch (retVal)
        {
            case -1:
                throw new ApplicationException("Measurement already running");
            case -2:
                throw new ApplicationException("Root file is not opened while save is enabled");
            default:
                break;
        }
        measurementFinishedEvent = new ManualResetEvent(false);
    }

    public void Stop()
    {
        int retVal = DAQ_Dev_Stop(daqDev);
        switch (retVal)
        {
            case -1:
                throw new ApplicationException("Measurement is not running");
            default:
                break;
        }
    }

    public void GetVersion(out int swMajor, out int swMinor, out int fpgaMajor, out int fpgaMinor)
    {
        DAQ_Dev_GetVersion(daqDev, out swMajor, out swMinor, out fpgaMajor, out fpgaMinor);
    }

    public void OpenNewRootFile(int channel, string fileName)
    {
        if (channel > 7 || channel < 0)
        {
            throw new ArgumentOutOfRangeException("Channel must be between 0 and 7");
        }
        byte byteValue = IntToByte(channel);
        int retVal = DAQ_Dev_OpenNewRootFile(daqDev, byteValue, fileName);
        switch (retVal)
        {
            case -1:
                throw new ApplicationException("Measurement is running, stop it first");
            case -2:
                throw new ApplicationException("Root file is already opened, close it first");
            case -3:
                throw new ApplicationException("Failed to open root file");
            default:
                break;
        }
    }

    public void CloseRootFile(int channel)
    {
        if (channel > 7 || channel < 0)
        {
            throw new ArgumentOutOfRangeException("Channel must be between 0 and 7");
        }
        byte byteValue = IntToByte(channel);
        int retVal = DAQ_Dev_CloseRootFile(daqDev, byteValue);
        switch (retVal)
        {
            case -1:
                throw new ApplicationException("Root file is not opened");
            default:
                break;
        }
    }

    public void CloseAllRootFile()
    {
        int retVal = DAQ_Dev_CloseAllRootFile(daqDev);
        switch (retVal)
        {
            default:
                break;
        }
    }

    public void StartNewStep()
    {
        int retVal = DAQ_Dev_StartNewStep(daqDev);
        switch (retVal)
        {
            case -1:
                throw new ApplicationException("Measurement already running");
            default:
                break;
        }
    }

    public void SetChannelStateBits(byte activeChannels)
    {
        int retVal = DAQ_Dev_SetChannelStateBits(daqDev, activeChannels);
        switch (retVal)
        {
            case -1:
                throw new ApplicationException("Measurement already running");
            default:
                break;
        }
    }

    public void GetMeasurementState(out bool measurementRunning, out bool[] daqPairIsRunning, out uint[] daqPairEventCount)
    {
        daqPairEventCount = new uint[4];
        daqPairIsRunning = new bool[4];

        // Allocate memory to hold the array in unmanaged memory
        IntPtr countDataArrayPtr = Marshal.AllocHGlobal(4 * sizeof(uint));
        IntPtr runningDataArrayPtr = Marshal.AllocHGlobal(4 * sizeof(bool));

        // Call the C++ function to populate the array
        int retVal = DAQ_Dev_GetMeasurementState(daqDev, out measurementRunning, runningDataArrayPtr, countDataArrayPtr);

        // Manually copy the data from unmanaged memory to the C# array
        for (int i = 0; i < 4; i++)
        {
            daqPairEventCount[i] = (uint)Marshal.ReadInt32(countDataArrayPtr, i * sizeof(uint));
            daqPairIsRunning[i] = Marshal.ReadByte(runningDataArrayPtr, i * sizeof(byte)) != 0;
        }

        // Free the allocated memory
        Marshal.FreeHGlobal(countDataArrayPtr);
        Marshal.FreeHGlobal(runningDataArrayPtr);

        switch (retVal)
        {
            default:
                break;
        }
    }

    public void SetWaveformSaveState(bool state)
    {
        int retVal = DAQ_Dev_SetWaveformSaveState(daqDev, state);
        switch (retVal)
        {
            case -1:
                throw new ApplicationException("Measurement already running");
            default:
                break;
        }
    }

    public void SetWaveformSendState(bool state)
    {
        int retVal = DAQ_Dev_SetWaveformSendState(daqDev, state);
        switch (retVal)
        {
            case -1:
                throw new ApplicationException("Measurement already running");
            default:
                break;
        }
    }

    public void SetEventLength(int length)
    {
        byte byteValue = IntToByte(length);
        int retVal = DAQ_Dev_SetEventLength(daqDev, byteValue);
        switch (retVal)
        {
            case -1:
                throw new ApplicationException("Root files are opened and measurement is running");
            case -2:
                throw new ApplicationException("Root files are opened");
            case -3:
                throw new ApplicationException("Measurement is running");
            default:
                break;
        }
    }

    private void SetCallback(CallbackDelegate callback)
    {
        DAQ_Dev_SetCallback(daqDev, callback);
    }

    private ManualResetEvent measurementFinishedEvent = new ManualResetEvent(false);

    private void CallbackFunction(MeasurementStatus status, string message)
    {
        //Console.WriteLine("Callback received status from C++: " + status.ToString());
        if (!string.IsNullOrWhiteSpace(message))
        {
            Console.WriteLine("Measurement Status message: " + message);
        }

        bool isRunning;
        uint[] dataCount;
        bool[] runningData;


        GetMeasurementState(out isRunning, out runningData, out dataCount);
        Console.WriteLine($"Running: {isRunning}");
        Console.WriteLine(string.Join(",", dataCount));
        Console.WriteLine(string.Join(",", runningData));

        measurementFinishedEvent.Set();
    }

    public static byte IntToByte(int number)
    {
        return (byte)(number & 0xFF);
    }
}
