// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include <vector>
#include "GameFramework/Pawn.h"
#include "MyPawn.generated.h"
class NetworkForController;
class NeuralNet;


UCLASS()
class SEMPROJECT_API AMyPawn : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AMyPawn();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called every frame
	virtual void Tick(float DeltaSeconds) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;

	UPROPERTY(EditAnywhere)
		USceneComponent* OurVisibleComponent;


	//public methods

	//private methods
private:
	void NetworkClosed();
	void collectAccelData();
	void timerToCollect();
	void Move_XAxis(float AxisValue);
	void Move_YAxis(float AxisValue);
	void StartGrowing();
	void StopGrowing();
	void initialRecordingPaused();
	void Accept();
	void Reject();
	void StartRecording();
	void Confirm();
	void SaveData();
	double adjustDouble(double val);

	//for neuralnets
	void setupNeuralNets();
	void respondToCircle();
	void circleCooldownReset();

	void firstRecordForNeuralNet();
	void secondRecordForNeuralNet();
	void thirdRecordForNeuralNet();
	void fourthRecordForNeuralNet();
	void fifthRecordForNeuralNet();
	void sixthRecordForNeuralNet();
	void seventhRecordForNeuralNet();
	void eighthRecordForNeuralNet();

	//public variables

	//private variables

private:
	int iterations;
	int answer;
	int startsIn;
	int dataConfirmed;
	int count;
	int numNets;
	int circleCount;
	float xAxis, yAxis;
	float accelXAxis, accelYAxis, accelZAxis;
	bool bGrowing;
	bool countdown;
	bool collectData;
	bool confirmData;
	bool writeData;
	bool record;
	bool circleCooldown;
	FVector CurrentVelocity;
	FTimerHandle networkTimerHandle;
	FTimerHandle skillTimerHandle;
	FTimerHandle startCollectionTimerHandle;
	FTimerHandle confirmTimerHandle;
	FTimerHandle saveDataTimerHandle;
	FTimerHandle setupNeuralNetsHandle;
	FTimerHandle circleCooldownHandle;
	FTimerHandle stopGrowingHandle;
	NetworkForController* controller;
	std::vector<double> inputs;

	//for the neural nets
	NeuralNet* net;

	//each set of FTimerHandle and vector<double> is concerned with one of the timers that record a set
	//of inputs for the neural net
	FTimerHandle firstNeuralNetHandle;
	std::vector<double> firstInputs;
	FTimerHandle secondNeuralNetHandle;
	std::vector<double> secondInputs;
	FTimerHandle thirdNeuralNetHandle;
	std::vector<double> thirdInputs;
	FTimerHandle fourthNeuralNetHandle;
	std::vector<double> fourthInputs;
	FTimerHandle fifthNeuralNetHandle;
	std::vector<double> fifthInputs;
	FTimerHandle sixthNeuralNetHandle;
	std::vector<double> sixthInputs;
	FTimerHandle seventhNeuralNetHandle;
	std::vector<double> seventhInputs;
	FTimerHandle eighthNeuralNetHandle;
	std::vector<double> eighthInputs;

};
