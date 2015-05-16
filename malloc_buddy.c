#define _BSD_SOURCE
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

typedef struct block_t 	block_t;

struct block_t{

	unsigned	reserved;		/* one if reserved. */
	char		kval;			/* current value of K.*/
	block_t*	next;			/* sucessor block in list. */
	block_t*	prev;			/* predecessor block in list. */
};

#define META_SIZE	(sizeof(block_t))
#define K_VALUE		(30)

char* global_memory = NULL;
block_t* free_list[K_VALUE+1];




/*adjust the size to be a multiple by 2. The number of shifts are returned in index*/
size_t adjust_size(size_t size, unsigned char* index)
{
	size_t new_size		=	1;
	*index 				=	0;
	while(new_size < size + META_SIZE){
		++(*index);
		new_size<<=1;
	}
	return new_size;	
}

/*remove the first element on index in free_list*/
block_t* remove_first(char index)
{
	block_t* block 		=	free_list[index];
	block_t* next		=	block->next;
	if(next){
		next->prev 		= 	NULL;
		free_list[index] = 	next;
	}else{

		free_list[index] = 	NULL;			
	}
	
	block->next 		= 	NULL;
	block->prev 		= 	NULL;	

	return block;
}

/*reserve the first element on index in free_list if such an element exists*/
block_t* reserve_first(char index)
{
	if(free_list[index]){
		block_t* block 		=	remove_first(index);
		block->reserved 	= 	1;
		return block;
	}
	return NULL;
}

/*split the block in two equal sizes*/
block_t* split_block(block_t* block)
{
	char kval			=	block->kval - 1;
	block->kval			=	kval;

	block_t* new_block	=	(char*)(block) + (1 << kval);
	new_block->kval		=	kval;
	new_block->reserved	=	0;
	new_block->next		=	NULL;
	new_block->prev		=	NULL;

	return new_block;
		
}



void* malloc(size_t size)
{
	block_t* block;

	if(size<=0)
		return NULL;
	

	unsigned char index;
	size = adjust_size(size,&index);

	if(index>K_VALUE)
		return NULL;

	if(!global_memory){ /*first time*/
		global_memory 		=	sbrk(1<<K_VALUE);

		if(global_memory == (void*)-1)
			return NULL;
		

		block 				=	global_memory;
		block->reserved 	= 	0;
		block->kval			=	K_VALUE;
		block->next 		= 	NULL;
		block->prev 		= 	NULL;

		free_list[K_VALUE]	= 	block;	
	}	

	block = reserve_first(index);
	if(block)
		return (block+1);

	unsigned char new_index = index;

	/*find nearest available place in free_list*/ 
	while(!free_list[new_index] ){
		++new_index;
		if(new_index>K_VALUE)
			return NULL;
	}

	/*iterate through all indexes down to desired index and split blocks*/
	while(new_index > index){
		block = remove_first(new_index);

		block->reserved = 0;
		block->next = NULL;
		block->prev = NULL;

		block_t* new_block = split_block(block);

		--new_index;
		
		block->next = new_block;
		new_block->prev = block;

		free_list[new_index] = block;
	}


	block = reserve_first(index);
	return (block+1);
}



/*buddy = start + ((pointer - start) ^ (1 << K));*/
block_t* find_buddy(block_t* block)
{
	if(block->kval == K_VALUE)
		return NULL;

	block_t* buddy = global_memory + (((char*)(block) - global_memory) ^ (1<<(block->kval)));
	if(buddy->reserved || buddy->kval != block->kval)
		return NULL;

	return buddy;
}

/*remove form the free_list*/
void remove_from_free_list(block_t* block)
{
	if(free_list[block->kval] == block){
		remove_first(block->kval);
	}else{
		block_t* next = block->next;
		block_t* prev = block->prev;
		if(next){
			next->prev = prev;
		}
		if(prev){
			prev->next = next;	
		}
		block->prev = NULL;
		block->next = NULL;
	}

}

/*merge blocks if possible*/
block_t* merge_block(block_t* block)
{
	block_t* buddy = find_buddy(block);
	if(buddy){
		remove_from_free_list(buddy);
		remove_from_free_list(block);
		if(block>buddy){
			++(buddy->kval);
			block = buddy;
		}else{
			++(block->kval);
		}
		return merge_block(block);
	}else{
		return block;
	}
}


void free(void* ptr)
{

	if(!ptr)
		return;
	
	block_t* block = (block_t*)(ptr)-1;
	block->reserved = 0;
	block = 		merge_block(block);
	unsigned char kval = block->kval;
	block_t* next = free_list[kval];
	if(next){
		block->next = next;
		next->prev = block;	
	}
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
	

	block_t* block = (block_t*)(ptr)-1;

	size_t block_size = (1<<(block->kval))-META_SIZE;
	if(block_size>=size){
		return ptr;
	}

	void* ptr2 = malloc(size);
	if(!ptr2)
		return NULL;

	memcpy(ptr2 ,ptr,block_size);
	free(ptr);
	return ptr2;
}

