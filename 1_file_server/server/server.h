/*******************************************************
  > File Name: server.h
  > Author:Coniper
  > Describe: 
  > Created Time: 2021年08月27日
 *******************************************************/

#ifndef _SERVER_H
#define _SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

#include <fcntl.h>
#include <unistd.h>

#include <dirent.h>

#include <pthread.h>
#include <semaphore.h>

#define BFSZ 128
#define POLL_SZ 5
#define NMSZ 30
#define ERR_LOG(LOG,FD)    do{perror(LOG);close(FD);exit(-1);}while(0)

sem_t sem_1, sem_2;

typedef struct _file {
    int fd;
    int len;
    int clifd;
}file;

int server_init(int port);                      //初始化服务器
int wait_client(int listenfd);                  //等待及确认客户端连接
int setNoblock(int fd);                         //设置监听套接字为非阻塞
void getlink(int sockfd, int epfd);             //将新加入客户端套接子添加到epoll树中
void interaction(int clifd, int epfd);          //交互
int list(int clifd);                            //发送列表
int upfile(char *name, int length, int clifd);             //上传具体操作
void *process_r_1(void *);                            //上传线程 1
void *process_r_2(void *);                            //上传线程 2


#endif
