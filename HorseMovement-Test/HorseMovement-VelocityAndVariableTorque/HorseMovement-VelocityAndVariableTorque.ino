 /* Requirements:
 * 1. A ClearPath motor must be connected to Connector M-0.
 * 2. The connected ClearPath motor must be configured through the MSP software
 *    for Follow Digital Velocity Command, Bipolar PWM Command with Variable
 *    Torque mode (In MSP select Mode>>Velocity>>Follow Digital Velocity
 *    Command, then with "Bipolar PWM Command w/ Variable Torque")
 * 3. The ClearPath must have a defined Max Speed configured through the MSP
 *    software (On the main MSP window fill in the "Max Speed (RPM)" box with
 *    your desired maximum speed). Ensure the value of maxSpeed below matches
 *    this Max Speed.
 * 4. Set the PWM Deadband in MSP to 2.
 * 5. In MSP, ensure the two checkboxes for "Invert Torque PWM Input" and
 *    "Invert Speed PWM Input" are unchecked.
 * 6. A primary Torque Limit and Alternate Torque Limit must be defined using
 *    the Torque Limit setup window through the MSP software (To configure,
 *    click the "Setup..." button found under the "Torque Limit" label. Then
 *    fill in the textbox labeled "Alt Torque Limit (% of max)" and hit the
 *    Apply button). Use only symmetric limits. These limits must match the
 *    "torqueLimit" and "torqueLimitAlternate" variables defined below.
 * 7. The connected ClearPath motor must have its HLFB mode set to ASG with
 *    measured torque through the MSP software (select Advanced>>High Level
 *    Feedback [Mode]... then choose "ASG-Position, w/Measured Torque" or
 *    "ASG-Velocity, w/ Measured Torque" and hit the OK button).
 *    Select a 482 Hz PWM Carrier Frequency in this menu.
 *
  */

#include "ClearCore.h"

// Defines the motor's connector as ConnectorM0
#define motorL ConnectorM0
#define motorR ConnectorM1

// Select the baud rate to match the target device.
#define baudRate 9600

// This is the commanded speed limit in RPM (must match the MSP value). This speed
// cannot actually be commanded, so use something slightly higher than your real
// max speed here and in MSP.
double maxSpeed = 845;

// Defines the default torque limit and the alternate torque limit
// (must match MSP values)
double torqueLimit = 100.0;
double torqueLimitAlternate = 10.0;

// A PWM deadband of 2% prevents signal jitter from effecting a 0 RPM command
// (must match MSP value)
double pwmDeadBand = 2.0;

int idx;
int counter;
//0 = off
int state;
int ramp;
float threshold;
uint32_t onTime;


//long fakeVolt[] = {1, -1, 2, -2, 3, -3, 5, -5, 6, -6, 10, -10};
long fakeVolt[] = {9.9, -9.9, 9.9, -9.9, 9.9, -9.9, 9.9, -9.9, 9.9, -9.9, 9.9, -9.9};
long modelL[] = {200, -200};
long modelR[] = {-200, 200};
long modelTime[] = {500, 500};

