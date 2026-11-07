#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>    
#include <sys/mman.h>  
#include <semaphore.h>
#include <string.h>

const char sh_name[] = "/sh_name";
const size_t sh_len = 4096;
const char sh_name2[] = "/sh_name2";
const size_t sh_len2 = 4096;

// парент по сути готов, осталось доделать ребенка, чтение по семфорам, выполнение целевого действия

int main(void)
{
    int fd = shm_open(sh_name, O_RDWR, 0);
    if (fd == -1)
    {
        perror("ch_shm_open");
        _exit(1);
    }
    
    char *ptr = mmap(NULL, sh_len,
                    PROT_READ | PROT_WRITE,
                    MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED)
    {
        perror("ch_shm_mmap");
        _exit(1);
    }

    printf("%s\n", ptr);


    int fd2 = shm_open(sh_name2, O_RDWR, 0);
    if (fd2 == -1)
    {
        perror("ch_shm_open2");
        _exit(1);
    }
    
    char *ptr2 = mmap(NULL, sh_len2,
                    PROT_READ | PROT_WRITE,
                    MAP_SHARED, fd2, 0);
    if (ptr2 == MAP_FAILED)
    {
        perror("ch_shm_mmap2");
        _exit(1);
    }

    printf("%s\n", ptr2);



    //семфоры
    sem_t *sem_data = sem_open("/sem_data", 0);//можно ли родителю записывать новую строку
    sem_t *sem_free = sem_open("/sem_free", 1);//есть ли строка которую ребёнок должен прочитат

    sem_t *sem_data2 = sem_open("/sem_data2", 0);//можно ли родителю записывать новую строку
    sem_t *sem_free2 = sem_open("/sem_free2", 1);//есть ли строка которую ребёнок должен прочитат

    //основной код

    //имя
    char name[4096];
    
    sem_wait(sem_data);
    strcpy(name, ptr);
    sem_post(sem_free);

    int fdfile = open(name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fdfile < 0)
    {
        perror("file");        
        _exit(1);
    }

    // имя2
    char name2[4096];    
    sem_wait(sem_data2);
    strcpy(name2, ptr2);
    sem_post(sem_free2);

    int fdfile2 = open(name2, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fdfile2 < 0)
    {
        perror("file2");        
        _exit(1);
    }

    int first = 1;
    size_t cap = 4096;

    while (1)
    {
        size_t len = 0;

        char line[cap];
        if (first)
        {
            sem_wait(sem_data);
            strcpy(line, ptr);
            sem_post(sem_free);
        }
        else
        {
            sem_wait(sem_data2);
            strcpy(line, ptr2);
            sem_post(sem_free2);
        }

        if (line[0] == '\0') {
            break;  // выходим из внешнего цикла и доходим до return 0
        }

        int i = 0;
        int j = 0;



        while(1)
        {
            if (line[i] == '\0') break;
            char ch = line[i];

            if (ch != 'a' && ch != 'e' &&
                ch != 'i' && ch != 'o' &&
                ch != 'u' && ch != 'y' &&
                ch != 'A' && ch != 'E' &&
                ch != 'I' && ch != 'O' &&
                ch != 'U' && ch != 'Y')
            {
                line[j++] = ch;
            }
            i++;
        }

        len = j;
        line[len] = '\0';

        size_t written = 0;
        while (written < len)
        {
            ssize_t w;
            if(first)
            {
                w = write(fdfile, line + written, len - written);
            }
            else
            {
                w = write(fdfile2, line + written, len - written);
            }

            if (w < 0)
            {
                close(fdfile);
                close(fdfile2);
                return 1;
            }

            written += (size_t)w;
        }
        if(first)
        {
            write(fdfile, "\n", 1);
        }
        else
        {
            write(fdfile2, "\n", 1);
        }
        first = !first;
    }









    //чистим
    int mn = munmap(ptr, sh_len);
    int mn2 = munmap(ptr2, sh_len2);
    if(mn == -1)
    {
        perror("munmp");
        _exit(1);
    }
    if(mn2 == -1)
    {
        perror("munmp");
        _exit(1);
    }

    int cl = close(fd);
    int cl2 = close(fd2);
    if(cl == -1)
    {
        perror("closerr");
        _exit(1);
    }
    if(cl2 == -1)
    {
        perror("closerr");
        _exit(1);
    }


    return 0;
}