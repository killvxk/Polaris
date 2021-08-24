/*
 * Copyright 2021 NSG650
 * Copyright 2021 Sebastian
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "alloc.h"
#include "../mm/pmm.h"
#include "../mm/vmm.h"
#include "math.h"
#include "mem.h"
#include "string.h"

struct alloc_metadata {
	size_t pages;
	size_t size;
};

void *alloc(size_t size) {
	size_t page_count = DIV_ROUNDUP(size, PAGE_SIZE);

	void *ptr = (char *)pmm_allocz(page_count + 1);

	if (!ptr)
		return NULL;

	ptr += MEM_PHYS_OFFSET;

	struct alloc_metadata *metadata = ptr;
	ptr += PAGE_SIZE;

	metadata->pages = page_count;
	metadata->size = size;

	return ptr;
}

void free(void *ptr) {
	struct alloc_metadata *metadata = ptr - PAGE_SIZE;

	pmm_free((void *)metadata - MEM_PHYS_OFFSET, metadata->pages + 1);
}

void *realloc(void *ptr, size_t new_size) {
	if (!ptr)
		return alloc(new_size);

	// Reference metadata page
	struct alloc_metadata *metadata = ptr - PAGE_SIZE;

	if (DIV_ROUNDUP(metadata->size, PAGE_SIZE) ==
		DIV_ROUNDUP(new_size, PAGE_SIZE)) {
		metadata->size = new_size;
		return ptr;
	}

	void *new_ptr = alloc(new_size);
	if (new_ptr == NULL)
		return NULL;

	if (metadata->size > new_size) {
		// Copy all the data from the old pointer to the new pointer, within the
		// range specified by 'size'.
		memcpy(new_ptr, ptr, new_size);
	} else {
		memcpy(new_ptr, ptr, metadata->size);
	}

	free(ptr);

	return new_ptr;
}

void *liballoc_alloc(size_t size) {
	return pmm_allocz(size) + MEM_PHYS_OFFSET;
}

int liballoc_free(void *ptr, size_t size) {
	pmm_free(ptr - MEM_PHYS_OFFSET, size);
	return 0;
}

int liballoc_lock() {
	// We don't have threads yet... This should be enough
	asm volatile("cli");
	return 0;
}

int liballoc_unlock() {
	asm volatile("sti");
	return 0;
}
