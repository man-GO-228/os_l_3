
/**
 * gcc -Wall -Wextra -pthread c.c -o c &&
gcc -Wall -Wextra -pthread p.c -o p &&
rm -f /dev/shm/sem.sem_data /dev/shm/sem.sem_free \
      /dev/shm/sem.sem_data2 /dev/shm/sem.sem_free2 &&
./p 
*/




#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <sys/mman.h>  
#include <fcntl.h>    

#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>

const char sh_name[] = "/sh_name";
const size_t sh_len = 4096;

const char sh_name2[] = "/sh_name2";
const size_t sh_len2 = 4096;

int main()
{
    //первое 
    int shm_fd = shm_open(sh_name, O_RDWR | O_CREAT | O_TRUNC, 0600);
    if (shm_fd == -1)
    {
        perror("shm_open");
        _exit(1);
    }

    int shm_ftuncate = ftruncate(shm_fd, (off_t)(sh_len));
    if (shm_ftuncate == -1)
    {
        perror("shm_ftruncate");
        _exit(1);
    }

    void* shm_mmap = mmap(NULL, sh_len, 
        PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    if (shm_mmap == MAP_FAILED)
    {
        perror("shm_mmap");
        _exit(1);
    }

    //второе
    int shm_fd2 = shm_open(sh_name2, O_RDWR | O_CREAT | O_TRUNC, 0600);
    if (shm_fd2 == -1)
    {
        perror("shm_open2");
        _exit(1);
    }

    int shm_ftuncate2 = ftruncate(shm_fd2, (off_t)(sh_len2));
    if (shm_ftuncate2 == -1)
    {
        perror("shm_ftruncate2");
        _exit(1);
    }

    void* shm_mmap2 = mmap(NULL, sh_len2, 
        PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd2, 0);

    if (shm_mmap2 == MAP_FAILED)
    {
        perror("shm_mmap2");
        _exit(1);
    }

    //семафоры
    sem_t *sem_data = sem_open("/sem_data", O_CREAT | O_EXCL, 0600, 0);//можно ли родителю записывать новую строку
    sem_t *sem_free = sem_open("/sem_free", O_CREAT | O_EXCL, 0600, 1);//есть ли строка которую ребёнок должен прочитат
    if (sem_data == SEM_FAILED) {
        perror("sem_open");
        _exit(1);
    }
    if (sem_free == SEM_FAILED) {
        perror("sem_open");
        _exit(1);
    }

    sem_t *sem_data2 = sem_open("/sem_data2", O_CREAT | O_EXCL, 0600, 0);//можно ли родителю записывать новую строку
    sem_t *sem_free2 = sem_open("/sem_free2", O_CREAT | O_EXCL, 0600, 1);//есть ли строка которую ребёнок должен прочитат
    if (sem_data2 == SEM_FAILED) {
        perror("sem_open2");
        _exit(1);
    }
    if (sem_free2 == SEM_FAILED) {
        perror("sem_open2");
        _exit(1);
    }


    //дочь
    pid_t pid = fork();

    if (pid < 0)
    {   
        perror("fork");
        _exit(1);
    }
    if (pid == 0)
    {
        char *const argv[] = {"./c", NULL};
        char *const envp[] = {NULL};
        execve("./c", argv, envp);

        perror("execve");
        _exit(1);
    }


    char *ptr = (char*)shm_mmap;
    char *ptr2 = (char*)shm_mmap2;



    
    //основной код

    //имя
    char name[128];
    size_t name_len = 0;
    char c;

    while (name_len < sizeof(name) - 1)
    {
        ssize_t n = read(STDIN_FILENO, &c, 1);

        if (n <= 0)
        {
            perror("readerr");
            return 1;
        }

        if (c == '\n')
            break;

        name[name_len++] = c;
    }
    name[name_len] = '\0';
    sem_wait(sem_free);
    strcpy(ptr, name);
    sem_post(sem_data);

    /**
     * имя 2
     */

    char name2[128];
    size_t name_len2 = 0;

    while (name_len2 < sizeof(name2) - 1)
    {
        ssize_t n = read(STDIN_FILENO, &c, 1);

        if (n <= 0)
        {
            perror("readerr2");
            return 1;
        }

        if (c == '\n')
            break;

        name2[name_len2++] = c;
    }
    name2[name_len2] = '\0';
    sem_wait(sem_free2);
    strcpy(ptr2, name2);
    sem_post(sem_data2);






    char chunk[4096];
    char *line = NULL;

    size_t cap = 0;
    size_t len = 0;

    ssize_t n;
    int chet_nechet = 1;
    while ((n = read(STDIN_FILENO, chunk, sizeof(chunk))) > 0)
    {
        for (ssize_t i = 0; i < n; i++)
        {
            if (chunk[i] == '\n')
            {
                /*
                 * cтрока полностью получена
                 */
                    if (len > 0)
                    {
                    if (len >= sh_len) 
                    {
                        fprintf(stderr, "Строка слишком длинная, пропускаю\n");
                        len = 0;
                        continue;  // перейти к следующему байту входа
                    }
                    if (chet_nechet == 1)
                    {
                        line[len] = '\0';
                        sem_wait(sem_free);
                        strcpy(ptr, line);
                        sem_post(sem_data);
                        chet_nechet = 2;
                    }
                    else if(chet_nechet == 2)
                    {
                        line[len] = '\0';
                        sem_wait(sem_free2);
                        strcpy(ptr2, line);
                        sem_post(sem_data2);
                        chet_nechet = 1;
                    }    
                }
                len = 0;
            }
            else
            {
                /*
                 * Увеличиваем буфер,
                 * если места не хватает.
                 */

                if (len + 1 >= cap)
                {
                    size_t new_cap = cap ? cap * 2 : 256;

                    char *tmp = realloc(line, new_cap);

                    if (tmp == NULL)
                    {
                        free(line);
                        close(shm_fd);
                        close(shm_fd2);
                        waitpid(pid, NULL, 0);

                        return 1;
                    }

                    line = tmp;
                    cap = new_cap;
                }

                line[len++] = chunk[i];
            }
        }
    }

    if (chet_nechet == 1) {
        sem_wait(sem_free);
        ptr[0] = '\0';
        sem_post(sem_data);
    } else {
        sem_wait(sem_free2);
        ptr2[0] = '\0';
        sem_post(sem_data2);
    }


    //ждем заверш и чистим 
    pid_t wt = waitpid(pid, NULL, 0);
    if (wt == -1)
    {
        perror("waitpid");
        _exit(1);
    }


    sem_close(sem_data);
    sem_close(sem_free);
    sem_close(sem_data2);
    sem_close(sem_free2);

    sem_unlink("/sem_data");
    sem_unlink("/sem_free");
    sem_unlink("/sem_data2");
    sem_unlink("/sem_free2");





    int mn2 = munmap(shm_mmap2, sh_len2), 
        mn = munmap(shm_mmap, sh_len);
    if(mn == -1)
    {
        perror("munmp");
        _exit(1);
    }
    if(mn2 == -1)
    {
        perror("munmp2");
        _exit(1);
    }
    
    int cl = close(shm_fd),
        cl2 = close(shm_fd2);
    if(cl == -1)
    {
        perror("closerr");
        _exit(1);
    }
    if(cl2 == -1)
    {
        perror("closerr2");
        _exit(1);
    }


    int unl = shm_unlink(sh_name),
        unl2 = shm_unlink(sh_name2);
    if(unl == -1)
    {
        perror("unlinkerr");
        _exit(1);
    }
    if(unl2 == -1)
    {
        perror("unlinkerr2");
        _exit(1);
    }

    return 0;
}