/*
This file is the central link for the UE4 project.  It governs camera placement, starts connection with the
android app, starts the neural net, and records test cases for neural net training.
*/

#include "SemProject.h"
#include "MyPawn.h"
#include "NeuralNet.h"
#include "NetworkForController.h"
#include <sstream>
#include <iostream>
#include <fstream>

//this is the directory that the TestData recorded through the use of the 'r' key will be stored.
//The filename will be "TestData.txt" and is set in the SaveData() method at the end of this file.
FString SaveDirectory = FString("c:/Users/Rorc/Dropbox/School/Senior Year/CSC 491/SemProject/SemProject/Source/SemProject");

AMyPawn::AMyPawn()
{
	// Sets default values
	xAxis = 0;
	yAxis = 0;
	accelXAxis = 0;
	accelYAxis = 0;
	accelZAxis = 0;
	iterations = 0;
	count = 0;
	circleCount = 0;
	dataConfirmed = 0; //1 is our accept data signal, 2 is our reject data signal (set in AMyPawn::confirm())
	numNets = 1;
	startsIn = 3;
	countdown = true;
	collectData = false;
	confirmData = false;
	writeData = false;
	record = false;
	circleCooldown = false;
	

	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	AutoPossessPlayer = EAutoReceiveInput::Player0;

	//Create a dummy root component we can attach things to
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	//Create a camera and a visible object
	UCameraComponent* OurCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("OurCamera"));
	OurVisibleComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("OurVisibleComponent"));
	//Attach our camera and visible object to our root component.  Offset and rotate camera. (3rd person)
	OurCamera->AttachTo(RootComponent);
	OurCamera->SetRelativeLocation(FVector(-300.0f, 0.0f, 500.0f));
	OurCamera->SetRelativeRotation(FRotator(-45.0f, 0.0f, 0.0f));
	OurVisibleComponent->AttachTo(RootComponent);


}

// Called when the game starts or when spawned
void AMyPawn::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogTemp, Warning, TEXT("MyPawn: Calling joyInit"));
	controller = NetworkForController::joyInit();
	UE_LOG(LogTemp, Warning, TEXT("MyPawn: Called joyInit"));

	//create the NeuralNet
	UE_LOG(LogTemp, Warning, TEXT("Creating NeuralNet"));
	net = net->joyInit();
	UE_LOG(LogTemp, Warning, TEXT("Created NeuralNet"));

	//Set a timer to monitor the thread's active/inactive state
	GetWorld()->GetTimerManager().SetTimer(networkTimerHandle, this, &AMyPawn::NetworkClosed, 1.0f, true);
	
	//set timers to record testData for neuralNetwork testing.
	//in-game, this funcitonality is activated with the "r" key
	GetWorld()->GetTimerManager().SetTimer(skillTimerHandle, this, &AMyPawn::collectAccelData, 0.01f, true);
	GetWorld()->GetTimerManager().SetTimer(startCollectionTimerHandle, this, &AMyPawn::timerToCollect, 1.0f, true);
	GetWorld()->GetTimerManager().SetTimer(confirmTimerHandle, this, &AMyPawn::Confirm, 1.0f, true);
	GetWorld()->GetTimerManager().SetTimer(saveDataTimerHandle, this, &AMyPawn::SaveData, 1.0f, true);
	//Pause these timers until we're ready to record
	initialRecordingPaused();
	
	//set timers to monitor circles while game is running
	GetWorld()->GetTimerManager().SetTimer(setupNeuralNetsHandle, this, &AMyPawn::setupNeuralNets, 0.25f, true, 4.0f);
	//set a cooldown on the circle action, since the net recognizes a wide range of variations as circles
	GetWorld()->GetTimerManager().SetTimer(circleCooldownHandle, this, &AMyPawn::circleCooldownReset, 4.0f, true);


}

