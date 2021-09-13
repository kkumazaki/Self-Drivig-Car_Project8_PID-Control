#include <math.h>
#include <uWS/uWS.h>
#include <iostream>
#include <string>
#include "json.hpp"
#include "PID.h"

// for convenience
using nlohmann::json;
using std::string;

// For converting back and forth between radians and degrees.
constexpr double pi() { return M_PI; }
double deg2rad(double x) { return x * pi() / 180; }
double rad2deg(double x) { return x * 180 / pi(); }

// Checks if the SocketIO event has JSON data.
// If there is data the JSON object in string format will be returned,
// else the empty string "" will be returned.
string hasData(string s) {
  auto found_null = s.find("null");
  auto b1 = s.find_first_of("[");
  auto b2 = s.find_last_of("]");
  if (found_null != string::npos) {
    return "";
  }
  else if (b1 != string::npos && b2 != string::npos) {
    return s.substr(b1, b2 - b1 + 1);
  }
  return "";
}

int main() {
  uWS::Hub h;

  PID pid;
  /**
   * TODO: Initialize the pid variable.
   */
  // Kp, Ki, Kd
  //pid.Init(3.0, 10.0, 0.5); // [Tri#1] Too strong gain, so vehicle wander a lot at first!
  //pid.Init(0.10, 0.001, 1.0); //[Tri#2]  It can run the course stably and smoothly, but at the curve there's overshoot.
  //pid.Init(0.16, 0.001, 1.5); // [Tri#3] I made Kp bigger, and at the same time Kd bigger to increase dumpering. The vehicle wander at straight road.  
  //pid.Init(0.16, 0.002, 2.0); // [Tri#4] I made Kd bigger to increase dumpering. Still the  vehicle wander at straight road.  
  //pid.Init(0.16, 0.002, 4.0); // [Tri#5] I made Kd bigger to increase dumpering. Still the  vehicle wander at straight road.  
  //pid.Init(0.16, 0.002, 10.0); // [Tri#6] I made Kd bigger to increase dumpering. The vehicle movement is too quick.
  //pid.Init(0.12, 0.001, 1.0); // [Tri#7] It is middle of Kp=0.1 and Kp=0.15. In straight, it needs to be more smooth. In curve, it should steer quickly.
  pid.Init(0.08, 0.001, 1.0); // [Tri#8] Set the initial gain that prioritize smoothness, and make it bigger when the deviation is big (especially in curvve).

  pid.p_error = 0.0;
  pid.i_error = 0.0;
  pid.d_error = 0.0;

  pid.prev_cte = 0.0;

  pid.count = 1;

  h.onMessage([&pid](uWS::WebSocket<uWS::SERVER> ws, char *data, size_t length, 
                     uWS::OpCode opCode) {
    // "42" at the start of the message means there's a websocket message event.
    // The 4 signifies a websocket message
    // The 2 signifies a websocket event

    if (length && length > 2 && data[0] == '4' && data[1] == '2') {
      auto s = hasData(string(data).substr(0, length));

      if (s != "") {
        auto j = json::parse(s);

        string event = j[0].get<string>();

        if (event == "telemetry") {
          // j[1] is the data JSON object
          double cte = std::stod(j[1]["cte"].get<string>());
          double speed = std::stod(j[1]["speed"].get<string>());
          double angle = std::stod(j[1]["steering_angle"].get<string>());
          double steer_value;
          /**
           * TODO: Calculate steering value here, remember the steering value is
           *   [-1, 1].
           * NOTE: Feel free to play around with the throttle and speed.
           *   Maybe use another PID controller to control the speed!
           */

          // [Step#1] Adaptive Gain
          // Purpose: To drive smoothly, Kp should be smaller than 0.1.
          //          However, Kp should be bigger than 0.1 to avoid overshoot at curve.
          //          So, I change PID gains according to the deviation from the center.

          // Set the max gain when there's big overshoot at curve.
          // Value is twice as big as min gain.
          pid.Kp_max = 0.16;
          pid.Ki_max = 0.002;
          pid.Kd_max = 2.0;

          // Change the gain proportionally according to the deviation: cte.
          // If cte is smaller than 0.2[m], min gain. If cte is bigger than 1.2[m], max gain.
          double cte_min = 0.2;
          double cte_max = 1.2;

          pid.Kp = (pid.Kp_max - pid.Kp_min)/(cte_max - cte_min) * (std::abs(cte) - cte_min) + pid.Kp_min;
          pid.Ki = (pid.Ki_max - pid.Ki_min)/(cte_max - cte_min) * (std::abs(cte) - cte_min) + pid.Ki_min;
          pid.Kd = (pid.Kd_max - pid.Kd_min)/(cte_max - cte_min) * (std::abs(cte) - cte_min) + pid.Kd_min;
  
          if(std::abs(cte) < cte_min){
            pid.Kp = pid.Kp_min;
            pid.Ki = pid.Ki_min;
            pid.Kd = pid.Kd_min;
          }
          if(std::abs(cte) > cte_max){
            pid.Kp = pid.Kp_max;
            pid.Ki = pid.Ki_max;
            pid.Kd = pid.Kd_max;
          }

          // [Step#2] Calculate steering angle by PID Controller
          pid.p_error = cte;
          pid.d_error = cte - pid.prev_cte;
          pid.prev_cte = cte;
          pid.i_error += cte;
          
          steer_value = -pid.Kp*pid.p_error -pid.Kd*pid.d_error -pid.Ki*pid.i_error;
          if(steer_value >= 1.0){
            steer_value = 1.0;
          } else if(steer_value <= -1.0){
            steer_value = -1.0;
          }

          pid.count ++;

          std::cout << "Kp: " << pid.Kp << " Kd: " << pid.Kd 
                    << " Ki: " << pid.Ki << std::endl;

          std::cout << "P error: " << pid.p_error << " D error: " << pid.d_error 
                    << " I error: " << pid.i_error << std::endl;

          // DEBUG
          std::cout << "CTE: " << cte << " Steering Value: " << steer_value 
                    << std::endl;

          std::cout << "------------------------------"  << std::endl;

          json msgJson;
          msgJson["steering_angle"] = steer_value;
          msgJson["throttle"] = 0.3;
          auto msg = "42[\"steer\"," + msgJson.dump() + "]";
          std::cout << msg << std::endl;
          ws.send(msg.data(), msg.length(), uWS::OpCode::TEXT);
        }  // end "telemetry" if
      } else {
        // Manual driving
        string msg = "42[\"manual\",{}]";
        ws.send(msg.data(), msg.length(), uWS::OpCode::TEXT);
      }
    }  // end websocket message if
  }); // end h.onMessage

  h.onConnection([&h](uWS::WebSocket<uWS::SERVER> ws, uWS::HttpRequest req) {
    std::cout << "Connected!!!" << std::endl;
  });

  h.onDisconnection([&h](uWS::WebSocket<uWS::SERVER> ws, int code, 
                         char *message, size_t length) {
    ws.close();
    std::cout << "Disconnected" << std::endl;
  });

  int port = 4567;
  if (h.listen(port)) {
    std::cout << "Listening to port " << port << std::endl;
  } else {
    std::cerr << "Failed to listen to port" << std::endl;
    return -1;
  }
  
  h.run();
}