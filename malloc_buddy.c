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
}

#define META_SIZE	(sizeof(block_t));
#define K_VALUE		(20)

void* global_memory = NULL;
block_t* free_list[K_VALUE];




block_t* split_block(block_t* block)
{
	char kval = block->kval;
	block_t* next = block->next;
	if(next){
		next->prev = NULL;
		freelist[kval] = next;
	}
	block->reserved = 1;
	block->next = NULL;
	block->next = NULL;			
}

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


void* malloc(size_t size)
{
	block_t* block;

	if(size<=0)
		return NULL;

	int index;
	size = adjust_size(size,&index);


	if(global_memory){
		block_t* next;
		if(freelist[index]){
			block = freelist[index];

			next = block->next;
			if(next){
				next->prev = NULL;
				freelist[index] = next;			
			}else{
				freelist[index] = NULL;			
			}
			
			block->reserved = 1;
			block->next = NULL;
			block->next = NULL;	
			return (block+1);
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

		



	return (block+1);
}



void free(void* ptr)
{
}

