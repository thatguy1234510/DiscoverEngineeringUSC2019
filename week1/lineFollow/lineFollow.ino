#include <RedBot.h>

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


boolean lineFollowing = true;
boolean obstacleAvoiding = false;

RedBotMotors motors;
int MOTOR_POW = 150;
int PIVOT_SPEED = 150;
boolean STOP = false;
boolean RLflag = false; // right = true


RedBotSensor left = RedBotSensor(A3);   // initialize a left sensor object on A3
RedBotSensor center = RedBotSensor(A4); // initialize a center sensor object on A6
RedBotSensor right = RedBotSensor(A5);  // initialize a right sensor object on A7
#define LINETHRESHOLD 950

void setup()
{
    Serial.begin(9600);
    Serial.println("hello test"); // print "hello test" on the computer the arduino is plugged into 
    setupUltrasonic();
}

int ultraCountdown = 0; // only check ultra every 5 frames
int OBSTACLE_THRESH = 4;
int object_dist = 0;
boolean objectAhead = false;

RedBotBumper lBumper = RedBotBumper(3);  // initialzes bumper object on pin 3
RedBotBumper rBumper = RedBotBumper(5); // initialzes bumper object on pin 11
int lTouch = HIGH;
int rTouch = HIGH;

//linereader var inits
int lread;
int rread;
int cread;

