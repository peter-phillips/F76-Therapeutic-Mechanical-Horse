/* 
 * Title: FollowDigitalVelocity
 *
 * Objective:
 *    This example demonstrates control of the ClearPath-MC operational mode
 *    Follow Digital Velocity Command, Unipolar PWM Command.
 *
 * Description:
 *    This example enables a ClearPath motor and executes velocity moves based
 *    on the state of an analog input sensor. During operation, various move
 *    statuses are written to the USB serial port.
 *    Consider using Manual Velocity Control mode instead if you do not wish to 
 *    use an analog sensor to command velocity, if you require greater velocity 
 *    command resolution (i.e. more commandable positions), or if HLFB is needed 
 *    for "move done/at speed" status feedback.
 *
 * Requirements:
 * 1. A ClearPath motor must be connected to Connector M-0.
 * 2. The connected ClearPath motor must be configured through the MSP software
 *    for Follow Digital Velocity Command, Unipolar PWM Command mode (In MSP
 *    select Mode>>Velocity>>Follow Digital Velocity Command, then with
 *    "Unipolar PWM Command" selected hit the OK button).
 * 3. The ClearPath must have a defined Max Speed configured through the MSP
 *    software (On the main MSP window fill in the "Max Speed (RPM)" box with
 *    your desired maximum speed). Ensure the value of maxSpeed below matches
 *    this Max Speed.
 * 4. Ensure the "Invert PWM Input" checkbox found on the MSP's main window is
 *    unchecked.
 * 5. Ensure the Input A filter in MSP is set to 20ms, (In MSP
 *    select Advanced>>Input A, B Filtering... then in the Settings box fill in
 *    the textbox labeled "Input A Filter Time Constant (msec)" then hit the OK
 *    button).
 * 6. An analog input source (0-10V) connected to ConnectorA9 to control
 *    motor velocity.
 *
 * Links:
 * ** ClearCore Documentation: https://teknic-inc.github.io/ClearCore-library/
 * ** ClearCore Manual: https://www.teknic.com/files/downloads/clearcore_user_manual.pdf
 * ** ClearPath Manual (DC Power): https://www.teknic.com/files/downloads/clearpath_user_manual.pdf
 * ** ClearPath Manual (AC Power): https://www.teknic.com/files/downloads/ac_clearpath-mc-sd_manual.pdf
 * ** ClearPath Mode Informational Video: https://www.teknic.com/watch-video/#OpMode8
 *
 *
 * Copyright (c) 2020 Teknic Inc. This work is free to use, copy and distribute under the terms of
 * the standard MIT permissive software license which can be found at https://opensource.org/licenses/MIT
 */

#include "ClearCore.h"

// Defines the motor's connector as ConnectorM0
#define motorL ConnectorM0
#define motorR ConnectorM1


// The INPUT_A_FILTER must match the Input A filter setting in MSP
// (Advanced >> Input A, B Filtering...)
#define INPUT_A_FILTER 20

// Select the baud rate to match the target device.
#define baudRate 9600

// This is the commanded speed limit in RPM (must match the MSP value). This speed
// cannot actually be commanded, so use something slightly higher than your real
// max speed here and in MSP.
double maxSpeed = 845;

// Declares our user-defined helper function, which is used to command velocity 
// The definition/implementation of this function is at the bottom of
// this example.
bool CommandVelocity(long commandedVelocity);

int idx;
int counter;
//0 = off
int state;
int ramp;
long threshold;

//long fakeVolt[] = {1, -1, 2, -2, 3, -3, 5, -5, 6, -6, 10, -10};
long fakeVolt[] = {9.9, -9.9, 9.9, -9.9, 9.9, -9.9, 9.9, -9.9, 9.9, -9.9, 9.9, -9.9};

void setup() {
    // Put your setup code here, it will only run once:
   
    
    // Sets all motor connectors to the correct mode for Follow Digital
    // Velocity, Unipolar PWM mode.
    MotorMgr.MotorModeSet(MotorManager::MOTOR_ALL,
                          Connector::CPM_MODE_A_DIRECT_B_PWM);

    // Sets up serial communication and waits up to 5 seconds for a port to open.
    // Serial communication is not required for this example to run.
    Serial.begin(baudRate);
    uint32_t timeout = 5000;
    uint32_t startTime = millis();
    while (!Serial && millis() - startTime < timeout) {
        continue;
    }

    // Enables the motor
    motorL.EnableRequest(true);
    motorR.EnableRequest(true);
    Serial.println("Motor Enabled");
    idx = 0;
    counter = 0;
    state = 0;
    ramp = 0;
    threshold = 0;
}


