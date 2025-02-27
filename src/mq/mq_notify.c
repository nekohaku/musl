#include <mqueue.h>
#include <pthread.h>
#include <errno.h>
#include <sys/socket.h>
#include <signal.h>
#include <unistd.h>
#include "syscall.h"

struct args {
	pthread_barrier_t barrier;
	int sock;
	const struct sigevent *sev;
};

static void *start(void *p)
{
	struct args *args = p;
	char buf[32];
	ssize_t n;
	int s = args->sock;
	void (*func)(union sigval) = args->sev->sigev_notify_function;
	union sigval val = args->sev->sigev_value;

	pthread_barrier_wait(&args->barrier);
	n = recv(s, buf, sizeof(buf), MSG_NOSIGNAL|MSG_WAITALL);
	close(s);
	if (n==sizeof buf && buf[sizeof buf - 1] == 1)
		func(val);
	return 0;
}

#ifdef PS4
#ifdef PS4_LIBKERNEL_SYS
int kmq_notify(mqd_t, const struct sigevent*);
#else
static int kmq_notify(mqd_t mqd, const struct sigevent* sev)
{
	errno = ENOSYS;
	return -1;
}
#endif
#endif

int mq_notify(mqd_t mqd, const struct sigevent *sev)
{
	struct args args = { .sev = sev };
	pthread_attr_t attr;
	pthread_t td;
	int s;
	struct sigevent sev2;
	static const char zeros[32];

	if (!sev || sev->sigev_notify != SIGEV_THREAD)
#ifdef PS4
		return kmq_notify(mqd, sev);
#else
		return syscall(SYS_mq_notify, mqd, sev);
#endif

	s = socket(AF_NETLINK, SOCK_RAW|SOCK_CLOEXEC, 0);
	if (s < 0) return -1;
	args.sock = s;

	if (sev->sigev_notify_attributes) attr = *sev->sigev_notify_attributes;
	else pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_barrier_init(&args.barrier, 0, 2);

	if (pthread_create(&td, &attr, start, &args)) {
		close(s);
		errno = EAGAIN;
		return -1;
	}

	pthread_barrier_wait(&args.barrier);
	pthread_barrier_destroy(&args.barrier);

	sev2.sigev_notify = SIGEV_THREAD;
	sev2.sigev_signo = s;
	sev2.sigev_value.sival_ptr = (void *)&zeros;

	if (
#ifdef PS4
		kmq_notify(mqd, &sev2)
#else
		syscall(SYS_mq_notify, mqd, &sev2)
#endif
	 < 0) {
		pthread_cancel(td);
		int errno1 = errno;
		close(s);
		errno = errno1;
		return -1;
	}

	return 0;
}
