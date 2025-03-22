#include <iostream>
#include <libserial/SerialPort.h>
#include <libserial/SerialStream.h>
#include <chrono>
#include <thread>

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

    std::string command = "set MOT_l_speed_pwm 255\n";
    auto lastSendTime = std::chrono::steady_clock::now();

    while (true) {
        // Read available data every 10 ms
        while (serialPort.IsDataAvailable()) {
            std::string response;
            try {
                serialPort.ReadLine(response, '\n');
                if (!response.empty()) {
                    std::cout << response << std::endl;
                }
            } catch (const std::exception& e) {
                std::cerr << "Error reading from serial port: " << e.what() << std::endl;
            }
        }

        // Send command every 100 ms
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastSendTime);
        if (elapsed.count() >= 1000) {
            try {
                serialPort.Write(command);
                serialPort.DrainWriteBuffer();  // Ensure data is fully sent
                // std::cout << "Command sent: " << command;
            } catch (const std::exception& e) {
                std::cerr << "Error sending command: " << e.what() << std::endl;
            }
            lastSendTime = now;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // Close the serial port (this code is unreachable in the infinite loop)
    serialPort.Close();
    std::cout << "Serial port closed.\n";

    return 0;
}
