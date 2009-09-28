#include <Winsock2.h>
#include <stdio.h>

#include "shared.h"

#define D_PORT 4344
#define D_HOST "computer"
//#define D_HOST "localhost"
//#define D_HOST "DUBLIN-B4541FCD" // receive incoming connections from other computers
#define D_QUEUE 32
#define D_SOCKETS 16
#define D_INFO 256

DWORD WINAPI network_thread(LPVOID lpParameter)
{
	struct timeval tv;
	struct sockaddr_in addr;
	struct hostent *host;
	unsigned int descriptor;
	int result;
	int index;
	int cycle = 0;
	unsigned int sockets[D_SOCKETS];
	int sockets_index = 0;
	unsigned int maximun;
	char buffer[D_INFO];
	fd_set input;
	
	// My index
	int n;
	unsigned char *a; // Used for printing IP addresses
		
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
	
	descriptor = socket(PF_INET, SOCK_STREAM, 0); // create a socket
	memset(&addr, 0, sizeof(addr)); // get information about the host
	host = gethostbyname(D_HOST); // NULL if error
	printf("h_length = %d\n", host->h_length);
	a = (unsigned char *)host->h_addr_list[0];
	printf("Host address %d: %u.%u.%u.%u\n", n, a[0], a[1], a[2], a[3]);
	
	// bind the socket to an address and port
	memcpy(&addr.sin_addr, host->h_addr_list[0], sizeof(host->h_addr_list[0]));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(D_PORT);
	result = bind(descriptor, (struct sockaddr *)&addr, sizeof(addr)); // -1 if error	
	result = listen(descriptor, D_QUEUE); // put socket in listening state, returns -1 if error
	
	memset(sockets, 0, sizeof(sockets));
	maximun = descriptor;
	
	result = 0;
	while (program_exiting == 0)
	{
		FD_ZERO(&input);
		FD_SET(descriptor, &input);
		for (result = 0; result < sockets_index; result++)
		FD_SET(sockets[result], &input);
		
		// timeout structure
		tv.tv_sec = 0;
		tv.tv_usec = 0;
		result = select(maximun + 1, &input, NULL, NULL, &tv);
		//result = select(maximun + 1, &input, NULL, NULL, NULL);
		switch (result)
		{
		case -1:
			// error in select
			perror("select");
			break;
		case 0:
			// nothing to process
			break;
		default:
			// a number of sockets are ready for reading.
			if (FD_ISSET(descriptor , &input)) // check if the descriptor set is our listening one
			{
				sockets[sockets_index] = accept(descriptor, NULL, NULL);
				if (sockets[sockets_index] == -1)
				{
					perror("accept");
				}
				else
				{
					if (sockets[sockets_index] > maximun)
					maximun = sockets[sockets_index];
					sockets_index++;
				}
			}
			else
			{
				// one of the sockets is sending data. Find it
				for (index = 0; index < sockets_index; index++)
				{
					if (FD_ISSET(sockets[index], &input))
					{
						memset(buffer, 0, sizeof(buffer));
						
						// read information from socket
						result = recv(sockets[index], buffer, sizeof(buffer), 0);
						if (result == -1)
						perror("recv");
						else
						//printf("Received %d bytes from descriptor %d: %s\n", result, sockets[index], buffer);
						if (buffer[0] == 'f')
						{
							robot[0].v1 = 0.25;
							robot[0].v2 = 0.25;
						}
						else if (buffer[0] == 'b')
						{
							robot[0].v1 = -0.25;
							robot[0].v2 = -0.25;
						}
						else if (buffer[0] == 'l')
						{
							robot[0].v1 = -0.25;
							robot[0].v2 = 0.25;
						}
						else if (buffer[0] == 'r')
						{
							robot[0].v1 = 0.25;
							robot[0].v2 = -0.25;
						}
						else if (buffer[0] == 's')
						{
							robot[0].v1 = 0;
							robot[0].v2 = 0;
						}
					}
				}
			}
		}
		
		printf("%d\r", cycle++);
	}
	
	for (result = 0; result < sockets_index; result++)
	{
		closesocket(sockets[sockets_index]);
	}
	
	// Tidy up before exiting
	closesocket(descriptor);
	WSACleanup();
	
	printf("Exiting\n");
	
	return (0);
}
