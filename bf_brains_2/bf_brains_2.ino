#include <Wire.h>
#include <HIH61XX.h>
#include <RTClib.h>

// ^^ uncomment these for arduino ^^
// vv uncomment these for xcode vv

//#include <Arduino.h>
//#include <arduino.h>
//#include </Applications/Arduino.app/Contents/Resources/Java/libraries/Wire/Wire.h>
//#include </Users/sam/Documents/Arduino/libraries/RTClib/RTClib.h>
//#include </Users/sam/Documents/Arduino/libraries/HIH61XX/HIH61XX.h>
//#include </Users/sam/Documents/Arduino/libraries/HIH61XX/HIH61XX.cpp>

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
 y = query light
 h = query humidity + temperature
 
 R = reset   T = set time to system time
 
 ***********************************/

// e
int e[] = {
    2,7,1,8,2,8,1,8,2,8,4,5,9,0,4,5,2,3,5,3,6,0,2,8,7,4,
    7,1,3,5,2,6,6,2,4,9,7,7,5,7,2,4,7,0,9,3,6,9,9,9,5,9,
    5,7,4,9,6,6,9,6,7,6,2,7,7,2,4,0,7,6,6,3,0,3,5,3,5,4,
    7,5,9,4,5,7,1,3,8,2,1,7,8,5,2,5,1,6,6,4,2,7,4,2,7,4,
    6,6,3,9,1,9,3,2,0,0,3,0,5,9,9,2,1,8,1,7,4,1,3,5,9,6,
    6,2,9,0,4,3,5,7,2,9,0,0,3,3,4,2,9,5,2,6,0,5,9,5,6,3,
    0,7,3,8,1,3,2,3,2,8,6,2,7,9,4,3,4,9,0,7,6,3,2,3,3,8,
    2,9,8,8,0,7,5,3,1,9,5,2,5,1,0,1,9,0,1,1,5,7,3,8,3,4,
    1,8,7,9,3,0,7,0,2,1,5,4,0,8,9,1,4,9,9,3,4,8,8,4,1,6,
    7,5,0,9,2,4,4,7,6,1,4,6,0,6,6,8,0,8,2,2,6,4,8,0,0,1,
    6,8,4,7,7,4,1,1,8,5,3,7,4,2,3,4,5,4,4,2,4,3,7,1,0,7,
    5,3,9,0,7,7,7,4,4,9,9,2,0,6,9,5,5,1,7,0,2,7,6,1,8,3,
    8,6,0,6,2,6,1,3,3,1,3,8,4,5,8,3,0,0,0,7,5,2,0,4,4,9,
    3,3,8,2,6,5,6,0,2,9,7,6,0,6,7,3,7,1,1,3,2,0,0,7,0,9,
    3,2,8,7,0,9,1,2,7,4,4,3,7,4,7,0,4,7,2,3,0,6,9,6,9,7,
    7,2,0,9,3,1,0,1,4,1,6,9,2,8,3,6,8,1,9,0,2,5,5,1,5,1,
    0,8,6,5,7,4,6,3,7,7,2,1,1,1,2,5,2,3,8,9,7,8,4,4,2,5,
    0,5,6,9,5,3,6,9,6,7,7,0,7,8,5,4,4,9,9,6,9,9,6,7,9,4,
    6,8,6,4,4,5,4,9,0,5,9,8,7,9,3,1,6,3,6,8,8,9,2,3,0,0,
    9,8,7,9,3,1}; // length 500

// reset pin
int reset = 10;

// status led
int stat = 13;
boolean toggle = 0;

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

// photocell input (analog 1)
int photocell = A1;

// random seed pin
int seedpin = A2;

// debouncers & control
int currPos;
int prevPos;

int actmax = 255;
int actmin = 175;
boolean shouldPrint = false;
unsigned long prevtime;
boolean trigger = false; // solenoid trigger

float light;
float prevlight;
float hum = 0;
float prevhum = 0;
float temp = 0;
float prevtemp = 0;

int cyclecount = 0;
int direction = 0;
int variance = 1;
int pacing = 1;
float chaos = 0.0;

// timers
unsigned long dbstart;
unsigned long bumpstart;
unsigned long solstart;
int soltime = 50;
int bumptime = 75;
int dbtimer = 500;

// commands
String lastcommand;
String command = "stop";
int soltrigger[]    = { 0, 0, 0, 0, 0 };

// music
int lateness;
int numnotes;
int notes[]         = { 6, 6, 6, 6, 6, 6, 6, 6 }; // 6 = no note
int abstimes[]      = { 0, 0, 0, 0, 0, 0, 0, 0 }; // use these for absolute timing
int mintimes[]      = { 0, 0, 0, 0, 0, 0, 0, 0 }; // use these for % minutes
int sectimes[]      = { 0, 0, 0, 0, 0, 0, 0, 0 }; // use these for % minutes

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
    
    if (cmp == "light" || cmp == "all") {
        light = analogRead(photocell);
        Serial.print("amount of light -- ");
        Serial.println(light);
    }
    
    if (cmp == "hih" || cmp == "all") {
        hih.start(); // start HIH
        hih.update(); // request update
        
        
        
        Serial.print("Humidity -- ");
        Serial.print(hih.humidity(), 5);
        Serial.print(" RH (");
        Serial.print(hih.humidity_Raw());
        Serial.println(")");
        
        Serial.print("Temperature -- ");
        Serial.print(hih.temperature(), 5);
        Serial.print(" C (");
        Serial.print(hih.temperature_Raw());
        Serial.println(")");
    }
    
}

