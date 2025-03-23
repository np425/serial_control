#include <iostream>
#include <sstream>
#include <vector>
#include <libserial/SerialPort.h>
#include <chrono>
#include <thread>
#include <cmath>
#include <algorithm>
#include <numeric>

void stats(std::vector<double>& numbers) {
    double minVal = *std::min_element(numbers.begin(), numbers.end());
    double maxVal = *std::max_element(numbers.begin(), numbers.end());

    double avgVal = std::accumulate(numbers.begin(), numbers.end(), 0.0) / numbers.size();

    double minHertz = 1000.0 / minVal;
    double maxHertz = 1000.0 / maxVal;
    double avgHertz = 1000.0 / avgVal;


    std::cout << "Min = " << minVal << " ms (" << minHertz << " Hz),"
            << "Avg = " << avgVal << " ms (" << avgHertz << "Hz),"
            << "Max = " << maxVal << " ms (" << maxHertz << "Hz)" << std::endl;
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
        std::cout << "Serial port " << portName << " opened successfully.\n";
    } catch (const LibSerial::OpenFailed&) {
        std::cerr << "Error: Failed to open serial port " << portName << "!\n";
        return 1;
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // Commands to send
    std::string getCommand = "get\n";
    std::string setCommand = "set MOT_l_speed_pwm 255\n";
    std::string response;
    const long samples = 1000;

    std::vector<double> get_latencies;
    std::vector<double> set_latencies;

    get_latencies.reserve(samples);
    set_latencies.reserve(samples);

    auto begin_measure = std::chrono::steady_clock::now();

    for (long i = 0; i < samples; i++) {
        // Send "get" command and measure latency
        auto start = std::chrono::steady_clock::now();

        serialPort.FlushIOBuffers();
        serialPort.Write(getCommand);
        serialPort.ReadLine(response, '\n', 100);

        auto end = std::chrono::steady_clock::now();
        std::chrono::duration<double, std::milli> latency = end - start;
        double count_get = latency.count();

        // std::cout << "GET response: " << response 
        //         << " (Latency: " << count_get << " ms)" 
        //         << std::endl;
        
        // Send "set" command and measure latency
        start = std::chrono::steady_clock::now();

        serialPort.FlushIOBuffers();
        serialPort.Write(setCommand);
        serialPort.DrainWriteBuffer();
        // serialPort.ReadLine(response, '\n', 100);

        end = std::chrono::steady_clock::now();
        latency = end - start;
        double count_set = latency.count();
        
        // std::cout << "SET response: " << response 
        //         << " (Latency: " << count_set << " ms)" 
        //         << std::endl;

        get_latencies.push_back(count_get);
        set_latencies.push_back(count_set);

        // 1000 Hz
        //std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    auto end_measure = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::milli> latency = end_measure - begin_measure;
    double count_measure = latency.count();

    std::vector<double> combined_latencies;
    combined_latencies.reserve(samples);
    for (long i = 0; i < samples; i++) {
        combined_latencies.push_back(get_latencies[i] + set_latencies[i]);
    }

    double hertz = samples / (count_measure / 1000.0);

    std::cout << "GET Latency: ";
    stats(get_latencies);
    std::cout << "SET Latency: ";
    stats(set_latencies);
    std::cout << "BOTH Latency: ";
    stats(combined_latencies);
    std::cout << samples << " Samples processed in " << count_measure << " ms (" << hertz << " Hz)" << std::endl;

    serialPort.Close();
    std::cout << "Serial port closed.\n";

    return 0;
}
