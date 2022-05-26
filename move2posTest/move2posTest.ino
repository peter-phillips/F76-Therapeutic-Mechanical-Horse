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

#define INPUT_A_B_FILTER 5

int state;
uint32_t onTimeL = 0;
uint32_t onTimeR = 0;

int Lpos = 1;
int Rpos = 1;

int switchTime = 2000;


// Called once on ClearCore boot
void setup() {
   
    // Sets all motor connectors to the correct mode for Follow Digital
    // Velocity, Bipolar PWM mode.
    MotorMgr.MotorModeSet(MotorManager::MOTOR_ALL,
                          Connector::CPM_MODE_A_DIRECT_B_DIRECT);
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
}


void loop() {
    // Read the voltage on the analog sensor (0-10V).
    if(state == -1){
        Serial.println("Starting homing");
        Homing();
        Serial.println("Homing complete");
        state = 0;
    }
    //Checks Serial for new message
    String serialIn = CheckSerial();
    // Serial available and not in EM STOP MODE
    if(serialIn != "" && state != 2){
      //Set to start ramping up horse to full speed 
      if(serialIn == "on_h"){
        state = 1;
        onTimeL = millis();
        onTimeR = millis() + 500;
      }
      // Set to start ramping down horse
      else if(serialIn == "off_h"){
        state = 0;
      }
      // Set to 
      else if(serialIn == "em_stop"){
        state = 2;
        motorL.EnableRequest(false);
        motorR.EnableRequest(false);
      }
      //Print status of horse to Serial
      else if(serialIn == "stat_h"){
        PrintStatus();
      }
      //manual test of homing sequence
      else if(serialIn == "home_test"){
        motorL.EnableRequest(false);
        motorR.EnableRequest(false);
        Serial.println("Starting homing test");
        Homing();
        Serial.println("Homing test complete");
      }
    }

    // If horse on or off in ramp down
    if(state == 1){
      if(millis() - onTimeL > switchTime){
        MoveToPositionL();
      }
      if(millis() - onTimeR > switchTime){
        MoveToPositionR() ;
      }
    }
}


bool MoveToPositionR() {
    // Check if an alert is currently preventing motion
    if (motorR.StatusReg().bit.AlertsPresent) {
        Serial.println("Motor status: 'In Alert'. Move Canceled.");
        return false;
    }

      int positionNum;

    if (Rpos == 1){
      positionNum = 2;
    }
    else{
      positionNum = 1;
    }

    switch (positionNum) {
        case 1:
            // Sets Input A "off" for position 1
            motorR.MotorInAState(false);
            break;
        case 2:
            // Sets Input A "on" for position 2
            motorR.MotorInAState(true);
            break;
        default:
            // If this case is reached then an incorrect positionNum was entered
            return false;
    }

    // Ensures this delay is at least 2ms longer than the Input A, B filter
    // setting in MSP
    delay(2 + INPUT_A_B_FILTER);
    return true;
}

bool MoveToPositionL() {
    // Check if an alert is currently preventing motion
    if (motorR.StatusReg().bit.AlertsPresent) {
        Serial.println("Motor status: 'In Alert'. Move Canceled.");
        return false;
    }
    int positionNum;

    if (Lpos == 1){
      positionNum = 2;
    }
    else{
      positionNum = 1;
    }

    switch (positionNum) {
        case 1:
            // Sets Input A "off" for position 1
            motorL.MotorInAState(false);
            break;
        case 2:
            // Sets Input A "on" for position 2
            motorL.MotorInAState(true);
            break;
        default:
            // If this case is reached then an incorrect positionNum was entered
            return false;
    }

    // Ensures this delay is at least 2ms longer than the Input A, B filter
    // setting in MSP
    delay(2 + INPUT_A_B_FILTER);
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
        double percent = motorR.HlfbPercent();
        return percent;
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
        double percent = motorL.HlfbPercent();
        return percent;
    }
    else {
        return 0;
    }
}

//Homing both motors to center 
void Homing(){

//  bool toCenterL = true;
//  bool toCenterR = true;
  
  bool nDoneL = true;
  bool nDoneR = true;

    motorL.EnableRequest(true);
    motorR.EnableRequest(true);
  
  //limit torque to 5% of max
//  LimitTorque(5);

  //slowly rotate motors inward
//  CommandVelocityL(-16);
//  CommandVelocityR(16);
  
  while (nDoneL || nDoneR){
    if(nDoneL && ReadHlfbLeft() < -3){
      Serial.print("left stopped");
      Serial.println(ReadHlfbRight());
      motorL.MotorInBState(true);
      nDoneL = false;
    }
    if(nDoneR && ReadHlfbRight() > 5){
      Serial.print("right stopped");
      Serial.println(ReadHlfbRight());
      motorR.MotorInBState(true);
      nDoneR = false;
    }
  }
}

// Prints current status of horse to Serial
void PrintStatus(){
  if(state == 1){
    Serial.println("Horse is on and at full speed");
    return;
  }
  if(state == 0){
    Serial.println("Horse is off and ready");
    return;
  }
  if(state == 2){
    Serial.println("Emergency Stop was triggered on Horse, please cycle power on horse when safe to do so");
    return;
  }
}

//------------------------------------------------------------------------------
