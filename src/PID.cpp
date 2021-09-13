#include "PID.h"

/**
 * TODO: Complete the PID class. You may add any additional desired functions.
 */

PID::PID() {}

PID::~PID() {}

void PID::Init(double Kp_, double Ki_, double Kd_) {
  /**
   * TODO: Initialize PID coefficients (and errors, if needed)
   */
  this->Kp = Kp_;
  this->Ki = Ki_;
  this->Kd = Kd_;
  
  this->Kp_min = Kp_;
  this->Ki_min = Ki_;
  this->Kd_min = Kd_;

}

void PID::UpdateError(double cte) {
  /**
   * TODO: Update PID errors based on cte.
   */

  /*I didnt't use this function.*/

}

double PID::TotalError() {
  /**
   * TODO: Calculate and return the total error
   */

  /*I didnt't use this function.*/
  
  return 0.0;  // TODO: Add your total error calc here!
}