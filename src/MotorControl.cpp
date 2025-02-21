#include "MotorControl.h"

// Initialize INA219 and Ethernet server
Adafruit_INA219 ina219;
EthernetServer server(10000);
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
const char* motorStateStr[] = {"IDLE", "FORWARD", "REVERSE", "FINAL_REVERSE", "FAILURE", "STOPPED"};

// MAC address for Ethernet
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 201);  // Set static IP address

void setupMotorControl() {
    pinMode(DIR1, OUTPUT);
    pinMode(PWM1, OUTPUT);
    pinMode(PWM2, OUTPUT);
    pinMode(DIR2, OUTPUT);

    if (!ina219.begin()) {
        Serial.println("Couldn't find INA219");
        while (1);
    }
    Serial.println("Ina219 Ready");

    //Set up Comms
    Serial.begin(9600);
    Ethernet.begin(mac, ip);
    server.begin();
    // Print the assigned IP address and port to the serial monitor
    Serial.println("Ethernet ready");
    Serial.print("IP Address: ");
    Serial.println(Ethernet.localIP());
    Serial.print("Server is listening on port: ");

  return;
}

void loopMotorControl() {
    handleTCPClient();
    if (procedureStarted) {
        executeMotorProcedure();
    }
    return;
}

void driveMotorForward() {
    Serial.println("Drive Motor Forward Procedure");
    digitalWrite(DIR1, HIGH);
    //digitalWrite(DIR2, LOW);
    analogWrite(PWM1, 255);
    //analogWrite(PWM2, 0);
    return;
}

void driveMotorBackward() {
    Serial.println("Drive Motor Backward Procedure");
    digitalWrite(DIR1, LOW);
    //digitalWrite(DIR2, HIGH);
    analogWrite(PWM1, 255);
    //analogWrite(PWM2, 255);

    return;
}

void stopMotor() {
    analogWrite(PWM1, 0);
    //analogWrite(PWM2, 0);

    return;
}

void readCurrent() {
    float current_mA = ina219.getCurrent_mA(); // Read current in mA
    Serial.print("Current (mA): ");
    Serial.println(current_mA);

    return;
}

void handleTCPClient() {
    client = server.available();
    if (client) {
        if (client.connected()) {
            String command = client.readStringUntil('\n');
            Serial.print("Incoming Command: ");
            Serial.println(command);
            parseCommand(command);
        }
    }

    return;
}

void parseCommand(String command) {
    Serial.print("from parse Function: ");
    if (command.startsWith("S")) {

        int commaIndex1 = command.indexOf(',');
        String cInd1(commaIndex1);
        Serial.println("First comma Index: " + cInd1);

        int commaIndex2 = command.indexOf(',', commaIndex1 + 1);
        String cInd2(commaIndex2);
        Serial.println("Second comma Index: " + cInd2);

        globalTimeoutDuration = command.substring(commaIndex1 + 1, commaIndex2).toFloat();
        Serial.println("Global Timeout Duration: " + command.substring(commaIndex1 + 1, commaIndex2));
        
        globalReverseThreshold = command.substring(commaIndex2 + 1).toFloat();
        Serial.println("Global Reverse Threshold: " + command.substring(commaIndex2 + 1));
        
        startProcedure();
    }
    return;
}

void startProcedure() {
    Serial.println("Procedure is Starting");
    resetVariables();
    procedureStarted = true;
    motorState = FORWARD;
    forwardStartTime = millis();
    stateChangeTime = millis();
    driveMotorForward();
}

void executeMotorProcedure() {
    float current_mA = ina219.getCurrent_mA();
    Serial.print("Execute Motor Procedure: ");
    Serial.println(motorStateStr[motorState]);
    String current(current_mA);
    Serial.println("Motor Current:" + current);

    switch (motorState) {
        case FORWARD:
            if (millis() - stateChangeTime >= directionDelay) {
                if (current_mA >= 65.0) {
                    driveMotorBackward();
                    motorState = REVERSE;
                    stateChangeTime = millis();
                    forwardDuration = millis() - forwardStartTime;
                    reverseStartTime = millis();
                    String fwdDuration(forwardDuration);
                    Serial.println("Forward Duration:" + fwdDuration);
                }
            }
            break;

        case REVERSE:
            if (millis() - stateChangeTime >= directionDelay + inertiaDelay) {
                if (current_mA >= globalReverseThreshold) {
                    driveMotorForward();
                    motorState = FINAL_REVERSE;
                    stateChangeTime = millis();
                    reverseDuration = millis() - reverseStartTime;
                    finalReverseStartTime = millis();
                    String rvDuration(reverseDuration);
                    Serial.println("Forward Duration:" + rvDuration);
                }
            }
            break;

        case FINAL_REVERSE:
            if (millis() - stateChangeTime >= directionDelay + inertiaDelay) {
                if (current_mA >= globalReverseThreshold) {
                    stopMotor();
                    motorState = STOPPED;
                    finalReverseDuration = millis() - finalReverseStartTime;
                    sendCompletionMessage(true);
                    procedureStarted = false;
                    String frvDuration(finalReverseDuration);
                    Serial.println("Forward Duration:" + frvDuration);
                }
            }
            break;

        case FAILURE:
            sendCompletionMessage(false);
            procedureStarted = false;
            motorState = IDLE;
            break;

        default:
            break;
    }

    return;
}

void resetVariables(){
    digitalWrite(DIR1, LOW);
    digitalWrite(DIR2, LOW);
    analogWrite(PWM1, 0);
    analogWrite(PWM2, 0);

    return;
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
    return;
}

float calculateRMS(float readings[], int count) {
    float sumOfSquares = 0.0;
    for (int i = 0; i < count; i++) {
        sumOfSquares += readings[i] * readings[i];
    }
    return sqrt(sumOfSquares / count);
}
