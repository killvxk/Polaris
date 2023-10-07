#include <debug/debug.h>
#include <devices/console.h>
#include <fb/fb.h>
#include <fs/devtmpfs.h>
#include <fs/ramdisk.h>
#include <fs/streams.h>
#include <fs/tmpfs.h>
#include <fs/vfs.h>
#include <ipc/pipe.h>
#include <kernel.h>
#include <klibc/event.h>
#include <klibc/kargs.h>
#include <klibc/module.h>
#include <mm/mmap.h>
#include <net/arp.h>
#include <net/net.h>
#include <sched/sched.h>
#include <sys/prcb.h>
#include <sys/timer.h>

const char *module_list[] = {"/lib/modules/console.ko"};

#define MODULE_LIST_SIZE (sizeof(module_list) / sizeof(module_list[0]))

void syscall_hell(struct syscall_arguments *args) {
	kputchar_('A' + prcb_return_current_cpu()->cpu_number);
	args->ret = 0xbabe650;
}

void kernel_main(void *args) {
	vfs_init();
	tmpfs_init();
	devtmpfs_init();
	vfs_mount(vfs_root, NULL, "/", "tmpfs");
	vfs_create(vfs_root, "/dev", 0755 | S_IFDIR);
	vfs_mount(vfs_root, NULL, "/dev", "devtmpfs");
	streams_init();

	kprintf("Hello I am %s running on CPU%u\n",
			prcb_return_current_cpu()->running_thread->mother_proc->name,
			prcb_return_current_cpu()->cpu_number);

	if (args != NULL) {
		uint64_t *module_info = (uint64_t *)args;
		kprintf("Ramdisk located at 0x%p\n", module_info[0]);
		ramdisk_install(module_info[0], module_info[1]);
	}

	uint64_t mod_ret = 0;
	for (size_t i = 0; i < MODULE_LIST_SIZE; i++) {
		mod_ret = module_load(module_list[i]);
		if (mod_ret) {
			panic("Failed to load kernel module %s. Return value: 0x%p\n",
				  module_list[i], mod_ret);
			break;
		}
	}

	fbdev_init();

	syscall_register_handler(0x0, syscall_read);
	syscall_register_handler(0x1, syscall_write);
	syscall_register_handler(0x2, syscall_open);
	syscall_register_handler(0x3, syscall_close);
	syscall_register_handler(0x8, syscall_seek);
	syscall_register_handler(0x9, syscall_mmap);
	syscall_register_handler(0xb, syscall_munmap);
	syscall_register_handler(0x10, syscall_ioctl);
	syscall_register_handler(0x48, syscall_fcntl);
	syscall_register_handler(0x4f, syscall_getcwd);
	syscall_register_handler(0x50, syscall_chdir);
	syscall_register_handler(0x59, syscall_readdir);
	syscall_register_handler(0x101, syscall_openat);
	syscall_register_handler(0x102, syscall_mkdirat);
	syscall_register_handler(0x106, syscall_fstatat);
	syscall_register_handler(0x107, syscall_unlinkat);
	syscall_register_handler(0x109, syscall_linkat);
	syscall_register_handler(0x10b, syscall_readlinkat);
	syscall_register_handler(0x10c, syscall_fchmodat);
	syscall_register_handler(0x124, syscall_dup3);
	syscall_register_handler(0x125, syscall_pipe);
	syscall_register_handler(0xAA, syscall_hell);

	std_console_device =
		(vfs_get_node(vfs_root, "/dev/console", true))->resource;

	char *argv[] = {"/bin/init.elf", NULL};
	if (kernel_arguments.kernel_args & KERNEL_ARGS_INIT_PATH_GIVEN) {
		argv[0] = kernel_arguments.init_binary_path;
	}

	kprintf("Running init binary %s\n", argv[0]);

	if (!process_create_elf(
			"init", PROCESS_READY_TO_RUN, 20000, argv[0],
			prcb_return_current_cpu()->running_thread->mother_proc))
		panic("Failed to run init binary!\n");

	for (;;)
		pause();
}