void loop() {
    // Read the voltage on the analog sensor (0-10V).
    String serialIn = CheckSerial();
    if(serialIn != "" && state != 2){
      if(serialIn == "on_h"){
        state = 1;
        ramp = 1;
        threshold = 0;
      }
      else if(serialIn == "off_h"){
        state = 0;
      }
      else if(serialIn == "em_stop"){
        state = 2;
        threshold = 0;
        motorL.EnableRequest(false);
        motorR.EnableRequest(false);
      }
    }
    if(state == 1 && threshold < 1){
      threshold += .001;
    }
    if(state == 0 && threshold > 0){
      threshold -= .001;
    }
    
    if(state == 1 || (state == 0 && threshold > 0)){
      if(idx > 11){
        idx = 0;
      }
      float analogVoltage = fakeVolt[idx];
      if(counter >= 200){
        idx++;
        Serial.println(idx);
        counter = 0;
      }
      counter++;
      // Convert the voltage measured to a velocity within the valid range.
      long commandedVelocityR =
          static_cast<int32_t>(round(analogVoltage / 10 * maxSpeed * threshold));
      long commandedVelocityL =
          static_cast<int32_t>(round(analogVoltage / 10 * maxSpeed * threshold));
  
      // Move at the commanded velocity.
      CommandVelocityL(commandedVelocityR);
      CommandVelocityR(commandedVelocityL);
    }// See below for the detailed function definition.
    else{
      motorR.MotorInBDuty(0);
      motorL.MotorInBDuty(0);
    }
}

/*------------------------------------------------------------------------------
 * CommandVelocity
 *
 *    Command the motor to move using a velocity of commandedVelocity
 *    Prints the move status to the USB serial port
 *
 * Parameters:
 *    int commandedVelocity  - The velocity to command
 *
 * Returns: True/False depending on whether the velocity was successfully
 * commanded.
 */
bool CommandVelocityR(long commandedVelocity) {  
    if (abs(commandedVelocity) >= abs(maxSpeed)) {
        Serial.println("Move rejected, requested velocity at or over the limit. Motor Right");
        return false;
    }

    // Check if an alert is currently preventing motion
    if (motorR.StatusReg().bit.AlertsPresent) {
        Serial.println("Motor Right status: 'In Alert'. Move Canceled.");
        return false;
    }

    if (commandedVelocity >= 0) {
        motorR.MotorInAState(false);
    }
    else {
        motorR.MotorInAState(true);
    }
    // Delays to send the correct filtered direction.
    delay(2 + INPUT_A_FILTER);

    // Find the scaling factor of our velocity range mapped to the PWM duty
    // cycle range (255 is the max duty cycle).
    double scaleFactor = 255 / maxSpeed;

    // Scale the velocity command to our duty cycle range.
    int dutyRequest = abs(commandedVelocity) * scaleFactor;

    // Command the move.
    motorR.MotorInBDuty(dutyRequest);

    return true;
}

bool CommandVelocityL(long commandedVelocity) {  
    if (abs(commandedVelocity) >= abs(maxSpeed)) {
        Serial.println("Move rejected, requested velocity at or over the limit. Motor left");
        return false;
    }

    // Check if an alert is currently preventing motion
    if (motorL.StatusReg().bit.AlertsPresent) {
        Serial.println("Motor Left status: 'In Alert'. Move Canceled.");
        return false;
    }

    if (commandedVelocity >= 0) {
        motorL.MotorInAState(false);
    }
    else {
        motorL.MotorInAState(true);
    }
    // Delays to send the correct filtered direction.
    delay(2 + INPUT_A_FILTER);

    // Find the scaling factor of our velocity range mapped to the PWM duty
    // cycle range (255 is the max duty cycle).
    double scaleFactor = 255 / maxSpeed;

    // Scale the velocity command to our duty cycle range.
    int dutyRequest = abs(commandedVelocity) * scaleFactor;

    // Command the move.
    motorL.MotorInBDuty(dutyRequest);

    return true;
}

String CheckSerial(){
    if (Serial.available() > 0) {
      String data = Serial.readStringUntil('\n');
      Serial.print("You sent me: ");
      Serial.println(data);
      return data;
    }
    return "";
}

//------------------------------------------------------------------------------
