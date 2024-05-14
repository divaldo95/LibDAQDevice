#include "i2c_driver.h"




i2c_driver::i2c_driver()
{
	initialized = false;
	selected_bus = 10;
}


i2c_driver::~i2c_driver()
{
	if(initialized)
	{
		//delete mem_i2c;
	}
	
}


int i2c_driver::init(const char *XDMA_bypass_dev, uint32_t iic_base_addr, uint8_t pca9544a_addr)
{
	if(initialized)
		return -1;
	
	this->pca9544a_addr = pca9544a_addr;
	
	if(mem_i2c.init_mmap(XDMA_bypass_dev, iic_base_addr, iic_memory_length) < 0)
	{
		printf("i2c memory access init failed\n");
		initialized = false;
		return -2;
	}
	
	init_axi_i2c();
	
	if(pca9544a_addr != 0)
	{
		if(select_bus(0) < 0)
		{
			printf("i2c_driver::init i2c mux init failed\n");
			return -3;
		}
	}
	
	initialized = true;
	
	return 0;
}

void i2c_driver::init_axi_i2c()
{
	mem_i2c.write_32(i2c_SOFTR_reg, 0x0A);  //soft reset
	usleep(2000);
	mem_i2c.write_32(i2c_GIE_reg, 0);  //disable iic global interrupts
	mem_i2c.write_32(i2c_IER_reg, 0);  //disable all interrupts
	
	mem_i2c.write_32(i2c_CR_reg, 0x3);  //enable iic peripheral + tx fifo reset
	usleep(10);
	mem_i2c.write_32(i2c_CR_reg, 0x1);  //enable iic peripheral + tx fifo reset clear
	
	usleep(10000);
}

inline int i2c_driver::wait_TX_FIFO_empty(uint32_t timeout_ms)
{
	for(uint32_t i=0;i<timeout_ms*2;i++)
	{
		if(mem_i2c.read_32(i2c_SR_reg) & i2c_SR_TX_FIFO_EMPTY_bit)
		{
			return 0;
		}
		usleep(500);
	}
	return -1; //timeout
}

inline int i2c_driver::wait_bus_not_busy(uint32_t timeout_ms)
{
	for(uint32_t i=0;i<timeout_ms*2;i++)
	{
		if(!(mem_i2c.read_32(i2c_SR_reg) & i2c_SR_BUS_BUSY_bit))
		{
			return 0;
		}
		usleep(500);
	}
	return -1; //timeout
}

inline void i2c_driver::clear_isr_bits()
{
	if(mem_i2c.read_32(i2c_ISR_reg) & i2c_ISR_transmit_error)	
		mem_i2c.write_32(i2c_ISR_reg,  i2c_ISR_transmit_error);
	
	if(mem_i2c.read_32(i2c_ISR_reg) & i2c_ISR_arbitrtion_lost)	
		mem_i2c.write_32(i2c_ISR_reg, i2c_ISR_arbitrtion_lost);
}

int i2c_driver::select_bus(uint8_t bus)
{
	if(bus > 3)
		return -10;
	if(selected_bus != bus)
	{
		if(write_byte_retry(pca9544a_addr, 0x04 | bus, 3) < 0)
		{
			selected_bus = 10; 
			return -1;
		}
		else
		{
			selected_bus = bus;
			return 0;
		}
	}
	else
	{
		return 0;
	}
}	

int i2c_driver::write_byte(uint8_t ic_addr, uint8_t data)
{
	if(wait_bus_not_busy(1) < 0)
	{
		init_axi_i2c();
		usleep(1000);
		if(wait_bus_not_busy(2) < 0)
		{
			printf("i2c bus hard locked!!!\n");
			return -10;
		}
	}
	
	clear_isr_bits();
	
	mem_i2c.write_32(i2c_CR_reg, 0x3);  //enable iic peripheral + tx fifo reset
	usleep(10);
	mem_i2c.write_32(i2c_CR_reg, 0x1);  //enable iic peripheral + tx fifo reset clear
	usleep(10);
	
	mem_i2c.write_32(i2c_TX_FIFO_reg, 0x100 | ((uint32_t)(ic_addr<<1)));   // start bit 1 rw bit 0 ami write ezért nem foglalkozok vele
	mem_i2c.write_32(i2c_TX_FIFO_reg, 0x200 | ((uint32_t)data));  //last byte and stop bit 1
	
	usleep(200);
	
	if(mem_i2c.read_32(i2c_ISR_reg) & i2c_ISR_transmit_error)
	{
		return -2; // no ack?
	}
	if(mem_i2c.read_32(i2c_ISR_reg) & i2c_ISR_arbitrtion_lost)
	{
		return -3; // bus locked?
	}
	
	if(wait_bus_not_busy(1) < 0)
	{
		init_axi_i2c();
		return -4; //bus locked
	}
	
	return 0;
}

int i2c_driver::write_byte_retry(uint8_t ic_addr, uint8_t data, uint32_t retry)
{
	int ret = 0;
	for(uint32_t i=0;i<retry;i++)
	{
		if((ret = write_byte(ic_addr, data)) >= 0)
			break;
	}
	return ret;
}

