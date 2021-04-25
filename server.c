#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#define MAXLEN 100
#define NMAX 5

int main(int argc, char** argv) {
    if (argc != 3) {
        fprintf(stderr, "Wrong format! Expected number of clients and input file\n");
		return 0;
    }
    int n = atoi(argv[1]);
    if ((n < 1) || (n > NMAX)) {
        fprintf(stderr, "Wrong format! 1 <= N <= 5\n");
		return 0;
    } 
    key_t key = ftok("tmp_file", 'c');
    if (key == -1) {
        perror("ftok");
        return 1;
    }
    int memid = shmget(key, NMAX * MAXLEN + NMAX, 0666 | IPC_CREAT);
    if (memid == -1) {
        perror("shmget");
        return 1;
    }
    char* memaddr = shmat(memid, NULL, 0);
    int semid = semget(key, 6, 0666 | IPC_CREAT);
    if (semid == -1) {
        perror("semget");
        shmdt(memaddr);
        shmctl(memid, IPC_RMID, NULL);
        return 1;
    }
    for (int i = 0; i < 6; ++i) {
        semctl(semid, i, SETVAL, (int) 0);//sem[i] = 0
    }
    struct sembuf operation;
    operation.sem_num = 0;
    operation.sem_flg = 0;
    for (int i = 1; i <= n; ++i) {
        memaddr[0] = i;
        operation.sem_op = 3;
        semop(semid, &operation, 1);//sem[0] += 3
        operation.sem_op = 0;
        semop(semid, &operation, 1);//wait
    }
    printf("Connected\n");
    char str[MAXLEN + 1];
    FILE* file = fopen(argv[2], "r");
    if (!file) {
        fprintf(stderr, "Can't open file %s\n", argv[2]);
		return 0;
    }
    int stop = 0;
    while (!stop) {
        int len = 0;
        for (int i = 0; i < n; ++i) {
            if (fgets(str, MAXLEN + 1, file)) {
                strcpy(memaddr + len, str);
                len += strlen(str) + 1;
            }
            else {
                memaddr[len] = 0;
                stop = i + 1;
                break;
            }
        }
        for (int i = 1; i <= n; ++i) {
            if (!stop || (i < stop)) {
                operation.sem_op = 1;
                struct sembuf second;
                second.sem_num = i;
                second.sem_flg = 0;
                second.sem_op = 1;
                semop(semid, &operation, 1);//s[0]++
                semop(semid, &second, 1);//s[i]++
                operation.sem_op = 0;
                semop(semid, &operation, 1);//wait
            }
            else {
                semctl(semid, i, IPC_RMID, NULL);
            }
        }
    }
    for (int i = 0; i < 6; ++i) {
        semctl(semid, i, IPC_RMID, NULL);
    }
    shmdt(memaddr);
    shmctl(memid, IPC_RMID, NULL);
    fclose(file);
    return 0;
}