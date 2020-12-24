#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <fcntl.h>

#define MAX_EVENT 20
#define READ_BUF_LEN 256
#define BACK_LOG 1024

void setNonblocking(int sockfd)
{
    int opts;
    opts = fcntl(sockfd, F_GETFL);
    if (opts < 0)
    {
        perror("fcntl(sock,GETFL)");
        return;
    } //if

    opts = opts | O_NONBLOCK;
    if (fcntl(sockfd, F_SETFL, opts) < 0)
    {
        perror("fcntl(sock,SETFL,opts)");
        return;
    } //if
}

int main()
{

    int sock_fd; //监听套接字

    // 1.建立sock_fd套接字
    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket error");
        exit(1);
    }
    printf("server_sock_fd = %d\n", sock_fd);

    int on = 1;
    // 设置套接口的选项 SO_REUSEADDR 允许在同一个端口启动服务器的多个实例
    // setsockopt的第二个参数SOL SOCKET 指定系统中，解释选项的级别 普通套接字
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int)) == -1)
    {
        perror("setsockopt error \n");
        exit(1);
    }

    // 2.声明server addr结构体,client addr结构体
    struct sockaddr_in serv_addr;
    struct sockaddr_in clnt_addr;
    socklen_t serv_addr_size = sizeof(serv_addr);
    socklen_t clnt_addr_size = sizeof(clnt_addr);

    // 初始化结构体
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;                     //使用IPv4地址
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); //具体的IP地址
    serv_addr.sin_port = htons(8081);                   //端口

    // 3.bind
    bind(sock_fd, (struct sockaddr *)&serv_addr, serv_addr_size);

    // 4.listen
    listen(sock_fd, BACK_LOG);

    setNonblocking(sock_fd);

    /*声明epoll_event结构体变量，ev用于注册事件，数组用于回传要处理的事件*/
    struct epoll_event ev, events[MAX_EVENT];
    // 5.创建epoll实例
    int epfd = epoll_create1(0);

    /*设置监听描述符*/
    ev.data.fd = sock_fd;
    /*设置处理事件类型, 为边缘触发*/
    ev.events = EPOLLIN | EPOLLET;

    /*注册事件*/
    epoll_ctl(epfd, EPOLL_CTL_ADD, sock_fd, &ev);
    int sockfd;
    // 设置读缓冲区
    char buffer[READ_BUF_LEN];
    char str[] = "Hello World!";
    ssize_t n, ret;

    for (;;)
    {
        printf("epoll wait...\n");
        int nfds = epoll_wait(epfd, events, MAX_EVENT, -1);
        if (nfds <= 0)
            continue;
        printf("nfds = %d\n", nfds);

        for (int i = 0; i < nfds; i++)
        {
            // 新连接接入
            if (events[i].data.fd == sock_fd)
            {
                int clnt_sock = accept(sock_fd, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
                printf("accpet a new client fd=[%d]: %s:%d\n", clnt_sock, inet_ntoa(clnt_addr.sin_addr), clnt_addr.sin_port);
                // 设置非阻塞
                setNonblocking(clnt_sock);
                /*设置监听描述符*/
                ev.data.fd = clnt_sock;
                /*设置处理事件类型, 为边缘触发*/
                ev.events = EPOLLIN | EPOLLET;
                epoll_ctl(epfd, EPOLL_CTL_ADD, clnt_sock, &ev);
            }
            else if (events[i].events & EPOLLIN)
            { //
                if ((sockfd = events[i].data.fd) < 0)
                    continue;
                if ((ret = read(sockfd, buffer, sizeof(buffer))) <= 0)
                {
                    printf("client fd=[%d] close\n", sock_fd);
                    close(sockfd);
                    events[i].data.fd = -1;
                }
                else
                {
                    printf("clint fd=[%d] recv message: %s\n", sock_fd, buffer);

                    /*设置用于注册写操作文件描述符和事件*/
                    ev.data.fd = sockfd;
                    ev.events = EPOLLOUT | EPOLLET;
                    epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd, &ev);
                }
            }
            else if (events[i].events & EPOLLOUT)
            {
                if ((sockfd = events[i].data.fd) < 0)
                    continue;
                write(sockfd, str, sizeof(str));
                printf("clint fd=[%d] send message: %s\n", sock_fd, str);
                //if
                /*设置用于读的文件描述符和事件*/
                ev.data.fd = sockfd;
                ev.events = EPOLLIN | EPOLLET;
                /*修改*/
                epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd, &ev);
            }
        }
    }
}