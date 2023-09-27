#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

const int MAX_CLIENTS = 1000;
const int BUFFER_SIZE = 1024;

struct sockaddr_in address = {0};
int clientSockets[1000] = {0};
char buffer[1024] = {0};
char buff[1024] = {0};
fd_set readfds;
int serverSocket;
int newSocket;
int sd;
int valread;
int addrlen;

void sendMsg(int fd, int isBuff)
{
	if (!isBuff)
	{
		for (size_t i = 0; i < MAX_CLIENTS; i++)
		{
			if (clientSockets[i] != fd)
				send(clientSockets[i], buffer, strlen(buffer), 0);
		}
		bzero(buffer, BUFFER_SIZE);
		return;
	}
	for (size_t i = 0; i < MAX_CLIENTS; i++)
	{
		if (clientSockets[i] != fd)
			send(clientSockets[i], buff, strlen(buff), 0);
	}
	bzero(buffer, BUFFER_SIZE);
}

void openSocket(int port)
{
	if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) <= 0)
	{
		return;
	}

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port);

	if (bind(serverSocket, (struct sockaddr *)&address, sizeof(address)) < 0)
	{
		return;
	}

	if (listen(serverSocket, MAX_CLIENTS) < 0)
	{
		return;
	}

	addrlen = sizeof(address);
}

void handleClientMessages()
{
	int i = 0;
	for (i = 0; i < MAX_CLIENTS; i++)
	{
		sd = clientSockets[i];
		if (FD_ISSET(sd, &readfds))
		{
			if ((valread = recv(sd, buffer, BUFFER_SIZE, 0)) <= 0)
			{
				sprintf(buffer, "server: client %d just left\n", sd);
				sendMsg(sd, 0);
				close(sd);
				clientSockets[i] = 0;
			}
			else
			{
				buffer[valread - 1] = '\0';
				sprintf(buff, "client %d: %s\n", sd, buffer);
				sendMsg(sd, 1);
			}
		}
	}
}

void run(void)
{
	int i = 0;
	for (;;)
	{
		FD_ZERO(&readfds);
		FD_SET(serverSocket, &readfds);

		for (i = 0; i < MAX_CLIENTS; i++)
		{
			sd = clientSockets[i];
			if (sd > 0)
				FD_SET(sd, &readfds);
		}

		int activity = select(MAX_CLIENTS + 1, &readfds, NULL, NULL, NULL);
		if ((activity < 0) && (errno != EINTR))
		{
			return;
		}

		if (FD_ISSET(serverSocket, &readfds) || FD_ISSET(sd, &readfds))
		{
			if ((newSocket = accept(serverSocket, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
			{
				return;
			}
			for (size_t i = 0; i < MAX_CLIENTS; i++)
			{
				if (clientSockets[i] == 0)
				{
					clientSockets[i] = newSocket;
					break;
				}
			}
			sprintf(buffer, "server: client %d just arrived\n", newSocket);
			sendMsg(newSocket, 0);
		}
		handleClientMessages();
	}
}

int main(int ac, char **av)
{
	if (ac != 2)
	{
		printf("Wrong number of arguments");
		exit(0);
	}
	openSocket(atoi(av[1]));
	run();
}