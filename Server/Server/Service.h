#include <WinSock2.h>
#include <memory>
#include <thread>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <xaudio2.h>
#include <mutex> 
using namespace std;

#pragma comment(lib, "ws2_32.lib")

#define fourccRIFF MAKEFOURCC('R', 'I', 'F', 'F')
#define fourccDATA MAKEFOURCC('d', 'a', 't', 'a')
#define fourccFMT  MAKEFOURCC('f', 'm', 't', ' ')
#define fourccWAVE MAKEFOURCC('W', 'A', 'V', 'E')
#define fourccXWMA MAKEFOURCC('X', 'W', 'M', 'A')

#pragma once
class Service
{
public:
	Service(SOCKET cSocket);
	~Service();
	void Stream();

private:
	SOCKET clientSocket = INVALID_SOCKET;
	vector<string> fileLocations;
	vector<string> fileNames;

	void Receive(WORD* messageSize);
	void Receive(char* buffer, WORD messageSize);
	void Send(const char * writeBuffer, int writeLength);
	void FindSoundFiles();
	HRESULT ReadChunkData(HANDLE fileHandle, void * buffer, DWORD buffersize, DWORD bufferoffset);
	HRESULT FindChunk(HANDLE fileHandle, FOURCC fourcc, DWORD & chunkSize, DWORD & chunkDataPosition);
};

