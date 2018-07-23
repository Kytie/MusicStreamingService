#include "Client.h"

class VoiceCallback : public IXAudio2VoiceCallback
{
public:
	HANDLE bufferEndEventHandle;
	VoiceCallback() : bufferEndEventHandle(CreateEvent(NULL, FALSE, FALSE, NULL)){}
	~VoiceCallback()
	{
		CloseHandle(bufferEndEventHandle);
	}

	STDMETHOD_(void, OnStreamEnd()){}

	// Unused methods in this application
	STDMETHOD_(void, OnVoiceProcessingPassEnd()) { }
	STDMETHOD_(void, OnVoiceProcessingPassStart(UINT32 SamplesRequired)) {    }
	STDMETHOD_(void, OnBufferEnd(void*))
	{
		SetEvent(bufferEndEventHandle);
	}
	STDMETHOD_(void, OnBufferStart(void * pBufferContext)) {    }
	STDMETHOD_(void, OnLoopEnd(void * pBufferContext)) {    }
	STDMETHOD_(void, OnVoiceError(void * pBufferContext, HRESULT Error)) { }
};

// Initialise XAudio2 engine and master voice

HRESULT Client::Initialise()
{
	HRESULT hr;

	CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (FAILED(hr = XAudio2Create(&audioEngine, 0, XAUDIO2_DEFAULT_PROCESSOR)))
	{
		return hr;
	}
	if (FAILED(hr = audioEngine->CreateMasteringVoice(&masteringVoice)))
	{
		return hr;
	}
	return S_OK;
}

Client::Client(char* hName, char* pAdd)
{
	if (hName == "localhost")
	{
		serverHostName = "127.0.0.1";
	}
	else
	{
		serverHostName = hName;
	}
	serverPortAddress = pAdd;
	connectionSocket = CreateSocket();
}


Client::~Client()
{
	closesocket(connectionSocket);
	connectionSocket = INVALID_SOCKET;
	WSACleanup();
}

SOCKET Client::getSocket()
{
	return connectionSocket;
}

