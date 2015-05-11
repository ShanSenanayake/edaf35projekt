

#define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>




typedef struct block_t 	block_t;

struct block_t{

	size_t size; /*size including block_t*/
	block_t* next;	/*next block if exists, otherwise NULL*/
	bool free;
};


#define META_SIZE		(sizeof(block_t))
#define ALIGNMENT_SIZE	8
 



static block_t* global_memory = NULL;



block_t* alignment(block_t* block)
{
	unsigned ptr = block;
	ptr = ((ptr-1)&0xfffffff8)+ALIGNMENT_SIZE;
	return (block_t*)ptr;
}

block_t* block_exists(block_t** last,size_t size)
{

	block_t* block = global_memory;

	while(block)
	{
		if(block->free && block->size >= size)
		{
			block->free = false;
			return block;
		}

		*last = block;
		block = block->next;
	}
	return NULL;
}

block_t* extend_memory(block_t* last, size_t size)
{
	block_t* block = sbrk(size+ALIGNMENT_SIZE);
	block = alignment(block);
	if(block == (void*)-1)
		return NULL;
	
	block->size = size;
	block->free = false;
	block->next = NULL;


	if(last)
		last->next = block;
	else
		global_memory = block;

	return block;
	
}

void* malloc(size_t size)
{
	if(global_memory) {

		block_t* last;
		block_t* block = block_exists(&last,size+META_SIZE);
		
		if(block)
			return block+1;
		else
			return extend_memory(last,size+META_SIZE)+1;

	} else {
		return extend_memory(NULL,size+META_SIZE)+1;
	}
}



void free(void* ptr)
{
	if(!ptr)
		return;
	block_t* block = (block_t*)ptr-1;
	block->free = true;
}

void* calloc(size_t count, size_t size)
{
	size_t tot_size = count*size;
	void* ptr = malloc(tot_size);

	if(!ptr)
		return NULL;

	memset(ptr,0,tot_size);
	return ptr;
}

void* realloc(void* ptr, size_t size)
{
	if(!ptr)
		return malloc(size);
	
	void* ptr2 = malloc(size);
	block_t* block = (block_t*)ptr-1;
	if(!ptr2)
		return NULL;

	memcpy(ptr2 ,ptr,block->size-META_SIZE);
	free(block);
	return ptr2;
}

