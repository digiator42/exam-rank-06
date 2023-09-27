#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

const int MAX_CLIENTS = 128;
const int BUFFER_SIZE = 200000;

struct sockaddr_in address = {0};
int clientSockets[128] = {0};
char buffer[200000] = {0};
fd_set readfds;
int serverSocket;
int newSocket;
int sd;
int max_sd;
int valread;
int addrlen;

void openSocket()
{
	if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) <= 0)
	{
		return;
	}

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(6666);

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

				for (int i = 0; i < MAX_CLIENTS; i++)
				{
					if (clientSockets[i] != 0)
					{
						send(clientSockets[i], buffer, strlen(buffer), 0);
					}
				}
				close(sd);
				clientSockets[i] = 0;
			}
			else
			{
				buffer[valread] = '\0';
				char buff[BUFFER_SIZE] = {0};
				sprintf(buff, "client %d: %s\n", sd, buffer);
				for (int i = 0; i < MAX_CLIENTS; i++)
				{
					if (clientSockets[i] != sd)
					{
						send(clientSockets[i], buff, strlen(buff), 0);
					}
				}
				bzero(buffer, BUFFER_SIZE);
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
		max_sd = serverSocket;

		for (i = 0; i < MAX_CLIENTS; i++)
		{
			sd = clientSockets[i];
			if (sd > 0)
				FD_SET(sd, &readfds);
			if (sd > max_sd)
				max_sd = sd;
		}

		int activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
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
			for (int j = 0; j < MAX_CLIENTS; j++)
			{
				if (clientSockets[j] != newSocket)
				{
					send(clientSockets[j], buffer, strlen(buffer), 0);
				}
			}
			bzero(buffer, BUFFER_SIZE);
		}
		handleClientMessages();
	}
}

int main()
{
	openSocket();
	run();
}