SOCKET Client::CreateSocket()
{
	#define WINSOCK_MINOR_VERSION	2
	#define WINSOCK_MAJOR_VERSION	2
	SOCKET tempSock;
	WSADATA wsaData;
	int status = 0;

	status = WSAStartup(MAKEWORD(WINSOCK_MAJOR_VERSION, WINSOCK_MINOR_VERSION), &wsaData); // do I have to check this again?
	if (status != 0)
	{
		cout << "Unable to start sockets layer" << endl;
		return tempSock = INVALID_SOCKET;
	}

	//create a socket for the client.
	if ((tempSock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
	{
		cout << "could not create client socket";
		return tempSock = INVALID_SOCKET;
	}

	return tempSock;
}

void Client::ConnectToServer()
{
	struct sockaddr_in server;
	inet_pton(AF_INET, serverHostName, &(server.sin_addr));
	server.sin_family = AF_INET;
	server.sin_port = htons(atoi(serverPortAddress));
	if (connect(connectionSocket, (struct sockaddr *)&server, sizeof(server)) != 0)
	{
		cout << "connection to server failed" << endl;
		cin.ignore();
		return;
	}

	Service();
}

void Client::Service()
{
	HRESULT hr = S_OK;
	WORD messageSize = 0;
	char messageSend[sizeof(WORD)];
	char* readBuffer;
	DWORD audioSize = 0;
	WAVEFORMATEXTENSIBLE wfx = { 0 };
	VoiceCallback voiceCallback;
	XAUDIO2_VOICE_STATE state;
	int byteCount;
	size_t bytesStored = 0;
	int bufferIndex = 0;
	BYTE buffers[3][32768];
	XAUDIO2_BUFFER audioBuffer = { 0 };
	thread(&Client::Input, this).detach();

	if (Initialise() != S_OK)
	{
		cout << "Could not initialise.";
		return;
	}
	cout << "Hello and welcome to the music streaming service." << endl << endl
		<< "While playing a sound:" << endl
		<< "To pause enter \"pause\"." << endl
		<< "To play enter \"play\"." << endl << endl 
		<< "Please submit the number of a sound you would like to play." << endl << endl
		<< "Sounds:" << endl;

	Receive(&messageSize);
	int numberOfSounds = messageSize;
	for (int i = 0; i < numberOfSounds; i++)
	{
		Receive(&messageSize);
		readBuffer = new char[messageSize];
		Receive(readBuffer, messageSize);
		sounds.push_back(readBuffer);
		delete[] readBuffer;
	}

	for (size_t i = 0; i < sounds.size(); i++)
	{
		cout << sounds.at(i);
	}
	cout << "Command: ";

	while (1)
	{
		choice = TRUE;
		soundNum = "";
		while (soundNum == "")
		{
		}		
		
		messageSize = soundNum.size() + 1;
		memcpy(messageSend, &messageSize, sizeof(WORD));
		Send(messageSend, sizeof(messageSize));
		Send(soundNum.c_str(), messageSize);
		Receive(&audioSize);
		Send("RECIEVED", 9);
		Receive(&wfx);
		Send("RECIEVED", 9);

		if (FAILED(hr = audioEngine->CreateSourceVoice(&sourceVoice, (WAVEFORMATEX*)&wfx,
			0, XAUDIO2_DEFAULT_FREQ_RATIO,
			&voiceCallback, NULL, NULL
 			)))
		{
			audioBuffer.AudioBytes = 0;
		}

		if (FAILED(hr = sourceVoice->Start(0, 0)))
		{
			audioBuffer.AudioBytes = 0;
		}

		playControls = TRUE;
		while (bytesStored < audioSize)
		{
			while (sourceVoice->GetState(&state), state.BuffersQueued >= 2)
			{
				WaitForSingleObject(voiceCallback.bufferEndEventHandle, INFINITE);
			}

			Receive(&messageSize);
			Send("RECIEVED", 9);
			Receive(&buffers[bufferIndex][0], messageSize);
			Send("RECIEVED", 9);
			audioBuffer.AudioBytes = messageSize;
			audioBuffer.pAudioData = buffers[bufferIndex];

			if (FAILED(hr = sourceVoice->SubmitSourceBuffer(&audioBuffer)))
			{
				audioBuffer.AudioBytes = 0;
				return;
			}

			bufferIndex++;

			bufferIndex %= 3;
			bytesStored += audioBuffer.AudioBytes;
		}

		while (sourceVoice->GetState(&state), state.BuffersQueued > 0)
		{
			WaitForSingleObject(voiceCallback.bufferEndEventHandle, INFINITE);
		}

		choice = FALSE;
		playControls = FALSE;
		exit = FALSE;
		bytesStored= 0;
		bufferIndex = 0;

		cout << endl << "Would you like to play another song? (yes/no)" << endl;
		while (!choice)
		{
		}
		if (exit)
		{
			messageSize = 5;
			memcpy(messageSend, &messageSize, sizeof(WORD));
			Send(messageSend, sizeof(messageSize));
			readBuffer = new char[5];
			strcpy_s(readBuffer, 5, "EXIT");
			Send(readBuffer, 5);
			delete[] readBuffer;
			return;
		}

		cout << endl;
		for (size_t i = 0; i < sounds.size(); i++)
		{
			cout << sounds.at(i);
		}
		cout << "Command: ";
		
	}
}

void Client::Input()
{
	string input;
	unsigned int index;
	
	while (!exit)
	{
		cin >> input;
		for (size_t i = 0; i < input.size(); i++)
		{
			input[i] = toupper(input[i]);
		}
		if (input == "PAUSE")
		{
			if (playControls == TRUE)
			{
				sourceVoice->Stop();
			}
			else
			{
				cout << "\nbad command\nCommand: ";
			}
		}
		else if (input == "PLAY")
		{
			if (playControls == TRUE)
			{
				sourceVoice->Start();
			}
			else
			{
				cout << "\nbad command\nCommand: ";
			}
		}
		else if (input == "NO" )
		{
			if (choice == FALSE)
			{
				exit = TRUE;
				choice = TRUE;
			}
			else
			{
				cout << "\nbad command\nCommand: ";
			}
		}
		else if (input == "YES")
		{
			if (choice == FALSE)
			{
				exit = FALSE;
				choice = TRUE;
			}
			else
			{
				cout << "\nbad command\nCommand: ";
			}
		}
		else
		{ 
			if (choice == TRUE && playControls == FALSE)
			{

				for (size_t i = 0; i < input.size(); i++)
				{
					if (isdigit(input[i]))
					{
					}
					else
					{
						input = "-1";
					}
				}
				index = atoi(input.c_str());
				if (index < sounds.size())
				{
					soundNum = input;
				}
				else
				{
					cout << "\nbad command\nCommand: ";
				}
			}
			else
			{
				cout << "\nbad command\nCommand: ";
			}
		}
	}
}

void Client::Receive(WORD* messageSize)
{
	recv(connectionSocket, (char*)messageSize, sizeof(WORD), 0);
}

void Client::Receive(DWORD* audioSize)
{
	recv(connectionSocket, (char*)audioSize, sizeof(DWORD), 0);
}

void Client::Receive(char* readBuffer, WORD messageSize)
{
	recv(connectionSocket, readBuffer, messageSize, 0);
}

void Client::Receive(BYTE * buffer, WORD messageSize)
{
	recv(connectionSocket, (char*)buffer, messageSize, 0);
}

void Client::Receive(WAVEFORMATEXTENSIBLE * wfx)
{
	recv(connectionSocket, (char*)wfx, sizeof(WAVEFORMATEXTENSIBLE), 0);
}

void Client::Send(const char * writeBuffer, int writeLength)
{
	int bytesSent = send(connectionSocket, writeBuffer, writeLength, 0);
}