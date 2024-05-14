#ifndef i2c_driver_H
#define i2c_driver_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>

#include "memory_access.h"

#define iic_memory_length   4096


//iic controller registers
#define i2c_GIE_reg  		0x1C
#define i2c_ISR_reg	 		0x20
#define i2c_IER_reg  		0x28
#define i2c_SOFTR_reg 		0x40
#define i2c_CR_reg  		0x100
#define i2c_SR_reg  		0x104
#define i2c_TX_FIFO_reg 	0x108
#define i2c_RX_FIFO_reg 	0x10C
#define i2c_ADR_reg  		0x110
#define i2c_TX_FIFO_OCY_reg 0x114
#define i2c_RX_FIFO_OCY_reg 0x118
#define i2c_TEN_ADR_reg  	0x11C
#define i2c_RX_FIFO_PIRQ_reg 0x120
#define i2c_GPO_reg 		0x124

//i2c Status reg bits
#define i2c_SR_TX_FIFO_EMPTY_bit 0x80
#define i2c_SR_BUS_BUSY_bit 0x04

//i2c ISR reg bits
#define i2c_ISR_arbitrtion_lost 0x01
#define i2c_ISR_transmit_error  0x02



class i2c_driver
{
	private:
	bool initialized;
	memory_access mem_i2c;
	uint8_t pca9544a_addr;
	uint8_t selected_bus;
	
	public:
	
	i2c_driver();
	~i2c_driver();
	
	int init(const char *XDMA_bypass_dev, uint32_t iic_base_addr, uint8_t pca9544a_addr=0);
	
	inline int wait_TX_FIFO_empty(uint32_t timeout_ms);
	inline int wait_bus_not_busy(uint32_t timeout_ms);
	inline void clear_isr_bits();
	
	int select_bus(uint8_t bus);
	
	void init_axi_i2c();
	int write_byte(uint8_t ic_addr, uint8_t data);
	int write_byte_retry(uint8_t ic_addr, uint8_t data, uint32_t retry=3);
	int write_reg_8(uint8_t ic_addr, uint8_t reg_addr, uint8_t data, uint8_t bus = 0);
	int write_reg_8_retry(uint8_t ic_addr, uint8_t reg_addr, uint8_t data,uint8_t bus = 0, uint32_t retry = 3);
	int write_reg_16(uint8_t ic_addr, uint8_t reg_addr, uint16_t data, uint8_t bus=0);
	int write_reg_16_retry(uint8_t ic_addr, uint8_t reg_addr, uint16_t data, uint8_t bus=0, uint32_t retry = 3);
	void write_gpio_pin(uint8_t pin, uint8_t value);
	
	void dump_regs();
	
	
};





#endif //i2c_diver_H