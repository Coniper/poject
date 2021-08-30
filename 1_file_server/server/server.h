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

#define BFSZ 128
#define POLL_SZ 5
#define ERR_LOG(LOG,FD)    do{perror(LOG);close(FD);exit(-1);}while(0)

pthread_mutex_t mutex;

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
int menu(char *buf, int clifd, int epfd);       //列表、上传、下载、退出的具体实现
int upfile(char *name, int length, int clifd);             //上传具体操作
void *process_r_1(void *);                            //上传线程 1
void *process_r_2(void *);                            //上传线程 2

int server_init(int port)   //初始化服务器
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(-1 == sockfd){
        return -1;
    }

    //设置套接字属性 应用层的地址和端口重用
    int on = 1;     //定义操作的方式 1(打开) 0(关闭)
    if(-1 == setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)))
    {
        ERR_LOG("setsockopt", sockfd);
    }

    struct sockaddr_in addr;
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(port);
    addr.sin_addr.s_addr = inet_addr("0.0.0.0");

    if(-1 == bind(sockfd, (const struct sockaddr *)&addr, (socklen_t)sizeof(addr)))
    {
        ERR_LOG("bind", sockfd);
    }

    if(-1 == listen(sockfd, 5))
    {
        ERR_LOG("listen", sockfd);
    }

    printf("server init success\n");

    return sockfd;
}

