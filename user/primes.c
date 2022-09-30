#include "kernel/types.h"
#include "user.h"


int main(int argc, char* argv[])
{
    volatile
    int old_p[2];
    int new_p[2];
    int this_prime;
    int number;
    // init prepared
    //open pipe
    pipe(new_p);
    for (int i = 2; i < 36; i++)
    {
        write(new_p[1],&i,sizeof(int));
    }
    while (!fork())
    {
        // printf("Process %d created\n", getpid());
        old_p[0] = new_p[0];
        old_p[1] = new_p[1];
        pipe(new_p);
        close(old_p[1]);

        if(read(old_p[0],&this_prime,sizeof(int)) != sizeof(int)) break;
        printf("prime %d\n", this_prime);
        while (read(old_p[0],&number,sizeof(int)) == sizeof(int))
        {
            if (number % this_prime == 0)
            {
                continue;
            }
            write(new_p[1], &number, sizeof(int));
        }
        close(old_p[0]);
    }

    close(new_p[0]);
    close(new_p[1]);
    wait(&number);
    
    // printf("Process %d ended", getpid());
    exit(0);
}