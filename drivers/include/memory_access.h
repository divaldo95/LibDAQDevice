#ifndef memory_access_H
#define memory_access_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>

class memory_access
{
	private:
	uint32_t phys_addr;
	uint32_t phys_length;
	bool initialized;
	
	uint32_t mmap_length;
	void *mmap_start_addr;
	void *virt_memory;
	
	public:
	memory_access();
	~memory_access();
	
	int init_mmap(const char *XDMA_bypass_dev, uint32_t base_addr, uint32_t length);
	
	uint32_t read_32(uint32_t offset); //offset from base addr in bytes
	void     write_32(uint32_t offset, uint32_t data); //offset from base addr in bytes
			 
	uint16_t read_16(uint32_t offset); //offset from base addr in bytes
	void     write_16(uint32_t offset, uint16_t data); //offset from base addr in bytes
	
	uint8_t  read_8(uint32_t offset); //offset from base addr in bytes
	void     write_8(uint32_t offset, uint8_t data); //offset from base addr in bytes
	
	void write_multiple(uint32_t offset, uint8_t *source_array, uint32_t length);
	void read_multiple(uint32_t offset, uint8_t *dest_array, uint32_t length);
	
	void write_bit_32(uint32_t offset, uint8_t bit, uint8_t value);
	void write_bit_16(uint32_t offset, uint8_t bit, uint8_t value);
	void write_bit_8 (uint32_t offset, uint8_t bit, uint8_t value);
	
};


#endif //memory_access_H