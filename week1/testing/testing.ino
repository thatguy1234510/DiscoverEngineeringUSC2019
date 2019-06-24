// THIS IS BASICALLY A DRY RUN OF THE CODE PRINTED OVER SERIAL
#include <RedBot.h>

RedBotMotors motors;

//TODO: test pins
RedBotSensor left = RedBotSensor(A3);   // initialize a left sensor object on A3
RedBotSensor center = RedBotSensor(A4); // initialize a center sensor object on A4
RedBotSensor right = RedBotSensor(A5);  // initialize a right sensor object on A5
#define LINETHRESHOLD 950

RedBotBumper lBumper = RedBotBumper(3); // initialzes bumper object on pin 4
RedBotBumper rBumper = RedBotBumper(5); // initialzes bumper object on pin 6
int prevTouches[] = {1, 1};
int lTouch = 1;
int rTouch = 1;

//Ultrasonic Pins
const int TRIG_PIN = 2;
const int ECHO_PIN = A1;

// Anything over ~160 inches (23200 us pulse) is "out of range" or -1
const unsigned int MAX_DIST = 23200;
const long timeout  = 25000;

void setupUltrasonic(){
    // The Trigger pin will tell the sensor to range find
    pinMode(TRIG_PIN, OUTPUT);
    digitalWrite(TRIG_PIN, LOW);
    pinMode(ECHO_PIN, INPUT);
}

//returns in inches
int readUltrasonic(){
    unsigned long t1;
    unsigned long t2;
    unsigned long pulse_width;
    float cm;
    float inches;

    // Hold the trigger pin high for at least 10 us
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    
    pulse_width = pulseIn(ECHO_PIN, HIGH);

    // Calculate distance in centimeters and inches. The constants
    // are found in the datasheet, and calculated from the assumed speed
    //of sound in air at sea level (~340 m/s).
    //cm = pulse_width / 58.0;
    inches = pulse_width / 148.0;

    // Print out results
    if (pulse_width > MAX_DIST)
    {
        return -1;
    }
    else
    {
        return inches;
    }
}
// Wait at least 60ms before next measurement
//delay(60);

void setup()
{
    setupUltrasonic();
    Serial.begin(9600);
    Serial.println("hello world");
    motors.pivot(150);
}

void loop()
{
    delay(200);
    Serial.println("\nUltra distance:" + String(readUltrasonic()));
    lTouch = lBumper.read();
    rTouch = rBumper.read();

    if (lTouch != prevTouches[0] || rTouch != prevTouches[1])
    {
        Serial.println("\nlBumper: " + String(lTouch) + "\t rBumper: " + String(rTouch));
        prevTouches[0] = lTouch;
        prevTouches[1] = rTouch;
    }

    int lread = left.read();
    int cread = center.read();
    int rread = right.read();
    //Serial.println(String(lread)+" "+String(cread)+" "+String(rread));

    // if all sensors are on black or up in the air, stop the motors:
    if ((lread > LINETHRESHOLD) && (cread > LINETHRESHOLD) && (rread > LINETHRESHOLD))
    {
        Serial.println("bot up");
    }
    // all motors not on black:
    else if (((lread < LINETHRESHOLD) && (cread < LINETHRESHOLD) && (rread < LINETHRESHOLD)))
    {
        Serial.println("bot lost");
        //spiral
    }
    //center
    else if (cread > LINETHRESHOLD)
    {
        Serial.println("bot going");
    }
    //right
    else if (rread > LINETHRESHOLD)
    {
        Serial.println("bot pivoting right");
    }
    //left
    else if (lread > LINETHRESHOLD)
    {
        Serial.println("bot pivoting left");
    }

    
}
