#include <iostream>
#include <sstream>
#include <vector>
#include <libserial/SerialPort.h>
#include <chrono>
#include <thread>
#include <mutex>
#include <cmath>

std::mutex serialMutex;
std::chrono::steady_clock::time_point lastCommandSent;
std::chrono::steady_clock::time_point lastSensorSent;

void readThread(LibSerial::SerialPort &serialPort) {
    std::string response;

    while (true) {
        try {
            {
                // Blocking read until a complete line is received.
                std::lock_guard<std::mutex> lock(serialMutex);
                serialPort.ReadLine(response, '\n');
            }
            if (response.empty())
                continue;

            auto now = std::chrono::steady_clock::now();
            std::chrono::duration<double, std::milli> latency = now - lastSensorSent;
            std::cout << "Pong @ " << static_cast<int>(std::round(latency.count()))
                        << " ms (" << response << ")" << std::endl;
            lastSensorSent = now;
        } catch (const std::exception& e) {
            std::cerr << "Error reading from serial port: " << e.what() << std::endl;
        }
    }
}

void writeThread(LibSerial::SerialPort &serialPort, const std::string &command) {
    while (true) {
        try {
            {
                std::lock_guard<std::mutex> lock(serialMutex);
                serialPort.Write(command);
            }
            // Reduced delay for faster command throughput.
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        } catch (const std::exception &e) {
            std::cerr << "Error sending command: " << e.what() << std::endl;
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <serial_port>\n";
        return 1;
    }
    
    std::string portName = argv[1];
    LibSerial::SerialPort serialPort;
    try {
        serialPort.Open(portName);
        serialPort.SetBaudRate(LibSerial::BaudRate::BAUD_115200);
        serialPort.SetCharacterSize(LibSerial::CharacterSize::CHAR_SIZE_8);
        serialPort.SetStopBits(LibSerial::StopBits::STOP_BITS_1);
        serialPort.SetParity(LibSerial::Parity::PARITY_NONE);
        serialPort.SetFlowControl(LibSerial::FlowControl::FLOW_CONTROL_NONE);
        serialPort.SetSerialPortBlockingStatus(true);

        std::cout << "Serial port " << portName << " opened successfully.\n";
    } catch (const LibSerial::OpenFailed&) {
        std::cerr << "Error: Failed to open serial port " << portName << "!\n";
        return 1;
    }
    
    std::string command = "set MOT_l_speed_pwm 255\n";
    std::thread reader(readThread, std::ref(serialPort));
    std::thread writer(writeThread, std::ref(serialPort), std::cref(command));

    reader.join();
    writer.join();

    serialPort.Close();
    std::cout << "Serial port closed.\n";
    
    return 0;
}
