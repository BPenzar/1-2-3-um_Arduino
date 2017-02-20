#include "pitches.h"
#include <Adafruit_NeoPixel.h>

//------------------------------------------------------------------------------ Variablen ------------------------------------------------------------------------------ 

//--------------------------- LEDs ---------------------------
          /*--------------------------- LED-FARBCODE ---------------------------
          --------------------------- GAME-LED ---------------------------
          Weiß ==> Spielbereit - Spieler sollen sich einloggen
          Blau ==> Spieler sollen an die Startposition
          Grün ==> Im Spiel. Spieler dürfen laufen
          Rot  ==> Im Spiel. Spieler müssen stehen
          aus  ==> Spiel vorbei. (+ Spielerlampe Blau => Sieg / Rot => Verloren)
          --------------------------- Spieler-LED ---------------------------
          aus ==> Spieler nicht eingeloggt.
          Weiß ==> Spieler eingeloggt
          Grün ==> Spieler im Spiel
          Rot  ==> Spieler muss an den Anfang
          Blau ==> Spieler hat gewonnen */
          
// Verkettete Adressable LEDs
#define LED_ANZAHL 2  // in Reihe geschaltet, um Pins am Arduino zu sparen  --  Anzahl LEDs (Spieleranzahl + 1 (GameLED auf Index 0) )   
#define LED_PIN 2     // Pinadresse für LEDs
Adafruit_NeoPixel leds = Adafruit_NeoPixel(LED_ANZAHL, LED_PIN); // Index: 0 ==> Game-LED  --- Index: 1+2+... ==> SPIELER (1 bis x)
// LED-Farben -- (32-bit Farbangabe)
uint32_t green   = leds.Color(255, 0, 0);
uint32_t red     = leds.Color(0, 255, 0);
uint32_t blue    = leds.Color(0, 0, 255);
uint32_t white   = leds.Color(248,180,255); // An eigene LEDs anpassen
uint32_t aus     = leds.Color(0, 0, 0);

//--------------------------- Speaker + IR Sensor --------------------------- 
const int speaker = 3;      // 8-ohm speaker / Pietso   digital pin
const int irSense = A0;     // IR-Entfernungssensor     analog pin

//---------------------------  Game-Button --------------------------- 
const int gameButton = 12;       // Gamebutton
int gameButtonState = 0;         // Game Button-Wert
int gameState = 0;     

//---------------------------  Spieler-Button --------------------------- 
const int spielerButton_1 = 11;  // Button Spieler-1
int spieler_1_ButtonState = 0;   // Spieler-1 Button-Wert
int spieler_1_logging = 0;       // Spieler-1 im Spiel (ja/nein)

//--------------------------- Spielvariablen --------------------------- 
bool Stop = true;           // Spiellampe = Rot
int startbedingung = 0;     // 5-Fach Test (Spiler auf gewünschter Entfernung)
int testwert = 0;           // Wert zum ermitteln der Startposition

int vergleichswert = 0;             // erster Messwert Sensor
int messwert = 0;                // zweiter Messwert Sensor
int differenz = 0;               // Betrag der Differenz der beiden oberen Variablen 

int distance = 0;           // Empfangene Distanzvariable -- IR-Sensor
int averaging = 0;          // Durchschnittsdistanz (Durchschnittswert von 5 Messungen)

int anzahlVerloren = 0;     // Wenn Spieler das fünfte Mal zum Start muss, hat er verloren

//--------------------------- Zustand-Spiel --------------------------- 
enum zustand {Logging, Positioning, Play, Win, Lose, Noise};  // Enum für Spielzustände
enum zustand gamecheck;         // Gamecheck kann Zustände von zustand enum annehmen



// ------------------------------------------------------------------------------ SETUP ------------------------------------------------------------------------------
void setup() {
    Serial.begin(9600); // Serial Monitor window starten
    pinMode(gameButton, INPUT);
    pinMode(spielerButton_1, INPUT);  
    
    leds.begin();       // LEDs starten
    leds.show();      
    gamecheck = Noise;
}



                              
// ------------------------------------------------------------------------------ LOOP ------------------------------------------------------------------------------ 
void loop() {      
    gameButtonState = digitalRead(gameButton);
    
    if (gameButtonState == HIGH){  // Spiel Neustart (Spieler muss (/müssen) sich dann wieder einloggen)
      reset();
    }

    // Spielstatus
    switch(gamecheck){
      case Logging: 
           logging();
           break;
      case Positioning: 
           positioning();
           break;
      case Play:
           inGame();
           break;
      case Win:
           gameWin();
           break;
      case Lose:
           gameLose();
           break;
      case Noise:
           delay (200);
           break;
      default:
           delay (200);
    }
}
  

                            
//------------------------------------------------------------------------------ FUNKTIONEN---------------------------------------------------------------------------------------------------------------