int wait_client(int listenfd)   //等待及确认客户端连接
{
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    int connfd = accept(listenfd, (struct sockaddr *)&addr, &addrlen);
    if(-1 == connfd)
    {
        ERR_LOG("accept", listenfd);
    }
    printf("Client_ip: %s port: %u\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

    return connfd;
}

int setNoblock(int fd)  //设置监听套接字为非阻塞
{
    //得到文件文件描述符标志
    int flag = fcntl(fd, F_GETFL, 0);
    if(0 > flag)
    {
        perror("fcntl");
        return -1;
    }

    //在原有标志的基础上追加为非阻塞标志
    flag |= O_NONBLOCK;

    //将设置回文件描述符
    if(0 > fcntl(fd, F_SETFL, flag))
    {
        perror("fcntl");
        return -1;
    }

    return 0;
}

void getlink(int sockfd, int epfd)  //将新加入客户端套接子添加到epoll树中
{
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    int cli = accept(sockfd, (struct sockaddr *)&addr, &len);   
    if(0 > cli)
    {
        perror("accept");
        return (void)-1;
    }
    printf("Client_ip: %s port: %u\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

    struct epoll_event evt;
    evt.events  = EPOLLIN;
    evt.data.fd = cli;

    //向epfd指定epoll放入监测对象
    if(0 > epoll_ctl(epfd, EPOLL_CTL_ADD, cli, &evt))
    {
        perror("epoll_ctl - EPOLL_CTL_ADD");
        close(cli);         //创建失败关闭连接
        return (void)-1;
    }
}

void interaction(int clifd, int epfd)   //交互
{
    char *buf = (char *)malloc(BFSZ);

    int ret = recv(clifd, buf, BFSZ-1, 0);
    if(0 > ret)
    {
        perror("recv");
        free(buf);
        return (void)-1;
    }
    else if(0 == ret)   //若返回值等于0，故客户端已离开，需要剔除及释放空间
    {
        printf("client: %d is leave\n", clifd);
        
        //从epoll中剔除
        if(0 > epoll_ctl(epfd, EPOLL_CTL_DEL, clifd, NULL))
        {
            perror("epoll_ctl - EPOLL_CTL_DEL");
            return (void)-1;
        }
        close(clifd);
        free(buf);
        return (void)-1;
    }
    menu(buf, clifd, epfd); //调用菜单函数实现具体操作
}

int menu(char *buf, int clifd, int epfd)    //列表、上传、下载、退出的具体实现
{
    if(NULL != strstr(buf, "list")) //列表
    {
        DIR *dir = opendir("./data");
        if(NULL == dir)
        {
            perror("opendir");
            free(buf);
            return -1;
        }

        struct dirent *dbf;         //目录结构体
        struct stat file;           //文件属性结构体

        bzero(buf, sizeof(buf));    //置空缓冲，准备下方操作
        while((dbf = readdir(dir)))
        {
            lstat(dbf->d_name, &file);
            switch(file.st_mode & S_IFMT)   //获取文件属性，循环拼接文件名并放入发送缓冲
            {
                case S_IFDIR:  sprintf(buf+strlen(buf), "d%s ", dbf->d_name);   break;
                case S_IFREG:  sprintf(buf+strlen(buf), "-%s ", dbf->d_name);   break;
                default:       sprintf(buf+strlen(buf), "?%s ", dbf->d_name);   break;
            }
        }

        if(-1 == send(clifd, buf, strlen(buf), 0))  //发送目录信息
        {
            perror("send");
            return -1;
        }

        closedir(dir);  //关闭目录文件
    }
    else if(NULL != strstr(buf, "uploading"))   //上传
    {
#define nmsz 25
        char *p = strchr(buf, ':');
        p++;
        
        int length = atoi(p);                   //获取文件长度
        printf("len = %d\n", length);
        
        p = strchr(buf, ';');
        p++;
        
        char name[nmsz] = "./data/";            //为上传的文件，添加工作目录前缀
        strcat(name, p);   //拼接
        printf("file: %s\n", name);
        name[strlen(name)] = '\0';              //文件名后置位'\n'置为'\0'
        
        upfile(name, length ,clifd);                   //上传的IO操作
    }
    else if(NULL != strstr(buf, "download"))    //下载
    {
        //downfile();
    }
    else if(NULL != strstr(buf, "quit"))        //退出
    {
        send(clifd, "thank you for using\n", 25, 0);
        printf("client: %d is leave\n", clifd);
        //从epoll中剔除
        if(0 > epoll_ctl(epfd, EPOLL_CTL_DEL, clifd, NULL))
        {
            perror("epoll_ctl - EPOLL_CTL_DEL");
            free(buf);
            return -1;
        }
        close(clifd);
        free(buf);
    }
    else
    {
        printf("recvfrom-> %d : %s\n", clifd, buf);
    }
}

int upfile(char *name, int length, int clifd)   //上传具体操作
{

    int fd = open(name, O_WRONLY | O_CREAT, 0640);
    if(-1 == fd)
    {
        perror("open");
        return -1;
    }
    
    //封装文件大小及文件描述符为结构体，向线程传递
    file st;
    st.fd    = fd;
    st.len   = length;
    st.clifd = clifd;

    pthread_t tid_1, tid_2;
    if(-1 == pthread_create(&tid_1, NULL, process_r_1, (void *)&st))  //线程1
    {
        perror("thread create failed");
        close(fd);
        unlink(name);
        return -1;
    }
    if(-1 == pthread_create(&tid_2, NULL, process_r_2, (void *)&st))  //线程2
    {
        perror("thread create failed");
        close(fd);
        unlink(name);
        return -1;
    }

    //设置阻塞等待回收子线程资源
    char *ret_1, *ret_2;
    if(0 != pthread_join(tid_1, (void **)&ret_1))
	{
		perror("pthread_join");
		return -1;
	}
    if(0 != pthread_join(tid_2, (void **)&ret_2))
	{
		perror("pthread_join");
		return -1;
	}
    printf("ret_1: %s\nret_2: %s\n", ret_1, ret_2);
}

void *process_r_1(void *age)
{
    char buf[BFSZ] = {0};
    int count = 0;
    int fd = ((file*)age)->fd;
    while(1)
    {
        memset(buf, 0, sizeof(buf));
        int ret = recv(((file*)age)->clifd, buf, sizeof(buf), 0);
        if(ret < 0)
            break;
        count += ret;
    }
    write(fd, buf, ((file*)age)->len/2);
}

void *process_r_2(void *age)
{
    char buf[BFSZ] = {0};
    int count = 0;
    int fd = ((file*)age)->fd;
    lseek(fd, ((file*)age)->len, SEEK_SET);

    while(1)
    {
        memset(buf, 0, sizeof(buf));
        int ret = recv(((file*)age)->clifd, buf, sizeof(buf), 0);
        if(ret < 0)
            break;
        count += ret;
    }
    write(fd, buf, ((file*)age)->len - (((file*)age)->len/2));

}


#endif
