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
    private static extern void DAQ_Dev_Start(IntPtr daqDev);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void DAQ_Dev_Stop(IntPtr daqDev);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int DAQ_Dev_Init(IntPtr daqDev);

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
        DAQ_Dev_Start(daqDev);
    }

    public void Stop()
    {
        DAQ_Dev_Stop(daqDev);
    }

    private void SetCallback(CallbackDelegate callback)
    {
        DAQ_Dev_SetCallback(daqDev, callback);
    }

    private static void CallbackFunction(MeasurementStatus status, string message)
    {
        Console.WriteLine("Callback received status from C++: " + status.ToString());
        Console.WriteLine("Callback received from C++: " + message);
    }
}