// Called every frame
void AMyPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//Handle growing and shrinking based on our "Grow" action
	{
		float CurrentScale = OurVisibleComponent->GetComponentScale().X;
		if (bGrowing)
		{
			//Grow to double size over the course of one second
			CurrentScale += DeltaTime;
		}
		else
		{
			//shrink half as fast as we grow
			CurrentScale -= (0.5f * DeltaTime);
		}
		//Make sure we never shrink past our original size, or grow more than double
		CurrentScale = FMath::Clamp(CurrentScale, 1.0f, 2.0f);
		OurVisibleComponent->SetWorldScale3D(FVector(CurrentScale));
	}
	//update accelerometer data
	if (controller)
	{
		accelXAxis = controller->getAccelXAxis();
		accelYAxis = controller->getAccelYAxis();
		accelZAxis = controller->getAccelZAxis();
	}
	//Handle movement based on MoveX and MoveY Axes
	{
		if (controller)
		{
			xAxis = controller->getJoystickXAxis();
			yAxis = controller->getJoystickYAxis();
		}

		CurrentVelocity.X = FMath::Clamp(xAxis, -1.0f, 1.0f) * 100.0f;
		CurrentVelocity.Y = FMath::Clamp(yAxis, -1.0f, 1.0f) * 100.0f;
		if (!CurrentVelocity.IsZero())
		{
			FVector NewLocation = GetActorLocation() + (CurrentVelocity * DeltaTime);
			SetActorLocation(NewLocation);
		}
	}
}



// Called to bind functionality to input
void AMyPawn::SetupPlayerInputComponent(class UInputComponent* InputComponent)
{
	Super::SetupPlayerInputComponent(InputComponent);
	//Respond when our "Grow" key is pressed and released
	InputComponent->BindAction("Grow", IE_Pressed, this, &AMyPawn::StartGrowing);
	InputComponent->BindAction("Grow", IE_Released, this, &AMyPawn::StopGrowing);

	//Respond every frame to the two movement axes, "MoveX" and "MoveY"
	InputComponent->BindAxis("MoveX", this, &AMyPawn::Move_XAxis);
	InputComponent->BindAxis("MoveY", this, &AMyPawn::Move_YAxis);

	//Setup a "reject" key
	InputComponent->BindAction("Accept", IE_Pressed, this, &AMyPawn::Accept);
	InputComponent->BindAction("Reject", IE_Pressed, this, &AMyPawn::Reject);

	//setup the record key
	InputComponent->BindAction("Record", IE_Pressed, this, &AMyPawn::StartRecording);
}

void AMyPawn::Move_XAxis(float AxisValue)
{
	//Move 100 units per second forward or backward
	CurrentVelocity.X = FMath::Clamp(AxisValue, -1.0f, 1.0f) * 100.0f;
}

void AMyPawn::Move_YAxis(float AxisValue)
{
	//Move 100 units per second left or right
	CurrentVelocity.Y = FMath::Clamp(AxisValue, -1.0f, 1.0f) * 100.0f;
}

void AMyPawn::StartGrowing()
{
	bGrowing = true;
	GetWorld()->GetTimerManager().SetTimer(stopGrowingHandle, this, &AMyPawn::StopGrowing, 2.0f, false, 2.0f);
}

void AMyPawn::StopGrowing()
{
	bGrowing = false;
	GetWorld()->GetTimerManager().ClearTimer(stopGrowingHandle);
}

void AMyPawn::Accept()
{
	if (confirmData)
	{
		dataConfirmed = 1;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("not time to confirm/deny data"));
	}
}

void AMyPawn::Reject()
{
	if (confirmData)
	{
		dataConfirmed = 2;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("not time to confirm/deny data"));
	}
}

void AMyPawn::initialRecordingPaused()
{
	GetWorld()->GetTimerManager().PauseTimer(skillTimerHandle);
	GetWorld()->GetTimerManager().PauseTimer(startCollectionTimerHandle);
	GetWorld()->GetTimerManager().PauseTimer(confirmTimerHandle);
	GetWorld()->GetTimerManager().PauseTimer(saveDataTimerHandle);
}

