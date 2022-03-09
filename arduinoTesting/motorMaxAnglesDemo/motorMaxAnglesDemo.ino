
#define motor1Pin1 22
#define motor1Pin2 24

#define motor2Pin1 26
#define motor2Pin2 28




void setup() {
  pinMode(motor1Pin1, OUTPUT);
  pinMode(motor1Pin2, OUTPUT);
  pinMode(motor2Pin1, OUTPUT);
  pinMode(motor2Pin2, OUTPUT);

}

void loop() {
 delay(5000);
 //Pitch up to full
 digitalWrite(motor2Pin2, HIGH);
 digitalWrite(motor1Pin1, HIGH);
 delay(80);
 digitalWrite(motor2Pin2, LOW);
 digitalWrite(motor1Pin1, LOW);
 
 delay(1000);
  //pitch down to level
 digitalWrite(motor2Pin1, HIGH);
 digitalWrite(motor1Pin2, HIGH);
 delay(80);
 digitalWrite(motor2Pin1, LOW);
 digitalWrite(motor1Pin2, LOW);

delay(1000);
 //pitch down full
 digitalWrite(motor2Pin1, HIGH);
 digitalWrite(motor1Pin2, HIGH);
 delay(80);
 digitalWrite(motor2Pin1, LOW);
 digitalWrite(motor1Pin2, LOW);
 
delay(1000);

 //Pitch up to level
 digitalWrite(motor2Pin2, HIGH);
 digitalWrite(motor1Pin1, HIGH);
 delay(80);
 digitalWrite(motor2Pin2, LOW);
 digitalWrite(motor1Pin1, LOW);

delay(1000);
 //Roll

 digitalWrite(motor2Pin2, HIGH);
 digitalWrite(motor1Pin2, HIGH);
 delay(80);
 digitalWrite(motor2Pin2, LOW);
 digitalWrite(motor1Pin2, LOW);

 delay(1000);

 //Roll back to center

 digitalWrite(motor2Pin1, HIGH);
 digitalWrite(motor1Pin1, HIGH);
 delay(80);
 digitalWrite(motor2Pin1, LOW);
 digitalWrite(motor1Pin1, LOW);
 
 //Roll
 delay(1000);
 digitalWrite(motor2Pin1, HIGH);
 digitalWrite(motor1Pin1, HIGH);
 delay(80);
 digitalWrite(motor2Pin1, LOW);
 digitalWrite(motor1Pin1, LOW);

 delay(1000);

 //Roll back to center

 digitalWrite(motor2Pin2, HIGH);
 digitalWrite(motor1Pin2, HIGH);
 delay(80);
 digitalWrite(motor2Pin2, LOW);
 digitalWrite(motor1Pin2, LOW);
}