float humval() {
    hih.start(); // start HIH
    hih.update(); // request update
    return hih.humidity();
}

float tempval() {
    hih.start(); // start HIH
    hih.update(); // request update
    return hih.temperature();
}

void chime(int sol) {
    soltrigger[sol] = 1;
    trigger = true;
}

void makenotes(int hour, int minute, float humidity, float temperature) {
    numnotes = floor( lateness / 15 * 8 );
    chaos = lateness / 15;
    
    int seed = e[cyclecount % 500] % 5;
    
    direction = floor( random(3) ) - 1; // -1, 0, or 1
    
    (temperature > prevtemp) ? variance -- : variance ++ ;
    (humidity > prevhum) ? pacing -- : pacing ++ ;
    variance = constrain(variance, 0, 10);
    pacing = constrain(pacing, 0, 10);
    
    for (int i = 0; i < 8; i++) {
        int entropy = floor(chaos * random(5));
        if (i < numnotes) notes[i] = abs((seed + variance * direction + entropy) % 5);
        else notes[i] = 6; // 6 for non-playing note
    }
    
    for (int j = 0; j < 8; j++) {
        // 900 seconds per 15 minutes
        
        int entropy = floor(chaos * random(6*pacing));
        if (j < numnotes) {
            if (pacing == 0) {
                abstimes[j] = j; // one per second for the first 8 seconds
            }
            else {
                abstimes[j] = (6 * pacing) * j + entropy; // spread to max 1 minute apart
            }
            
            mintimes[j] = floor(abstimes[j]/60);
            sectimes[j] = abstimes[j] % 60;
        }
    }
    
    cyclecount++;
}

// setup
void setup() {
    digitalWrite(reset, HIGH);
    
    pinMode(stat, OUTPUT);
    
    pinMode(reset, OUTPUT);
    pinMode(act1, OUTPUT);
    pinMode(act2, OUTPUT);
    pinMode(actind1, OUTPUT);
    pinMode(actind2, OUTPUT);
    
    randomSeed(seedpin);
    
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
    
    digitalWrite(stat, toggle);
    
    //          CHIME TIMING
    
    DateTime now = RTC.now();
    if (now.hour() >= 8 && now.hour() < 23) {
        trigger = false;
        // open hours: 8am to 11pm
        
        lateness = now.hour() - 7;
        
        if (now.unixtime() != prevtime) {
            
            toggle = !toggle;
            
            // ACTUATOR MOVEMENT
            // -- every 5 minutes opens slightly if getting lighter, closes if getting darker
            if (now.minute() % 5 == 0 && now.second() == 0) {
                
                if (light > prevlight && currPos > actmin) {
                    command = "bumpdown";
                    bumpstart = millis();
                }
                if (light < prevlight && currPos < actmax) {
                    command = "bumpup";
                    bumpstart = millis();
                }
                
                prevlight = light;
                
            }
            
            
            // MUSIC ALGORITHM
            if (now.minute() % 15 == 0 && now.second() == 0) {
                makenotes(now.hour(), now.minute(), humval(), tempval());
            }
            
            for(int t = 0; t < 8; t++) {
                if (now.minute() % 15 == mintimes[t]) {
                    // now minute matches minute chime
                    if (now.second() == sectimes[t]) {
                        if (notes[t] != 6) chime(notes[t]);
                    }
                }
            }
            
            if (now.second() % 2 == 0) chime(0);
            
            if (now.second() % 3 == 0) chime(1);
            
            if (now.second() % 4 == 0) chime(2);

            if (now.second() % 5 == 0) chime(3);
            
            if (now.second() % 6 == 0) chime(4);
            
            if (trigger) solstart = millis();
            prevtime = now.unixtime();
        }
        
        else trigger = false;
    }
    
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
            case 121: // y
                queryStatus("light");
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
        
//        for (int i = 0; i < 5; i++) {
//            digitalWrite(solenoids[i], LOW);
//        }
    }
    
    else if (command == "up") {
        // actuator up
        digitalWrite(actind1, HIGH);
        digitalWrite(act1, HIGH);
        digitalWrite(act2, LOW);
    }
    
    else if (command == "down") {
        // actuator down
        if (currPos > 175) {
            digitalWrite(actind2, HIGH);
            digitalWrite(act1, LOW);
            digitalWrite(act2, HIGH);
        }
        
        else {
            command = "stop";
            Serial.println("!! limit reached !!");
            digitalWrite(actind2, LOW);
            digitalWrite(act2, LOW);
            digitalWrite(act1, LOW);
        }
        
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
            soltrigger[i] = 0;
        }
    }
    
    
    if (millis() - solstart > soltime) {
        for (int i = 0; i < 5; i++) {
            // turn solenoids off
            digitalWrite(solenoids[i], LOW);
        }
    }
    
    delay(10); // for serial
}