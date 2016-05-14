// Fill out your copyright notice in the Description page of Project Settings.

/*
This class runs as a separate thread and governs the socket for receiving and parsing
data from the android app.
Uses a UDP socket on port 4444 to receive data
NOTE: The thread does not close on its own if the UE4 project closes.
for now, the android app MUST initiate the disconnect in order for the program to end cleanly.
*/

#include "SemProject.h"
#include "NetworkForController.h"	

NetworkForController* NetworkForController::controller = NULL;
const int REQ_WINSOCK_VER = 2;
const int DEFAULT_PORT = 4444;
const int TEMP_BUFFER_SIZE = 1024;



NetworkForController::NetworkForController()
{
	UE_LOG(LogTemp, Warning, TEXT("NetworkForController: ENTERED THE CONSTRUCTOR"));
	joystickXAxis = 0;
	joystickYAxis = 0;
	isFinished = false;
	accelXAxis = 0;
	accelYAxis = 0;
	accelZAxis = 0;
	gravityXAxis = 0;
	gravityYAxis = 0;
	gravityZAxis = 0;
	thread = FRunnableThread::Create(this, TEXT("NetworkForController"), 0, TPri_BelowNormal);
}

NetworkForController::~NetworkForController()
{
	delete thread;
	thread = NULL;
}

//Init the thread
bool NetworkForController::Init()
{
	bool iRet = false;
	WSADATA wsaData;
	UE_LOG(LogTemp, Warning, TEXT("NetworkForController: Init Winsock"));
	if (WSAStartup(MAKEWORD(REQ_WINSOCK_VER, 0), &wsaData) == 0)
	{
		if (LOBYTE(wsaData.wVersion) >= REQ_WINSOCK_VER)
		{
			UE_LOG(LogTemp, Warning, TEXT("NetworkForController: Initialized"));
			iRet = true;
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("NetworkForController: Required Winsock Version not supported"))
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("NetworkForController: Startup failed"));
	}
	return iRet;
}

//Run the thread 
uint32 NetworkForController::Run() {

	connectSocket = INVALID_SOCKET;
	connectedSocket = INVALID_SOCKET;
	sockaddr_in sockAddr = { 0 };

	//create the socket
	UE_LOG(LogTemp, Warning, TEXT("NetworkForController: creating socket"));
	connectSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (connectSocket == INVALID_SOCKET)
	{
		UE_LOG(LogTemp, Error, TEXT("NetworkForController: socket creation failed"));
		CleanupWinsock();
		return 1;
	}
	UE_LOG(LogTemp, Warning, TEXT("NetworkForController: Created"));
	UE_LOG(LogTemp, Warning, TEXT("NetworkForController: Binding Socket"));
	SetServerSockAddr(&sockAddr, DEFAULT_PORT);

	//bind the socket
	if (bind(connectSocket, (struct sockaddr *)&sockAddr, sizeof(sockAddr)) != 0)
	{
		UE_LOG(LogTemp, Error, TEXT("NetworkForController: could not bind socket"));
		CleanupWinsock();
		return 1;
	}
	UE_LOG(LogTemp, Warning, TEXT("NetworkForController: bound"));


	HandleConnection(connectSocket);
	UE_LOG(LogTemp, Warning, TEXT("NetworkForController: HandleConnection Finished"));
	return 0;
}

