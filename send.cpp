#include <iostream>
#include <libserial/SerialPort.h>
#include <libserial/SerialStream.h>
#include <thread>
#include <chrono>

int main(int argc, char* argv[]) {
    // Check if the user provided a serial port as an argument
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <serial_port>\n";
        return 1;
    }

    std::string portName = argv[1];

    // Open the serial port
    LibSerial::SerialPort serialPort;
    try {
        serialPort.Open(portName);
        serialPort.SetBaudRate(LibSerial::BaudRate::BAUD_115200);
        serialPort.SetCharacterSize(LibSerial::CharacterSize::CHAR_SIZE_8);
        serialPort.SetStopBits(LibSerial::StopBits::STOP_BITS_1);
        serialPort.SetParity(LibSerial::Parity::PARITY_NONE);
        serialPort.SetFlowControl(LibSerial::FlowControl::FLOW_CONTROL_NONE); // Disable flow control
        serialPort.SetSerialPortBlockingStatus(true);

        std::cout << "Serial port " << portName << " opened successfully.\n";
    } catch (const LibSerial::OpenFailed&) {
        std::cerr << "Error: Failed to open serial port " << portName << "!\n";
        return 1;
    }

    // Read input from user, send over serial, and receive response
    std::string command = "set MOT_l_speed_pwm 255\n";
    while (true) {
        try {
            // Send command
            serialPort.FlushInputBuffer();
            serialPort.Write(command);
            //serialPort.FlushOutputBuffer();
            serialPort.DrainWriteBuffer();  // Ensure data is fully sent
            std::cout << "Command sent: " << command << std::endl;

            // Wait for response from the device
            std::string response;
            if (serialPort.IsDataAvailable()) {
                serialPort.Read(response, 1000, 200);
            }

            // Display response if available
            if (!response.empty()) {
                std::cout << "Device response: " << response << std::endl;
            } else {
                std::cout << "No response received.\n";
            }

        } catch (const std::exception& e) {
            std::cerr << "Error communicating with device: " << e.what() << std::endl;
        }
    }

    // Close the serial port
    serialPort.Close();
    std::cout << "Serial port closed.\n";

    return 0;
}

