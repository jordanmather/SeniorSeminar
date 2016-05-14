// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "AllowWindowsPlatformTypes.h"
#include <WinSock2.h>
#include "HideWindowsPlatformTypes.h"

class AMyPawn;

class SEMPROJECT_API NetworkForController : public FRunnable
{
public:
	NetworkForController();
	virtual ~NetworkForController();

private:
	SOCKET connectSocket;							//the socket to listen for a connection
	SOCKET connectedSocket;							//The socket that is connected
	FThreadSafeCounter stopTaskCounter;				//Stop thread with this counter
	static NetworkForController* controller;		//Static singleton with access to this thread
	FRunnableThread* thread;						//The running thread
	bool isFinished;


private:
	float joystickXAxis, joystickYAxis;
	float accelXAxis, accelYAxis, accelZAxis;
	float gravityXAxis, gravityYAxis, gravityZAxis;

public: //getters
	float getJoystickXAxis();
	float getJoystickYAxis();
	float getAccelXAxis();
	float getAccelYAxis();
	float getAccelZAxis();

public:
	virtual bool Init();
	virtual uint32 Run();
	virtual void Stop();

public:
	void CleanupWinsock();
	void SetServerSockAddr(sockaddr_in *pSockAddr, int portNumber);
	void HandleConnection(SOCKET connectedSocket);
	void readData(FString *Fs);
	void EnsureCompletion();						//Make sure the thread shutsdown
	static NetworkForController* joyInit();			//static init method
	static void shutdownThread();					//static kill thread
	static bool IsThreadFinished();

};
