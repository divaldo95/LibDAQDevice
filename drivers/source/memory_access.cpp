#include "memory_access.h"


memory_access::memory_access()
{
	initialized = false;
	mmap_start_addr = NULL;
}


memory_access::~memory_access()
{
	if(initialized)
	{
		munmap(mmap_start_addr, mmap_length);
		initialized = false;
	}
}

int memory_access::init_mmap(const char *XDMA_bypass_dev, uint32_t base_addr, uint32_t length)
{
		if(initialized)
			return -10;
		
		phys_addr = base_addr;
		phys_length = length;
		
		if(length == 0)
			return -1;
		
		uint32_t page_size = sysconf(_SC_PAGESIZE); //usually 4096 byte
		
		uint32_t start_page_addr = base_addr & (~(page_size-1));
		uint32_t page_offset = base_addr - start_page_addr;
		
		uint32_t n_pages = length / page_size;
		if(length % page_size)
			n_pages++;
		if(page_offset)
			n_pages++;
		
		mmap_length = n_pages * page_size;
		
		
		int fd;
        /* Open XDMA_bypass_dev */ 
        fd = open(XDMA_bypass_dev, O_RDWR | O_SYNC);
        if (fd < 0) 
		{
            printf("%s open failed\n", XDMA_bypass_dev);   
            return -2;
        }
        else
        {
            //printf("/dev/mem open ok\n");
        }

        /* mmap phys to virtual*/
        mmap_start_addr = mmap(NULL, mmap_length , PROT_READ|PROT_WRITE, MAP_SHARED, fd, start_page_addr);
        if(mmap_start_addr == NULL)
        {
                printf("mmap failed!\n");
				close(fd);
                return -3;
        }	
        //virt_memory = (void*)((uint32_t)mmap_start_addr + page_offset);
		virt_memory = (((char*)mmap_start_addr) + page_offset);

        printf("mmap ok  phys_addr:%x mmap_start_addr:%lx virt_memory:%lx length:%u, pages:%u\n", base_addr, (uint64_t)mmap_start_addr, (uint64_t)virt_memory, length, n_pages);
		close(fd);
		initialized = true;
		
		return 0;
}

uint32_t memory_access::read_32(uint32_t offset) //offset from base addr in bytes
{
	if(initialized)
		return (*((uint32_t *)(((char*)virt_memory) + offset)));
	else
		return 0;
}

void memory_access::write_32(uint32_t offset, uint32_t data) //offset from base addr in bytes
{
	if(initialized)
		*((uint32_t *)(((char*)virt_memory) + offset)) = data;
}

uint16_t memory_access::read_16(uint32_t offset) //offset from base addr in bytes
{
	if(initialized)
		return (*((uint16_t *)(((char*)virt_memory) + offset)));
	else
		return 0;
}

void memory_access::write_16(uint32_t offset, uint16_t data) //offset from base addr in bytes
{
	if(initialized)
		*((uint16_t *)(((char*)virt_memory) + offset)) = data;
}

uint8_t memory_access::read_8(uint32_t offset) //offset from base addr in bytes
{
	if(initialized)
		return (*((uint8_t *)(((char*)virt_memory) + offset)));
	else
		return 0;
}

void memory_access::write_8(uint32_t offset, uint8_t data) //offset from base addr in bytes
{
	if(initialized)
		*((uint8_t *)(((char*)virt_memory) + offset)) = data;
}

void memory_access::write_multiple(uint32_t offset, uint8_t *source_array, uint32_t length)
{
	if(source_array == NULL || length == 0 || (offset + length) > phys_length)
		return;
	memcpy(((uint8_t *)(((char*)virt_memory) + offset)), source_array, length);
}

void memory_access::read_multiple(uint32_t offset, uint8_t *dest_array, uint32_t length)
{
	if(dest_array == NULL || length == 0 || (offset + length) > phys_length)
		return;
	memcpy(dest_array, ((uint8_t *)(((char*)virt_memory) + offset)), length);
}

void memory_access::write_bit_32(uint32_t offset, uint8_t bit, uint8_t value)
{
	if(bit > 31) 
		return;
	
	uint32_t orig = read_32(offset);
	
	orig &= ~(1<<bit);
	if(value)
		orig |=  (1<<bit);
	
	write_32(offset, orig);
}

void memory_access::write_bit_16(uint32_t offset, uint8_t bit, uint8_t value)
{
	if(bit > 15) 
		return;
	
	uint16_t orig = read_16(offset);
	
	orig &= ~(1<<bit);
	if(value)
		orig |=  (1<<bit);
	
	write_16(offset, orig);
}

void memory_access::write_bit_8(uint32_t offset, uint8_t bit, uint8_t value)
{
	if(bit > 7) 
		return;
	
	uint8_t orig = read_8(offset);
	
	orig &= ~(1<<bit);
	if(value)
		orig |=  (1<<bit);
	
	write_8(offset, orig);
}

