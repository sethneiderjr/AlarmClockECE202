//All pins have PWM
#include <Adafruit_GFX.h> //needed for display, core library for all graphics
#include <Adafruit_PCD8544.h> //needed for display, specifically the Nokia5110 library
#include <Wire.h> //allows I2C device communication for US Sensor

//Pins for Ultrasonic Sensor
#define echoPin 1
#define trigPin 2

//LCD Pins for Nokia5110 LCD display (these pins can change and be whatever GPIO just doing this for now)
#define LCD_CLK 8
#define LCD_DIN 7
#define LCD_DC 6
#define LCD_CE 5
#define LCD_RST 4

Adafruit_PCD8544 display = Adafruit_PCD8544(LCD_CLK, LCD_DIN, LCD_DC, LCD_CE, LCD_RST); //from a tutorial just use this from library, sets up Nokia5110 LCD Display

//Button pins
#define BTN_SET 9 //set button (set alarm, set time)
#define BTN_INC 10 //increment button (hours,minutes,seconds)
#define BTN_NEXT 11 //next time button (hour->min->sec->hour)
#define BTN_STP1 12 //stop alarm button 1
#define BTN_STP2 15 //stop alarm button 2
#define BTN_STP3 16 //stop alarm button 3
int sequenceCount = 0; //count to keep track of number of buttons clicked in correct order (up to two since three buttons total, keep track of first two)

#define LED_PIN 13 //light-emitting diode pin
int brightness = 255; //for LEDPulse, initally LED brightness is 255 (max)

#define Piezo_PIN 14 //the piezo alarm

#define PIR_SENSOR 37 //PIR sensor pin - senses motion

//Motor Pins F = Forward B = Backward; uses L9110H H-Bridge which has two motor directions, allowing it to go backwards and give enough current to motors
#define LEFTMOTORF 3 //L9110H H-Bridge also able to be controlled by PWM, so using AnalogWrite to give it varying speed levels
#define LEFTMOTORB 4
#define RIGHTMOTORF 20
#define RIGHTMOTORB 21
bool reversing = false; //boolean variable to know when motors need to reverse
int motorspeed = 100; //default pwm speed for motors (39.2%)

int hours = 12, minutes = 0, seconds = 0; //default time before setting it manually
int alarmHours = 12, alarmMinutes = 0, alarmSeconds = 15; //default alarm time can also set manually

//keep track of if you are in set mode or not, so need boolean variables
bool alarmSet = false; //initially alarm wont go off until user sets it
bool alarmActive = false; //keep track if alarm goes off or not after it is set
int snooze = 0; //snooze variable to keep track of how many times user hit BTN_STP

unsigned long lastUpdate = 0; //variable to make time go forward one second, millis() for runtime is in unsigned long, so lastUpdate is also to be unsigned long
unsigned long LEDlastUpdate = 0; //seperate for LED
unsigned long buttonTime = 0; //for button time between the three alarm buttons, if they take too long then theyre cooked
unsigned long messageTime = 0; //for displaying temporary STOP button messages
unsigned long reverseTime = 0; //for motor reverse mode to let them reverse for set amount of time

String Message = ""; //global message variable to display temporarily
bool showMessage = false; //boolean message variable to tell when to display temporary message

enum Mode {NORMAL, SET_TIME, SET_ALARM}; //three different modes, enum for better readability (just userdefined datatype, Mode can be normal, in time setting, or alarm setting)
Mode currentMode = NORMAL; //set current mode to normal
int settingField = 0; //setting field for different modes 0-normal, 1-timeset, 2-alarmset

void setup(){
  display.begin(); //initialize nokia5110 so its ready
  display.setContrast(60); //default contrast for nokia5110
  display.clearDisplay(); //clear anything old
  display.display(); //show display from logic below

  pinMode(BTN_SET, INPUT_PULLUP); //pullup so when button pressed will send signal out
  pinMode(BTN_INC, INPUT_PULLUP); //button increment
  pinMode(BTN_NEXT, INPUT_PULLUP); //button next
  pinMode(BTN_STP1, INPUT_PULLUP); //button stop
  pinMode(BTN_STP2, INPUT_PULLUP);
  pinMode(BTN_STP3, INPUT_PULLUP);

  //pinMode(Piezo_PIN, OUTPUT); //OUTPUT provides higher current, so use a 470/1k ohm resistor usually (from arduino)
  //digitalWrite(Piezo_PIN, LOW); //piezo alarm off

  pinMode(LEFTMOTORF, OUTPUT); //motors
  pinMode(LEFTMOTORB, OUTPUT);
  pinMode(RIGHTMOTORF, OUTPUT);
  pinMode(RIGHTMOTORB, OUTPUT);

  pinMode(PIR_SENSOR, INPUT); //PIR Sensor
  pinMode(trigPin, OUTPUT); //US Sensor
  pinMode(echoPin, INPUT); //US Sensor
  }