// -------------------------------------------------------------------------- SPIELBEDINGUNGEN --------------------------------------------------------------------------
// ---------------------------------- Spiel Reset----------------------------------
void reset(){
      spieler_1_logging = 0;
      led_farbe(1,aus);
      delay(1000);
      gameButtonState = LOW;
      gameState = 0;
      gamecheck = Logging;
}
// ---------------------------------- Spieler einloggen----------------------------------
void logging(){
        /* Spieler für einloggen auf ihre Button drücken 
           Spieler==> Werden weiß wenn eingellogt, gehen aus wenn ausgeloggt
           Game-Button drüken, um das Spiel zu starten (min. 1 Spieler muss eingeloggt sein!) */
    gameButtonState = digitalRead(gameButton);
    spieler_1_ButtonState = digitalRead(spielerButton_1);
    led_farbe(0, white);

    Serial.print("GameButton: ");
    Serial.println(gameButtonState);
    Serial.print("GameState: ");
    Serial.println(gameState);
    Serial.print("Spieler1 Button State: ");
    Serial.println(spieler_1_ButtonState); 
    Serial.print("Spieler1 LoggingState: ");
    Serial.println(spieler_1_logging);

     if (spieler_1_ButtonState == HIGH){
      delay(200);
       if (spieler_1_logging == 0){
            spieler_1_logging = 1;
            led_farbe(1,white);
       }
       else {
          spieler_1_logging = 0;
          led_farbe(1,aus);
       }
    }
    if (gameButtonState == HIGH && spieler_1_logging == 1){
        gameState = 1;
    }
    
    if (gameState == 1 && spieler_1_logging == 1){
      Serial.print("GameState: ");
      Serial.print(gameState);
      Serial.print("Spieler1 LoggingState: ");
      Serial.println(spieler_1_logging);
      delay(2000);
      systemstartMelodie();
      gamecheck = Positioning;
    }
    else{
      logging();
    }
}
// ---------------------------------- Spieler an die Startpositionen----------------------------------
void positioning() {
    /* Spieler müssen auf ihre Startpositionen*/
    led_farbe(0, blue);      
    led_farbe(1, red);                                
    
    testwert = irRead() ;
    if(testwert > 500 && testwert < 600) {                  // Bei 5 erfolgreichen Tests gamecheck = Play
        led_farbe(1,green);
        Serial.print("Start Test -- Positiv: ");
        Serial.println(testwert);
        startbedingung++;
    }
    else {
        led_farbe(1, red);
        Serial.print("Start Test -- Negativ: ");            // Bei negativem Test startbedingung = 0
        Serial.println(testwert);
        startbedingung = 0;
    }
    if (startbedingung == 5){                              // Startbedingung erfüllt
        delay(1000);                                        // 2 Sekunden warten
        startMelodie();                                     // Startmelodie
        led_farbe(0, green); // Game-LED;                          
        led_farbe(1, green); // Spieler-1-LED
        startbedingung = 0;
        
        gamecheck = Play; // Spiel Starten
        Stop = false;     // Spieler darf direkt laufen
    }
}
//  ---------------------------------- Im Spiel ----------------------------------
void inGame() {
  /* Verhalten der Spieler während des Spiels prüfen. Laufen während sie nicht dürfen, Sieg, etc. */
    
    int randomtime = randomTime(); // Zufallswert in Abhängigkeit von Bewegungsphase (rot 1-10 / grün 1-4)
 
    // --- Game-LED green --- Laufphase
    if ( Stop == false ) {
        for (int j = 0; j < randomtime; j++) {
            vergleichswert = irRead();
            Serial.print("vergleichswert (Bewegungsphase): ");
            Serial.println(vergleichswert);
            if ( vergleichswert < 100 ) {
                gamecheck = Win;
                break;
            }
            delay (900); // ca. 1sek pro randomtime-Einheit warten
        }
    }
    
    // --- Game-LED red --- Stopphase
    else if ( Stop == true ) {
        delay(300);
        vergleichswert = irRead(); 
        Serial.print("Vergleichswert: ");
        Serial.println(vergleichswert);
        
        for (int k=0; k < randomtime; k++ ) {
            messwert = irRead() ;
            Serial.print("Messwert: ");
            Serial.println(messwert);
            differenz = abs((messwert-vergleichswert));  //   abs() => Betrag!
            Serial.print("Differenz: ");
            Serial.println(differenz);
            
            if ( differenz > 40 ) { // Spieler bei unerlaubter Bewegung zurück an Startposition
                gamecheck = Lose;  
                break;
            }
            delay (900); // ca. 1sek pro randomtime-Einheit warten
        }
    }
    // Spieler noch im Spiel 
    if (gamecheck == Play) changeLight(); // Spielerlampe wechsel + Laufen erlauben/verbieten
}

//  ---------------------------------- Spiel-Sieg ----------------------------------
void gameWin(){
     led_farbe(0, aus); // Game-LED                                   
     led_farbe(1, blue); // Game-LED                    
     siegMelodie();
     gamecheck = Noise;
}

