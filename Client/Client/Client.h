#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <iostream>
#include <memory>
#include <thread>
#include <string>
#include <vector>
#include <mutex>
#include <xaudio2.h>

using namespace std;

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Xaudio2.lib")
#pragma once

class Client
{
public:
	Client(char* hName, char* pAdd);
	~Client();
	SOCKET getSocket();
	void ConnectToServer();

private:
	SOCKET connectionSocket = INVALID_SOCKET;
	char* serverHostName;
	char* serverPortAddress;
	void Receive(WORD* messageSize);
	void Receive(DWORD* audioSize);
	void Receive(char* readBuffer, WORD messageSize);
	void Receive(BYTE * buffer, WORD messageSize);
	void Receive(WAVEFORMATEXTENSIBLE * wfx);
	void Send(const char * writeBuffer, int writeLength);
	IXAudio2 * audioEngine = nullptr;
	IXAudio2MasteringVoice* masteringVoice = nullptr;
	IXAudio2SourceVoice* sourceVoice = nullptr;
	vector<string> sounds;
	bool exit = FALSE;
	bool choice = FALSE;
	bool inputControls = FALSE;
	bool playControls = FALSE;
	string soundNum;

	HRESULT Initialise();
	SOCKET CreateSocket();
	void Service();
	char* Receive();
	char* Send();
	void Input();
};

