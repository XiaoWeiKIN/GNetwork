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
#include <sys/poll.h>
#include <fcntl.h>

#define READ_BUF_LEN 256
#define BACK_LOG 1024
#define OPEN_MAX 1024

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

    struct pollfd client[OPEN_MAX];
    client[0].fd = sock_fd;
    client[0].events = POLLIN;
    int conn_amount = 1;
    int sockfd, ret;
    // 设置读缓冲区
    char buffer[READ_BUF_LEN];
    while (1)
    {
        printf("poll ...\n");
        int res = poll(client, conn_amount + 1, -1);
        printf("poll result is %d\n", res);
        // 新连接接入
        if (client[0].revents & POLLIN)
        {
            int clnt_sock = accept(sock_fd, (struct sockaddr *)&clnt_addr, &clnt_addr_size);

            printf("accpet a new client fd=[%d]: %s:%d\n", clnt_sock, inet_ntoa(clnt_addr.sin_addr), clnt_addr.sin_port);
            // 将套接字存储到数组中
            conn_amount++;
            client[conn_amount].fd = clnt_sock;
            client[conn_amount].events = POLLIN;
        }
        // 遍历所有的连接
        for (int i = 1; i <= conn_amount; i++)
        {
            if ((sockfd = client[i].fd) < 0)
                continue;
            if (client[i].revents & POLLIN)
            {
                if ((ret = read(sockfd, buffer, sizeof(buffer))) <= 0)
                {
                    printf("client fd=[%d] close\n", sock_fd);
                    client[i].fd = -1;

                    close(sockfd);
                }
                else
                {
                    printf("clint fd=[%d] recv message: %s\n", sockfd, buffer);

                    char str[] = "Hello World!";
                    write(sockfd, str, sizeof(str));
                    printf("clint fd=[%d] send message: %s\n", sockfd, str);
                }
            }
        }
    }
}