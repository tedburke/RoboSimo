//
// socks.c - Ted Burke - 27-9-2009
//
// This file contains the network communication thread function for RoboSimo.
//

#include <Winsock2.h>
#include <stdio.h>

#include "shared.h"

//#define D_PORT 4344
#define D_PORT 4009
//#define D_HOST "computer"
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
	//int cycle = 0;
	unsigned int sockets[D_SOCKETS];
	int sockets_index = 0;
	unsigned int maximun;
	char buffer[D_INFO];
	fd_set input;
		
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
	
	descriptor = socket(PF_INET, SOCK_STREAM, 0); // create a socket
	
	// get information about the host
	char hostname[50];
	result = gethostname(hostname, 50);
	memset(&addr, 0, sizeof(addr));
	host = gethostbyname(hostname); // NULL if error
	printf("Host name: %s\n", hostname);
	//printf("h_length = %d\n", host->h_length);
	unsigned char *a; // Used for printing IP addresses
	a = (unsigned char *)host->h_addr_list[0];
	printf("Host address: %u.%u.%u.%u\n", a[0], a[1], a[2], a[3]);
	
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
						{
							//printf("Received %d bytes from descriptor %d: %s\n", result, sockets[index], buffer);
							if (buffer[0] == 27)
							{
								// An arrow key was pressed.
								// These keys generate a 3-byte code.
								switch(buffer[2])
								{
								case 'A': // Up arrow key
									robot[0].v1 = 0.25;
									robot[0].v2 = 0.25;
									break;
								case 'B': // Down arrow key
									robot[0].v1 = -0.25;
									robot[0].v2 = -0.25;
									break;
								case 'C': // Right arrow key
									robot[0].v1 = 0.25;
									robot[0].v2 = -0.25;
									break;
								case 'D': // Left arrow key
									robot[0].v1 = -0.25;
									robot[0].v2 = 0.25;
									break;
								}
							}
							else
							{
								switch(buffer[0])
								{
								case 'f':
									robot[0].v1 = 0.25;
									robot[0].v2 = 0.25;
									break;
								case 'b':
									robot[0].v1 = -0.25;
									robot[0].v2 = -0.25;
									break;
								case 'r':
									robot[0].v1 = 0.25;
									robot[0].v2 = -0.25;
									break;
								case ' ':
								case 's':
									robot[0].v1 = 0;
									robot[0].v2 = 0;
									break;
								}
							}
						}
					}
				}
			}
		}
		
		//printf("%d\r", cycle++);
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
