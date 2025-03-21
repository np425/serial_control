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
    std::string command;
    while (true) {
        std::cout << "Enter command to send (or type 'exit' to quit): ";
        std::getline(std::cin, command);

        if (command == "exit") break;

        // Ensure the command is correctly formatted
        command += "\n";  // Matches `picocom`'s default newline conversion

        try {
            // Send command
            serialPort.FlushIOBuffers();
            serialPort.Write(command);
            serialPort.DrainWriteBuffer();  // Ensure data is fully sent
            std::this_thread::sleep_for(std::chrono::milliseconds(50));  // Give time for transmission

            std::cout << "Command sent: " << command << std::endl;

            // Wait for response from the device
            std::string response;
            char byte;
            while (serialPort.IsDataAvailable()) {
                serialPort.ReadByte(byte, 10);
                response += byte;
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

