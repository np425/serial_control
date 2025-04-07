#include <iostream>
#include <unistd.h>
#include <termios.h>
#include <libserial/SerialPort.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <libserial/SerialPort.h>
#include <chrono>
#include <thread>
#include <cmath>
#include <algorithm>
#include <numeric>
using namespace std;

LibSerial::SerialPort serialPort;

void set_motors_speed(float left_speed, float right_speed) {
    std::string cmd_left = "set MOT_l_speed_rad_s " + std::to_string(left_speed) + "\n";
    std::string cmd_right = "set MOT_r_speed_rad_s " + std::to_string(right_speed) + "\n";
    std::string response;

    serialPort.FlushIOBuffers();
    serialPort.Write(cmd_left);
    serialPort.DrainWriteBuffer();
    serialPort.FlushIOBuffers();
    serialPort.ReadLine(response, '\n', 100);
    serialPort.Write(cmd_right);
    serialPort.DrainWriteBuffer();
    serialPort.ReadLine(response, '\n', 100);
}

int main(int argc, char* argv[]){
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <serial_port>\n";
        return 1;
    }

    std::string portName = argv[1];
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

    // std::this_thread::sleep_for(std::chrono::seconds(1));
    // set_motors_speed(15, 15);
    // std::this_thread::sleep_for(std::chrono::seconds(1));
    // set_motors_speed(0, 0);
    
  termios oldt, newt;
  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  float left_speed = 0.0, right_speed = 0.0;
  const float steps[7] = {0.1, 0.5, 1.0, 2.0, 3.0, 4.0, 5.0};
  int step_idx = 0;
  float step = steps[step_idx];
  cout << "Arrow keys adjust speeds. q decreases step, w increases step, z exits.\n";
  while(true){
    char c;
    if(read(STDIN_FILENO, &c, 1) < 1) break;
    if(c=='\033'){
      char seq[2];
      if(read(STDIN_FILENO, &seq[0], 1) < 1) break;
      if(read(STDIN_FILENO, &seq[1], 1) < 1) break;
      cout << "\033[2J\033[H";
      if(seq[0]=='['){
        if(seq[1]=='D') { left_speed += step; if (left_speed > 15) left_speed = 15; }
        else if(seq[1]=='B'){ left_speed -= step; if(left_speed < 0) left_speed = 0; }
        else if(seq[1]=='C') { right_speed += step; if (right_speed > 15 ) right_speed = 15; }
        else if(seq[1]=='A'){ right_speed -= step; if(right_speed < 0) right_speed = 0; }
      }
    }
    else if(c=='q'){
      if(step_idx > 0){ step_idx--; }
      step = steps[step_idx];
      cout << "\033[2J\033[H";
    }
    else if(c=='w'){
      if(step_idx < 6){ step_idx++; }
      step = steps[step_idx];
      cout << "\033[2J\033[H";
    }
    else if(c=='z') {
        set_motors_speed(0, 0);
        break;
    }
    cout << "Motor step: " << step << "\n";
    cout << "Left motor speed (rad/s): " << left_speed << "\n";
    cout << "Right motor speed (rad/s): " << right_speed << "\n";
    set_motors_speed(left_speed, right_speed);
  }
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  return 0;
  
}
