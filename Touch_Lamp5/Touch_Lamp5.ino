/*
Touch Lamp/Portable Outlet
-this program allows you to create a touch-controlled outlet; this can be used, for example, to turn any standard lamp into a touch lamp

Touch_Lamp5.ino

By Gabriel Staples
Written: 9 Jan. 2014
Last Revised: 23 Jan. 2014
Visit my website at: http://electricrcaircraftguy.blogspot.com/
-If you would like to contact me, please go to my website above, then click the "Contact Me" tab at the top of the page.

===================================================================================================
  LICENSE & DISCLAIMER
  
  Copyright (C) 2014 Gabriel Staples.  All right reserved.
  
  This code was written entirely at home, during my own personal time, and is neither a product of work nor my employer.
  It is owned entirely by myself.
  
  ------------------------------------------------------------------------------------------------
  License: GNU General Public License Version 3 (GPLv3) - http://www.gnu.org/licenses/gpl.html
  ------------------------------------------------------------------------------------------------

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see http://www.gnu.org/licenses/
===================================================================================================

PROGRAM NOTES & SETUP INFO:
-use pin 7 as the touch sensor and pins 2 & 3 as the output signals to the relays; in my case I am using 1 relay per outlet socket, for a total of two AC outlet sockets
-for safety against static discharge into the Arduino, when you touch the touch wire, put a 10K resistor in series between your touch wire and the Arduino pin 7.
/////////////-use pin 12 as a "button" to trigger calibration mode; simply touch a jumper wire from D12 to GND to begin an automatic 5~10 sec calibration sequence, which will
 detect what the touch sensitivity threshold should be to adequately detect your touch; normal operation automatically begins when calibration sequence is complete
-Note: the relay I am using is on when signal pin is LOW; therefore it is said to be "active LOW"
-onboard LED13 will be ON when the lamp is ON and OFF when the lamp is off, /////////////and it will blink rapidly during calibration

REVISION HISTORY (newest on TOP):
-21 Jan. 2014 - added smoothing (sample averaging) to get more consistent touch readings.  See
 here for a great sample tutorial on the concept: http://arduino.cc/en/Tutorial/Smoothing 
//////////-modified 20 Jan. 2014 to have a calibration option to automatically determine the correct touch sensitivity threshold
-modified 9 Jan. 2014 to make it a full touch lamp with a relay
-modified 30 Dec. 2013 in order to make code act as a toggle switch to turn on and off LED 13; for more help and info using a "button" as a toggle switch, 
 see here: http://arduino.cc/en/Tutorial/Switch
-Note: for a touch sensor, such as this, bouncing problems can happen both during contact on (touch start) and contact off (touch end), so I am debouncing 
 both contact on and contact off touches.
*/

//Include Libraries
#include <EEPROM.h> ///////////////MAY NOT BE NECESSARY ANYMORE UNLESS I AM HAVING A USER-PERFORMED CALIBRATION

//Set up Global variables
const int lamp = 2; //control pin for outlet 1 (the lamp)
const int outlet2 = 3; //control pin for outlet 2 (secondary device)
const int touchPin = 7; //the pin we will touch, to be read as a capacitive input pin
const int calPin = 12; //the calibration pin; briefly ground it to enter automatic calibration mode
const boolean RELAY_ON = LOW; //define a constant
const boolean RELAY_OFF = HIGH; //define a constant
boolean pin_touched = false; //true if the pin is being touched, false if it is not being touched
boolean pin_touched_old = false; //the previous value of pin_touched
int touch_threshold;
boolean lamp_state = RELAY_OFF;
unsigned long time1 = 0; //ms; used for debouncing
unsigned long time2 = 0; //ms; used for outputting data to the Serial monitor
unsigned long debounce_t = 0; //ms of debounce time for the touch sensor input; default 200ms; increase if you discover bouncing (reading of multiple touches) during a single touch or untouch action
                                //For an actual 120V touch lamp application, you might set this value to something more like 2000~5000ms in order to prevent the user from switching the lamp on and off more rapidly than
                                //every 2~5 seconds or so, since that's plenty fast enough for normal use, and deters bothersome little kids from seeing how fast they can touch the lamp to turn it on and off.
int touch_val = 0; //the current, individual reading
const int num_readings = 5; //# of touch reading samples to average
const int num_avgs = num_readings + 1; //# of averaged touch readings to store in an array
int readings[num_readings];
float averages[num_avgs];
int index = 0; //the index of the current reading
int index_avgs = 0; //the index of the current average
int total = 0; //the running total of the raw readings
float avg_touch_val = 0; //the avg. of the readings
float avg_touch_val_old = 0; //the avg. of the readings from (num_readings) samples back in time
float slope = 0; //the touch sensor slope, or derivative.  slope = avg_touch_val - avg_touch_val_old 


void setup() {
  Serial.begin(115200);
  pinMode(lamp, OUTPUT);
  pinMode(outlet2, OUTPUT);
  pinMode(calPin, INPUT_PULLUP);
  
  //ensure lamp and other output are both OFF
  digitalWrite(lamp, lamp_state);
  digitalWrite(outlet2, lamp_state);
  
//  //retrieve and print the touch_threshold
//  touch_threshold = EEPROM.read(0); //read EEPROM memory location 0, where the touch_threshold value is stored
//  if (touch_threshold == 255){ //if the touch_threshold has never been determined (255 is the default value in an EEPROM memory location)
////    doCalibration();
//  }
//  Serial.print("touch_threshold = ");
//  Serial.println(touch_threshold);

  //instantiate all readings to 0
  for (int i = 0; i < num_readings; i++){
    readings[i] = 0;
  }
  for (int i = 0; i < num_avgs; i++){
    averages[i] = 0;
  }
}


