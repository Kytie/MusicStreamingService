#include "CLient.h"

#define HOST_NAME "localhost"
#define HOST_PORT "8081" //if no connection is made try a different port.

int main(void)
{
	Client client(HOST_NAME, HOST_PORT);
	if (client.getSocket() != INVALID_SOCKET)
	{
		client.ConnectToServer();
	}
}
