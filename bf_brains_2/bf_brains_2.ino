#include <Arduino.h>
#include <arduino.h>
#include </Applications/Arduino.app/Contents/Resources/Java/libraries/Wire/Wire.h>
#include </Users/sam/Documents/Arduino/libraries/RTClib/RTClib.h>

RTC_DS1307 RTC;

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
int solind1 = 3;
int solind2 = 4;
int solind3 = 5;
int solind4 = 6;
int solind5 = 7;

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
unsigned long solstart;
int soltime = 100;
int bumptime = 750;
int dbtimer = 500;

// commands
String lastcommand;
String command = "stop";
int soltrigger[5];

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
    
}

// setup
void setup() {
    digitalWrite(reset, HIGH);
    
    pinMode(reset, OUTPUT);
    pinMode(act1, OUTPUT);
    pinMode(act2, OUTPUT);
    pinMode(actind1, OUTPUT);
    pinMode(actind2, OUTPUT);
    pinMode(solind1, OUTPUT);
    pinMode(solind2, OUTPUT);
    pinMode(solind3, OUTPUT);
    pinMode(solind4, OUTPUT);
    pinMode(solind5, OUTPUT);
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
    
    DateTime now = RTC.now();
//    if (now.second() % 5 == 0 && now.unixtime() != prevtime) {
//        soltrigger = 1;
//        prevtime = now.unixtime();
//        solstart = millis();
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
            // subtract 48 because the ascii code for 1 is 49
            soltrigger[incoming - 49] = 1;
            solstart = millis();
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
            case 82: // R
                digitalWrite(reset, LOW);
                break;
            case 84: // T
                RTC.adjust(DateTime(__DATE__, __TIME__));
                break;
            case 32:
                command = "stop";
                break;
        }
        
        lastcommand = command;
    }
    
    
    //          ACTUATOR PRINTING
    
    if (millis() - dbstart > dbtimer) {
        // debounce timer
        
        if (shouldPrint) {
            queryStatus("actuator");
        }
        
        // if command isn't stop, but actuator is not moving
        // i.e. if it's autostopped at the top or bottom
        
        if (currPos == prevPos) command = "stop";
        if (command == "stop") shouldPrint = false;
        else shouldPrint = true;
        
        // reset shit
        dbstart = millis();
        prevPos = currPos;
    }
    
    
    //          COMMANDS
    
    if (command == "stop") {
        // shut it DOWN
        digitalWrite(actind1, LOW);
        digitalWrite(actind2, LOW);
        digitalWrite(act2, LOW);
        digitalWrite(act1, LOW);
        digitalWrite(solind1, LOW);
        digitalWrite(solind2, LOW);
        digitalWrite(solind3, LOW);
        digitalWrite(solind4, LOW);
        digitalWrite(solind5, LOW);
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
            // solenoid trigger isn't 0
            
            switch(i+1) {
                    // turn relevant solenoids on
                case 1:
                    digitalWrite(solind1, HIGH);
                    break;
                case 2:
                    digitalWrite(solind2, HIGH);
                    break;
                case 3:
                    digitalWrite(solind3, HIGH);
                    break;
                case 4:
                    digitalWrite(solind4, HIGH);
                    break;
                case 5:
                    digitalWrite(solind5, HIGH);
                    break;
            }
            if (millis() - solstart > soltime) {
                // turn solenoids off
                digitalWrite(solind1, LOW);
                digitalWrite(solind2, LOW);
                digitalWrite(solind3, LOW);
                digitalWrite(solind4, LOW);
                digitalWrite(solind5, LOW);
                soltrigger[i] = 0;
            }
        }
    }
    
    delay(10); // for serial
}