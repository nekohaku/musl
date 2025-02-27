#define _BSD_SOURCE
#include <sys/uio.h>
#include <unistd.h>
#include "syscall.h"

#ifndef PS4

ssize_t preadv(int fd, const struct iovec *iov, int count, off_t ofs)
{
	return syscall_cp(SYS_preadv, fd, iov, count,
		(long)(ofs), (long)(ofs>>32));
}

weak_alias(preadv, preadv64);

#else

ssize_t preadv(int fd, const struct iovec *iov, int count, off_t ofs);

ssize_t preadv64(int fd, const struct iovec *iov, int count, off_t ofs)
{
	return pread(fd, iov, count, ofs);
}

#endif
