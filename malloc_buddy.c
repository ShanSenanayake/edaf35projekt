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





void* malloc(size_t size)
{
	block_t* block;
	size

	if(size<=0)
		return NULL;

	if(global_memory){
	
	}else{
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