void loop(){
  checkButtons(); //have handlebuttons in loop so that they can be pressed anytime

  if(currentMode == NORMAL && millis() - lastUpdate >= 900){ //basically when the update time reaches 1 second (1000ms) it increments the time by 1 second, 900 since 100 delay
    lastUpdate = millis(); //since millis() is total runtime(never resets), lastUpdate has to be reassigned each time or else it will always be >=1000ms  
    incrementTime();
  }

  if(alarmSet && !alarmActive && hours == alarmHours && minutes == alarmMinutes && seconds == alarmSeconds){
    alarmActive = true; //if the alarm time is the same as the current time the alarm goes off
  }

  if(alarmActive){
    LEDPulse(LED_PIN); //call LEDPulse to turn on LED
    //digitalWrite(Piezo_PIN, HIGH); //turn on Piezo alarm with pin 14 (really loud)
    MotorsOn();
  }else{
    analogWrite(LED_PIN, 0); //turn off LED (0% PWM)
    //digitalWrite(Piezo_PIN, LOW); //Piezo off if alarmActive is false
    MotorsOff();
  }

  displayTime();//display time loop will keep up with the time increment
  delay(100); //100ms delay since button connection will bounce (learned from ECE251)
}

void incrementTime(){ //time logic in military time since that is simpler (0-24, not am and pm)
  seconds++; //only increment seconds at the base, which builds up to minutes and hours
  if(seconds >= 60){ //if seconds will increment over 59, reset to 0, and increment minutes
    seconds = 0;
    minutes++;
  }
  if(minutes >= 60){ //same as seconds, but will increment hours
    minutes = 0;
    hours++;
  }
  if(hours >= 24){ //miltary time for hours, if over 23, will reset back to 0
    hours = 0;
  }
}

