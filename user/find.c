#include "kernel/types.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "kernel/stat.h"
#include "user.h"
#define DEBUG

void find(char* dir, char* target)
{
    
    int fd, fd_sub;
    struct dirent a;
    struct stat st;
    //chdir(dir);
    fd = open(dir, O_RDONLY);
    read(fd,&a,sizeof(a));
    while (a.inum > 0)
    {
        #ifdef DEBUG
        printf("checking %s",a.name);
        #endif
        if((a.name[0] == '.' && a.name[1] == '\0') || (a.name[0] == '.' &&  a.name[1] == '.' && a.name[2] == '\0'))
        {
            if(read(fd,&a,sizeof(a)) != sizeof(a))
            {
                break;
            }
            #ifdef DEBUG
            printf("\n");
            #endif
            continue;
        }
        
        fd_sub = open(a.name, O_RDONLY);
        if(fstat(fd_sub, &st) < 0)
        {
            #ifdef DEBUG
            printf("\n");
            #endif
            printf("cannot stat %s\n", a.name);
            close(fd_sub);
            continue;
        }
        close(fd_sub);
        
        #ifdef DEBUG
        printf(" type %d, size %d\n", st.type,st.size);
        #endif
        if(st.type == T_DIR)
        {
            int dir_size = 0, a_size = 0;
            while (dir[dir_size]!='\0')
            {
                dir_size++;
            }
            while (a.name[a_size]!='\0')
            {
                a_size++;
            }
            
            char new_dir[a_size+dir_size+2];
            int i = 0;
            for (; i < dir_size; i++)
            {
                new_dir[i] = dir[i];
            }
            new_dir[i++] = '/';
            for(;i< dir_size + 2 + a_size;i++)
            {
                new_dir[i] = a.name[i-dir_size-1];
            }
            #ifdef DEBUG
            printf("dir size = %d\ntarget size = %d\n", dir_size, a_size);
            printf("expand to %s\n", new_dir);
            #endif
            find(new_dir, target);
            // if(find(new_dir, target, find_return))
            // {
            //     a_size = 0;
            //     while (find_return[a_size]!='\0')
            //     {
            //         a_size++;
            //     }
            //     i = 0;
            //     for (; i < dir_size; i++)
            //     {
            //         result[i] = dir[i];
            //     }
            //     result[++i] = '/';
            //     for(;i< dir_size + 2 + a_size;i++)
            //     {
            //         result[i] = find_return[i];
            //     }
            //     return;
            // }
        }
        else if(st.type == T_FILE)
        {
            int i = 0;
            while (1)
            {
                if(a.name[i] != target[i])
                {
                    break;
                }
                if(a.name[i] == '\0' && target[i] == '\0')
                {
                    printf("%s/%s\n", dir, a.name);
                    break;
                }
                i++;
            }
        }
        if(read(fd,&a,sizeof(a)) != sizeof(a))
        {
            break;
        }
    }
    return;
}

void main(int argc, char* argv[])
{
    int i = 0;
    if(argc == 1 || argc > 3)
    {
        printf("Invalid parameter\n");
        exit(0);   
    }
    else if(argc == 2) 
    {
        find(".",argv[1]);
        exit(0);
    }


    while (argv[1][i] != '\0')
    {
        i++;
    }
    printf("dir length %d\n",i);
    i--;
    if(argv[1][i] == '/')
    {
        argv[1][i] = '\0';
        printf("Corrected path\n");
    }
    find(argv[1],argv[2]);
    exit(0);
}