/*******************************************************
  > File Name: server.c
  > Author:Coniper
  > Describe: 
  > Created Time: 2021年08月20日
 *******************************************************/
#include "server.h"

int main(int argc, char *argv[])
{
    //命令行插入端口
    if(2 != argc)
    {
        printf("Uage <port>\n");
        goto ERR_STEP;
    }

    //创建Epoll
    int epfd = epoll_create(POLL_SZ);
    if(0 > epfd)
    {
        perror("epoll_creat");
        goto ERR_STEP;
    }

    //调用初始化服务器函数
    int listenfd = server_init(atoi(argv[1]));
    if(0 > listenfd)
    {
        perror("socket");
        goto ERR_STEP;
    }
    printf("listen: %d, epoll: %d\n", listenfd, epfd);

    //设置套接字为非阻塞
    if(0 > setNoblock(listenfd))
    {
        goto ERR_STEP_C;
    }

    //epoll事件结构体
    struct epoll_event evt;
    evt.events  = EPOLLIN;
    evt.data.fd = listenfd;

    //目前只有一个监听套接字，把监听套接字放入epoll中
    if(0 > epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &evt))
    {
        ERR_LOG("epoll_ctl - EPOLL_CTL_ADD", listenfd);
    }

    printf("wait for a client\n");

    for(;;)
    {
        struct epoll_event evts[POLL_SZ];

        //从epfd[指定的epoll]监测的对象及事件等到结果，并保存到evts中，返回值实际发生的数量
        int sum = epoll_wait(epfd, evts, POLL_SZ, 600000);
        if(0 > sum)
        {
            perror("epoll_wait");
            break;
        }
        else if(0 == sum)
        {
            printf("timeout");
            continue;
        }

        //循环遍历要监测的事件
        for(int i = 0; i < sum; i++)
        {
            if(listenfd == evts[i].data.fd) //事件响应是监听套接，故是有新的连接
            {
                getlink(listenfd, epfd);
            }
            else
            {
                interaction(evts[i].data.fd, epfd);
            }
        }
    }

ERR_STEP_C:
    close(listenfd);

ERR_STEP:
    return 0;
}

