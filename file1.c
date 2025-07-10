#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <pthread.h>
#include <signal.h>

#define SHM_SIZE 1024
#define SHM_KEY 1234
#define SEM_WRITE "/sem_write"
#define SEM_READ "/sem_read"

int flag = 0; 
sem_t *sem_write;
sem_t *sem_read; 
int shmid; 
void *ptr; 

void *func(void *arg) {
    while (!flag) {
        long host_id = gethostid();
        printf("Host ID: %ld\n", host_id); 
        memcpy(ptr, &host_id, sizeof(host_id));

        sem_post(sem_write);
        sem_wait(sem_read);
        sleep(1); 
    }
    return NULL;
}

void handle_sigint(int signo) {
    if (signo == SIGINT) {
        printf("\nget SIGINT; %d\n", signo);
        flag = 1; 

        if (sem_write != SEM_FAILED) {
            sem_close(sem_write);
            sem_unlink(SEM_WRITE);
        }
        if (sem_read != SEM_FAILED) {
            sem_close(sem_read);
            sem_unlink(SEM_READ);
        }
        if (ptr != (void *)-1) {
            shmdt(ptr);
        }
        if (shmid != -1) {
            shmctl(shmid, IPC_RMID, NULL);
        }

        _exit(0);
    }
}

int main() {
    pthread_t thread; 
    signal(SIGINT, handle_sigint);

    shmid = shmget(SHM_KEY, SHM_SIZE, IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    ptr = shmat(shmid, NULL, 0);
    if (ptr == (void *)-1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    sem_write = sem_open(SEM_WRITE, O_CREAT, 0666, 0);
    if (sem_write == SEM_FAILED) {
        perror("sem_open (write)");
        exit(EXIT_FAILURE);
    }

    sem_read = sem_open(SEM_READ, O_CREAT, 0666, 0);
    if (sem_read == SEM_FAILED) {
        perror("sem_open (read)");
        exit(EXIT_FAILURE);
    }


    pthread_create(&thread, NULL, func, NULL);
    printf("Press Enter ...\n");
    getchar(); 
    flag = 1; 
    pthread_join(thread, NULL); 


    sem_close(sem_write);
    sem_unlink(SEM_WRITE);
    sem_close(sem_read);
    sem_unlink(SEM_READ);
    shmdt(ptr);
    shmctl(shmid, IPC_RMID, NULL);

    return 0;
}
