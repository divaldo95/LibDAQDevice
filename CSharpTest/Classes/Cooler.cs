namespace LibDAQ.Classes
{
	public class Cooler
	{
        public enum CoolerState
        {
            On = 0,
            Off = 1,
            TemoeratureSensorError = 2,
            ThermalRunaway = 3,
            BlockedFan = 4
        };

        public Cooler()
        {

        }

        public Cooler(string psoc_out)
        {
            if (psoc_out.Contains("ERROR") || psoc_out.Contains("invalid"))
            {
                throw new Exception(psoc_out);
            }
            if (psoc_out.Length <= 0)
            {
                throw new Exception("psoc_out length is " + psoc_out.Length + ": " + psoc_out);
            }
            if (psoc_out.IndexOf('*') < 0)
            {
                throw new Exception("Could not find * character in psoc_out: " + psoc_out);
            }
            psoc_out = psoc_out.Remove(0, psoc_out.IndexOf('*') + 1); //remove everything until first *
            if (psoc_out.IndexOf('*') < 0)
            {
                throw new Exception("Could not find seocnd * character in psoc_out: " + psoc_out);
            }
            psoc_out = psoc_out.Remove(psoc_out.IndexOf('*'), 1); //remove last *

            string[] splitted = psoc_out.Split(',');
            if (splitted.Length != 9)
            {
                throw new Exception("Cooler parse count error");
            }

            State1 = int.Parse(splitted[0]);
            State2 = int.Parse(splitted[1]);

            int stableflags = int.Parse(splitted[2]);
            if ((stableflags & 1) > 0)
            {
                TempStableFlag1 = true;
            }
            else
            {
                TempStableFlag1 = false;
            }

            if ((stableflags & 2) > 0)
            {
                TempStableFlag2 = true;
            }
            else
            {
                TempStableFlag2 = false;
            }

            Cooler1Temp = double.Parse(splitted[3]);
            Cooler2Temp = double.Parse(splitted[4]);
            Peltier1Current = double.Parse(splitted[5]);
            Peltier1Voltage = double.Parse(splitted[6]);
            Peltier2Current = double.Parse(splitted[7]);
            Peltier2Voltage = double.Parse(splitted[8]);

            //Save timestamp
            Timestamp = new DateTimeOffset(DateTime.UtcNow).ToUnixTimeSeconds();
        }

        public string GetState(int state)
        {
            switch ((CoolerState)state)
            {
                case CoolerState.On:
                    return "Cooler ON";
                case CoolerState.Off:
                    return "Cooler OFF";
                case CoolerState.BlockedFan:
                    return "Cooler Blocked";
                case CoolerState.TemoeratureSensorError:
                    return "Cooler temperature sensor error";
                case CoolerState.ThermalRunaway:
                    return "Cooler thermal runaway error";
                default:
                    return "Unknown state";
            }
        }

        public override string ToString()
        {
            string retVal = "State1 = " + GetState(State1) + "\n"
                            + "State2 = " + GetState(State2) + "\n"
                            + "Temp Stable Flag 1 = " + TempStableFlag1.ToString() + "\n"
                            + "Temp Stable Flag 2 = " + TempStableFlag2.ToString() + "\n"
                            + "Cooler 1 Temperature = " + Cooler1Temp + "\n"
                            + "Cooler 2 Temperature = " + Cooler2Temp + "\n"
                            + "Peltier 1 Voltage = " + Peltier1Voltage + "\n"
                            + "Peltier 1 Current = " + Peltier1Current + "\n"
                            + "Peltier 2 Voltage = " + Peltier2Voltage + "\n"
                            + "Peltier 2 Current = " + Peltier2Current + "\n";
            return retVal;
        }

        public int State1 { get; set; }
        public int State2 { get; set; }
        public bool TempStableFlag1 { get; set; }
        public bool TempStableFlag2 { get; set; }
        public double Cooler1Temp { get; set; }
        public double Cooler2Temp { get; set; }
        public double Peltier1Voltage { get; set; }
        public double Peltier2Voltage { get; set; }
        public double Peltier1Current { get; set; }
        public double Peltier2Current { get; set; }
        public long Timestamp { get; set; }
    }
}