//critical timing vars //TODO: tune all of these
int BACK_TIME = 1000;             //dist to move back //millis
int TURN_TIME = 500;              //angle of turn //millis
int DEG90_TIME = 700;             //TODO: TUNE 90deg! //millis
int DRIVE_FULL_BODY_TIME = 10000; //amount of time to drive fully past obstacle //millis
int INIT_SPIRAL_TIME = 1000;
int SPIRAL_TIME_INCREMENT = 1000;
void loop() 
{
    int lBumperState = lBumper.read(); // default INPUT state is HIGH, it is LOW when bumped
    int rBumperState = rBumper.read(); // default INPUT state is HIGH, it is LOW when bumped
    //normal state: bumpers not triggered
    if (lBumperState == HIGH && rBumperState == HIGH)
    {
        if (ultraCountdown % 5 == 0) // check ultra every 5 frames
        {
            object_dist = readUltrasonic();
            objectAhead = object_dist < OBSTACLE_THRESH && object_dist!=-1;
            ultraCountdown = (ultraCountdown + 1) % 5;
        }
        if (objectAhead) // if ultra finds obstacle
        {
            Serial.println("ultra detected object");
            lineFollowing = false;
            //rotate until obstacle out of vision
            int m1 = millis();
            while (objectAhead && millis()< m1+5000)
            {
                delay(60);
                object_dist = readUltrasonic();
                objectAhead = object_dist != -1 && object_dist < OBSTACLE_THRESH;
                ultraCountdown = (ultraCountdown + 1) % 5;
                motors.pivot(PIVOT_SPEED);
            }

            lBumperState = lBumper.read(); // default INPUT state is HIGH, it is LOW when bumped
            rBumperState = rBumper.read(); // default INPUT state is HIGH, it is LOW when bumped
            while ((lBumperState != LOW) && (rBumperState != LOW))
            {
                lBumperState = lBumper.read(); // default INPUT state is HIGH, it is LOW when bumped
                rBumperState = rBumper.read(); // default INPUT state is HIGH, it is LOW when bumped
                motors.drive(MOTOR_POW);
            }
            //FIXME: watch this
            //We can exit this logic here bc the bumpers are touching and the bumper algs can take it from here
        }
        else
        {
            lineFollowing = true;
        }
        // linefollowing/finding code:
        if (lineFollowing)
        {
            //Serial.println("following line");
            //Serial.print("IR Sensor Readings: ");
            int lread = left.read();
            int cread = center.read();
            int rread = right.read();

            if (!STOP)
            {
                if (cread > LINETHRESHOLD)
                {
                    motors.drive(MOTOR_POW);
                    Serial.println("bot going");
                }
                else if (rread > LINETHRESHOLD)
                {
                    motors.pivot(PIVOT_SPEED);
                    RLflag = true;
                    Serial.println("bot pivoting right");
                }
                else if (lread > LINETHRESHOLD)
                {
                    motors.pivot(-PIVOT_SPEED);
                    RLflag = false;
                    Serial.println("bot pivoting left");
                }
            }
            // if all sensors are on black or up in the air, stop the motors:
            if ((lread > LINETHRESHOLD) && (cread > LINETHRESHOLD) && (rread > LINETHRESHOLD))
            {
                motors.brake();
                motors.stop();
                STOP = true;
                Serial.println("bot up");
            }
            // all motors not on black:
            //TODO: test
            else if (((lread < LINETHRESHOLD) && (cread < LINETHRESHOLD) && (rread < LINETHRESHOLD)))
            {
                Serial.println("bot lost");
                int t1;
                boolean lineFound = false;
                int spiralTime = INIT_SPIRAL_TIME;
                //spiral until we find a line
                while (!lineFound)
                {   
                    Serial.println("bot spiraling");
                    cread = center.read();
                    t1 = millis();
                    //turn 90deg or until finding line
                    while (cread < LINETHRESHOLD && millis() < t1 + DEG90_TIME)
                    {
                        if (RLflag) // turn left or right based on what side we left from
                        {
                            motors.pivot(PIVOT_SPEED);
                        }
                        else
                        {
                            motors.pivot(-PIVOT_SPEED);
                        }
                        cread = center.read();
                    }
                    if (cread > LINETHRESHOLD)
                    {
                        lineFound = true;
                        break;
                    }

                    t1 = millis();
                    // Drive straight for spiralTime or until finding line:
                    while (cread < LINETHRESHOLD && millis() < t1 + spiralTime)
                    {
                        motors.drive(MOTOR_POW); 
                        cread = center.read();
                    }

                    if (cread > LINETHRESHOLD)
                    {
                        lineFound = true;
                        break; //unecessary but i think it makes it more readable
                    }

                    spiralTime += SPIRAL_TIME_INCREMENT;  // increment the spiral time so we expand our radius
                    
                }
            }
            else
            {
                STOP = false;
            }
        }
    }
    //one of the bumpers triggered state:
    else if (lBumperState == LOW || rBumperState == LOW)
    {
        Serial.println("bumper triggered");
        if (lBumperState == LOW && rBumperState == LOW) //both bumpers // by default backs up and turns right
        {
            RLflag = false; // if lost turn back left

            //back up:
            int t1 = millis();
            while (millis() < t1 + BACK_TIME)
                motors.drive(-MOTOR_POW / 2);

            //turn:
            t1 = millis();
            while (millis() < t1 + TURN_TIME)
                motors.pivot(PIVOT_SPEED);

            //drive until you hit the obstacle again or evade it:
            t1 = millis();
            while ((millis() < t1 + DRIVE_FULL_BODY_TIME) && !(lBumperState == LOW || rBumperState == LOW))
                motors.drive(MOTOR_POW / 2);
        }
        else if (lBumperState == LOW) //left // backs up and turns right
        {
            RLflag = false; //if lost turn back left

            //back up:
            int t1 = millis();
            while (millis() < t1 + BACK_TIME)
                motors.drive(-MOTOR_POW / 2);

            //turn:
            t1 = millis();
            while (millis() < t1 + TURN_TIME)
                motors.pivot(PIVOT_SPEED);

            //drive until you hit the obstacle again or evade it:
            t1 = millis();
            while ((millis() < t1 + DRIVE_FULL_BODY_TIME) && !(lBumperState == LOW || rBumperState == LOW))
                motors.drive(MOTOR_POW / 2);
        }
        else if (rBumperState == LOW) //right // backs up and turns left
        {
            RLflag = true; // if lost turn back right

            //back up:
            int t1 = millis();
            while (millis() < t1 + BACK_TIME)
                motors.drive(-MOTOR_POW / 2);

            //turn:
            t1 = millis();
            while (millis() < t1 + TURN_TIME)
                motors.pivot(-PIVOT_SPEED);

            //drive until you hit the obstacle again or evade it:
            t1 = millis();
            while ((millis() < t1 + DRIVE_FULL_BODY_TIME) && !(lBumperState == LOW || rBumperState == LOW))
                motors.drive(MOTOR_POW / 2);
        }
    }
}


