#define _BSD_SOURCE
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

typedef struct block_t 	block_t;

struct block_t{

	unsigned	reserved:1;		/* one if reserved. */
	char		kval;			/* current value of K.*/
	block_t*	next;			/* sucessor block in list. */
	block_t*	prev;			/* predecessor block in list. */
};

#define META_SIZE	(sizeof(block_t))
#define K_VALUE		(23)

void* global_memory = NULL;
block_t* free_list[K_VALUE+1];




/*adjust the size to be a multiple by 2. The number of shifts are returned in index*/
size_t adjust_size(size_t size, int* index)
{
	size_t new_size = 1;
	*index = 0;
	while(new_size<=size+META_SIZE){
		new_size<<=1;
		++(*index);
	}

	return new_size;	
}

block_t* remove_first(int index)
{

	block_t* block 		=	free_list[index];
	block_t* next		=	block->next;
	if(next){
		next->prev 		= 	NULL;
		free_list[index] = 	next;
	}else{

		free_list[index] = 	NULL;			
	}
	
	block->reserved 	= 	1;
	block->next 		= 	NULL;
	block->next 		= 	NULL;	

	return block;
}


block_t* split_block(block_t* block)
{
	char kval = block->kval-1;

	block_t* new_block = block + (1 << kval);
	new_block->kval = kval;
	new_block->reserved = 0;

	block->kval = kval;

	return new_block;
		
}



void* malloc(size_t size)
{
	block_t* block;

	if(size<=0)
		return NULL;

	int index;
	size = adjust_size(size,&index);


	if(global_memory){
		if(free_list[index]){
			return (remove_first(index)+1);
		}

	}else{ /*first time*/
		global_memory = sbrk(1<<K_VALUE);
		block = global_memory;

		if(global_memory == (void*)-1)
			return NULL;

		block->reserved = 0;
		block->kval = K_VALUE;
		block->next = NULL;
		block->prev = NULL;

		free_list[K_VALUE-1] = block;	
	}	

	int new_index = index;
	while(!free_list[new_index++])
		;
	while(new_index > index){
		block = free_list[new_index];
		block_t* next = block->next;
		if(next){
			next->prev = NULL;
			free_list[new_index] = next;
		}

		block->next = NULL;
		block_t* new_block = split_block(block);
		--new_index;
		block_t* old_first = free_list[new_index];
		if(old_first){
			old_first->prev = block;
			block->next = old_first;
		}
		new_block->next = block;
		block->prev = new_block;
		free_list[new_index] = new_block;
		block = new_block;
	}


	return (remove_first(index)+1);
}



/*buddy = start + ((pointer - start) ^ (1 << K));*/
block_t* find_buddy(block_t* block)
{
	if(block->kval = K_VALUE-1)
		return NULL;

	block_t* buddy = global_memory + (((void*)(block) - global_memory) ^ (1<<(block->kval)));
	if(!buddy->reserved || buddy->kval != block->kval)
		return NULL;

	return buddy;
}

void remove_from_free_list(block_t* block)
{
	block->next->prev = block->prev;
	block->prev->next = block->next;
	block->prev = NULL;
	block->next = NULL;

}

block_t* deallocate(block_t* block)
{
	block_t* buddy = find_buddy(block);
	if(buddy){
		remove_from_free_list(block);
		if(block>buddy){
			++(buddy->kval);
			block = buddy;
		}else{
			++(block->kval);
		}
		deallocate(block);
	}else{
		return block;
	}
}


void free(void* ptr)
{
	block_t* block = (block_t*)(ptr)-1;
	block->reserved = 0;
	deallocate(block);
	char kval = block->kval;
	block->next = free_list[kval];
	block->next->prev = block;
	free_list[kval] = block;

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
	
	block_t* block = (block_t*)ptr-1;

	int dummy;
	size = adjust_size(size,&dummy);


	size_t block_size = (1<<block->kval)-META_SIZE;
	if(block_size>=size)
		return ptr;

	void* ptr2 = malloc(size);
	if(!ptr2)
		return NULL;

	memcpy(ptr2 ,ptr,block_size);
	free(ptr);
	return ptr2;
}



