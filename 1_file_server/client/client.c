/*******************************************************
  > File Name: client.c
  > Author:Coniper
  > Describe: 
  > Created Time: 2021年08月19日
 *******************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <unistd.h>

#include <arpa/inet.h>

#include <pthread.h>

#define BFSZ 128

int clientinit(int port);
void menu();
int senddata(int sockfd, char *buf);

int main(int argc, char *argv[])
{
    if(2 > argc)
    {
        perror("need server port\n");
        return -1;
    }

    int sockfd = clientinit(atoi(argv[1]));
    if(-1 == sockfd)
    {
        goto ERR_STEP;
    }

    char buf[BFSZ] = {0};

    while(1)
    {
        menu();

        printf("input: ");
        fgets(buf, sizeof(buf), stdin);

        if(NULL != strstr(buf, "list"))
        {
            senddata(sockfd, buf);
            memset(buf, 0, sizeof(buf));

            if(0 >= recv(sockfd, buf, sizeof(buf), 0))
            {
                printf("server disconnect\n");
                goto ERR_STEP;
            }
            else
            {
                printf("Server_List: \033[;34m%s\033[0m\n", buf);
            }
        }
        else if(NULL != strstr(buf, "uploading"))
        {
#define nmsz 10
            char *p = &buf[nmsz];
            p[strlen(p)-1] = '\0';

            int fd = open(p, O_RDONLY);
            if(-1 == fd)
            {
                perror("open");
                continue;
            }
            int len = lseek(fd, 0, SEEK_END);
            close(fd);

            char *ret = (char *)malloc(BFSZ);
            sprintf(ret, "uploading :%d;%s", len, p);

            if(-1 == senddata(sockfd, ret))
            {
                free(ret);
                break;
            }

            
            int sendfile(sockfd, fd)
            {

                pthread_t tid_1, tid_2;
                if(0 > pthread_create(&tid_1, NULL, thread_1, NULL))
                {
                    perror("thread_create_1");
                    return -1;

                }
                if(0 > pthread_create(&tid_2, NULL, thread_2, NULL))
                {
                    perror("thread_creat_2");
                    return -1;
                }

                char *ret;
                if(0 != pthread_join(tid_1, (void **)&ret))
                {
                    perror("pthread_join");
                    return -1;
                }
                if(0 != pthread_join(tid_2, (void **)&ret))
                {
                    perror("pthread_join");
                    return -1;
                }
            }
            void *thread_1()
            {
                int fd = open()
            }

            free(ret);
        }
        else if(NULL != strstr(buf, "download"))
        {

        }
        else if(NULL != strstr(buf, "quit"))
        {
            printf("thank for using client\n");
            break;
        }

    }

ERR_STEP:
    close(sockfd);

    return 0;
}

int clientinit(int port)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(-1 == sockfd)
    {
        perror("socket");
        exit(-1);
    }

    struct sockaddr_in sockaddr;
    sockaddr.sin_family      = AF_INET;
    sockaddr.sin_port        = htons(port);
    sockaddr.sin_addr.s_addr = inet_addr("192.168.164.128");

    if(0 != connect(sockfd, (const struct sockaddr *)&sockaddr, (socklen_t)sizeof(sockaddr)))
    {
        perror("connect");
        return -1;
    }

    return sockfd;
}
        
void menu()
{
    printf("\n*********************************************\n");
    printf("   1.list  2.uploading  3.download  4.quit   \n");
    printf("*********************************************\n\n");
}

int senddata(int sockfd, char *buf) 
{
        size_t ret = send(sockfd, buf, strlen(buf), 0);
        if(-1 == ret)
        {
            perror("send");
            close(sockfd);
            return -1;
        }
        else if(0 == ret)
        {
            printf("server network anomaly\n");
            return -1;
        }

        return 0;
}