void AMyPawn::StartRecording()
{
	if (record)
	{
		record = false;

		//pause the recording timers
		initialRecordingPaused();

		//unpause the NeuralNetwork Timers
		GetWorld()->GetTimerManager().UnPauseTimer(firstNeuralNetHandle);
		GetWorld()->GetTimerManager().UnPauseTimer(secondNeuralNetHandle);
		GetWorld()->GetTimerManager().UnPauseTimer(thirdNeuralNetHandle);
		GetWorld()->GetTimerManager().UnPauseTimer(fourthNeuralNetHandle);
		GetWorld()->GetTimerManager().UnPauseTimer(fifthNeuralNetHandle);
		GetWorld()->GetTimerManager().UnPauseTimer(sixthNeuralNetHandle);
		GetWorld()->GetTimerManager().UnPauseTimer(seventhNeuralNetHandle);
		GetWorld()->GetTimerManager().UnPauseTimer(eighthNeuralNetHandle);

		UE_LOG(LogTemp, Warning, TEXT("****STOP RECORDING****"));
	}
	else
	{
		record = true;
		//unpause the recording timers
		GetWorld()->GetTimerManager().UnPauseTimer(skillTimerHandle);
		GetWorld()->GetTimerManager().UnPauseTimer(startCollectionTimerHandle);
		GetWorld()->GetTimerManager().UnPauseTimer(confirmTimerHandle);
		GetWorld()->GetTimerManager().UnPauseTimer(saveDataTimerHandle);

		//pause the NeuralNetwork Timers while we record
		GetWorld()->GetTimerManager().PauseTimer(firstNeuralNetHandle);
		GetWorld()->GetTimerManager().PauseTimer(secondNeuralNetHandle);
		GetWorld()->GetTimerManager().PauseTimer(thirdNeuralNetHandle);
		GetWorld()->GetTimerManager().PauseTimer(fourthNeuralNetHandle);
		GetWorld()->GetTimerManager().PauseTimer(fifthNeuralNetHandle);
		GetWorld()->GetTimerManager().PauseTimer(sixthNeuralNetHandle);
		GetWorld()->GetTimerManager().PauseTimer(seventhNeuralNetHandle);
		GetWorld()->GetTimerManager().PauseTimer(eighthNeuralNetHandle);

		UE_LOG(LogTemp, Warning, TEXT("****START RECORDING****"));
	}
}

/*
triggered by a timer that runs every 1/4 second, this method iterates through and 
activates 8 timers that will record the inputs for the circle Neural net
*/
void AMyPawn::setupNeuralNets()
{
	if (numNets == 1)
	{
		GetWorld()->GetTimerManager().SetTimer(firstNeuralNetHandle, this, &AMyPawn::firstRecordForNeuralNet, 0.01f, true);
		++numNets;
	}
	else if (numNets == 2)
	{
		GetWorld()->GetTimerManager().SetTimer(secondNeuralNetHandle, this, &AMyPawn::secondRecordForNeuralNet, 0.01f, true);
		++numNets;
	}
	else if (numNets == 3)
	{
		GetWorld()->GetTimerManager().SetTimer(thirdNeuralNetHandle, this, &AMyPawn::thirdRecordForNeuralNet, 0.01f, true);
		++numNets;
	}
	else if (numNets == 4)
	{
		GetWorld()->GetTimerManager().SetTimer(fourthNeuralNetHandle, this, &AMyPawn::fourthRecordForNeuralNet, 0.01f, true);
		++numNets;
	}
	else if (numNets == 5)
	{
		GetWorld()->GetTimerManager().SetTimer(fifthNeuralNetHandle, this, &AMyPawn::fifthRecordForNeuralNet, 0.01f, true);
		++numNets;
	}
	else if (numNets == 6)
	{
		GetWorld()->GetTimerManager().SetTimer(sixthNeuralNetHandle, this, &AMyPawn::sixthRecordForNeuralNet, 0.01f, true);
		++numNets;
	}
	else if (numNets == 7)
	{
		GetWorld()->GetTimerManager().SetTimer(seventhNeuralNetHandle, this, &AMyPawn::seventhRecordForNeuralNet, 0.01f, true);
		++numNets;
	}
	else if (numNets == 8)
	{
		GetWorld()->GetTimerManager().SetTimer(eighthNeuralNetHandle, this, &AMyPawn::eighthRecordForNeuralNet, 0.01f, true);
		++numNets;
	}
	else if(numNets >= 9)
	{
		//all eight timers have been activated, deactivate this timer.
		GetWorld()->GetTimerManager().ClearTimer(setupNeuralNetsHandle);
	}
	else
	{
		//an error has occured, deactivate this timer
		GetWorld()->GetTimerManager().ClearTimer(setupNeuralNetsHandle);
	}
}

/*
This method is called when a circle is recognized.  It prints a message to the output channel and also
triggers StartGrowing()
*/
void AMyPawn::respondToCircle()
{
	if (!circleCooldown)
	{
		UE_LOG(LogTemp, Warning, TEXT("A circle was made: %d"), circleCount);
		StartGrowing();
		circleCooldown = true;
		++circleCount;
	}
}

/*
resets the cooldown on the circleCooldown, allowing the circle skill to be used again.
*/
void AMyPawn::circleCooldownReset()
{
	if (circleCooldown)
	{
		circleCooldown = false;
	}
}

