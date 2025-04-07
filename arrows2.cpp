#include <iostream>
#include <unistd.h>
#include <termios.h>
#include <libserial/SerialPort.h>
#include <sstream>
#include <vector>
#include <chrono>
#include <thread>
#include <cmath>
#include <algorithm>
#include <numeric>
using namespace std;

LibSerial::SerialPort serialPort;

void set_motors_speed(float left_speed, float right_speed) {
    string cmd_left = "set MOT_l_speed_rad_s " + to_string(left_speed) + "\n";
    string cmd_right = "set MOT_r_speed_rad_s " + to_string(right_speed) + "\n";
    string response;
    serialPort.FlushIOBuffers();
    serialPort.Write(cmd_left);
    serialPort.DrainWriteBuffer();
    serialPort.ReadLine(response, '\n', 100);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    serialPort.FlushIOBuffers();
    serialPort.Write(cmd_right);
    serialPort.DrainWriteBuffer();
    serialPort.ReadLine(response, '\n', 100);

    // string cmd_left = "set MOT_l_speed_rad_s " + to_string(left_speed) + "\n";
    // string cmd_right = "set MOT_r_speed_rad_s " + to_string(right_speed) + "\n";
    // string cmd = cmd_left+cmd_right;
    // string response;
    // serialPort.FlushIOBuffers();
    // serialPort.Write(cmd);
    // serialPort.DrainWriteBuffer();
    // serialPort.ReadLine(response, '\n', 100);
    // serialPort.ReadLine(response, '\n', 100);
}

int main(int argc, char* argv[]){
    if(argc < 2){
        cerr << "Usage: " << argv[0] << " <serial_port>\n";
        return 1;
    }
    string portName = argv[1];
    try {
        serialPort.Open(portName);
        serialPort.SetBaudRate(LibSerial::BaudRate::BAUD_115200);
        serialPort.SetCharacterSize(LibSerial::CharacterSize::CHAR_SIZE_8);
        serialPort.SetStopBits(LibSerial::StopBits::STOP_BITS_1);
        serialPort.SetParity(LibSerial::Parity::PARITY_NONE);
        serialPort.SetFlowControl(LibSerial::FlowControl::FLOW_CONTROL_NONE);
        cout << "Serial port " << portName << " opened successfully.\n";
    } catch (const LibSerial::OpenFailed&) {
        cerr << "Error: Failed to open serial port " << portName << "!\n";
        return 1;
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    
    float left_speed = 0.0, right_speed = 0.0;
    const float max_speed = 15.0;
    const float steps[7] = {0.5, 1.0, 2.0, 3.0, 4.0, 5.0};
    int step_idx = 0;
    float step = steps[step_idx];
    
    cout << "Controls:\n";
    cout << "  W: Increase overall forward speed\n";
    cout << "  S: Decrease overall forward speed\n";
    cout << "  A: Turn left (decrease left speed, increase right speed)\n";
    cout << "  D: Turn right (increase left speed, decrease right speed)\n";
    cout << "  Q: Fine-tune left motor upward\n";
    cout << "  Z: Fine-tune left motor downward\n";
    cout << "  E: Fine-tune right motor upward\n";
    cout << "  C: Fine-tune right motor downward\n";
    cout << "  R: Increase step size (coarser control)\n";
    cout << "  F: Decrease step size (finer control)\n";
    cout << "  X: Exit (motors set to 0)\n";
    
    while(true){
        char c;
        if(read(STDIN_FILENO, &c, 1) < 1) break;
        // If it's an escape sequence (arrow keys), ignore them.
        if(c == '\033'){
            char dummy;
            read(STDIN_FILENO, &dummy, 1);
            read(STDIN_FILENO, &dummy, 1);
            continue;
        }
        char key = toupper(c);
        bool update = false;
        switch(key){
            case 'W': // Increase overall forward speed
                left_speed += step;
                right_speed += step;
                update = true;
                break;
            case 'S': // Decrease overall forward speed
                left_speed -= step;
                right_speed -= step;
                update = true;
                break;
            case 'A': // Turn left: decrease left, increase right
                left_speed -= step;
                right_speed += step;
                update = true;
                break;
            case 'D': // Turn right: increase left, decrease right
                left_speed += step;
                right_speed -= step;
                update = true;
                break;
            case 'Q': // Fine-tune left motor upward
                left_speed += step;
                update = true;
                break;
            case 'Z': // Fine-tune left motor downward
                left_speed -= step;
                update = true;
                break;
            case 'E': // Fine-tune right motor upward
                right_speed += step;
                update = true;
                break;
            case 'C': // Fine-tune right motor downward
                right_speed -= step;
                update = true;
                break;
            case 'R': // Increase step size (coarser)
                if(step_idx < 5) step_idx++;
                step = steps[step_idx];
                update = true;
                break;
            case 'F': // Decrease step size (finer)
                if(step_idx > 0) step_idx--;
                step = steps[step_idx];
                update = true;
                break;
            case 'X': // Exit
                set_motors_speed(0, 0);
                tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
                return 0;
            default:
                break;
        }
        // Clamp speeds to [0, max_speed]
        left_speed = min(max(left_speed, 0.0f), max_speed);
        right_speed = min(max(right_speed, 0.0f), max_speed);
        if(update){
            cout << "\033[2J\033[H"; // clear terminal
            cout << "Current step: " << step << "\n";
            cout << "Left motor speed (rad/s): " << left_speed << "\n";
            cout << "Right motor speed (rad/s): " << right_speed << "\n";
            set_motors_speed(left_speed, right_speed);
        }
    }

    
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return 0;
}
