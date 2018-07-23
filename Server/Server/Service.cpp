#include "Service.h"


Service::Service(SOCKET cSocket)
{
	clientSocket = cSocket;
	FindSoundFiles();
}


Service::~Service()
{
	closesocket(clientSocket);
	clientSocket = INVALID_SOCKET;
}

void Service::FindSoundFiles()
{
	ifstream infile;
	string temp;
	infile.open("..\\sound_files\\sounds.txt", ifstream::in);
	if (infile.is_open())
	{
		while (infile.good())
		{
			getline(infile, temp, ',');
			fileLocations.push_back(temp);
		}
		infile.close();
	}

	for (size_t i = 0; i < fileLocations.size(); i++)
	{
		fileNames.push_back(fileLocations.at(i).substr(15));
		fileNames.at(i).insert(0, to_string(i) + ".");
		fileNames.at(i).insert(fileNames.at(i).size(), "\n");
	}
}

void Service::Stream()
{
	if (fileNames.size() == 0)
	{
		return;
	}

	mutex mutex;
	string message = "";
	WAVEFORMATEXTENSIBLE wfx = { 0 };
	BYTE *dataBuffer = new BYTE[32768];
	DWORD chunkPosition;
	DWORD filetype;
	DWORD chunkSize;
	char *readBuffer = NULL;
	int fileNum;
	int wchars_num;
	HANDLE fileHandle;
	WORD messageSize;
	char messageSend[sizeof(WORD)];
	char audioSend[sizeof(DWORD)];
	size_t bytesSent;
	wchar_t* fileAddress;

	messageSize = fileNames.size();
	memcpy(messageSend, &messageSize, sizeof(WORD));
	Send(messageSend, sizeof(messageSize));

	for (size_t i = 0; i < fileNames.size(); i++)
	{
		message = fileNames.at(i).c_str();
		messageSize = message.size() + 1;
		memcpy(messageSend, &messageSize, sizeof(WORD));
		Send(messageSend, sizeof(messageSize));
		Send(message.c_str(), messageSize);
	}

	//sound streaming
	while (1)
	{
		Receive(&messageSize);
		readBuffer = new char[messageSize];
		Receive(readBuffer, messageSize);
		if (strcmp(readBuffer, "") == 0)
		{
			return;
		}
		else
		{
			if (strcmp(readBuffer, "EXIT") == 0)
			{
				delete[] dataBuffer;
				delete[] readBuffer;
				return;
			}
			fileNum = atoi(readBuffer);
			delete[] readBuffer; //delete readbuffer here as it is being modified later, avoid memory leaks.
			wchars_num = MultiByteToWideChar(CP_UTF8, 0, fileLocations.at(fileNum).c_str(), -1, NULL, 0);
			fileAddress = new wchar_t[wchars_num];
			MultiByteToWideChar(CP_UTF8, 0, fileLocations.at(fileNum).c_str(), -1, fileAddress, wchars_num);

			mutex.lock();
			fileHandle = CreateFile(fileAddress, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
			delete[] fileAddress; //delete fileAddress here as it is being modified later, avoid memory leaks.
			if (fileHandle == INVALID_HANDLE_VALUE)
			{
				return;
			}
			if (SetFilePointer(fileHandle, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
			{
				CloseHandle(fileHandle);
				return;
			}
			// Make sure we have a RIFF wave file
			FindChunk(fileHandle, fourccRIFF, chunkSize, chunkPosition);
			ReadChunkData(fileHandle, &filetype, sizeof(DWORD), chunkPosition);
			if (filetype != fourccWAVE)
			{
				CloseHandle(fileHandle);
				return;
			}
			// Locate the 'fmt ' chunk, and copy its contents into a WAVEFORMATEXTENSIBLE structure. 
			FindChunk(fileHandle, fourccFMT, chunkSize, chunkPosition);
			ReadChunkData(fileHandle, &wfx, chunkSize, chunkPosition);
			// Fill out the audio data buffer with the contents of the 'data' chunk
			FindChunk(fileHandle, fourccDATA, chunkSize, chunkPosition);
			mutex.unlock();

			memcpy(audioSend, &chunkSize, sizeof(DWORD));
			Send(audioSend, sizeof(DWORD));
			readBuffer = new char[9];
			Receive(readBuffer, 9);
			Send((char*)&wfx, sizeof(wfx));
			Receive(readBuffer, 9);

			bytesSent = 0;
			while (bytesSent < chunkSize)
			{
				if (bytesSent + 32768 > chunkSize)
				{					
					int sizeLeft = chunkSize - bytesSent;
					mutex.lock();
					ReadChunkData(fileHandle, dataBuffer, sizeLeft, chunkPosition);
					mutex.unlock();
					chunkPosition = 0;
					const char *data = reinterpret_cast<const char*>(dataBuffer);
					messageSize = sizeLeft;
					memcpy(messageSend, &messageSize, sizeof(WORD));
					Send(messageSend, sizeof(messageSize));
					Receive(readBuffer, 9);
					Send(data, sizeLeft);
					Receive(readBuffer, 9);
					bytesSent += sizeLeft;

				}
				else
				{
					mutex.lock();
					ReadChunkData(fileHandle, dataBuffer, 32768, chunkPosition);
					mutex.unlock();
					chunkPosition += 32768;
					const char *data = reinterpret_cast<const char*>(dataBuffer);
					messageSize = 32768;
					memcpy(messageSend, &messageSize, sizeof(WORD));
					Send(messageSend, sizeof(messageSize));
					Receive(readBuffer, 9);
					Send(data, 32768);
					Receive(readBuffer, 9);
					bytesSent += 32768;
				}
			}
			CloseHandle(fileHandle);
			delete[] readBuffer;
		}
	}
}

void Service::Receive(WORD* messageSize)
{
	recv(clientSocket, (char*)messageSize, sizeof(messageSize), 0);
}

void Service::Receive(char* readBuffer, WORD messageSize)
{
	recv(clientSocket, readBuffer, messageSize, 0);
}

void Service::Send(const char * writeBuffer, int writeLength)
{
	send(clientSocket, writeBuffer, writeLength, 0);
}

HRESULT Service::FindChunk(HANDLE fileHandle, FOURCC fourcc, DWORD & chunkSize, DWORD & chunkDataPosition)
{
	HRESULT hr = S_OK;
	DWORD chunkType;
	DWORD chunkDataSize;
	DWORD riffDataSize = 0;
	DWORD fileType;
	DWORD bytesRead = 0;
	DWORD offset = 0;

	if (SetFilePointer(fileHandle, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}
	while (hr == S_OK)
	{
		if (ReadFile(fileHandle, &chunkType, sizeof(DWORD), &bytesRead, NULL) == 0)
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
		}
		if (ReadFile(fileHandle, &chunkDataSize, sizeof(DWORD), &bytesRead, NULL) == 0)
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
		}
		switch (chunkType)
		{
		case fourccRIFF:
			riffDataSize = chunkDataSize;
			chunkDataSize = 4;
			if (ReadFile(fileHandle, &fileType, sizeof(DWORD), &bytesRead, NULL) == 0)
			{
				hr = HRESULT_FROM_WIN32(GetLastError());
			}
			break;

		default:
			if (SetFilePointer(fileHandle, chunkDataSize, NULL, FILE_CURRENT) == INVALID_SET_FILE_POINTER)
			{
				return HRESULT_FROM_WIN32(GetLastError());
			}
		}

		offset += sizeof(DWORD) * 2;
		if (chunkType == fourcc)
		{
			chunkSize = chunkDataSize;
			chunkDataPosition = offset;
			return S_OK;
		}

		offset += chunkDataSize;
		if (bytesRead >= riffDataSize)
		{
			return S_FALSE;
		}
	}
	return S_OK;
}

HRESULT Service::ReadChunkData(HANDLE fileHandle, void * buffer, DWORD buffersize, DWORD bufferoffset)
{
	HRESULT hr = S_OK;
	DWORD bytesRead;

	if (SetFilePointer(fileHandle, bufferoffset, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}
	if (ReadFile(fileHandle, buffer, buffersize, &bytesRead, NULL) == 0)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
	}
	return hr;
}