void setup() {
   
    // Sets all motor connectors to the correct mode for Follow Digital
    // Velocity, Bipolar PWM mode.
    MotorMgr.MotorModeSet(MotorManager::MOTOR_ALL,
                          Connector::CPM_MODE_A_PWM_B_PWM);
    // Put the motor connector into the HLFB mode to read bipolar PWM (the
    // correct mode for ASG w/ Measured Torque)
    motorL.HlfbMode(MotorDriver::HLFB_MODE_HAS_BIPOLAR_PWM);
    motorR.HlfbMode(MotorDriver::HLFB_MODE_HAS_BIPOLAR_PWM);
    // Set the HFLB carrier frequency to 482 Hz
    motorL.HlfbCarrier(MotorDriver::HLFB_CARRIER_482_HZ);
    motorR.HlfbCarrier(MotorDriver::HLFB_CARRIER_482_HZ);
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
        threshold = 0.001;
        onTime = millis();
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
      threshold += .01;
      Serial.print("threshold: ");
      Serial.println(threshold);
      
    }
    if(state == 0 && threshold > 0){
      threshold -= .01;
    }
    
    if(state == 1 || (state == 0 && threshold > 0)){
       if(millis() - onTime > modelTime[idx]){
        idx += 1;
        onTime = millis();
      }
      if(idx > TIME_SERIES_MAX_IDX){
        idx = 0;
      }
      

      // Convert the voltage measured to a velocity within the valid range.
      long commandedVelocityR =
          static_cast<int32_t>(round(modelR[idx] * threshold));
      long commandedVelocityL =
          static_cast<int32_t>(round(modelL[idx] * threshold));
  
      // Move at the commanded velocity.
      Serial.println("Sending velocity");
      Serial.println(commandedVelocityR);
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
        Serial.println("Move rejected, requested velocity at or over the limit. Motor right");
        return false;
    }

    // Check if an alert is currently preventing motion
    if (motorR.StatusReg().bit.AlertsPresent) {
        Serial.println("Motor right status: 'In Alert'. Move Canceled.");
        return false;
    }

    // If there is a deadband defined, the range of the PWM scale is reduced.
    double rangeUnsigned = 127.5 - (pwmDeadBand / 100 * 255);

    // Find the scaling factor of our velocity range mapped to the PWM duty cycle
    // range (the PWM to the ClearPath is bipolar, so the range starts at a 50%
    // duty cycle).
    double scaleFactor = rangeUnsigned / maxVelocity;


    // Scale the velocity command to our duty cycle range.
    double dutyRequest;
    if (commandedVelocity < 0) {
        dutyRequest = 127.5 - (pwmDeadBand / 100 * 255)
                      + (commandedVelocity * scaleFactor);
    }
    else if (commandedVelocity > 0) {
        dutyRequest = 127.5 + (pwmDeadBand / 100 * 255)
                      + (commandedVelocity * scaleFactor);
    }
    else {
        dutyRequest = 128.0;
    }
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

    // If there is a deadband defined, the range of the PWM scale is reduced.
    double rangeUnsigned = 127.5 - (pwmDeadBand / 100 * 255);

    // Find the scaling factor of our velocity range mapped to the PWM duty cycle
    // range (the PWM to the ClearPath is bipolar, so the range starts at a 50%
    // duty cycle).
    double scaleFactor = rangeUnsigned / maxVelocity;


    // Scale the velocity command to our duty cycle range.
    double dutyRequest;
    if (commandedVelocity < 0) {
        dutyRequest = 127.5 - (pwmDeadBand / 100 * 255)
                      + (commandedVelocity * scaleFactor);
    }
    else if (commandedVelocity > 0) {
        dutyRequest = 127.5 + (pwmDeadBand / 100 * 255)
                      + (commandedVelocity * scaleFactor);
    }
    else {
        dutyRequest = 128.0;
    }
    // Command the move.
    motorL.MotorInBDuty(dutyRequest);

    return true;
}

//Limits torque of both motors 
bool LimitTorque(double limit) {
    if (limit > torqueLimit || limit < torqueLimitAlternate) {
        Serial.println("Torque limiting rejected, invalid torque requested.");
        return false;
    }
    Serial.print("Limit torque to: ");
    Serial.print(limit);
    Serial.println("%.");

    // Find the scaling factor of our torque range mapped to the PWM duty cycle
    // range (255 is the max duty cycle).
    double scaleFactor = 255 / (torqueLimit - torqueLimitAlternate);

    // Scale the torque limit command to our duty cycle range.
    int dutyRequest = (torqueLimit - limit) * scaleFactor;

    // Command the new torque limit.
    motorL.MotorInADuty(dutyRequest);
    motorR.MotorInADuty(dutyRequest);

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

int ReadHlbfRight(){
    // Check the current state of the ClearPath's HLFB.
    MotorDriver::HlfbStates hlfbState = motorR.HlfbState();

    // Write the HLFB state to the serial port
    if (hlfbState == MotorDriver::HLFB_HAS_MEASUREMENT) {
        // Writes the torque measured, as a percent of motor peak torque rating
        return int(round(motorR.HlfbPercent()));
    }
    else {
        return 0;
    }
}

int ReadHlbfLeft(){
    // Check the current state of the ClearPath's HLFB.
    MotorDriver::HlfbStates hlfbState = motorL.HlfbState();

    // Write the HLFB state to the serial port
    if (hlfbState == MotorDriver::HLFB_HAS_MEASUREMENT) {
        // Writes the torque measured, as a percent of motor peak torque rating
        return int(round(motorL.HlfbPercent()));
    }
    else {
        return 0;
    }
}

//------------------------------------------------------------------------------
