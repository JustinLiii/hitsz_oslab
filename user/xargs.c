#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user.h"

int readLineAndSplit(int fd, char** result, int* argcnt)
{
    int i = 0, j = 0;
    char bf;
    while (1)
    {
        if(read(fd, &bf, sizeof(bf))!= sizeof(bf)){
            result[i][j] = '\0';
            *argcnt = i+1;
            return 0;
        }
        if(bf == '\n')
        {
            result[i][j] = '\0';
            *argcnt = i+1;
            return 1;
        }
        else if(bf == ' ')
        {
            result[i][j] = '\0';
            i++;
            j = 0;
            continue;
        }
        else 
        {
            result[i][j++] = bf;
        }
    }
    
}

int main(int argc, char* argv[])
{
    int fd, argcnt, result = 1, childnum = 0;
    //fd = open("stdin", O_RDONLY);
    fd = 0;
    char* params[50];
    for (int i = 0; i < 50; i++)
    {
        params[i] = (char*)malloc(50*sizeof(char));
    }
    while (result)
    {
        result = readLineAndSplit(fd,params, &argcnt);
        //printf("%d\n",result);
        if(argc <= 1){
            for (int i = 0; i < argcnt; i++)
            {
                printf("%s ", params[i]);
            }
            printf("\n");
        }
        //准备参数
        else 
        {
            //printf("Proceed with input and args\n");
            int new_argc = argc-1+argcnt;
            char* new_argv[new_argc+1];
            int i;
            for (i = 0; i < argc-1; i++)
            {
                new_argv[i] = argv[i+1];
            }
            for (; i < new_argc; i++)
            {
                new_argv[i] = params[i-argc+1];
            }
            new_argv[new_argc] = 0;
            
            if(fork()==0)
            {
                exec(argv[1],new_argv);
            }
            else
            {
                childnum++;
            }
        }
    }
    
    for (int i = 0; i < childnum; i++)
    {
        wait(&result);
    }
    
    
    
    exit(0);
}