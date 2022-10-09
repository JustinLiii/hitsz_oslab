#include "kernel/types.h"
#include "user.h"

int main (int argc, char* argv[]){
    int ret, p[2], wait_ret;
    char* recive_len[5];
    pipe(p);
    ret = fork();
    if (ret == 0)
    {
        // son
        read(p[0], recive_len, 5);
        printf("%d: received %s\n", getpid(),recive_len);
        write(p[1],"pong", 5);
        close(p[1]);
        exit(0);
    }
    else {
        // father
        write(p[1],"ping", 5);
        sleep(1);
        read(p[0], recive_len, 5);
        close(p[0]);
        printf("%d: received %s\n", getpid(),recive_len);
        wait(&wait_ret);
        exit(0);
    }
    exit(0);
}