void checkButtons(){
  static bool prevSet = HIGH, prevInc = HIGH, prevNext = HIGH, prevSTP1 = HIGH, prevSTP2 = HIGH, prevSTP3 = HIGH; //bool bc true or false that high or low
  //if function just saw if a button is LOW, then it will call too many times, so need previous state so when button is pressed, then it does the task ONCE

  bool currSet = digitalRead(BTN_SET); //current set/inc/next linked to buttons which will go low if button is pressed
  bool currInc = digitalRead(BTN_INC);
  bool currNext = digitalRead(BTN_NEXT);
  bool currSTP1 = digitalRead(BTN_STP1);
  bool currSTP2 = digitalRead(BTN_STP2);
  bool currSTP3 = digitalRead(BTN_STP3);

  // STOP Button Sequence (3 Buttons Total)
  if (alarmActive) { //buttons only will work when alarm is active
    if (prevSTP1 == HIGH && currSTP1 == LOW) { //first button
      sequenceCount = 1; //1 correct
      buttonTime = millis(); //update time
      showTempMessage("One Correct!"); //show message
    }
    if (sequenceCount == 1) { //if 1st button correct
      if (prevSTP2 == HIGH && currSTP2 == LOW && millis() - buttonTime <= 1000) { //user has 1 second to press next correct button
        sequenceCount = 2; //2 correct
        buttonTime = millis(); //update time again
        showTempMessage("Two Correct!");
      } else if (millis() - buttonTime > 1000) { //if they took over a second reset the count back to 0 so they can try again
        sequenceCount = 0;
        showTempMessage("Too Slow!");
      }
    }
    if (sequenceCount == 2) {//if 2 buttons hit correctly
      if (prevSTP3 == HIGH && currSTP3 == LOW && millis() - buttonTime <= 1000) { //less time to get right (last one so they really cant mess up)
        showTempMessage("All Correct!");
        alarmActive = false; //turn off alarm
        if (snooze == 0 || snooze == 1) { //snooze logic, will have two snoozes, hence if it equals 0 or 1 it will do this logic
          SnoozeAlarm(); //call this to set alarm in 1 minute
          snooze++;
          sequenceCount = 0; //reset here if they fully succeed or else will leave at 2
        } else {
          alarmSet = false; //finally turn off alarm fully after two snoozes
          snooze = 0; //reset snooze to 0 for next alarm set by user
          sequenceCount = 0; //reset sequence here since for next alarm as well
        }
      } else if (millis() - buttonTime > 1000) { //back to the start if they mess up last one still
        sequenceCount = 0;
        showTempMessage("Almost!");
      }
    }
  }

  // SET Button Pressed
  if(prevSet == HIGH && currSet == LOW){ //if BTN_SET low, and originally not in mode, then go into mode (all same for 3 buttons)
    if(currentMode == NORMAL){
      currentMode = SET_TIME;
      settingField = 0;
    }else if(currentMode == SET_TIME){
      currentMode = SET_ALARM;
      settingField = 0;
    }else if(currentMode == SET_ALARM){
      currentMode = NORMAL;
      alarmSet = true; //alarm is now set once out of alarm setting
    }
  }

  // INC Button
  if(prevInc == HIGH && currInc == LOW) { //do an increment when button pressed
    if(currentMode == SET_TIME){ //for general time
      if(settingField == 0){hours = (hours + 1) % 24;} //increment hour, so on so forth, modulo so it only goes through 0-23, as well as min/sec
      if(settingField == 1){minutes = (minutes + 1) % 60;}
      if(settingField == 2){seconds = (seconds + 1) % 60;}
    } else if(currentMode == SET_ALARM) { //for alarm time
      if(settingField == 0){alarmHours = (alarmHours + 1) % 24;} //exact same as above
      if(settingField == 1){alarmMinutes = (alarmMinutes + 1) % 60;}
      if(settingField == 2){alarmSeconds = (alarmSeconds + 1) % 60;}
    }
  }

  // NEXT Button
  if(prevNext == HIGH && currNext == LOW){ //do next when button pressed, three options
    settingField++;
    if(settingField > 2){ //if the settingField is over 2, reset it to 0 since i only have 3 settings
      settingField = 0;
    }
  }

  prevSet = currSet; //remember where setting was before for each of these 4 button functions so button only activates ONCE
  prevInc = currInc;
  prevNext = currNext;
  prevSTP1 = currSTP1;
  prevSTP2 = currSTP2;
  prevSTP3 = currSTP3;
}

void showTempMessage(String message) { //function to display temporary message with the STOP buttons
  Message = message; //set global Message to the passed message
  messageTime = millis(); //set the curret runtime to the message time to count for one second accurately
  showMessage = true; //boolean variable to true
}

void displayTime(){
  display.clearDisplay(); //clear in case anything was there before dont want something old there
  display.setTextColor(BLACK); //i think it can be white as well using WHITE but it goes invisible so no
  display.setCursor(0, 0); //set cursor setting x,y for display, nokia5110 lcd is 84x48 pixels

  if(currentMode == SET_TIME){ //if it is in set time mode the top will say set time:, same with the next parts these are all at top of display
    display.println("Set Time:");
  }else if(currentMode == SET_ALARM){ //if alarm mode will say set alarm at top
    display.println("Set Alarm:");
  }else if(alarmActive){
    display.println("ALARM ON"); //when alarm goes off you can say something stupid at the top
  }else{
    display.println("Current Time:"); //if not in other modes, this will be at the top
  }

  display.setTextSize(1); //text size makes the text bigger or smaller. 2 makes text too big so 1 
  display.setCursor(0, 20); //set cursor again

  if(currentMode == SET_ALARM){ //how to see your alarm time and in this mode it shows that default alarm time that you can change
    printTime(alarmHours, alarmMinutes, alarmSeconds); //now printtime uses the alarm time to show us
  }else{
    printTime(hours, minutes, seconds); //by default, printtime uses the general time to show the user
  }

  if(currentMode == SET_TIME || currentMode == SET_ALARM){ // when setting, this highlights under where it is setting
  int x = 0; //variable x is where line will start on x-axis of screen
    if(settingField == 1){ //logic for where underline sits, settingField 1 is minutes, settingField 2 is seconds
      x = 17; //x placement for underlining minutes
      }else if(settingField == 2){ //seconds
        x = 35; //x placement for underlining seconds
      }
  display.drawLine(x, 28, x + 12, 28, BLACK); // underline, (xstart,ystart,xend,yend,color), all values trial and error 
  }

  if (showMessage && millis() - messageTime <= 1000) { //message shows for once second, which lines up with the button press times
    display.setCursor(0, 40); //show alarm button messages at the bottom of screen
    display.println(Message); //print message to display
  } else {
    showMessage = false; //when over a second, also make boolean back to false
  }

  display.display(); //display whole display with everything
}

