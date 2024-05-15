using System;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.IO.Ports;

namespace LibDAQ.Classes
{

    public class SerialPortHandler
    {
        private SerialPort? _serialPort = null;
        private static object _lockObject = new object();
        private AutoResetEvent _messageReceived;
        private int _timeout = 10000;

        private int TimeoutCounter { get; set; } = 0;
        public string LastLine { get; private set; } = "";

        public SerialPortHandler(string Port, int Baud, int Timeout = 10000)
        {
            string _Port = Port;

            _messageReceived = new AutoResetEvent(false);
            _timeout = Timeout;
            if (_serialPort == null)
            {
                _serialPort = new SerialPort();
                // Set the read/write timeouts
                _serialPort.ReadTimeout = _timeout;
                _serialPort.WriteTimeout = _timeout;
                _serialPort.PortName = _Port;
                _serialPort.BaudRate = Baud;
            }
        }

        public void Start()
        {
            if (_serialPort != null && !_serialPort.IsOpen)
            {
                _serialPort.Open();
            }
        }

        public void Stop()
        {
            if (_serialPort != null && _serialPort.IsOpen)
            {
                _serialPort.Close();
            }
        }

        protected void TimeoutHandler(bool timeoutHappened = false)
        {
            if (timeoutHappened)
            {
                TimeoutCounter++;
            }
            else
            {
                TimeoutCounter = 0;
            }

            //Try to restart serial communication
            if (TimeoutCounter >= 3)
            {
                Stop();
                Start();
            }
            
        }

        protected void WriteCommand(string command)
        {
            lock (_lockObject)
            {
                if (_serialPort == null)
                {
                    return;
                }
                if (!_serialPort.IsOpen)
                {
                    return;
                }
                int retry_cnt = 0;
                int retry_num = 3;
                while (retry_cnt < retry_num)
                {
                    try
                    {
                        Thread.Sleep(100);
                        _serialPort.DiscardInBuffer();
                        _serialPort.Write(command + System.Convert.ToChar(System.Convert.ToUInt32("0x0D", 16)));
                        LastLine = _serialPort.ReadLine();
                        if (LastLine.ToLower().Contains("ERROR".ToLower()) || LastLine.ToLower().Contains("invalid".ToLower())) //if returned with error, increase counter and try again
                        {
                            Console.WriteLine("Serial port returned with error. Trying again... (" + retry_cnt + "/" + retry_num + ")");
                            retry_cnt++;
                            Thread.Sleep(2000);
                        }
                        else //if returned without error then exit from while loop
                        {
                            break;
                        }
                    }
                    catch (TimeoutException)
                    {
                        retry_cnt++;
                        Thread.Sleep(2000);
                        continue;
                    }
                    catch (Exception e)
                    {
                        //Trace.WriteLine("Serial port error: " + e.Message);
                        throw new Exception(e.Message);
                    }
                }
                if (retry_cnt >= retry_num)
                {
                    throw new Exception("Serial port timeout");
                }
            }

        }
    }
}
