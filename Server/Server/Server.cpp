#include "Server.h"

Server::Server(char *hName, char *pAdd)
{
	host_name = hName;
	port_address = pAdd;
	serverSocket = CreateSocket();
}

Server::~Server()
{
	closesocket(serverSocket);
	serverSocket = INVALID_SOCKET;
	WSACleanup();
}

SOCKET Server::getSocket()
{
	return serverSocket;
}



SOCKET Server::CreateSocket()
{
	#define WINSOCK_MINOR_VERSION	2
	#define WINSOCK_MAJOR_VERSION	2
	WSADATA wsaData;
	struct addrinfo * data;
	struct addrinfo hints;
	int status = 0;
	SOCKET tempSock;

	status = WSAStartup(MAKEWORD(WINSOCK_MAJOR_VERSION, WINSOCK_MINOR_VERSION), &wsaData);
	if (status != 0)
	{
		return tempSock = INVALID_SOCKET;
	}

	// Resolve the server address and port
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_IP;
	hints.ai_flags = AI_PASSIVE;
	if (getaddrinfo(host_name, port_address, &hints, &data))
	{
		return tempSock = INVALID_SOCKET;
	}
	// Create the server socket
	tempSock = socket(data->ai_family, data->ai_socktype, data->ai_protocol);
	if (tempSock == INVALID_SOCKET)
	{
		freeaddrinfo(data);
		return tempSock = INVALID_SOCKET;
	}
	// Setup for listening on this socket
	status = ::bind(tempSock, data->ai_addr, static_cast<int>(data->ai_addrlen));
	if (status == SOCKET_ERROR)
	{
		closesocket(tempSock);
		freeaddrinfo(data);
		return tempSock = INVALID_SOCKET;
	}
	freeaddrinfo(data);
	// Start listening
	status = listen(tempSock, SOMAXCONN);
	if (status == SOCKET_ERROR)
	{
		closesocket(tempSock);
		return tempSock = INVALID_SOCKET;
	}

	return tempSock;
}