void printTime(int h, int m, int s){ //this displays the time from hour,minute,second logic from above. 
  char time[9]; //character array to store the time characters. sprintf used next to convert a string into array characters
  sprintf(time, "%02d:%02d:%02d", h, m, s); //sprintf stores this time string into the time character array, which just uses less lines of code instead of assigning each slot individually
  display.println(time); //display onto screen
}

void SnoozeAlarm(){ //make user hate this clock, snoozealarm sets alarm in 1 minute again, used when user presses alarm button first time
  alarmSeconds = seconds;
  alarmHours = hours;
  alarmMinutes = minutes + 1; //only change is alarm minutes in 1 minute
  if(alarmMinutes >= 60){ //same logic as incementTime()
    alarmMinutes = 0;
    alarmHours = (alarmHours + 1) % 24;
  }
  alarmSet = true;
}

void LEDPulse(int led_pin){ //uses PWM to control LED brightness. 0 = 0%, 255 = 100%
  if(millis() - LEDlastUpdate >= 1){ //update time every 1ms can change
    LEDlastUpdate = millis();
    brightness -= 15; //increment brightness down by a multiple 15 (divisible by 15 = 17)
    if(brightness <=0){ //if hits no brightness, then go all the way back up to fade down again
      brightness = 255;
    }
    analogWrite(LED_PIN, brightness); //pin max of 40mA, reccomend 20mA, using 10k resistor to draw a lot of current
  }
}

long SeeDistance(){ //Code from harshkzz on GitHub https://www.youtube.com/watch?v=sXNcZdf4NS0&ab_channel=INOVATRIX, standard US Sensor setup in cm
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2); //since microseconds, will slightly delay 1000ms when on, but i dont care that it will be off some microseconds
  digitalWrite(trigPin, HIGH);//send out signal
  delayMicroseconds(10);//takes 10us to get good reading
  digitalWrite(trigPin, LOW);//stop sending signal
  long duration = pulseIn(echoPin, HIGH);//read echo
  long distance = duration / 58.2; //convert echo into cm distance
  return distance;
} //this will be used in MotorsOn so it will be able to detect objects in front and not run into them

void MotorsOn(){
  if(reversing && millis() - reverseTime < 700){
    analogWrite(LEFTMOTORF, 0);
    analogWrite(LEFTMOTORB, 100); //39.2% pwm reverse
    analogWrite(RIGHTMOTORF, 0);
    analogWrite(RIGHTMOTORB, 40); //15.6% pwm reverse, slower so it turns left
    return;//dont read rest of logic
  }else{
    reversing = false; //stop reversing when over time
  }

  if(SeeDistance() <= 15.00){ //if the distance of an object is less than or equal to 15cm
    reverseTime = millis(); //start reverse time
    reversing = true;
    return; //stop logic to start reverse logic at top
  }

  if(digitalRead(PIR_SENSOR) == HIGH){ //if PIR sensor detects motion, up the motorspeed to go away faster
    motorspeed = 195; //76.5% pwm
  }

  analogWrite(LEFTMOTORF, motorspeed);
  analogWrite(LEFTMOTORB, 0);
  analogWrite(RIGHTMOTORF, motorspeed);
  analogWrite(RIGHTMOTORB, 0);
}

void MotorsOff(){ //turn off motors
  analogWrite(LEFTMOTORF, 0);
  analogWrite(LEFTMOTORB, 0);
  analogWrite(RIGHTMOTORF, 0);
  analogWrite(RIGHTMOTORB, 0);
}