int i2c_driver::write_reg_8(uint8_t ic_addr, uint8_t reg_addr, uint8_t data, uint8_t bus)
{
	if(pca9544a_addr != 0)
	{
		if(select_bus(bus) < 0)
			return -20;
		usleep(10);
	}
	
	if(wait_bus_not_busy(1) < 0)
	{
		init_axi_i2c();
		usleep(1000);
		if(wait_bus_not_busy(2) < 0)
		{
			printf("i2c bus hard locked!!!\n");
			return -10;
		}
	}
	
	clear_isr_bits();
	
	mem_i2c.write_32(i2c_CR_reg, 0x3);  //enable iic peripheral + tx fifo reset
	usleep(10);
	mem_i2c.write_32(i2c_CR_reg, 0x1);  //enable iic peripheral + tx fifo reset clear
	usleep(10);
	
	mem_i2c.write_32(i2c_TX_FIFO_reg, 0x100 | ((uint32_t)(ic_addr<<1)));   // start bit 1 rw bit 0 ami write ezért nem foglalkozok vele
	mem_i2c.write_32(i2c_TX_FIFO_reg,((uint32_t)reg_addr));
	mem_i2c.write_32(i2c_TX_FIFO_reg, 0x200 | ((uint32_t)data));  //last byte and stop bit 1
	
	usleep(200);
	
	if(mem_i2c.read_32(i2c_ISR_reg) & i2c_ISR_transmit_error)
	{
		return -2; // no ack?
	}
	if(mem_i2c.read_32(i2c_ISR_reg) & i2c_ISR_arbitrtion_lost)
	{
		return -3; // bus locked?
	}
	
	if(wait_bus_not_busy(1) < 0)
	{
		init_axi_i2c();
		return -4; //bus locked
	}
	
	return 0;
}

int i2c_driver::write_reg_8_retry(uint8_t ic_addr, uint8_t reg_addr, uint8_t data, uint8_t bus, uint32_t retry)
{
	int ret = 0;
	for(uint32_t i=0;i<retry;i++)
	{
		if((ret = write_reg_8(ic_addr, reg_addr, data, bus)) >= 0)
			break;
	}
	return ret;
}

int i2c_driver::write_reg_16(uint8_t ic_addr, uint8_t reg_addr, uint16_t data, uint8_t bus)
{
	if(pca9544a_addr != 0)
	{
		if(select_bus(bus) < 0)
			return -20;
		usleep(10);
	}
	
	if(wait_bus_not_busy(1) < 0)
	{
		init_axi_i2c();
		usleep(1000);
		if(wait_bus_not_busy(2) < 0)
		{
			printf("i2c bus hard locked!!!\n");
			return -10;
		}
	}
	
	clear_isr_bits();

	
	mem_i2c.write_32(i2c_CR_reg, 0x3);  //enable iic peripheral + tx fifo reset
	usleep(10);
	mem_i2c.write_32(i2c_CR_reg, 0x1);  //enable iic peripheral + tx fifo reset clear
	usleep(10);
	
	
	mem_i2c.write_32(i2c_TX_FIFO_reg, 0x100 | ((uint32_t)(ic_addr<<1)));   // start bit 1 rw bit 0 ami write ezért nem foglalkozok vele
	mem_i2c.write_32(i2c_TX_FIFO_reg,((uint32_t)reg_addr));
	mem_i2c.write_32(i2c_TX_FIFO_reg,((uint32_t)data & 0xFF));
	mem_i2c.write_32(i2c_TX_FIFO_reg, 0x200 | ((uint32_t)((data>>8) & 0xFF)));  //last byte and stop bit 1
	
	usleep(200);
	
	if(mem_i2c.read_32(i2c_ISR_reg) & i2c_ISR_transmit_error)
	{
		return -2; // no ack?
	}
	if(mem_i2c.read_32(i2c_ISR_reg) & i2c_ISR_arbitrtion_lost)
	{
		return -3; // bus locked?
	}
	
	if(wait_bus_not_busy(1) < 0)
	{
		init_axi_i2c();
		return -4; //bus locked
	}
	
	return 0;
}

int i2c_driver::write_reg_16_retry(uint8_t ic_addr, uint8_t reg_addr, uint16_t data, uint8_t bus, uint32_t retry)
{
	int ret = 0;
	for(uint32_t i=0;i<retry;i++)
	{
		if((ret = write_reg_16(ic_addr, reg_addr, data, bus)) >= 0)
			break;
	}
	return ret;
}

void i2c_driver::dump_regs()
{
	printf("------------------------------i2c reg dump----------------------\n");
	printf("i2c STATUS REG:%x\n", mem_i2c.read_32(i2c_SR_reg));
	printf("i2c CR REG:%x\n", mem_i2c.read_32(i2c_CR_reg));
	printf("i2c ISR REG:%x\n", mem_i2c.read_32(i2c_ISR_reg));
	printf("----------------------------------------------------------------\n");
}

void i2c_driver::write_gpio_pin(uint8_t pin, uint8_t value)
{
	if(pin > 7)
		return;
	
	mem_i2c.write_bit_32(i2c_GPO_reg, pin, value);
}
