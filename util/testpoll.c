#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>
#include <mqueue.h>

#define MAX_SIZE 128

int main(int argc, char argv)
{
	mqd_t fd;
	struct mq_attr attr;
	char buffer[MAX_SIZE + 1];

	struct pollfd fds[1];
	

	/* initialize the queue attributes */
	attr.mq_flags = 0;
	attr.mq_maxmsg = 5;
	attr.mq_msgsize = MAX_SIZE;
	attr.mq_curmsgs = 0;

	/* cleanup for multiple runs... */
	mq_unlink ("/TESTQ");

	/* create the message queue */
	fd = mq_open ("/TESTQ", O_CREAT | O_RDWR | O_NONBLOCK, S_IWUSR | S_IRUSR, &attr);

	fds[0].fd = (int)fd;
	fprintf(stderr, "#1: fd=%d/%p\n", fds[0].fd, fd);
	fds[0].events = POLLIN;

	if (!poll(&fds, 1, 1000))
	{
		fprintf(stderr, "#1 poll fail: %d:%s\n", errno, strerror(errno));
	}
	fflush(stderr);

	/* most likely it will crash here... if the system is not like BSD where 
         * it have ptr to file descriptor as mqd_t
         */
	fds[0].fd = *((int*)fd);
	fprintf(stderr, "#2: fd=%d\n", fds[0].fd);
	fflush(stderr);
	fds[0].events = POLLIN;

	if (!poll(&fds, 1, 1000))
	{
		fprintf(stderr, "#2 poll fail: %d:%s\n", errno, strerror(errno));
	}

}

