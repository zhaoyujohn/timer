#define _GNU_SOURCE
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <stdio.h>
#include <sched.h>
#include <signal.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <semaphore.h> 

//A stack for the container
#define STACK_SIZE (1024 * 1024)
static char stack[STACK_SIZE];

//The semaphore
sem_t semaphore;

int child(void* arg) {
    int semval;

    //Print the semaphore state as read from the new namespace
    //for (int i = 0; i < 6; i++) {
    //    sem_getvalue(&semaphore, &semval);
    //    printf("Semaphore state: %d.\n", semval);
    //    sleep(1);
    //}
    sem_wait(&semaphore);
    printf("hehe\n");
    return 1;
}

int main() {
    int i = 1;
    void* ptr = static_cast<void*>(&i);
    int* ptr2 = static_cast<int*>(ptr);
    //Init a shared POSIX semaphore with the value zero
    sem_init(&semaphore, 0, 0);

    //Create the child namespace
    pid_t pid = clone(child, stack + STACK_SIZE, CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_UNTRACED, NULL);

    //Wait, then post the semaphore
    sleep(3);
    printf("Posting semaphore: %lu\n", pid);
    sem_post(&semaphore);

    //Wait for it to return
    waitpid(pid, NULL, __WALL);

    return 0;
}
