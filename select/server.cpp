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

int fd[1024]; //连接的fd数组
#define BUF_SIZE 1024

int main(void)
{
    int yes = 1;

    int sock_fd; //监听套接字

    // 1.建立sock_fd套接字
    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket error");
        exit(1);
    }
    printf("server_sock_fd = %d\n", sock_fd);

    // 设置套接口的选项 SO_REUSEADDR 允许在同一个端口启动服务器的多个实例
    // setsockopt的第二个参数SOL SOCKET 指定系统中，解释选项的级别 普通套接字
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
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
    bind(sock_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    // 4.listen
    listen(sock_fd, 1024);

    // 文件描述符集的定义
    fd_set fdsr;
    int maxsock = sock_fd;
    struct timeval tv;
    // 设置读缓冲区
    char buffer[BUF_SIZE];
    int conn_amount = 0;

    while (1)
    {
        // 清空描述符集
        FD_ZERO(&fdsr);
        // 添加server_sock_fd到描述符集
        FD_SET(sock_fd, &fdsr);
        //设置超时时间为阻塞30s，将这个参数设置为NULL,表明此时select为阻塞模式
        tv.tv_sec = 30;
        tv.tv_usec = 0;

        // 重新添加所有的描述符到描述符集合
        for (int i = 0; i < conn_amount; i++)
        {
            if (fd[i] != 0)
            {
                FD_SET(fd[i], &fdsr);
            }
        }

        //如果文件描述符中有连接请求 会做相应的处理，实现I/O的复用 多用户的连接通讯
        printf("select...\n");

        int res = select(maxsock + 1, &fdsr, NULL, NULL, &tv);
        if (res < 0) //没有找到有效的连接 失败
        {
            perror("select error!\n");
            break;
        }
        else if (res == 0) // 指定的时间到，
        {
            printf("timeout \n");
            continue;
        }

        printf("select result is %d\n", res);

        if (FD_ISSET(sock_fd, &fdsr))
        {
            int clnt_sock = accept(sock_fd, (struct sockaddr *)&clnt_addr, &clnt_addr_size);

            printf("accpet a new client fd=[%d]: %s:%d\n", clnt_sock, inet_ntoa(clnt_addr.sin_addr), clnt_addr.sin_port);
            // 将套接字存储到数组中
            fd[conn_amount++] = clnt_sock;

            if (clnt_sock > maxsock)
            {
                maxsock = clnt_sock;
            }
        }

        //下面这个循环是非常必要的，因为你并不知道是哪个连接发过来的数据，所以只有一个一个去找。
        for (int i = 0; i < conn_amount; i++)
        {
            if (FD_ISSET(fd[i], &fdsr))
            {
                res = recv(fd[i], buffer, sizeof(buffer), 0);
                //如果客户端主动断开连接，会进行四次挥手，会出发一个信号，此时相应的套接字会有数据返回，告诉select，我的客户断开了，你返回-1

                if (res <= 0) //客户端连接关闭，清除文件描述符集中的相应的位
                {
                    printf("client fd=[%d] close\n", fd[i]);
                    close(fd[i]);
                    FD_CLR(fd[i], &fdsr);
                    fd[i] = 0;
                    conn_amount--;
                }
                //否则有相应的数据发送过来 ，进行相应的处理
                else
                {
                    if (res < BUF_SIZE)
                        memset(&buffer[res], '\0', 1);
                    printf("clint fd=[%d] recv message: %s\n", fd[i], buffer);

                    char str[] = "Hello World!";
                    write(fd[i], str, sizeof(str));

                    printf("clint fd=[%d] send message: %s\n", fd[i], str);
                }
            }
        }
    }
}