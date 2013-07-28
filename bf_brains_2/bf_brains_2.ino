#include <Arduino.h>
#include <arduino.h>
#include </Applications/Arduino.app/Contents/Resources/Java/libraries/Wire/Wire.h>
#include </Users/sam/Documents/Arduino/libraries/RTClib/RTClib.h>
#include </Users/sam/Documents/Arduino/libraries/HIH61XX/HIH61XX.h>
#include </Users/sam/Documents/Arduino/libraries/HIH61XX/HIH61XX.cpp>

RTC_DS1307 RTC;
HIH61XX hih(0x27, 2);

/***********************************
 
 BELLFLOWER CONTROL
 written by Sam Galison
 
 instructions:

 -- actuator movement --
 i = up      o = bump up
 k = down    l = bump down
 space = stop
 
 -- solenoid control --
 1 - 5 = trigger solenoids
 
 -- debug --
 q = query everything
 a = query actuator
 s = query solenoids
 t = query time
 h = query humidity + temperature
 
 R = reset   T = set time to system time
 
 ***********************************/

// reset pin
int reset = 10;

// actuator driver pins
int act1 = 8;
int act2 = 9;

// actuator indicator pins
int actind1 = 11;
int actind2 = 12;

// solenoid pins
int solenoids[] = { 3, 4, 5, 6, 7 };

// actuator pot input pin (analog 0)
int positionFeedback = A0;

// debouncers & control
int currPos;
int prevPos;
boolean shouldPrint = false;
unsigned long prevtime;

// timers
unsigned long dbstart;
unsigned long bumpstart;
unsigned long solstart[5];
int soltime = 100;
int bumptime = 750;
int dbtimer = 500;

// commands
String lastcommand;
String command = "stop";
int soltrigger[5];
int hbell[] = { 1 };            // on the hour
int mbell[] = { 10, 11, 15 };   // minute repetitions
int sbell[] = { 5, 20, 30 };    // second repetition

// functions
int getPosition() {
    // utility function for mapping acutator position
    int sensed = analogRead(positionFeedback);
    int mapped = map(sensed, 25, 970, 0, 255);
    int final = constrain(mapped, 0, 255);
    return final;
}

void queryStatus(String cmp) {
    Serial.print("querying ");
    Serial.println(cmp);
    
    if (cmp == "actuator" || cmp == "all") {
        Serial.print("actuator -- cmd: ");
        Serial.print(command);
        Serial.print("\t\t::\tcurrent position:\t");
        Serial.println(currPos);
    }
    
    if (cmp == "solenoids" || cmp == "all") {
        Serial.print("solenoids -- ");
        for (int i = 0; i < 5; i++) {
            Serial.print(soltrigger[i]);
            if (i < 4) Serial.print(" | ");
        }
        Serial.println();
    }
    
    if (cmp == "clock" || cmp == "all") {
        Serial.print("clock -- ");
        DateTime now = RTC.now();
        
        Serial.print(now.year(), DEC);
        Serial.print('/');
        Serial.print(now.month(), DEC);
        Serial.print('/');
        Serial.print(now.day(), DEC);
        Serial.print(' ');
        Serial.print(now.hour(), DEC);
        Serial.print(':');
        Serial.print(now.minute(), DEC);
        Serial.print(':');
        Serial.print(now.second(), DEC);
        Serial.println();
    }
    
    if (cmp == "hih" || cmp == "all") {
        hih.start(); // start HIH
        hih.update(); // request update
        
        Serial.print("Humidity: ");
        Serial.print(hih.humidity(), 5);
        Serial.print(" RH (");
        Serial.print(hih.humidity_Raw());
        Serial.println(")");
        
        Serial.print("Temperature: ");
        Serial.print(hih.temperature(), 5);
        Serial.print(" C (");
        Serial.print(hih.temperature_Raw());
        Serial.println(")");
    }
    
}

// setup
void setup() {
    digitalWrite(reset, HIGH);
    
    pinMode(reset, OUTPUT);
    pinMode(act1, OUTPUT);
    pinMode(act2, OUTPUT);
    pinMode(actind1, OUTPUT);
    pinMode(actind2, OUTPUT);
    
    for (int i = 0; i < 5; i++) {
        pinMode(solenoids[i], OUTPUT);
    }
    
    pinMode(positionFeedback, INPUT);
    
    dbstart = millis();
    
    Wire.begin();
    RTC.begin();
    Serial.begin(9600);
    
    if (! RTC.isrunning()) {
        Serial.println("RTC is NOT running!");
        // following line sets the RTC to the date & time this sketch was compiled
        //RTC.adjust(DateTime(__DATE__, __TIME__));
    }
    
    queryStatus("all");
    
    delay(250);
    Serial.println("-- setup complete --");
}

