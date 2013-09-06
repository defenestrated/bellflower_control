//#include <Wire.h>
//#include <HIH61XX.h>
//#include <RTClib.h>

// ^^ uncomment these for arduino ^^
// vv uncomment these for xcode vv

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
 i = up      o = bump up        u = up to limit
 k = down    l = bump down      j = down to limit
 
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
int e[] = {2,7,1,8,2,8,1,8,2,8,4,5,9,0,4,5,2,3,5,3,6,0,2,8,7,4,7,1,3,5,2,6,6,2,4,9,7,7,5,7,2,4,7,0,9,3,6,9,9,9,5,9,5,7,4,9,6,6,9,6};

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
int downlimit = 175;
int uplimit = 255;

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
int variance = 5;
int pacing = 5;
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
//    Serial.println("making notes...");
//    Serial.print("hour: ");
//    Serial.println(hour);
    
    numnotes = int(floor(lateness / 15.0 * 8.0));
    chaos = lateness / 15.0;
//    Serial.print("numnotes: ");
//    Serial.print(numnotes);
//    Serial.print(" chaos: ");
//    Serial.println(chaos);
    
    int pos = cyclecount % 500;
    int seed = e[pos];
//    Serial.print("seed: ");
//    Serial.println(seed);
    
    direction = int(floor( random(3.0) ) - 1.0); // -1, 0, or 1

//    Serial.print("direction: ");
//    Serial.println(direction);
//    
//    Serial.print("past temp: ");
//    Serial.print(prevtemp);
//    Serial.print(" curr temp: ");
//    Serial.print(temperature);
//    Serial.print(" past hum: ");
//    Serial.print(prevhum);
//    Serial.print(" curr hum: ");
//    Serial.println(humidity);
    
    (temperature > prevtemp) ? variance -- : variance ++ ;
    (humidity > prevhum) ? pacing -- : pacing ++ ;
    variance = constrain(variance, 0, 10);
    pacing = constrain(pacing, 0, 10);

//    Serial.print("variance: ");
//    Serial.print(variance);
//    Serial.print(", pacing: ");
//    Serial.println(pacing);
//    Serial.print("notes: ");
    
    for (int i = 0; i < 8; i++) {
        int entropy = int(floor(chaos * random(5.0)));
        if (i < numnotes) notes[i] = abs((seed + variance * direction + entropy) % 5);
        else notes[i] = 6; // 6 for non-playing note
//        Serial.print(notes[i]);
//        Serial.print(" ");
    }
    
//    Serial.println();
//    Serial.print("timing: ");
    
    for (int j = 0; j < 8; j++) {
        // 900 seconds per 15 minutes
        
        int entropy = int(floor(chaos * random(6.0*pacing)));
        if (j < numnotes) {
            if (pacing == 0) {
                abstimes[j] = j; // one per second for the first 8 seconds
            }
            else {
                abstimes[j] = (6 * pacing) * j + entropy; // spread to max 1 minute apart
            }
            
            mintimes[j] = int(floor(float(abstimes[j])/60.0));
            sectimes[j] = abstimes[j] % 60;
            
//            Serial.print(mintimes[j]);
//            Serial.print(":");
//            Serial.print(sectimes[j]);
//            Serial.print(" ");
        }
    }

    prevtemp = temperature;
    prevhum = humidity;
    
//    Serial.println();
    
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
    
    prevtemp = tempval(); // preset past weather variables
    prevhum = humval();
    
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
            // -- open for the first four hours, close for the last four
            
            if (now.hour() < 12 && now.minute() == 0 && now.second() == 0) {
                float portion = (now.hour() - 7) / 4; // 1/4, 1/2, 3/4, or 1
                downlimit = actmax - int(floor(portion*(actmax - actmin)));
                command = "limdown";
            }
            
            if (now.hour() > 18 && now.minute() == 0 && now.second() == 0) {
                float portion = (now.hour() - 18) / 4; // 1/4, 1/2, 3/4, or 1
                uplimit = actmin + int(floor(portion*(actmax - actmin)));
                command = "limup";
            }
            
            // flex on the hour from noon until 6pm
            if ((now.hour() >= 12 && now.hour() <=18) && now.minute() == 0 && now.second() == 0) {
                command = "up";
            }
            if ((now.hour() >= 12 && now.hour() <=18) && now.minute()== 1 && now.second() == 0) {
                command = "limdown";
            }
            
            
//            if (now.minute() % 5 == 0 && now.second() == 0) {
//                queryStatus("light");
//            
//                if (light > prevlight && currPos > actmin || light > 300) {
//                    command = "bumpdown";
//                    bumpstart = millis();
//                }
//                if (light < prevlight || light < 200) {
//                    command = "bumpup";
//                    bumpstart = millis();
//                }
//                
//                prevlight = light;
//                
//            }

            
            // MUSIC ALGORITHM
            if (now.minute() % 1 == 0 && now.second() == 0) {
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
            
            if (trigger) solstart = millis();
            prevtime = now.unixtime();
        }
        
        else trigger = false;
    }
    if (now.hour() < 8 || now.hour() >= 23) {
        for (int n = 0; n < 8; n++) {
            notes[n] = 6;
        }
        if (currPos != actmax) {
            command = "up";
        }
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
            case 117: // u
                Serial.print("up limit: ");
                Serial.println(uplimit);
                command = "limup";
                break;
            case 106: // j
                Serial.print("down limit: ");
                Serial.println(downlimit);
                command = "limdown";
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
    
    if (millis() - dbstart > dbtimer && currPos > 250) {
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
        if (currPos > actmin) {
            digitalWrite(actind2, HIGH);
            digitalWrite(act2, HIGH);
            digitalWrite(act1, LOW);
            if (millis() - bumpstart > bumptime) command = "stop";
        }
        else {
            command = "stop";
            Serial.println("!! limit reached !!");
            digitalWrite(actind2, LOW);
            digitalWrite(act2, LOW);
            digitalWrite(act1, LOW);
        }
    }
    
    else if (command == "limup") {
        // defined up
        if (currPos < uplimit) {
            digitalWrite(actind1, HIGH);
            digitalWrite(act1, HIGH);
            digitalWrite(act2, LOW);
        }
        else {
            command = "stop";
            Serial.println("!! computed up limit reached !!");
            digitalWrite(actind1, LOW);
            digitalWrite(act2, LOW);
            digitalWrite(act1, LOW);
        }
    }
    
    else if (command == "limdown") {
        // defined down
        if (currPos > downlimit) {
            digitalWrite(actind2, HIGH);
            digitalWrite(act2, HIGH);
            digitalWrite(act1, LOW);
        }
        else {
            command = "stop";
            Serial.println("!! computed down limit reached !!");
            digitalWrite(actind2, LOW);
            digitalWrite(act2, LOW);
            digitalWrite(act1, LOW);
        }
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