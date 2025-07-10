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
        sem_wait(sem_write);
        long host_id;

        memcpy(&host_id, ptr, sizeof(host_id));
        printf("Host ID: %ld\n", host_id);
        sem_post(sem_read);
    }
    return NULL;
}

void handle_sigint(int signo) {
    if (signo == SIGINT) {
        printf("\nget SIGINT; %d\n", signo);
        flag = 1; 

        if (sem_write != SEM_FAILED) {
            sem_close(sem_write);
        }
        if (sem_read != SEM_FAILED) {
            sem_close(sem_read);
        }
        if (ptr != (void *)-1) {
            shmdt(ptr);
        }

        _exit(0);
    }
}

int main() {
    pthread_t thread; 
    signal(SIGINT, handle_sigint);
    int shm_error = 0; 
    int sem_write_error = 0; 
    int sem_read_error= 0; 
    
    while ((shmid = shmget(SHM_KEY, SHM_SIZE, 0666)) == -1) {
        if (!shm_error) {
            perror("shmget (retrying)");
            shm_error = 1; 
        }
        sleep(1); // Ждем, пока разделяемая память не будет создана
    }

    ptr = shmat(shmid, NULL, 0);
    if (ptr == (void *)-1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    // Подключаемся к семафорам
    while ((sem_write = sem_open(SEM_WRITE, 0)) == SEM_FAILED) {
        if (!sem_write_error) {
            perror("sem_open (write, retrying)");
            sem_write_error = 1; 
        }
        sleep(1); 
    }
    while ((sem_read = sem_open(SEM_READ, 0)) == SEM_FAILED) {
        if (!sem_read_error) {
            perror("sem_open (read, retrying)");
            sem_read_error = 1; 
        }
        sleep(1); 
    }

    pthread_create(&thread, NULL, func, NULL);
    printf("Press Enter to stop...\n");
    getchar(); 
    flag = 1; 
    pthread_join(thread, NULL); 

    sem_close(sem_write);
    sem_close(sem_read);
    shmdt(ptr);

    return 0;
}