void loop() {
  
//  if (digitalRead(calPin)==LOW){
////    doCalibration();
//  }
  
  //check the state of the touch sensor to see if it is being touched; only read the pin, however, if the debounce time has passed.
  //Note: the debounce time is the time elapsed since the start of the last touch (finger on), or the start of the last UNtouch (finger off), since a touch sensor can have 
  //bounce both during contact on and during contact off
  if (millis() - time1 > debounce_t){ //if the debounce time has elapsed
    //GET A NEW READING
    getReading();
    
//    if (millis() - time2 >= 500){ //if 500ms has elapsed since last output
      //update time2
      time2 = millis();
      //print out the touch sensor reading once every 0.5 seconds
//      Serial.print("avg_touch_val now = ");
//      Serial.print(avg_touch_val);
//      Serial.print("\t");
//      Serial.print("avg_touch_val from ");
//      Serial.print(num_readings);
//      Serial.print(" samples ago = ");
//      Serial.println(avg_touch_val_old); 

      Serial.print(avg_touch_val);
      Serial.print("\t");
      Serial.print(avg_touch_val_old); 
      Serial.print("\t");
      Serial.println(slope);
//    }
    if (avg_touch_val - avg_touch_val_old > 1.0){ //pin is being touched
      pin_touched = true;
    }
    else{ //pin is NOT being touched
      pin_touched = false;
    }
    // if the input just went from UNTOUCHED to TOUCHED and and we've waited long enough
    // to ignore any noise on the circuit, toggle the output pin and remember
    // the time
    if (pin_touched == true && pin_touched_old == false) {
      time1 = millis(); //store the current time, in ms, since the start of a new touch was just detected
      toggleLamp(); //turn the lamp on if off, and off if on
      //print the values to see what the touch levels were when it was triggered
      Serial.println("***TOUCH OCCURED***");
//      Serial.print(avg_touch_val);
//      Serial.print("\t");
//      Serial.println(avg_touch_val_old);       
//      Serial.println("*******************");
    }
    //also store the time at the start of an UNtouch (ie: finger from on to off the sensor)
    else if (pin_touched == false && pin_touched_old == true){ //if the pin just went from touched to untouched
      time1 = millis(); //also store the time, in order to debounce the untouch action, since a touch sensor can bounce during contact on or contact off (unlike regular switches or buttons, which only bounce during contact on).
      Serial.println("***UN-TOUCH OCCURED***");
    }
    pin_touched_old = pin_touched; //update variable
  }
}


//Here you will do something useful; I am simply toggling lamp 13 on and off in order to demonstrate the use of a capacitive touch sensor as a toggle switch
void toggleLamp(){ 
  lamp_state = !lamp_state; //invert lamp state
  digitalWrite(lamp, lamp_state); //write to lamp
  digitalWrite(outlet2, lamp_state); //turn on or off the other outlet too
  
  //toggle LED 13 as necessary, to indicate lamp state
  if (lamp_state==RELAY_ON){
    digitalWrite(13,HIGH);
  }
  else{ //lamp is off
    digitalWrite(13,LOW);
  }
}


////lamp calibration
//void doCalibration(){
//  //prepare for calibration
//  touch_threshold = 0; //reset to prepare for calibration
//  boolean led_state_start = digitalRead(13); //store LED state
//  boolean led_state = LOW; //initialize
//  
//  //reset all readings to 0
//  for (int i = 0; i < num_readings_long; i++){
//    readings_long[i] = 0;
//  }
//  
//  Serial.println("***************CALIBRATION START*******************");
//  unsigned long t_start = millis();
//  for (unsigned int i=1; i < 5000; i++){
////    Serial.println(touch_threshold); //for debugging
//    getReading();
//    touch_threshold = max(touch_threshold,avg_touch_val);
//    //blink LED 13 to indicate calibration is happening
//    if (i % 25 == 0){ //for every ___ iterations
//      led_state = !led_state;
//      digitalWrite(13,led_state);
//    }
//  }
//  unsigned long t_cal = millis() - t_start;
//  Serial.print("cal time (ms): ");
//  Serial.println(t_cal);
//  Serial.print("touch_threshold = ");
//  Serial.println(touch_threshold);
//  Serial.println("***************CALIBRATION END*******************");
//  EEPROM.write(0,touch_threshold); //store value into EEPROM
//  digitalWrite(13, led_state_start); //restore LED state
//}


void getReading(){
//  delay(1); //delay between reads for read stability; give the pin time to settle before rapidly reading it again

  //subtract the last reading
  total -= readings[index];
  //get a new reading
  touch_val = readCapacitivePin(touchPin);
  readings[index] = touch_val;
  //add the reading to the total
  total += readings[index];
  //increment the index
  index++;
  //if at the end of the array....
  if (index >= num_readings){
    index = 0; //reset to beg. of array
  }
  //calc. the avg. reading
  avg_touch_val = (float)total/num_readings;
  
  //store the avg. reading
  averages[index_avgs] = avg_touch_val;
  index_avgs++; //increment; note: since the index was just incremented, it now represents the location of the OLDEST average, which is what we will compare the 
                //current average against to see if a touch has occured
  if (index_avgs >= num_avgs){
    index_avgs = 0; //reset index to beg. of array
  }  
  avg_touch_val_old = averages[index_avgs]; //see notes above
  slope = avg_touch_val - avg_touch_val_old; //calculate the slope.  Positive means the sensor was just touched.  Negative means it was just *un*touched
}