/*
these methods are all pretty much the same, except for where they store their inputs.
They had to be separated so they could be individually called by the timer they use.
*/
void AMyPawn::firstRecordForNeuralNet()
{
	if (firstInputs.size() >= 400)
	{
		if (net->CheckDataForCircle(firstInputs) == 1)
		{
			respondToCircle();
		}
		firstInputs.clear();
	}
	else
	{
		firstInputs.push_back(adjustDouble(accelXAxis));
		firstInputs.push_back(adjustDouble(accelZAxis));
	}
}
void AMyPawn::secondRecordForNeuralNet()
{
	if (secondInputs.size() >= 400)
	{
		if (net->CheckDataForCircle(secondInputs) == 1)
		{
			respondToCircle();
		}
		secondInputs.clear();
	}
	else
	{
		//switch statement to save to the correct 
		secondInputs.push_back(adjustDouble(accelXAxis));
		secondInputs.push_back(adjustDouble(accelZAxis));
	}
}
void AMyPawn::thirdRecordForNeuralNet()
{
	if (thirdInputs.size() >= 400)
	{
		if (net->CheckDataForCircle(thirdInputs) == 1)
		{
			respondToCircle();
		}
		thirdInputs.clear();
	}
	else
	{
		//switch statement to save to the correct 
		thirdInputs.push_back(adjustDouble(accelXAxis));
		thirdInputs.push_back(adjustDouble(accelZAxis));
	}
}
void AMyPawn::fourthRecordForNeuralNet()
{
	if (fourthInputs.size() >= 400)
	{
		if (net->CheckDataForCircle(fourthInputs) == 1)
		{
			respondToCircle();
		}
		fourthInputs.clear();
	}
	else
	{
		//switch statement to save to the correct 
		fourthInputs.push_back(adjustDouble(accelXAxis));
		fourthInputs.push_back(adjustDouble(accelZAxis));
	}
}
void AMyPawn::fifthRecordForNeuralNet()
{
	if (fifthInputs.size() >= 400)
	{
		if (net->CheckDataForCircle(fifthInputs) == 1)
		{
			respondToCircle();
		}
		fifthInputs.clear();
	}
	else
	{
		//switch statement to save to the correct 
		fifthInputs.push_back(adjustDouble(accelXAxis));
		fifthInputs.push_back(adjustDouble(accelZAxis));
	}
}
void AMyPawn::sixthRecordForNeuralNet()
{
	if (sixthInputs.size() >= 400)
	{
		if (net->CheckDataForCircle(sixthInputs) == 1)
		{
			respondToCircle();
		}
		sixthInputs.clear();
	}
	else
	{
		//switch statement to save to the correct 
		sixthInputs.push_back(adjustDouble(accelXAxis));
		sixthInputs.push_back(adjustDouble(accelZAxis));
	}
}
void AMyPawn::seventhRecordForNeuralNet()
{
	if (seventhInputs.size() >= 400)
	{
		if (net->CheckDataForCircle(seventhInputs) == 1)
		{
			respondToCircle();
		}
		seventhInputs.clear();
	}
	else
	{
		//switch statement to save to the correct 
		seventhInputs.push_back(adjustDouble(accelXAxis));
		seventhInputs.push_back(adjustDouble(accelZAxis));
	}
}
void AMyPawn::eighthRecordForNeuralNet()
{
	if (eighthInputs.size() >= 400)
	{
		if (net->CheckDataForCircle(eighthInputs) == 1)
		{
			respondToCircle();
		}
		eighthInputs.clear();
	}
	else
	{
		//switch statement to save to the correct 
		eighthInputs.push_back(adjustDouble(accelXAxis));
		eighthInputs.push_back(adjustDouble(accelZAxis));
	}
}

/*
check to see if the network is supposed to close, then shutdown the thread
*/
void AMyPawn::NetworkClosed()
{
	if (controller->IsThreadFinished())
	{
		//thread is finished
		UE_LOG(LogTemp, Warning, TEXT("MyPawn: Thread Finished"));
		//delete runnable
		NetworkForController::shutdownThread();
		controller = NULL;
		//clear timer
		GetWorld()->GetTimerManager().ClearTimer(networkTimerHandle);
	}
}

//*******RECORDING METHODS*****
/*
These methods govern recording new test data.  They begin as paused, but can be activated using the 'r' key.
*/

