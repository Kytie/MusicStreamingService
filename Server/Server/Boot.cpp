#include "Server.h"
#include "Service.h"
#include <thread>

#define HOST_NAME "localhost"
#define HOST_PORT "8081" //if no connection is made try a different port.

void ServiceBoot(SOCKET clientSocket)
{
	if (clientSocket != INVALID_SOCKET)
	{
		Service service(clientSocket);
		thread(&Service::Stream, &service).join();
	}
	else
	{
		cout << "Could not connect to client" << endl;
	}
}

int main(void)
{
	Server server(HOST_NAME, HOST_PORT);
	if (server.getSocket() != INVALID_SOCKET)
	{
		
		while (1)
		{
			SOCKET clientSocket = accept(server.getSocket(), nullptr, nullptr);
			thread(ServiceBoot, clientSocket).detach();
		}
	}
	return 0;
}
