#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#define MAXLEN 100
#define NMAX 5

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Wrong format! Expected output file\n");
		return 0;
    }
    key_t key = ftok("tmp_file", 'c');
    if (key == -1) {
        perror("ftok");
        return 1;
    }
    int memid = shmget(key, NMAX * MAXLEN + NMAX, 0666);
    if (memid == -1) {
        fprintf(stderr, "Error while connecting to the server\n");
        perror("shmget");
        return 1;
    }
    char* memaddr = shmat(memid, NULL, 0);
    int semid = semget(key, 6, 0666);
    if (semid == -1) {
        fprintf(stderr, "Error while connecting to the server\n");
        perror("semget");
        return 1;
    }    
    FILE* file = fopen(argv[1], "w");
    struct sembuf operation;
    operation.sem_num = 0;
    operation.sem_flg = 0;
    operation.sem_op = -2;
    semop(semid, &operation, 1);//sem[0] -= 2
    int num = memaddr[0];
    operation.sem_op = -1;
    semop(semid, &operation, 1);//sem[0] = 0
    printf("My num is %d\n", num);
    while (1) {
        operation.sem_num = num;
        if (semop(semid, &operation, 1) == -1) {//sem[num]--
            break;
        }
        int count = 1;
        int i = 0;
        int j = 0;
        fseek(file, 0, SEEK_END);
        fprintf(file, "%d:", num);
        char str[MAXLEN + 1];
        while (count <= num) {
            if (!memaddr[i]) {
                ++count;
            }
            else if (count == num) {
                str[j++] = memaddr[i];
            }
            ++i;
        }
        str[j] = '\0';
        fprintf(file, "%s", str);
        fflush(file);
        operation.sem_num = 0;
        semop(semid, &operation, 1);//sem[0] = 0
    }
    shmdt(memaddr);
    fclose(file);
    return 0;
}