/*
This method decides if the test case should be a circle or non-circle, and 
counts down for three seconds.
*/
void AMyPawn::timerToCollect()
{
	if (countdown && record)
	{
		//called every second
		if (startsIn == 3)
		{
			//decide what answer is
			answer = (int)(2.0*rand() / double(RAND_MAX));
			if (answer == 1)
			{
				UE_LOG(LogTemp, Warning, TEXT("Make a circle in..."));
			}
			else
			{
				answer = -1;
				UE_LOG(LogTemp, Warning, TEXT("Do something else in..."));
			}
		}
		if (startsIn > 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("%d"), startsIn);
			startsIn--;
		}
		else if (startsIn == 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("Start!"));
			countdown = false;
			startsIn = 3;
			collectData = true;
		}
	}

}

/*
val: the double to truncate
this method truncates a double to four decimal places.
It is used to reduce the noise in the data received from the Android's linear accelerometer
*/
double AMyPawn::adjustDouble(double val)
{
	int temp = 0;
	double adjustedVal = 0;
	temp = val * 10000;
	adjustedVal = temp;
	adjustedVal = adjustedVal / 10000;
	if (adjustedVal > 100.0 || adjustedVal < -100.0)
	{
		UE_LOG(LogTemp, Error, TEXT("input value is abnormally high"));
	}
	return adjustedVal;
}

/*
this method collects 200 sets of x,z values and inserts them in a vector<double>.
After, it asks the user to confirm that they followed the correct output request.
*/
void AMyPawn::collectAccelData()
{
	if (collectData && record)
	{
		if (iterations >= 200)
		{
			collectData = false; //don't collect any more data while we process what we have
			confirmData = true;

			iterations = 0;
			UE_LOG(LogTemp, Warning, TEXT("Waiting for confimation, x to accept, z to reject"));
		}
		else
		{
			inputs.push_back(adjustDouble(accelXAxis));
			//inputs.push_back(accelYAxis);
			inputs.push_back(adjustDouble(accelZAxis));
			iterations++;
		}
	}
}

/*
this method waits for the user to confirm or deny the data recorded in collectAccelData()
dataConfirmed is initialized to 0 in the constructor, and is changed in the Accept() and Reject() methods
the user's response determines if the process continues to SaveData() or cycles around to timerToCollect()
*/
void AMyPawn::Confirm()
{
	if (confirmData && record)
	{
		if (dataConfirmed == 1) //data confirmed, write data
		{
			UE_LOG(LogTemp, Warning, TEXT("the input has been accepted"));
			confirmData = false;
			dataConfirmed = 0;
			writeData = true;
		}
		else if (dataConfirmed == 2) //data denied, move on
		{
			UE_LOG(LogTemp, Warning, TEXT("the input has been rejected"));
			dataConfirmed = 0;
			confirmData = false;
			countdown = true;
			inputs.clear();
		}
	}
}

/*
this method writes the current inputData to a file called TestData.txt located in SaveDirectory, which is an FString that 
is set at the top of this file.
The input vector is then cleared at the end of the method for the next run through the cycle.
*/
void AMyPawn::SaveData()
{
	if (writeData && record)
	{
		//we have a set of input data
		//make input string
		UE_LOG(LogTemp, Warning, TEXT("Inputs.size is: %d"), inputs.size());
		FString inputString = "in:";
		for (int i = 0; i <= inputs.size()-1; i++)
		{
			inputString.Append(" " + FString::SanitizeFloat(inputs[i]));
		}
		inputString.Append("\r\n");

		FString outputString = "out: ";
		outputString.AppendInt(answer);
		outputString.Append("\r\n");

		//FOR GATHERING TEST DATA
		//print input string to file


		FString FileName = FString("TestData.txt");

		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();



		if (PlatformFile.CreateDirectoryTree(*SaveDirectory))
		{
			//get absolute file path
			FString AbsoluteFilePath = SaveDirectory + "/" + FileName;
			IFileHandle* handle = PlatformFile.OpenWrite(*AbsoluteFilePath, true);
			//Allow overwriting or file doesn't already exist
			if (handle)
			{
				handle->Write((const uint8*)TCHAR_TO_ANSI(*inputString), inputString.Len());
				handle->Write((const uint8*)TCHAR_TO_ANSI(*outputString), outputString.Len());
			}
			//UE_LOG(LogTemp, Warning, TEXT("data Saved"));
			UE_LOG(LogTemp, Warning, TEXT("Amount Saved: %d"), count);
			count++;
			delete(handle);
		}

		//clear and ready for next round through
		writeData = false;
		inputs.clear();
		countdown = true;
	}
}

