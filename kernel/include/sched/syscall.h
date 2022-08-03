#ifndef SYSCALL_H
#define SYSCALL_H

#include <stddef.h>
#include <stdint.h>

struct syscall_arguments {
	uint64_t syscall_nr;
	uint64_t args0;
	uint64_t args1;
	uint64_t args2;
	uint64_t args3;
	uint64_t args4;
	uint64_t args5;
	uint64_t ret;
};

typedef void (*syscall_handler_t)(struct syscall_arguments *);

extern syscall_handler_t syscalls[];

void syscall_install_handler(void);
#define syscall_register_handler(A, B) syscalls[A] = B;
void syscall_handle(struct syscall_arguments *args);
void syscall_helper_copy_to_user(uintptr_t user_addr, void *buffer,
								 size_t count);

#endif
