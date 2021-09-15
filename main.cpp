#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <thread>

#define KB (512)
#define MB (1024 * KB)
#define GB (1024 * MB)

void test()
{
	while (1) {
		printf("hehe\n");
		sleep(5);
	}
	sleep(-1);
}

int main(int argc, char* argv[])
{
	char* p;
	int fd = open("/sys/fs/cgroup/memory/test/aa/memory.oom_control", O_WRONLY);
	if (fd < 0) {
		return false;
	}
	int tmp = 10;
	int ret = write(fd, &tmp, sizeof(tmp));
	if (ret != sizeof(tmp)) {
		return false;
	}
	std::thread ttt(test);

again:
	while ((p = (char*)malloc(KB))) {
		memset(p, 0, KB);
		//sleep(1);
		usleep(50);
	}
		

	sleep(1);

	goto again;

	return 0;
}