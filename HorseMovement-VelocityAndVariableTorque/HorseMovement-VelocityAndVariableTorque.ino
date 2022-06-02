 /* Requirements:
  *  
 * 1. A ClearPath motor must be connected to Connector M-0 and M-1
 * 
 * 2. The connected ClearPath motor must be configured through the MSP software
 *    for Follow Digital Velocity Command, Bipolar PWM Command with Variable
 *    Torque mode (In MSP select Mode>>Velocity>>Follow Digital Velocity
 *    Command, then with "Bipolar PWM Command w/ Variable Torque")
 *    
 * 3. The ClearPath must have a defined Max Speed configured through the MSP
 *    software (On the main MSP window fill in the "Max Speed (RPM)" box with
 *    your desired maximum speed). Ensure the value of maxSpeed below matches
 *    this Max Speed. Max Speed of motors is 840
 *    
 * 4. Set the PWM Deadband in MSP to 1.
 * 
 * 5. In MSP, ensure the two checkboxes for "Invert Torque PWM Input" and
 *    "Invert Speed PWM Input" are unchecked.
 *    
 * 6. A primary Torque Limit and Alternate Torque Limit must be defined using
 *    the Torque Limit setup window through the MSP software (To configure,
 *    click the "Setup..." button found under the "Torque Limit" label. Then
 *    fill in the textbox labeled "Alt Torque Limit (% of max)" and hit the
 *    Apply button). Use only symmetric limits. These limits must match the
 *    "torqueLimit" and "torqueLimitAlternate" variables defined below. Torque
 *    limit should be 100% and alternate should be 5% for homing.
 *    
 * 7. The connected ClearPath motor must have its HLFB mode set to ASG with
 *    measured torque through the MSP software (select Advanced>>High Level
 *    Feedback [Mode]... then choose "ASG-Position, w/Measured Torque" or
 *    "ASG-Velocity, w/ Measured Torque" and hit the OK button).
 *    Select a 482 Hz PWM Carrier Frequency in this menu.
 *
  */

//NOTE:
//POSITIVE RPM = CCW Rotation
//NEGATIVE RPM = CW Rotation
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
double torqueLimitAlternate = 5.0;

// A PWM deadband of 2% prevents signal jitter from effecting a 0 RPM command
// (must match MSP value)
double pwmDeadBand = 1.0;

int idxL;
int idxR;
int counter;
//0 = off
int state;
int ramp;
float threshold;
uint32_t onTimeL;
uint32_t onTimeR;

//max index of time model
#define TIME_SERIES_MAX_IDX 4

// RPM of L and right motor from model
//long modelL[] = {50, 100, 50, -50, -100, -50, 25, -25, 25, -25};
//long modelR[] = {25, -25, 25, -25, -50, -100, -50, 50, 100, 50};
////duration in MS of corresponding RMP idx
//long modelTimeL[] = {300, 400, 300, 400, 400, 300, 200, 200, 200, 200};
//long modelTimeR[] = {200, 200, 200, 200, 300, 400, 200, 300, 400, 300};

long modelL[] = {-55, 50, -55, 50};
long modelR[] = {-50, 50, -50, 50};
long modelTimeL[] = {500, 500, 500, 500};
long modelTimeR[] = {500, 500, 500, 500};

// Called once on ClearCore boot
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
    //Set torque to limit 100% and command velocity to 0
    motorR.MotorInBDuty(0);
    motorL.MotorInBDuty(0);
    LimitTorque(100);
    //Set up intial variables
    idxL = 0;
    idxR = 0;
    counter = 0;
    state = -1;
    ramp = 0;
    threshold = 0;
}


void loop() {
    // Read the voltage on the analog sensor (0-10V).
    if(state == -1){
//        Serial.println("Starting homing");
//        Homing();
//        Serial.println("Homing complete");
        state = 0;
    }
    //Checks Serial for new message
    String serialIn = CheckSerial();
    // Serial available and not in EM STOP MODE
    if(serialIn != "" && state != 2){
      //Set to start ramping up horse to full speed 
      if(serialIn == "on_h"){
        state = 1;
        ramp = 1;
        threshold = 1;
        onTimeR = millis();
        onTimeL = millis();
      }
      // Set to start ramping down horse
      else if(serialIn == "off_h"){
        state = 0;
      }
      // Set to 
      else if(serialIn == "em_stop"){
        state = 2;
        threshold = 0;
        motorL.EnableRequest(false);
        motorR.EnableRequest(false);
      }
      //Print status of horse to Serial
      else if(serialIn == "stat_h"){
        PrintStatus();
      }
      //manual test of homing sequence
      else if(serialIn == "home_test"){
        Serial.println("Starting homing test");
        Homing();
        Serial.println("Homing test complete");
      }
      else if (serialIn == "enable"){
        motorL.EnableRequest(true);
        motorR.EnableRequest(true);
      }
      else if (serialIn == "disable"){
        motorL.EnableRequest(false);
        motorR.EnableRequest(false);
      }
    }
    // Ramp up threshold to provide smooth start to movement
    if(state == 1 && threshold < 1){
      threshold += .01;
    }
    // Ramp down threshold to provide smooth stop to movement
    if(state == 0 && threshold > 0){
      threshold -= .01;
    }

    // If horse on or off in ramp down
    if(state == 1 || (state == 0 && threshold > 0)){
       if(millis() - onTimeR > modelTimeR[idxR]){
        idxR += 1;
        onTimeR = millis();
        
      }
      // IDX rap around
      if(idxR > TIME_SERIES_MAX_IDX){
        idxR = 0;
      }

      if(millis() - onTimeL > modelTimeL[idxL]){
        idxL += 1;
        onTimeL = millis();
        
      }
      // IDX rap around
      if(idxL > TIME_SERIES_MAX_IDX){
        idxL = 0;
      }
      

      // new RPM with threshold factored
      long commandedVelocityR =
          static_cast<int32_t>(round(modelR[idxR] * threshold));
      long commandedVelocityL =
          static_cast<int32_t>(round(modelL[idxL] * threshold));
  
      // Move at the commanded velocity.
      CommandVelocityL(commandedVelocityL);
      CommandVelocityR(commandedVelocityR);
    }
    // If off hold motors in position
    else{
      motorR.MotorInBDuty(0);
      motorL.MotorInBDuty(0);
    }
}

