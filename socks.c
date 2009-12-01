//
// socks.c - Ted Burke - 27-9-2009
//
// This file contains the network communication thread function for RoboSimo.
//

#include <Winsock2.h>
#include <stdio.h>

#include "shared.h"

#define D_PORT 4009
#define D_QUEUE 32
#define D_SOCKETS 16
#define D_INFO 256

// This string will be set to the network address
// info to be displayed on the screen.
char network_address_display_string[100] = "Checking network address...";

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
	char recv_buffer[D_INFO];
	unsigned char send_buffer[12];
	fd_set input;
		
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
	
	descriptor = socket(PF_INET, SOCK_STREAM, 0); // create a socket
	
	// get information about the host
	char hostname[50];
	result = gethostname(hostname, 50);
	memset(&addr, 0, sizeof(addr));
	host = gethostbyname(hostname); // NULL if error
	
	// Create a string that will be used to display the
	// computer's network address info on the screen.
	unsigned char *a; // Used for printing IP addresses
	a = (unsigned char *)host->h_addr_list[0];
	sprintf(network_address_display_string, "  IP Address: %u.%u.%u.%u, Port: %d", a[0], a[1], a[2], a[3], D_PORT);
	
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
						memset(recv_buffer, 0, sizeof(recv_buffer));
						
						// read information from socket
						result = recv(sockets[index], recv_buffer, sizeof(recv_buffer), 0);
						if (result == -1)
						{
							perror("recv");
						}
						else
						{
							//printf("Received %d bytes from descriptor %d: %s\n", result, sockets[index], buffer);
							/*LATA = recv_buffer[0];
							LATB = recv_buffer[1];
							LATC = recv_buffer[2];
							LATD = recv_buffer[3];
							CCPR1L = recv_buffer[4];
							CCPR2L = recv_buffer[5];
							ADRESH = recv_buffer[6];*/
							robot[0].v1 = ((recv_buffer[3] | 0x02) - (recv_buffer[3] | 0x01)) *
												(recv_buffer[4]/255.0);
							robot[0].v2 = ((recv_buffer[3] | 0x08) - (recv_buffer[3] | 0x04)) *
												(recv_buffer[5]/255.0);
							
							/*
							if (recv_buffer[0] == 27)
							{
								// An arrow key was pressed.
								// These keys generate a 3-byte code.
								switch(recv_buffer[2])
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
								switch(recv_buffer[0])
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
							}*/
						}
						// Send sensor readings back to client
						send_buffer[0] = 0; // PORTA
						send_buffer[1] = 0; // PORTB
						send_buffer[2] = 0; // PORTC
						send_buffer[3] = 0; // PORTD
						send_buffer[4] = 127; // AN0
						send_buffer[5] = 255; // AN1
						send_buffer[6] = 0; // AN2
						send_buffer[7] = 0; // AN3
						send_buffer[8] = 0; // AN4
						send_buffer[9] = 0; // AN5
						send_buffer[10] = 0; // AN6
						send_buffer[11] = 0; // AN7
						result = send(sockets[index], send_buffer, sizeof(send_buffer), 0);
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
	
	return (0);
}
