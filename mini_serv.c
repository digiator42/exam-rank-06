#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

const int BUFFER_SIZE = 10000;
const int MAX_CLINTS = 1000;

char buffer[10000] = {0};
char buff[10000 + 100] = {0};
int clients[1000] = {0};
int serverSocket;
int newSocket;
int sd;
int valread;
int len;
fd_set readFds;
struct sockaddr_in servaddr = {0};
int cnt = 0;

void sendMSG(int fd, int isBuff, char *msg)
{
	fd = fd > 3 ? fd - 4 : fd;
	if (!isBuff)
	{
		sprintf(buffer, msg, fd);
		for (size_t i = 0; i < MAX_CLINTS; i++)
		{
			if (clients[i] != fd + 4)
				send(clients[i], buffer, strlen(buffer), 0);
		}
		bzero(buffer, BUFFER_SIZE);
		return;
	}
	int i = 0;
	int k = 0;
	char temp[10000] = {0};
	msg = &buffer[0];
	while (buffer[i])
	{
		if (buffer[i] == '\n')
		{
			strncpy(temp, msg, i - k);
			sprintf(buff, "client %d: %s\n", fd, temp);
			for (size_t j = 0; j < MAX_CLINTS; j++)
			{
				if (clients[j] != fd + 4)
					send(clients[j], buff, strlen(buff), 0);
			}
			k += strlen(temp) + 1;
			bzero(buff, BUFFER_SIZE);
			msg = &buffer[i + 1];
		}
		i++;
	}
	bzero(buffer, BUFFER_SIZE);
}

void openSocket(int port)
{
	serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSocket < 0)
	{
		printf("socket creation failed...\n");
		exit(0);
	}

	bzero(&servaddr, sizeof(servaddr));

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

void handleClientsMsgs()
{
	for (size_t i = 0; i < MAX_CLINTS; i++)
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
			else
				sendMSG(sd, 1, "client %d: %s\n");
		}
	}
}

int run()
{
	for (;;)
	{
		FD_ZERO(&readFds);
		FD_SET(serverSocket, &readFds);
		int max_fd = serverSocket;
		for (size_t i = 0; i < MAX_CLINTS; i++)
		{
			sd = clients[i];
			if (sd > 0)
				FD_SET(sd, &readFds);
			if (max_fd < sd)
				max_fd = sd;
		}
		int activity = select(max_fd + 1, &readFds, NULL, NULL, NULL);
		if (activity < 0 && errno != EINTR)
			return printf("select error");

		if (FD_ISSET(serverSocket, &readFds))
		{
			newSocket = accept(serverSocket, (struct sockaddr *)&servaddr, (socklen_t *)&len);
			if (newSocket < 0)
			{
				printf("server acccept failed...\n");
				exit(0);
			}
			for (size_t i = 0; i < MAX_CLINTS; i++)
			{
				if (clients[i] == 0)
				{
					clients[i] = newSocket;
					break;
				}
			}
			sendMSG(newSocket, 0, "server: client %d just arrived\n");
		}
		handleClientsMsgs();
	}
}

int main(int ac, char **av)
{
	if (ac != 2)
		return printf("wrong numbers of arguments");
	openSocket(atoi(av[1]));
	return run();
}