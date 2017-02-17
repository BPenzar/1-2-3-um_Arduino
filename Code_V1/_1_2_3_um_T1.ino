#include "pitches.h"
#include <stdio.h>
#include <stdlib.h>

// ----- Variablen -----

const int PLredPin = 9;     // PIN'S für playerLED: links
const int PLgreenPin = 10;  //                      dritte von links
const int PLbluePin = 11;   //                      rechts
const int GAMEredPin = 5;   // PIN'S für GAMELED:   links
const int GAMEgreenPin = 6; //                      dritte von links
const int GAMEbluePin = 7;  //                      rechts
const int speaker = 8;      // 8-ohm speaker
const int irSense = A0;     // IR-Entfernungssensor analog pin A0

// --------- Spielvariablen -----------

bool Stop = true;           // Spiellampe = Rot
bool playerOne = true;      // Spieler im Spiel
int startbedingung = 0;     // Tickt 5 mal also 5 Messungen
int testwert = 0;           // Wert zum ermitteln der Startposition
int preval = 0;             // erster Messwert Sensor
int val = 0;                // zweiter Messwert Sensor
int calc = 0;               // Betrag der Differenz der beiden oberen Variablen

enum zustand {Start, Play, Win, Lose};  // Zustand wo im Spiel man ist
enum zustand gamecheck = Start;

// ----- IR Sensor -----

int distance = 0;           // Distanzvariable
int averaging = 0;          // Durchschnittsdistanz






// ----------------------------------------- SETUP ------------------------------------------------
void setup() {
    Serial.begin(9600); // Serial Monitor window
    // ----- Pins für gameLED -------
    pinMode(5, OUTPUT);
    pinMode(6, OUTPUT);
    pinMode(7, OUTPUT);
    // ----- Pins für playerLED -----
    pinMode(9, OUTPUT);
    pinMode(10, OUTPUT);
    pinMode(11, OUTPUT);
    // ----- Systemstartmelodie -----
    systemstartMelodie();
    playerLedColor(0, 0, 255);
}

// ----------------------------------------- LOOP ------------------------------------------------
void loop() {
    // ----------------------------------------- Startbedingung prüfen ------------------------------------------------


    if ( gamecheck == Win ){
        gameLedColor(255, 200, 200);
        siegMelodie();
        delay(3000);                                            // ÜBERGANGSLÖSUNG
        gamecheck = Start;                                      // ÜBERGANGSLÖSUNG --> 3s warten und neustart
    }

    while (gamecheck == Start) {
       checkStart();                                            // Funktion Startbedingung prüfen
    }

    //  ---------------- Startbedingung erfüllt -> Spiel geht los! -----------------------------------------
    while (gamecheck == Play){
        checkGame();                                            // Funktion Verhalten im Spiel

    }
    // --- Spieler hat sich bei Rot bewegt ---
    if (gamecheck == Lose) {
        playerLedColor(255, 0, 0);                          // Spieler ROT
        verlorenMelodie();
        gamecheck = Start;
    }
}

// ----------------------------------------------- Funktionen ---------------------------------------------------------

void checkStart() {
    gameLedColor(0, 0, 255);                                // gameLED auf Blau stellen
    testwert = irRead() ;
    if(testwert > 600 && testwert < 630) {                  // Bei 5 erfolgreichen Tests gamecheck = Play
        Serial.print("Start Test -- Positiv: ");
        Serial.println(testwert);
        startbedingung++;
    }
    else {
        Serial.print("Start Test -- Negativ: ");            // Bei negativem Test startbedingung == 0
        Serial.println(testwert);
        startbedingung = 0;
    }

    if ( startbedingung == 5){                              // Startbedingung erfüllt
        delay(2000);                                        // 2 Sekunden warten
        startMelodie();                                     // Startmelodie
        playerLedColor(0, 255, 0);                          // Beide Lampen Grün
        gameLedColor(0, 255, 0);
        startbedingung = 0;
        gamecheck = Play;
        Stop = false;                                       // gameLED GRÜN
    }
}

void checkGame() {
    // --- Wenn große Lampe Grün ist ---
    if ( Stop == false ) {
        for (int j = 0; j<=randomTime()*2; j++) {
            preval = irRead() ;
            Serial.print("preval(GRÜN): ");
            Serial.println(preval);
            if ( preval > 865 ) {
                gamecheck = Win;
                break;
            }
        }
        if (gamecheck == Play) changeLight();               // Rot werden
    }

    // --- Wenn große Lampe rot ist ---
    if ( Stop == true ) {
        preval = irRead();                                  // 1 Messung zum vergleich ob sich bewegt wurde in Rotphase
        Serial.print("Preval: ");
        Serial.println(preval);
        for (int k=0; k<=randomTime()*4; k++ ) {            // 4x die Sekunde neue Messung
            val = irRead() ;
            Serial.print("Val: ");
            Serial.println(val);
            calc = abs((val-preval));                       // Wird mit erster Messung verglichen
            Serial.print("Calc: ");
            Serial.println(calc);
            if ( calc > 10 ) {
                gamecheck = Lose;                           // Spieler aus dem Spiel -> Zurück an Startbedingung
                break;
            }
        }
        if (gamecheck == Play) changeLight();               // Ist Spieler noch im Spiel -> Grün werden

        }
}






