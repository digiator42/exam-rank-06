#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>

const int MAX_CLIENTS = 1000;
const int BUFFER_SIZE = 1024;

char buffer[1024] = {0};
char buff[1024 + 100] = {0};
int clients[1024] = {0};
int serverSocket;
int newSocket;
int len;
int valread;
int sd;
fd_set readFds;
struct sockaddr_in servaddr = {0};

void sendMSG(int fd, int isBuff, char *msg)
{
	fd = fd > 3 ? fd - 4 : fd;
	if (!isBuff)
	{
		sprintf(buffer, msg, fd);
		for (size_t i = 0; i < MAX_CLIENTS; i++)
		{
			if (clients[i] != fd + 4)
				send(clients[i], buffer, strlen(buffer), 0);
		}
		bzero(buffer, BUFFER_SIZE);
		return;
	}
	sprintf(buff, msg, fd, buffer);
	for (size_t i = 0; i < MAX_CLIENTS; i++)
	{
		if (clients[i] != fd + 4)
			send(clients[i], buff, strlen(buff), 0);
	}
	bzero(buffer, BUFFER_SIZE);
}

void openSocket(int port)
{
	serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSocket <= 0)
	{
		printf("socket creation failed...\n");
		exit(0);
	}

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = INADDR_ANY;
	servaddr.sin_port = htons(port);

	if ((bind(serverSocket, (const struct sockaddr *)&servaddr, sizeof(servaddr))) < 0)
	{
		printf("socket bind failed...\n");
		exit(0);
	}
	if (listen(serverSocket, 10) < 0)
	{
		printf("cannot listen\n");
		exit(0);
	}
	len = sizeof(servaddr);
}

void handleClientMsgs()
{
	for (size_t i = 0; i < MAX_CLIENTS; i++)
	{
		sd = clients[i];
		if (FD_ISSET(sd, &readFds))
		{
			if ((valread = recv(sd, buffer, BUFFER_SIZE, 0)) <= 0)
			{
				sendMSG(sd, 0, "server: client %d just left\n");
				close(sd);
				clients[i] = 0;
			}
			buffer[valread - 1] = 0;
			sendMSG(sd, 1, "client %d: %s\n");
		}
	}
}

int run()
{
	int i = 0;
	for (;;)
	{
		FD_ZERO(&readFds);
		FD_SET(serverSocket, &readFds);

		for (size_t i = 0; i < MAX_CLIENTS; i++)
		{
			sd = clients[i];
			if (sd > 0)
				FD_SET(sd, &readFds);
		}
		int activity = select(MAX_CLIENTS + 1, &readFds, NULL, NULL, NULL);
		if (activity <= 0 && errno != EINTR)
			return printf("select error");

		if (FD_ISSET(serverSocket, &readFds) || FD_ISSET(sd, &readFds))
		{
			newSocket = accept(serverSocket, (struct sockaddr *)&servaddr, (socklen_t *)&len);
			if (newSocket < 0)
			{
				printf("server acccept failed...\n");
				exit(0);
			}
			for (size_t i = 0; i < MAX_CLIENTS; i++)
			{
				if (clients[i] == 0)
				{
					clients[i] = newSocket;
					break;
				}
			}
			sprintf(buffer, "server: client %d just arrived\n", newSocket);
			sendMSG(newSocket, 0, "server: client %d just arrived\n");
		}
		handleClientMsgs();
	}
}

int main(int ac, char **av)
{
	if (ac != 2)
		return printf("wrong numbers of arguments\n");
	openSocket(atoi(av[1]));
	return run();
}