/*
connectedSocket: the socket that we will receive data on
This method continually listens for incoming data on connectedSocket
It then calls readData and passes it the data received
*/
void NetworkForController::HandleConnection(SOCKET connectedSocket)
{

	UE_LOG(LogTemp, Warning, TEXT("NetworkForController: Entered HandleConnection"));
	char tempBuffer[TEMP_BUFFER_SIZE];
	int slen;
	struct sockaddr_in si_other;
	slen = sizeof(si_other);

	while (!isFinished)
	{
		memset(tempBuffer, '\0', TEMP_BUFFER_SIZE);
		int retval = recvfrom(connectedSocket, tempBuffer, TEMP_BUFFER_SIZE, 0, (struct sockaddr *) &si_other, &slen);
		if (retval == 0)
		{
			break; //connection has been closed
		}
		else if (retval == SOCKET_ERROR)
		{
			UE_LOG(LogTemp, Error, TEXT("socket error while receiving"));
		}
		else
		{
			FString Fs = FString(UTF8_TO_TCHAR(tempBuffer));
			readData(&Fs);
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("NetworkForController: Socket closed"));
	isFinished = true;
}

/*
Fs: the data to read
This method takes an FString and parses it to the correct variable
The data is sent from the android app in the format [double device, double xAxis, double yAxis, double zAxis]
device values:
	0: joystick
	1: linear accelerometer
	2: gravity
	3: close connection
*/
void NetworkForController::readData(FString *Fs)
{
	FString passedString = *Fs;
	//UE_LOG(LogTemp, Warning, TEXT("%s"), *passedString);
	FString leftString;
	FString rightString;
	passedString.Split(",", &leftString, &rightString, ESearchCase::IgnoreCase, ESearchDir::FromStart);
	if (leftString.Equals("0.0"))
	{
		rightString.Split(",", &leftString, &rightString, ESearchCase::IgnoreCase, ESearchDir::FromStart);
		joystickXAxis = FCString::Atof(*leftString) / 10; //divide to create a number between -1 and 1
		rightString.Split(",", &leftString, &rightString, ESearchCase::IgnoreCase, ESearchDir::FromStart);
		joystickYAxis = FCString::Atof(*leftString) / 10; //divide to create a number between -1 and 1
	}
	else if (leftString.Equals("1.0"))
	{
		rightString.Split(",", &leftString, &rightString, ESearchCase::IgnoreCase, ESearchDir::FromStart);
		accelXAxis = FCString::Atof(*leftString);
		rightString.Split(",", &leftString, &rightString, ESearchCase::IgnoreCase, ESearchDir::FromStart);
		accelYAxis = FCString::Atof(*leftString);
		rightString.Split(";", &leftString, &rightString, ESearchCase::IgnoreCase, ESearchDir::FromStart);
		accelZAxis = FCString::Atof(*leftString);
	}
	else if (leftString.Equals("2.0"))
	{
		rightString.Split(",", &leftString, &rightString, ESearchCase::IgnoreCase, ESearchDir::FromStart);
		gravityXAxis = FCString::Atof(*leftString);
		rightString.Split(",", &leftString, &rightString, ESearchCase::IgnoreCase, ESearchDir::FromStart);
		gravityYAxis = FCString::Atof(*leftString);
		rightString.Split(";", &leftString, &rightString, ESearchCase::IgnoreCase, ESearchDir::FromStart);
		gravityZAxis = FCString::Atof(*leftString);
	}
	else if (leftString.Equals("3.0"))
	{
		//end the loop, we're closing down
		isFinished = true;
	}
	else
	{
		//leftString has an error
		//uncomment below lines to print out the string that gave an error
		//UE_LOG(LogTemp, Warning, TEXT("NetworkForController::Print leftString has an error"));
		//UE_LOG(LogTemp, Warning, TEXT("NetworkForController::Print %s"), *leftString);

	}

}

/*
A collection of getters for the joystick and linear accelerometer values
*/
float NetworkForController::getJoystickXAxis()
{
	return controller->joystickXAxis;
}

float NetworkForController::getJoystickYAxis()
{
	return controller->joystickYAxis;
}
float NetworkForController::getAccelXAxis()
{
	return controller->accelXAxis;
}
float NetworkForController::getAccelYAxis()
{
	return controller->accelYAxis;
}
float NetworkForController::getAccelZAxis()
{
	return controller->accelZAxis;
}




void NetworkForController::SetServerSockAddr(sockaddr_in *pSockAddr, int portNumber)
{
	//set family, port and find IP
	pSockAddr->sin_family = AF_INET;
	pSockAddr->sin_port = htons(portNumber);
	pSockAddr->sin_addr.s_addr = INADDR_ANY;
}

//Do a joy initialize of the thread
NetworkForController* NetworkForController::joyInit() {

	if (!controller && FPlatformProcess::SupportsMultithreading()) {

		controller = new NetworkForController();

	}
	return controller;
}

//Stop the thread
void NetworkForController::Stop() {

	stopTaskCounter.Increment();

}

//Ensure that the thread stop
void NetworkForController::EnsureCompletion() {

	closesocket(connectSocket);
	closesocket(connectedSocket);
	Stop();
	thread->WaitForCompletion();

}

void NetworkForController::CleanupWinsock()
{
	UE_LOG(LogTemp, Warning, TEXT("NetworkForController: Cleaning up winsock"));
	//clean winsock
	if (WSACleanup != 0)
	{
		UE_LOG(LogTemp, Error, TEXT("NetworkForController: Cleanup Failed"));
		//TODO: handle the cleanup error somehow
	}
	UE_LOG(LogTemp, Warning, TEXT("NetworkForController: Done"));
}

//Shut the thread down
void NetworkForController::shutdownThread() {

	if (controller) {

		controller->EnsureCompletion();
		delete controller;
		controller = NULL;

	}
}

bool NetworkForController::IsThreadFinished()
{
	if (controller) return controller->isFinished;
	return true;
}