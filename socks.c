#ifdef WIN32
#include <Winsock2.h>
#else
#include <sys/types.h>
#include <sys/socket.h>

#define closesocket close
#endif
#include <stdio.h>

#define D_PORT 4344
//#define D_HOST "localhost"
#define D_HOST "DUBLIN-B4541FCD" // receive incoming connections from other computers
#define D_QUEUE 32
#define D_SOCKETS 16
#define D_INFO 256

int main(int argc, char **argv)
{
	struct timeval tv;
	struct sockaddr_in addr;
	struct hostent *host;
	unsigned int descriptor;
	int result;
	int index;
	int cycle = 0;
	int delay = 0;
	unsigned int sockets[D_SOCKETS];
	int sockets_index = 0;
	unsigned int maximun;
	char buffer[D_INFO];
	fd_set input;
	
	// My index
	int n;
	unsigned char *a; // Used for printing IP addresses
	
	/* read the delay if any */
	if (argc > 1)
	delay = atol(argv[1]);
	else
	delay = 0;
	
	#ifdef WIN32
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
	#endif /* WIN32 */
	
	/* create a socket */
	descriptor = socket(PF_INET, SOCK_STREAM, 0);
	if (descriptor == -1)
	{
		perror("socket");
		return (1);
	}
	
	/* get information about the host */
	memset(&addr, 0, sizeof(addr));
	host = gethostbyname(D_HOST);
	if (host == NULL)
	{
		perror("gethostbyname");
		closesocket(descriptor);
		#ifdef WIN32
		WSACleanup();
		#endif
		return (1);
	}
	
	printf("h_length = %d\n", host->h_length);
	a = (unsigned char *)host->h_addr_list[0];
	for (n=0 ; n<1 ; n++)
	{
		printf("Host address %d: %u.%u.%u.%u\n", n, a[0], a[1], a[2], a[3]);
	}
	
	/* bind the socket to an address and port */
	memcpy(&addr.sin_addr, host->h_addr_list[0], sizeof(host->h_addr_list[0]));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(D_PORT);
	result = bind(descriptor, (struct sockaddr *)&addr, sizeof(addr));
	if (result == -1)
	{
		perror("bind");
		closesocket(descriptor);
		#ifdef WIN32
		WSACleanup();
		#endif
		return (1);
	}
	
	/* listen for connections */
	result = listen(descriptor, D_QUEUE);
	if (result == -1)
	{
		perror("listen");
		closesocket(descriptor);
		#ifdef WIN32
		WSACleanup();
		#endif
		return (1);
	}
	
	memset(sockets, 0, sizeof(sockets));
	maximun = descriptor;
	
	result = 0;
	while (result != -1)
	{
		FD_ZERO(&input);
		FD_SET(descriptor, &input);
		for (result = 0; result < sockets_index; result++)
		FD_SET(sockets[result], &input);
		
		tv.tv_sec = delay;
		tv.tv_usec = 0;
		if (delay == -1)
		result = select(maximun + 1, &input, NULL, NULL, NULL);
		else
		result = select(maximun + 1, &input, NULL, NULL, &tv);
		switch (result)
		{
			/* error in select */
		case -1:
			perror("select");
			break;
			
			/* nothing to process */
		case 0:
			break;
			
			/* a number of sockets are ready for reading */
		default:
			/* check if the descriptor set is our listening one */
			if (FD_ISSET(descriptor , &input))
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
			/* one of the sockets is sending data. Find it */
			else
			{
				for (index = 0; index < sockets_index; index++)
				{
					if (FD_ISSET(sockets[index], &input))
					{
						memset(buffer, 0, sizeof(buffer));
						
						/* read information from socket */
						result = recv(sockets[index], buffer, sizeof(buffer), 0);
						if (result == -1)
						perror("recv");
						else
						printf("Received %d bytes from descriptor %d: %s\n", result, sockets[index], buffer);
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
	
	closesocket(descriptor);
	#ifdef WIN32
	WSACleanup();
	#endif
	
	printf("Exiting\n");
	
	return (0);
}
