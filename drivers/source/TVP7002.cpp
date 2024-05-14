#include "TVP7002.h"

TVP7002::TVP7002()
{
	initialized = false;
	bg_coarse_gain_reg_val = 0;
	fine_offset_LSB_reg_val = 0;
}


TVP7002::~TVP7002()
{
	if(initialized)
	{
		
	}
}


int TVP7002::init(i2c_driver *i2c_, uint8_t bus, const char *XDMA_bypass_dev, bool reset_first)
{
	if(initialized)
		return -1;
	
	if(i2c_ == NULL)
		return -1;
	i2c = i2c_;
	
	tvp_addr = tvp7002_i2c_addr;
	this->bus = bus;
	
	if(reset_first)
	{
		hard_reset();
	}
	
	int ret;
	if((ret = set_default_config()) < 0)
	{
		printf("TVP7002 set_default_config error code:%d\n", ret);
		return -2;
	}
	
	initialized = true;
	
	return 0;
}

void TVP7002::hard_reset()  //all adc
{
	//hard reset thr axi_i2c gpio
	i2c->write_gpio_pin(0,0); //gpio 0 L
	usleep(1000);
	i2c->write_gpio_pin(0,1); //gpio 0 H
	usleep(1000);
}

int TVP7002::set_coarse_gain(uint8_t channel, float gain)
{
	if(channel > 2)
	{
		printf("set_gain() invalid ADC channel (valid: 0,1,2)");
		return -1;
	}
	if( gain < 0.5 || gain > 2.0)
	{
		printf("gain between 0.5-2!!!");
		return -1;
	}	
	gain -= 0.5;
	gain *=10;
		
	uint8_t data = (uint8_t)gain; // 0-15
	if(data > 0x0F)
		data = 0x0F;
	printf("gain reg data:%x\n", data);
	
	int ret;
	if(channel == 0) //R
	{
		ret = i2c->write_reg_8_retry(tvp_addr, tvp7002_r_coarse_gain, data, bus);
	}
	else if(channel == 1) //G
	{
		bg_coarse_gain_reg_val = bg_coarse_gain_reg_val & 0x0F | data<<4;
		ret = i2c->write_reg_8_retry(tvp_addr, tvp7002_b_g_coarse_gain, bg_coarse_gain_reg_val, bus);
	}
	else //B
	{
		bg_coarse_gain_reg_val = bg_coarse_gain_reg_val & 0xF0 | data;
		ret = i2c->write_reg_8_retry(tvp_addr, tvp7002_b_g_coarse_gain, bg_coarse_gain_reg_val, bus);
	}
	
	if(ret < 0)
		return -2;
	
	return 0;
}

int TVP7002::set_fine_gain(uint8_t channel, double gain)
{
	if(channel > 2)
	{
		printf("set_fine_gain() invalid ADC channel (valid: 0,1,2)");
		return -1;
	}
	
	if(gain < 1.0 || gain > 2.0)
	{
		printf("set_fine_gain() invalid gain range (valid: 1.0 - 2.0)\n");
		return -1;
	}
	
	gain -= 1.0;
	const double step = 1.0/255.0;
	
	uint8_t value = (uint8_t)((gain+(step/2)) / step);
	
	int ret;
	if(channel == 0) //R
	{
		ret = i2c->write_reg_8_retry(tvp_addr, tvp7002_r_fine_gain, value, bus);
	}
	else if(channel == 1) //G
	{
		ret = i2c->write_reg_8_retry(tvp_addr, tvp7002_g_fine_gain, value, bus);
	}
	else //B
	{
		ret = i2c->write_reg_8_retry(tvp_addr, tvp7002_b_fine_gain, value, bus);
	}
	
	if(ret < 0)
		return -2;
	
	return 0;
}


int TVP7002::set_coarse_offset(uint8_t channel, int value)  //offset dac after gain stage!!!
{
	if( value > 124 || value < -124)
	{
		printf("offset between -124 - +124!!!");
		return -1;
	}	
	value /= 4;
	
	uint8_t data;
	if(value >= 0)
	{
		data = (uint8_t)value;
	}
	else
	{
		value *= -1;
		data = (uint8_t)value | 1<<5;
	}
	
	printf("offset data:%x\n", data);
	
	uint8_t reg_addr;
	if(channel == 0) //R
	{
		reg_addr = tvp7002_r_coarse_offset;
	}
	else if(channel == 1) //G
	{
		reg_addr = tvp7002_g_coarse_offset;
	}
	else //B
	{
		reg_addr = tvp7002_b_coarse_offset;
	}
		
	if(i2c->write_reg_8_retry(tvp_addr, reg_addr, data, bus) < 0)
		return -2;
	
	return 0;
}

int TVP7002::set_fine_offset(uint8_t channel, int value)  
{
	if( value > 511 || value < -511)
	{
		printf("offset between -511 - +511!!!");
		return -1;
	}	
	
	
	uint16_t data;
	if(value >= 0)
	{
		data = value | 1<<9;
	}
	else
	{
		//value = ~value;
		value = value & 0x1FF;
		data = value;
	}
	
	printf("fine offset data:%x\n", data);
	
	uint8_t reg_addr;
	if(channel == 0) //R
	{
		reg_addr = tvp7002_r_fine_offset_MSB;
		fine_offset_LSB_reg_val = fine_offset_LSB_reg_val & 0xCF | (data & 0x03)<<4;
	}
	else if(channel == 1) //G
	{
		reg_addr = tvp7002_g_fine_offset_MSB;
		fine_offset_LSB_reg_val = fine_offset_LSB_reg_val & 0xF3 | (data & 0x03)<<2;
	}
	else //B
	{
		reg_addr = tvp7002_b_fine_offset_MSB;
		fine_offset_LSB_reg_val = fine_offset_LSB_reg_val & 0xFC | (data & 0x03)<<0;
	}
		
	if(i2c->write_reg_8_retry(tvp_addr, reg_addr, (uint8_t)(data>>2), bus) < 0)
		return -2;
	
	if(i2c->write_reg_8_retry(tvp_addr, tvp7002_fine_offset_LSB, fine_offset_LSB_reg_val, bus) < 0)
		return -2;
	
	return 0;
}

