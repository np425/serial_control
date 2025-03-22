#include <iostream>
#include <libserial/SerialPort.h>
#include <libserial/SerialStream.h>
#include <thread>
#include <chrono>

void readThread(LibSerial::SerialPort &serialPort) {
    std::string response;

    while (true) {
        try {
            while (serialPort.IsDataAvailable()) {
                serialPort.ReadLine(response, '\n');
                if (!response.empty()) {
                    std::cout << response << std::endl;
                }            
            }
        } catch (const std::exception& e) {
            std::cerr << "Error in read thread: " << e.what() << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

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

    // Start the reading thread
    std::thread reader(readThread, std::ref(serialPort));

    // Main thread for sending commands
    std::string command = "set MOT_l_speed_pwm 255\n";
    while (true) {
        try {
            serialPort.Write(command);
            serialPort.DrainWriteBuffer();  // Ensure data is fully sent
            std::cout << "Command sent: " << command;
        } catch (const std::exception& e) {
            std::cerr << "Error communicating with device: " << e.what() << std::endl;
        }
        // Send command at regular intervals
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Although unreachable in this infinite loop,
    // proper thread joining and port closing should be done if the loop ever ends.
    reader.join();
    serialPort.Close();
    std::cout << "Serial port closed.\n";

    return 0;
}
