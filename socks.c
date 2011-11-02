//
// socks.c - Ted Burke - 2-11-2011
//
// This file contains the network communication thread function for RoboSimo.
//

#define WINVER 0x0501

#include <windows.h>
#include <Winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#include "shared.h"

#define DEFAULT_PORT_1 "4009"
#define DEFAULT_PORT_2 "4010"

// This string will be set to the network address
// info to be displayed on the screen.
char network_address_display_string[100] = "Checking network address...";

DWORD WINAPI network_thread(LPVOID lpParameter)
{
	// Retrieve input parameter (indicates whether this thread for player 1 or 2)
	int player_number = *((int *)lpParameter);
	
	WSADATA wsaData;
	SOCKET ListenSocket = INVALID_SOCKET,
	ClientSocket = INVALID_SOCKET;
	struct addrinfo *result = NULL, hints;
	unsigned char recvbuf[6];
	unsigned char sendbuf[12];
	int iResult, iSendResult;
	int recvbuflen = 6;
	int sendbuflen = 12;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != 0)
	{
		printf("WSAStartup failed: %d\n", iResult);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	if (player_number == 1)
	{
		iResult = getaddrinfo(NULL, DEFAULT_PORT_1, &hints, &result);
	}
	else
	{
		iResult = getaddrinfo(NULL, DEFAULT_PORT_2, &hints, &result);
	}
	
	if (iResult != 0)
	{
		printf("getaddrinfo failed: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	// Set on screen network address string
	int i;
	char ac[80];
	gethostname(ac, sizeof(ac));
	struct hostent *phe = gethostbyname(ac);
	for (i = 0; phe->h_addr_list[i] != 0; ++i)
	{
		struct in_addr addr;
		memcpy(&addr, phe->h_addr_list[i], sizeof(struct in_addr));
		sprintf(network_address_display_string, "   IP Address: %s, Ports: %s, %s",
					inet_ntoa(addr), DEFAULT_PORT_1, DEFAULT_PORT_2);
	}
	
	// Create a SOCKET for connecting to server
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET)
	{
		printf("socket failed: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	// Set the socket to non-blocking mode
	u_long iMode = 1;
	iResult = ioctlsocket(ListenSocket, FIONBIO, &iMode);
	
	// Setup the TCP listening socket
	iResult = bind( ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR)
	{
		printf("bind failed: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	freeaddrinfo(result);

	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR)
	{
		printf("listen failed: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
	
	if (player_number == 1)
	{
		printf("Listening for client on port %s...\n", DEFAULT_PORT_1);
	}
	else
	{
		printf("Listening for client on port %s...\n", DEFAULT_PORT_2);
	}
	
	while(program_exiting == 0)
	{
		// Accept a client socket
		ClientSocket = accept(ListenSocket, NULL, NULL);
		if (ClientSocket == INVALID_SOCKET)
		{
			// Nobody is connecting yet
			Sleep(1);
			continue;
		}

		printf("A client has connected...\n");

		while(program_exiting == 0)
		{
			// Receive data
			int no_data_count = 0;
			iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
			if (iResult > 0)
			{
				no_data_count = 0; // reset no data counter
				robot[player_number - 1].LATA = recvbuf[0];
				robot[player_number - 1].LATB = recvbuf[1];
				robot[player_number - 1].LATC = recvbuf[2];
				robot[player_number - 1].LATD = recvbuf[3];
				robot[player_number - 1].CCPR1L = recvbuf[4];
				robot[player_number - 1].CCPR2L = recvbuf[5];
			}
			
			// Send the "sensor readings" back to the client
			sendbuf[0] = 0; // PORTA
			sendbuf[1] = 0; // PORTB
			sendbuf[2] = 0; // PORTC
			sendbuf[3] = 0; // PORTD
			sendbuf[4] = robot[player_number - 1].AN[0]; // AN0 - front left light sensor
			sendbuf[5] = robot[player_number - 1].AN[1]; // AN1 - front right light sensor
			sendbuf[6] = robot[player_number - 1].AN[2]; // AN2 - back left light sensor
			sendbuf[7] = robot[player_number - 1].AN[3]; // AN3 - back right light sensor
			sendbuf[8] = robot[player_number - 1].AN[4]; // AN4
			sendbuf[9] = robot[player_number - 1].AN[5]; // AN5
			sendbuf[10] = robot[player_number - 1].AN[6]; // AN6
			sendbuf[11] = robot[player_number - 1].AN[7]; // AN7
			iSendResult = send(ClientSocket, sendbuf, sendbuflen, 0);
			if (iSendResult == SOCKET_ERROR)
			{
				//printf("Send failed: %d\n", WSAGetLastError());
				break;
			}
			
			// Short pause to ensure that this thread doesn't
			// hog all the processor time
			Sleep(10);			
		}

		// Stop robot moving
		robot[player_number - 1].LATA = 0;
		robot[player_number - 1].LATB = 0;
		robot[player_number - 1].LATC = 0;
		robot[player_number - 1].LATD = 0;
		robot[player_number - 1].CCPR1L = 0;
		robot[player_number - 1].CCPR2L = 0;

		// Clean up
		closesocket(ClientSocket);
		fprintf(stderr, "Connection closed\n");
	}
	
	closesocket(ListenSocket);
	WSACleanup();

	return (0);
}