int TVP7002::set_input_mux(uint8_t channel, uint8_t input)
{
	if(channel > 2)
	{
		printf("set_input_mux() invalid ADC channel (valid: 0,1,2)");
		return -1;
	}
	if( input > 2)
	{
		printf("valid input: 0,1,2");
		return -1;
	}	
	
	if(channel == 0) //R
	{
		input_mux_reg_val = input_mux_reg_val & 0xCF | input<<4;
	}
	else if(channel == 1) //G
	{
		input_mux_reg_val = input_mux_reg_val & 0xF3 | input<<2;
	}
	else //B
	{
		input_mux_reg_val = input_mux_reg_val & 0xFC | input<<0;
	}
	
	//printf("mux regval:%x\n", input_mux_reg_val);
	
	int ret = i2c->write_reg_8_retry(tvp_addr, tvp7002_input_mux_sel_1, input_mux_reg_val, bus);
	if(ret < 0)
		return -2;
	
	return 0;
}

int TVP7002::set_default_config() 
{
int ret;
   uint8_t data;
   data = 0xF6;  //sync controls
   if((ret = i2c->write_reg_8_retry(tvp_addr, tvp7002_sync_control_1, data, bus)) < 0)
   {
	   printf("ret val:%d\n", ret);
	   return -1;
   }
	  
   
   
   data = 0x9E;  //ext clamp ext coast active high 
   if(i2c->write_reg_8_retry(tvp_addr, tvp7002_h_pll_and_clamp_control, data, bus) < 0)
	   return -2;
   
   data = 0x58;  //bottom level fine clamp
   if(i2c->write_reg_8_retry(tvp_addr, tvp7002_sync_on_green_threshold, data, bus) < 0)
	   return -3;
   
   data = 0x12;  //disable hpll
   if(i2c->write_reg_8_retry(tvp_addr, tvp7002_misc_control_1, data, bus) < 0)
	   return -4;
   
   data = 0x62;  //data out enable 
   if(i2c->write_reg_8_retry(tvp_addr, tvp7002_misc_control_2, data, bus) < 0)
	   return -5;
   
   data = 0x00;  //clock rising edge 
   if(i2c->write_reg_8_retry(tvp_addr, tvp7002_misc_control_3, data, bus) < 0)
	   return -6;
   
   data = 0x80;  //adc bias current >110MHz   or F0 to max current?
   if(i2c->write_reg_8_retry(tvp_addr, tvp7002_ADC_setup, data, bus) < 0)
	   return -7;
   
   data = 0x00;  //  disable alc
   if(i2c->write_reg_8_retry(tvp_addr, tvp7002_alc_enable, data, bus) < 0)
	   return -8;
   
   data = 0x03;  //  sync bypass
   if(i2c->write_reg_8_retry(tvp_addr, tvp7002_sync_bypass, data, bus) < 0)
	   return -9;
   
   data = 0x60;  //  all ch power up
   if(i2c->write_reg_8_retry(tvp_addr, tvp7002_power_control, data, bus) < 0)
	   return -10;
   
   data = 0xC0;  //  ext clk sel
   if(i2c->write_reg_8_retry(tvp_addr, tvp7002_inp_mux_sel_2, data, bus) < 0)
	   return -11;
   
   
   //offset , gain
   
   bg_coarse_gain_reg_val = 0x55;
   if(i2c->write_reg_8_retry(tvp_addr, tvp7002_b_g_coarse_gain, bg_coarse_gain_reg_val, bus) < 0)
	   return -12;
 
   data = 0x00;
   if(i2c->write_reg_8_retry(tvp_addr, tvp7002_r_coarse_offset, data, bus) < 0)
		return -13;
	
   data = 0x00;
   if(i2c->write_reg_8_retry(tvp_addr, tvp7002_g_coarse_offset, data, bus) < 0)
		return -14;

   data = 0x00;
   if(i2c->write_reg_8_retry(tvp_addr, tvp7002_b_coarse_offset, data, bus) < 0)
		return -15;
	
   input_mux_reg_val = 0;
   if(i2c->write_reg_8_retry(tvp_addr, tvp7002_input_mux_sel_1, input_mux_reg_val, bus) < 0)
		return -16;
	
	
	
	//data = 0x2A;  //input 3 
   //i2c->write_reg_8_retry(tvp_addr, tvp7002_input_mux_sel_1, data, bus);
   /*
   data = 0x00;  //green 1 input
   //data = 0x04;  //green 2 input 
   i2c->write_reg_8_retry(tvp_addr, tvp7002_input_mux_sel_1, data, bus);
   
   data = 0x77;  //default gain 1.2x 0.7v
   //data = 0xAA; 
   i2c->write_reg_8_retry(tvp_addr, tvp7002_b_g_coarse_gain, data, bus);
   
   data = 0x10;  //default coarse offset
   data = 0x0; 
   //data = 25;
   i2c->write_reg_8_retry(tvp_addr, tvp7002_g_coarse_offset, data, bus);
   */
   
   
   
   
   return 0;
}

