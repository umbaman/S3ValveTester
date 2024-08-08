# Motor Control Project

This project is designed to control a motor using a TB6612FNG motor driver and monitor its current using an INA219 sensor. The project is implemented on an STM32 Nucleo F767ZI board and programmed using PlatformIO in Visual Studio Code. The motor operates in a sequence of directional movements, with the current and time for each direction being recorded and processed. The results are communicated over a TCP connection.

## Features

- **Motor Control**: Controls motor direction and speed using the TB6612FNG motor driver.
- **Current Sensing**: Monitors motor current using the INA219 sensor.
- **RMS Calculation**: Computes RMS current values for each motor movement direction.
- **TCP Communication**: Receives control commands and sends results over Ethernet.
- **Timeout Handling**: Automatically stops the motor if current thresholds are not reached within the specified timeout duration.
- **Inertia Management**: Accounts for motor inertia during directional changes.
- **Real-Time Feedback**: Sends a completion message with results (pass/fail, average currents, and time taken) after each procedure.

## Hardware Requirements

- **STM32 Nucleo F767ZI Board**
- **TB6612FNG Motor Driver**
- **INA219 Current Sensor**
- **DC Motor**
- **Power Supply for Motor and Logic**
- **Ethernet Connection (for TCP communication)**

## Software Requirements

- **PlatformIO** (Integrated with Visual Studio Code)
- **STM32 Arduino Core** (PlatformIO configuration)
- **Adafruit INA219 Library**
- **STM32Ethernet Library**

## Installation

1. **Clone the Repository:**

    ```sh
    git clone https://github.com/your-username/MotorControlProject.git
    cd MotorControlProject
    ```

2. **Open with PlatformIO:**
   - Launch Visual Studio Code.
   - Open the cloned project folder.
   - PlatformIO should automatically recognize the project.

3. **Install Dependencies:**
   - PlatformIO will handle dependency installation through the `platformio.ini` file.

4. **Upload to Nucleo Board:**
   - Connect your Nucleo F767ZI board.
   - Use PlatformIO to build and upload the code to the board.

## Usage

1. **Start the Procedure:**
   - The procedure begins when the board receives an 'S' character from the TCP connection, along with the timeout duration and reverse current threshold.
   - The motor moves forward until it reaches 65mA, then reverses twice with a current threshold defined by the reverse current threshold.

2. **TCP Communication:**
   - Results are sent back over the TCP connection after the procedure is completed. The format is:
     ```
     P/F, avg_current_forward, avg_current_reverse1, avg_current_reverse2, time_forward, time_reverse1, time_reverse2
     ```

3. **Handling Failures:**
   - If the motor does not reach the current threshold within the specified timeout, the procedure is aborted, and a failure message is sent.

## File Structure

MotorControlProject/
├── include/
│ └── MotorControl.h # Header file with declarations
├── src/
│ ├── main.cpp # Main entry point of the program
│ └── MotorControl.cpp # Implementation of motor control logic
├── platformio.ini # PlatformIO project configuration file
└── README.md # This README file


## Configuration

- **Direction Delay**: Time between direction changes to account for motor inertia.
- **Timeout Duration**: Time within which the motor must reach the current threshold.
- **Current Thresholds**: Set points for stopping motor movements.

## Contributing

Feel free to contribute to this project by submitting issues or pull requests. Please make sure to follow the project's coding standards and thoroughly test your changes.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
