#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <iostream>


using namespace std;

#pragma comment(lib, "ws2_32.lib")
#pragma once

class Server
{
public:
	Server(char *hName, char *sAdd);
	~Server();
	void ServerListen();
	SOCKET getSocket();

private:
	SOCKET serverSocket = INVALID_SOCKET;
	char* host_name;
	char* port_address;


	SOCKET CreateSocket();
};