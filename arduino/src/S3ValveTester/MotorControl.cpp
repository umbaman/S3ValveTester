#include "MotorControl.h"

// Initialize INA219 and Ethernet server
Adafruit_INA219 ina219;
EthernetServer server(80);
EthernetClient client;

// Global variables
float forwardReadings[100];
float reverse1Readings[100];
float reverse2Readings[100];
int countForward = 0;
int countReverse1 = 0;
int countReverse2 = 0;
int globalTimeoutDuration = 7000; // default timeout duration
float globalReverseThreshold = 100.0; // default reverse threshold
int stateChangeTime = 0;
int forwardStartTime = 0;
int reverseStartTime = 0;
int finalReverseStartTime = 0;
int forwardDuration = 0;
int reverseDuration = 0;
int finalReverseDuration = 0;
bool procedureStarted = false;
MotorState motorState = IDLE;

// MAC address for Ethernet
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

void setupMotorControl() {
    // Initialize motor driver pins
    pinMode(AIN1, OUTPUT);
    pinMode(AIN2, OUTPUT);
    pinMode(PWMA, OUTPUT);
    pinMode(BIN1, OUTPUT);
    pinMode(BIN2, OUTPUT);
    pinMode(PWMB, OUTPUT);
    pinMode(STBY, OUTPUT);

    // Set standby to HIGH to enable the motor driver
    digitalWrite(STBY, HIGH);

    // Initialize INA219
    ina219.begin();

    // Start Ethernet server
    Ethernet.begin(mac);
    server.begin();
}

void loopMotorControl() {
    handleTCPClient();
    if (procedureStarted) {
        executeMotorProcedure();
    }
}

void handleTCPClient() {
    client = server.available();
    if (client) {
        if (client.connected()) {
            String command = client.readStringUntil('\n');
            parseCommand(command);
        }
    }
}

void parseCommand(String command) {
    if (command.startsWith("S")) {
        int commaIndex1 = command.indexOf(',');
        int commaIndex2 = command.indexOf(',', commaIndex1 + 1);
        
        globalTimeoutDuration = command.substring(commaIndex1 + 1, commaIndex2).toInt();
        globalReverseThreshold = command.substring(commaIndex2 + 1).toFloat();
        
        startProcedure();
    }
}

void startProcedure() {
    resetVariables();
    procedureStarted = true;
    motorState = FORWARD;
    forwardStartTime = millis();
    stateChangeTime = millis();
    driveMotorForward();
}

void resetVariables() {
    countForward = 0;
    countReverse1 = 0;
    countReverse2 = 0;
    forwardDuration = 0;
    reverseDuration = 0;
    finalReverseDuration = 0;
}

void executeMotorProcedure() {
    float current_mA = ina219.getCurrent_mA();

    switch (motorState) {
        case FORWARD:
            if (millis() - stateChangeTime >= directionDelay) {
                if (countForward < 100) forwardReadings[countForward++] = current_mA;
                if (current_mA >= 65.0) {
                    driveMotorBackward();
                    motorState = REVERSE;
                    stateChangeTime = millis();
                    forwardDuration = millis() - forwardStartTime;
                    reverseStartTime = millis();
                } else if (millis() - stateChangeTime >= globalTimeoutDuration) {
                    motorState = FAILURE;
                }
            }
            break;

        case REVERSE:
            if (millis() - stateChangeTime >= directionDelay + inertiaDelay) {
                if (countReverse1 < 100) reverse1Readings[countReverse1++] = current_mA;
                if (current_mA >= globalReverseThreshold) {
                    driveMotorForward();
                    motorState = FINAL_REVERSE;
                    stateChangeTime = millis();
                    reverseDuration = millis() - reverseStartTime;
                    finalReverseStartTime = millis();
                } else if (millis() - stateChangeTime >= globalTimeoutDuration) {
                    motorState = FAILURE;
                }
            }
            break;

        case FINAL_REVERSE:
            if (millis() - stateChangeTime >= directionDelay + inertiaDelay) {
                if (countReverse2 < 100) reverse2Readings[countReverse2++] = current_mA;
                if (current_mA >= globalReverseThreshold) {
                    stopMotor();
                    motorState = STOPPED;
                    finalReverseDuration = millis() - finalReverseStartTime;
                    sendCompletionMessage(true);
                    procedureStarted = false;
                } else if (millis() - stateChangeTime >= globalTimeoutDuration) {
                    motorState = FAILURE;
                }
            }
            break;

        case FAILURE:
            sendCompletionMessage(false);
            procedureStarted = false;
            motorState = IDLE;
            break;

        case STOPPED:
            // Ensure message is only sent once
            if (motorState != FINAL_REVERSE) {
                sendCompletionMessage(false);
                motorState = FINAL_REVERSE;
            }
            break;

        default:
            break;
    }
}

void driveMotorForward() {
    digitalWrite(AIN1, HIGH);
    digitalWrite(AIN2, LOW);
    digitalWrite(PWMA, HIGH); // Full speed
}

void driveMotorBackward() {
    digitalWrite(AIN1, LOW);
    digitalWrite(AIN2, HIGH);
    digitalWrite(PWMA, HIGH); // Full speed
}

void stopMotor() {
    digitalWrite(AIN1, LOW);
    digitalWrite(AIN2, LOW);
    digitalWrite(PWMA, LOW); // Stop motor
}

void sendCompletionMessage(bool success) {
    String message = success ? "P," : "F,";
    message += String(calculateRMS(forwardReadings, countForward)) + ",";
    message += String(calculateRMS(reverse1Readings, countReverse1)) + ",";
    message += String(calculateRMS(reverse2Readings, countReverse2)) + ",";
    message += String(forwardDuration) + ",";
    message += String(reverseDuration) + ",";
    message += String(finalReverseDuration);
    client.println(message);
}

float calculateRMS(float readings[], int count) {
    float sumOfSquares = 0.0;
    for (int i = 0; i < count; i++) {
        sumOfSquares += readings[i] * readings[i];
    }
    return sqrt(sumOfSquares / count);
}