// loop
void loop() {
    // get current actuator position
    currPos = getPosition();
    
    //          CHIME TIMING
    
//    DateTime now = RTC.now();
//    if (now.hour() > 8 && now.hour() < 22) {
//        // open hours: 8am to 11pm
//        
//        if (now.second() % 5 == 0 && now.unixtime() != prevtime) {
//            soltrigger[0] = 1;
//            prevtime = now.unixtime();
//            solstart[0] = millis();
//        }
//        
//    }
    
    //          SERIAL INPUT
    
    if (Serial.available() > 0) {
        int incoming = Serial.read();
        
        dbstart = millis();
        
        Serial.print("input: ");
        Serial.print(incoming);
        Serial.print("\t letter: ");
        Serial.write(incoming);
        Serial.println();
        
        if (incoming > 48 && incoming < 54) {
            // input is one of the 1-5 keys
            // subtract 49 because the ascii code for 1 is 49
            soltrigger[incoming - 49] = 1;
            solstart[incoming-49] = millis();
            queryStatus("solenoids");
        }
        
        switch(incoming){
            case 105: // i
                command = "up";
                break;
            case 107: // k
                command = "down";
                break;
            case 111: // o
                command = "bumpup";
                bumpstart = millis();
                break;
            case 108: // l
                command = "bumpdown";
                bumpstart = millis();
                break;
            case 97: // a
                queryStatus("actuator");
                break;
            case 115: // s
                queryStatus("solenoids");
                break;
            case 116: // t
                queryStatus("clock");
                break;
            case 113: // q
                queryStatus("all");
                break;
            case 104: // h
                queryStatus("hih");
                break;
            case 82: // R
                digitalWrite(reset, LOW);
                break;
            case 84: // T
                Serial.print("setting time to ");
                Serial.print(__DATE__);
                Serial.print(" -- ");
                Serial.println(__TIME__);
                RTC.adjust(DateTime(__DATE__, __TIME__));
                break;
            case 32:
                command = "stop";
                break;
        }
        
        lastcommand = command;
    }
    
    
    //          ACTUATOR AUTOSTOP
    
    if (millis() - dbstart > dbtimer) {
        if (currPos == prevPos) command = "stop";
        dbstart = millis();
    }
    
    
    //          COMMANDS
    
    if (command == "stop") {
        // shut it DOWN
        digitalWrite(actind1, LOW);
        digitalWrite(actind2, LOW);
        digitalWrite(act2, LOW);
        digitalWrite(act1, LOW);
        
        for (int i = 0; i < 5; i++) {
            digitalWrite(solenoids[i], LOW);
        }
    }
    
    else if (command == "up") {
        // actuator up
        digitalWrite(actind1, HIGH);
        digitalWrite(act1, HIGH);
        digitalWrite(act2, LOW);
    }
    
    else if (command == "down") {
        // actuator down
        digitalWrite(actind2, HIGH);
        digitalWrite(act1, LOW);
        digitalWrite(act2, HIGH);
    }
    
    else if (command == "bumpup") {
        // short up
        digitalWrite(actind1, HIGH);
        digitalWrite(act1, HIGH);
        digitalWrite(act2, LOW);
        if (millis() - bumpstart > bumptime) command = "stop";
    }
    
    else if (command == "bumpdown") {
        // short down
        digitalWrite(actind2, HIGH);
        digitalWrite(act2, HIGH);
        digitalWrite(act1, LOW);
        if (millis() - bumpstart > bumptime) command = "stop";
    }
    
    
    //          SOLENOID TRIGGERING
    
    for (int i = 0; i < 5; i++) {
        if (soltrigger[i]) {
            digitalWrite(solenoids[i], HIGH);
            
            if (millis() - solstart[i] > soltime) {
                // turn solenoids off
                digitalWrite(solenoids[i], LOW);
                soltrigger[i] = 0;
            }
        }
    }
    
    delay(10); // for serial
}