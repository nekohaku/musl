#define _BSD_SOURCE
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include "syscall.h"

void *sbrk(intptr_t inc)
{
#ifndef PS4
	if (inc) return (void *)__syscall_ret(-ENOMEM);
	return (void *)__syscall(SYS_brk, 0);
#else
	errno = ENOSYS;
	return -1;
#endif
}
