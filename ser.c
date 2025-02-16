#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>
#include <stdbool.h>
#define MAXLINE 128
#define SERV_PORT 11111
#define OPEN_MAX 1024

int main(int argc, char *argv[])
{
	int i, listenfd, connfd, sockfd;
	ssize_t n;
	char buf[MAXLINE];
	char otherfd[20];
	socklen_t clilen;
	struct sockaddr_in cliaddr, servaddr;
	struct epoll_event ev,events[OPEN_MAX];

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	    
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(SERV_PORT);

	bind(listenfd,(struct sockaddr*)&servaddr, sizeof(servaddr));

	listen(listenfd, 20);

	int epollfd = epoll_create(OPEN_MAX);
	if (epollfd == -1)
	{
		perror("epoll_create");
		exit(EXIT_FAILURE);
	}
	
	ev.events = EPOLLIN;
	ev.data.fd = listenfd;
	if(epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev) == -1)
	{
		perror("epoll_ctl");
		exit(EXIT_FAILURE);
	}

	int cnt = 0;int num = 0;	//记录当前连接的客户端数量
	int indep_client_fd = -1;	//存储独立客户端的文件描述符
	bool isIndependent = false;	//标记是否有独立客户端连接

	while(1)
	{
		int nfds = epoll_wait(epollfd, events, OPEN_MAX, -1);
		if (nfds == -1)
		{
			perror("epoll_wait");
			exit(EXIT_FAILURE);
		}

		for(i = 0; i<nfds; i++)
		{
			if(events[i].data.fd == listenfd)  //有新的连接
			{
				clilen = sizeof(cliaddr);
				connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &clilen);
				if(connfd == -1)
				{
					perror("accept");
					exit(EXIT_FAILURE);
				}
			
				cnt++;

				if(!isIndependent)  //第一个连接的客户端为独立客户端
				{
					isIndependent = true;
					indep_client_fd = connfd;
					printf("Received connection from independent client:%s\n",inet_ntoa(cliaddr.sin_addr));
					printf("indep_client_fd = %d\n",connfd);
					ev.events = EPOLLIN | EPOLLET;
					ev.data.fd = connfd;
					if(epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &ev) == -1)
					{
						perror("epoll_ctl");
						exit(EXIT_FAILURE);
					}
				}
				else
				{
					printf("Received connection from client: %s\n", inet_ntoa(cliaddr.sin_addr));
					ev.events = EPOLLIN | EPOLLET;
					ev.data.fd = connfd;
					otherfd[num] = connfd;
					if(epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &ev) == -1)
					{
						perror("epoll_ctl");
						exit(EXIT_FAILURE);
					}
					write(indep_client_fd,"weather",n);

					num++;
				}
			}	
			else   //已连接客户端有数据到达
			{
				sockfd = events[i].data.fd;
				n = read(sockfd, buf, MAXLINE);
				printf("sockfd = %d\n",sockfd);
				if(n<=0)
				{
					printf("Client closed connection: %s\n",inet_ntoa(cliaddr.sin_addr));
					if(epoll_ctl(epollfd, EPOLL_CTL_DEL, sockfd, NULL) == -1)
					{
						perror("epoll_ctl");
						exit(EXIT_FAILURE);
					}
					close(sockfd);
					cnt--;
				}
				else   //接收到客户端发送的数据
				{
					if(sockfd == indep_client_fd)//独立客户端发送给其他客户端
					{
						printf("Received message from independent client:%s\n",buf);
						for(int j = 0; j<num;j++)
						{	
							write(otherfd[j], buf, n);
						}
						memset(buf,0,sizeof(buf));
					}
					else   //其他客户端，发送给独立客户端
					{
						printf("Received message from client:%s\n",buf);
						write(indep_client_fd, buf, n);
						memset(buf,0,sizeof(buf));
					}

				}

			}
		}
	}
	close(listenfd);
	close(epollfd);
}