/*------------------------------------------------------------------------------
 * CommandVelocity
 *
 *    Command the motor to move using a velocity of commandedVelocity
 *
 * Parameters:
 *    int commandedVelocity  - The velocity to command in rpm
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
        motorR.MotorInBDuty(0);
        return false;
    }

    // If there is a deadband defined, the range of the PWM scale is reduced.
    double rangeUnsigned = 127.5 - (pwmDeadBand / 100 * 255);

    // Find the scaling factor of our velocity range mapped to the PWM duty cycle
    // range (the PWM to the ClearPath is bipolar, so the range starts at a 50%
    // duty cycle).
    double scaleFactor = rangeUnsigned / maxSpeed;


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
        motorL.MotorInBDuty(0);
        return false;
    }

    // If there is a deadband defined, the range of the PWM scale is reduced.
    double rangeUnsigned = 127.5 - (pwmDeadBand / 100 * 255);

    // Find the scaling factor of our velocity range mapped to the PWM duty cycle
    // range (the PWM to the ClearPath is bipolar, so the range starts at a 50%
    // duty cycle).
    double scaleFactor = rangeUnsigned / maxSpeed;


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

//Checks Serial for new messages from Raspberry pi
String CheckSerial(){
    if (Serial.available() > 0) {
      String data = Serial.readStringUntil('\n');
      Serial.print("You sent me: ");
      Serial.println(data);
      return data;
    }
    return "";
}

//Read HLFB of right motor, returns percent of torque or 0 if no torque read
double ReadHlfbRight(){
    // Check the current state of the ClearPath's HLFB.
    MotorDriver::HlfbStates hlfbState = motorR.HlfbState();

    // Write the HLFB state to the serial port
    if (hlfbState == MotorDriver::HLFB_HAS_MEASUREMENT) {
        // Writes the torque measured, as a percent of motor peak torque rating
        return motorR.HlfbPercent();
    }
    else {
        return 0;
    }
}
//Read HLFB of left motor, returns percent of torque or 0 if no torque read
double ReadHlfbLeft(){
    // Check the current state of the ClearPath's HLFB.
    MotorDriver::HlfbStates hlfbState = motorL.HlfbState();

    if (hlfbState == MotorDriver::HLFB_HAS_MEASUREMENT) {
        // Writes the torque measured, as a percent of motor peak torque rating
        return motorL.HlfbPercent();
    }
    else {
        return 0;
    }
}

//Homing both motors to center 
void Homing(){

  bool toCenterL = false;
  bool toCenterR = false;
  
  bool nDoneL = true;
  bool nDoneR = true;
  
  //limit torque to 10% of max
  LimitTorque(10);

  //slowly rotate motors outward
  CommandVelocityL(-32);
  CommandVelocityR(32);
  
  while (nDoneL || nDoneR){
    
//    if(toCenterL && ReadHlfbLeft() < 2){
//      toCenterL = false;
//      Serial.print("L CENTER: ");
//      Serial.println(ReadHlfbLeft());
//    }
//    
//    if(toCenterR && ReadHlfbRight() < 2){
//      toCenterR = false;
//      Serial.print("R CENTER: ");
//      Serial.println(ReadHlfbRight());
//    }
//    if (toCenterR && ReadHlfbRight() < 2.5){
//      Serial.println(ReadHlfbRight());
//    }
    Serial.println(ReadHlfbLeft());
    if(nDoneL && !toCenterL && ReadHlfbLeft() < -2){
      CommandVelocityL(0);
      Serial.print("left stopped: ");
      Serial.println(ReadHlfbLeft());
      nDoneL = false;
    }
    if(nDoneR && !toCenterR && ReadHlfbRight() > 2){
      CommandVelocityR(0);
      Serial.print("right stopped: ");
      Serial.println(ReadHlfbRight());
      nDoneR = false;
    }
//    if(nDoneL && !toCenterL){
//      Serial.println(ReadHlfbLeft());
//    }
  }

  //start spinning motors back and reset torque delay to upright and stop
  CommandVelocityL(128);
  CommandVelocityR(-128);
  LimitTorque(100);
  delay(1500);
  CommandVelocityL(0);
  CommandVelocityR(0);
}

// Prints current status of horse to Serial
void PrintStatus(){
  if(state == 1 && threshold < 1){
    Serial.println("Horse is on and ramping to full speed");
    return;
  }
  if(state == 1 && threshold == 1){
    Serial.println("Horse is on and at full speed");
    return;
  }
  if(state == 0 && threshold > 0){
    Serial.println("Horse is ramping down to a stop");
    return;
  }
  if(state == 0 && threshold == 0){
    Serial.println("Horse is off and ready");
    return;
  }
  if(state == 2){
    Serial.println("Emergency Stop was triggered on Horse, please cycle power on horse when safe to do so");
    return;
  }
}

//------------------------------------------------------------------------------
