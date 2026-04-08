#define STB_TRUETYPE_IMPLEMENTATION

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "acyacsl.h"

typedef struct Allocation Allocation;
struct Allocation {
	void *ptr;
	Usize size;
	Allocation *previous;
};

Usize totalMemoryAllocated = 0;
Allocation *lastAllocation = NULL;

void *mymalloc(size_t size) {
	unsigned char *memory  = malloc(size + sizeof(Allocation));
	Allocation *allocation = NULL;
	unsigned char *ptr     = NULL;

	if (memory == NULL) {
		assert(0 && "wtf");
		return NULL;
	} else {
		allocation = (Allocation *)memory;
		ptr        = memory + sizeof(Allocation);

		allocation->ptr = ptr;
		allocation->size = size;
		allocation->previous = lastAllocation;
		lastAllocation = allocation;

		memset(ptr, 0, size);
		totalMemoryAllocated += size;
		//printf("stbtt +%ldb, total = %ldKiB, %lx\n", size, totalMemoryAllocated / 1024, (U64)ptr);
		return ptr;
	}
}
void myfree(void *ptr) {
	Allocation *tmp = lastAllocation;
	Allocation *next = NULL;

	if (ptr == NULL) {
		return;
	}

	for (;;) {
		if (tmp == NULL) {
			printf("could not find %lx\n", (U64)ptr);
			assert(0);
		} else if (tmp->ptr == ptr) {
			totalMemoryAllocated -= tmp->size;
			//printf("stbtt -%ldb, total = %ldKiB, %lx\n", tmp->size, totalMemoryAllocated / 1024, (U64)ptr);
			if (next != NULL) {
				next->previous = tmp->previous;
			} else {
				lastAllocation = tmp->previous;
			}
			free(tmp);

			break;
		} else {
			next = tmp;
			tmp = tmp->previous;
		}
	}
}
#define STBTT_malloc(x, u) ((void)u, mymalloc(x))
#define STBTT_free(x, u) ((void)u, myfree(x))
#include "stb_truetype.h"
