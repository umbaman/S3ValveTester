#ifndef MOTORCONTROL_H
#define MOTORCONTROL_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <STM32Ethernet.h>

// Function declarations
void setupMotorControl();
void loopMotorControl();
void driveMotorForward();
void driveMotorBackward();
void stopMotor();
void readCurrent();
float calculateRMS(float readings[], int count);
void sendCompletionMessage(bool success);
void handleTCPClient();
void parseCommand(String command);
void startProcedure();
void executeMotorProcedure();
void resetVariables();

// Constants for Adafruit TB6612 motor driver
const int DIR1 = D11;
const int PWM1 = D10;
const int PWM2 = D8;
const int DIR2 = D9;

const int directionDelay = 100; // ms
const int inertiaDelay = 1000;  // ms

// INA219 current sensor
extern Adafruit_INA219 ina219;

// Ethernet server
extern EthernetServer server;

// TCP client
extern EthernetClient client;

// Global variables
extern float forwardReadings[100];
extern float reverse1Readings[100];
extern float reverse2Readings[100];
extern int countForward;
extern int countReverse1;
extern int countReverse2;
extern int globalTimeoutDuration;
extern float globalReverseThreshold;
extern int stateChangeTime;
extern int forwardStartTime;
extern int reverseStartTime;
extern int finalReverseStartTime;
extern int forwardDuration;
extern int reverseDuration;
extern int finalReverseDuration;
extern bool procedureStarted;

// Motor states
enum MotorState { IDLE, FORWARD, REVERSE, FINAL_REVERSE, FAILURE, STOPPED };
extern MotorState motorState;

#endif