/* Durchschnitt aus den letzten 5 Messwerten (um Messungenauigkeiten vorzubeugen)
 * Standardwerte: del = 50, counts = 5
 * !!!!EIN IRREAD DAUERT ALSO 250 MS!!!! */

int irRead() {
    averaging = 0;
    for (int i=0; i<=5; i++) { // sampling of [counts] readings from sensor
        distance = analogRead(irSense);
        averaging = averaging + distance;
        delay(50);     //  Wait 55 ms between each read
                        //  According to datasheet time between each read
                        //  is -38ms +/- 10ms. Waiting 55 ms assures each
                        //  read is from a different sample
    }
    distance = averaging / 5;  // Average out readings
    return(distance);               // Return value
}

// playerLED in bestimmter Farbe leuchten lassen (RGB)
void playerLedColor(int a, int b, int c) {
    analogWrite(PLredPin, a);
    analogWrite(PLgreenPin, b);
    analogWrite(PLbluePin, c);
}

//gameLED in bestimmter Farbe leuchten lassen (RGB)
void gameLedColor(int a, int b, int c) {
    analogWrite(GAMEredPin, a);
    analogWrite(GAMEgreenPin, b);
    analogWrite(GAMEbluePin, c);
}

/* gibt je nachdem ob Rot- oder Grünphase ist eine zufällige Zeit aus
 * ROT: zwischen 1-10s
 * GRÜN: zwischen 1-3s */
const int randomTime(){
    int randomtime = 0;
    if (Stop == true) {
        randomtime = rand() %10 + 1;
    }
    else {
        randomtime = rand() %4 + 1;
    }
    return(randomtime);
}

// Wechselt die Farbe der gameLED und den bool Stop
void changeLight() {
    if(Stop == false){
        Stop = true;
        gameLedColor(255, 0, 0);
    }
    else {
        Stop = false;
        gameLedColor(0, 255, 0);
    }
}

// ------------------------------------------- Melodien ---------------------------------------------------------------
void systemstartMelodie() {
    // ----- Noten für Melodie und Länge der Noten -----
    int melody[] = {NOTE_C4, NOTE_G3, NOTE_G3, NOTE_A3, NOTE_G3, 0, NOTE_B3, NOTE_C4};
    int noteDurations[] = {4, 8, 8, 4, 4, 4, 4, 4};

    for (int thisNote = 0; thisNote <= 8; thisNote++){
            int noteDuration = 1000 / noteDurations[thisNote];
            tone(speaker, melody[thisNote], noteDuration);
            int pauseBetweenNotes = noteDuration * 1.30;
            delay(pauseBetweenNotes);
            noTone(speaker);
    }
}

void startMelodie() {
// ----- Noten für Melodie und Länge der Noten -----
    int melody[] = {NOTE_A3, NOTE_A3, NOTE_A3, NOTE_C4};
    int noteDurations[] = {4, 4, 4, 4};

    for (int thisNote = 0; thisNote <= 4; thisNote++){
                int noteDuration = 1000 / noteDurations[thisNote];
                tone(speaker, melody[thisNote], noteDuration);
                int pauseBetweenNotes = noteDuration * 1.30;
                delay(pauseBetweenNotes);
                noTone(speaker);
    }
}

void verlorenMelodie() {
        // ----- Noten für Melodie und Länge der Noten -----
        int melody[] = {NOTE_G3, NOTE_D4, 0, NOTE_D4, NOTE_D4, NOTE_C4, NOTE_B3, NOTE_G3, NOTE_E3, 0, NOTE_E3, NOTE_C3};
        int noteDurations[] = {4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4};


        for (int thisNote = 0; thisNote <= 12; thisNote++){
                int noteDuration = 1000 / noteDurations[thisNote];
                tone(speaker, melody[thisNote], noteDuration);
                int pauseBetweenNotes = noteDuration * 1.30;
                delay(pauseBetweenNotes);
                noTone(speaker);
        }
}

void siegMelodie() {
// ----- Noten für Melodie und Länge der Noten -----
    int melody[] = {NOTE_C4, NOTE_E4, NOTE_G4, 0, NOTE_C5, NOTE_G4, NOTE_C5};
    int noteDurations[] = {4, 4, 4, 4, 2, 4, 2};

    for (int thisNote = 0; thisNote <= 7; thisNote++){
                int noteDuration = 1000 / noteDurations[thisNote];
                tone(speaker, melody[thisNote], noteDuration);
                int pauseBetweenNotes = noteDuration * 1.30;
                delay(pauseBetweenNotes);
                noTone(speaker);
    }
}