//  ---------------------------------- Spiel-Verloren ----------------------------------
void gameLose(){
    anzahlVerloren ++;

    if (anzahlVerloren >= 5){
      led_farbe(0, aus); // Game-LED
      led_farbe(1, red); // Spieler-1-LED
      verlorenMelodie();
      anzahlVerloren = 0;
      gamecheck = Noise;
    }
    else{
      fehltrittMelodie();
      gamecheck = Positioning;  
    }
}



// --------------------------------------------------------------------------------- ZUSATZ FUNKTIONEN---------------------------------------------------------------------------------
// LED'S
void led_farbe(int index, uint32_t farbe){      // LED_FARBEN   (Game/Spieler, Farbe);
  leds.setPixelColor(index, farbe);
  leds.show();
}


// RANDOM_TIME
int randomTime(){
    int randomtime = 0;
    if (Stop == true) {                 // STOP_PHASE (1-10sek)
        randomtime = random(1,10);
    }
    else {                              // BEWEGUNGS_PHASE (1-4sek)
        randomtime = random(1,4);
    }
    return(randomtime);
}


// SPIELSTATUS WECHSEL
void changeLight() {
    if(Stop == false){
        Stop = true;
        led_farbe(0, red); // Game-LED
    }
    else {
        Stop = false;
        led_farbe(0, green); // Game-LED
    }
}


// IR-SENSOR Lesung (Durchschnitt)
/* Durchschnitt aus den letzten 5 Messwerten (um Messungenauigkeiten vorzubeugen)
 * IRREAD() => 250 MS! */
int irRead() {
    averaging = 0;
    for (int i=0; i<=10; i++) { // IR-Sensor_Wert Einlesen (5x)
        distance = analogRead(irSense);
        averaging = averaging + distance;
        delay(55);      /*  50 ms zwischen Lesewerten warten.
                          According to datasheet time between each read
                          is -38ms +/- 10ms. */
    }
    distance = averaging / 5;       // Durchschnitt der Lesewerte in distance
    return(distance);               // Rückgabewert
}


// ----- MELODIEN ------
//  MELODIE 1
void systemstartMelodie() {
    // ----- Noten für Melodie und Länge der Noten -----
    int melody[] = {NOTE_C4, NOTE_G3, NOTE_G3, NOTE_A3, NOTE_G3, 0, NOTE_B3, NOTE_C4};
    int noteDurations[] = {4, 8, 8, 4, 4, 4, 4, 4};

    for (int thisNote = 0; thisNote < 8; thisNote++){
            int noteDuration = 1000 / noteDurations[thisNote];
            tone(speaker, melody[thisNote], noteDuration);
            int pauseBetweenNotes = noteDuration * 1.30;
            delay(pauseBetweenNotes);
            noTone(speaker);
    }
}

//  MELODIE 2
void startMelodie() {
// ----- Noten für Melodie und Länge der Noten -----
    int melody[] = {NOTE_A3, NOTE_A3, NOTE_A3, NOTE_C4};
    int noteDurations[] = {4, 4, 4, 4};

    for (int thisNote = 0; thisNote < 4; thisNote++){
                int noteDuration = 1000 / noteDurations[thisNote];
                tone(speaker, melody[thisNote], noteDuration);
                int pauseBetweenNotes = noteDuration * 1.30;
                delay(pauseBetweenNotes);
                noTone(speaker);
    }
}

//  MELODIE 3
void fehltrittMelodie() {
        tone(speaker, NOTE_A3, 1000);
}

//  MELODIE 4
void verlorenMelodie() {
        // ----- Noten für Melodie und Länge der Noten -----
        int melody[] = {NOTE_G3, NOTE_D4, 0, NOTE_D4, NOTE_D4, NOTE_C4, NOTE_B3, NOTE_G3, NOTE_E3, 0, NOTE_E3, NOTE_C3};
        int noteDurations[] = {4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4};

        for (int thisNote = 0; thisNote < 12; thisNote++){
                int noteDuration = 1000 / noteDurations[thisNote];
                tone(speaker, melody[thisNote], noteDuration);
                int pauseBetweenNotes = noteDuration * 1.30;
                delay(pauseBetweenNotes);
                noTone(speaker);
        }
}

//  MELODIE 5
void siegMelodie() {
// ----- Noten für Melodie und Länge der Noten -----
    int melody[] = {NOTE_C4, NOTE_E4, NOTE_G4, 0, NOTE_C5, NOTE_G4, NOTE_C5};
    int noteDurations[] = {4, 4, 4, 4, 2, 4, 2};

    for (int thisNote = 0; thisNote < 7; thisNote++){
                int noteDuration = 1000 / noteDurations[thisNote];
                tone(speaker, melody[thisNote], noteDuration);
                int pauseBetweenNotes = noteDuration * 1.30;
                delay(pauseBetweenNotes);
                noTone(speaker);
    }
}
