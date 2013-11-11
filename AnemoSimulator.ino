
#define DEBUG    1
#define MAX      120
#define TRUE     1
#define FALSE    0

//use pin 11 on the Mega instead, otherwise there is a frequency cap at 31 Hz
#define PWM_PIN 9
#define PWM_LENGHT 70 // ancho del pulso 0-255

#define RXPIN 2
#define TXPIN 3

#define RELE1 4 // cambia de anemo a pulsos de arduino
#define RELE2 5 // botonera pulsador
#define RELE3 6 // botonera elevacion
#define RELE4 7 // botonera giro


#define STATUS 10     // led verde de la placa
#define BTN_ENABLE 12 // pulsador activar manual


byte buffer[MAX], mbuffer[MAX];
int counter=0, mcounter=0;
String texto, txt;
int result=0;
boolean btn_status=1, blink_led=0, every1secs=0;
unsigned long int uptime=0,time=0,time_overflow=0,last1secs=0,timetoreset1=0,timetoreset2=0;

 
#if defined(ARDUINO) && ARDUINO >= 100
    #include <Arduino.h>
  #else
    #include <WProgram.h>
#endif



#include <SoftwareSerial.h>
SoftwareSerial mserial =  SoftwareSerial(RXPIN, TXPIN);

#include <PWM.h>
#include <EEPROM.h>
#include "memoria.h"
#include "comunicacion.h"



void setup() {
  loadConfig(); // from EEPROM
  // arrancar puerto serie
  Serial.begin(9600);
  
  // software serial for modem
  pinMode(RXPIN, INPUT);
  pinMode(TXPIN, OUTPUT);
  mserial.begin(9600);
  
  // salidas
  initPIN(RELE1);
  initPIN(RELE2);
  initPIN(RELE3);
  initPIN(RELE4);
  
  initPIN(STATUS);
  initPIN(13);
  
  pinMode(BTN_ENABLE, INPUT);
  digitalWrite(BTN_ENABLE, HIGH); // activar pullup
  
  //initialize all timers except for 0, to save time keeping functions
  InitTimersSafe(); 

  //sets the frequency for the specified pin
  bool success = SetPinFrequencySafe(PWM_PIN, settings.frequency);
  
  //if the pin frequency was set successfully, turn pin 13 on
  if(success) {
    digitalWrite(13, HIGH);
  }
  
  
  if(settings.last_status == 1) {
      emergencia(1);
  }
  else {
      emergencia(0);
  }
  
  /*
  texto=__DATE__; texto+=" "; texto+=__TIME__;
  sendFrameAscii(texto);
  mserial.print(texto);
  */
  delay(500);
}

void loop() {
  timers();
  
  btn_status=digitalRead(BTN_ENABLE);
  if(btn_status == 0) {
    emergencia(!settings.last_status);
    // esperar un poco para evitar dobles pulsaciones
    delay(500);
  }
  
  loopReadSerial();
  loopReadModem();
  

  if(timetoreset1 > 0 && uptime > timetoreset1) {
    digitalWrite(RELE2, LOW);
    timetoreset1=0;
    digitalWrite(STATUS, LOW);
    texto="***RESET1 END***";
    sendFrameAscii(texto);
  }


  if(timetoreset1 !=0 && every1secs) {
      blink_led=!(blink_led);
      digitalWrite(STATUS, blink_led);
  }
  
  if(timetoreset2 > 0 && uptime > timetoreset2) {
    digitalWrite(RELE2, LOW);
    digitalWrite(RELE3, LOW);
    digitalWrite(RELE4, LOW);
    timetoreset2=0;
    texto="***RESET2 END***";
    sendFrameAscii(texto);
  }
 
  
  delay(5);
}


