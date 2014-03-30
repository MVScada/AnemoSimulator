#define DEBUG    1
#define MAX      120
#define TRUE     1
#define FALSE    0
#define ENABLE_XBEE 1
#define RHIGH LOW //Reles de logica inversa
#define RLOW HIGH //Reles de logica inversa
//define RLOW LOW //Reles de logica directa
//define RHIGH HIGH //Reles de logica directa

//use pin 11 on the Mega instead, otherwise there is a frequency cap at 31 Hz
#define PWM_PIN 9
#define PWM_LENGHT 70 // ancho del pulso 0-255

#define RXPIN 2
#define TXPIN 3

#define RELE1 4 // cambia de anemo a pulsos de arduino
#define RELE2 5 // botonera pulsador

#define XRXPIN 6
#define XTXPIN 7


#define STATUS 10     // led rojo de la placa
#define BTN_ENABLE 12 // pulsador activar manual
#define RST_XBEEPIN 11 // conectado al pin 5 de la Xbee


byte buffer[MAX], mbuffer[MAX];
int counter=0, mcounter=0;
String texto, txt;
int result=0, time_overflow=0;
unsigned long time=0, uptime=0; //time=miliseg uptime=seg
unsigned long iamalive=0, rele_time=0;
boolean xbee_started=FALSE;


 
#if defined(ARDUINO) && ARDUINO >= 100
    #include <Arduino.h>
  #else
    #include <WProgram.h>
#endif


#include <SoftwareSerial.h>
SoftwareSerial mserial =  SoftwareSerial(RXPIN, TXPIN);
SoftwareSerial debug   =  SoftwareSerial(XRXPIN, XTXPIN);

#if ENABLE_XBEE
  #include <XBee.h>
  XBee xbee = XBee();
  
  XBeeAddress64 CoordinatorAddr64 = XBeeAddress64(0x00000000, 0x00000000);
#endif

#include <avr/wdt.h>
#include <PWM.h>
#include <EEPROM.h>
#include "memoria.h"
#include "comunicacion.h"



void setup() {
  wdt_disable();
  loadConfig(); // from EEPROM
  
  // salidas
  initPIN(RELE1);
  initPIN(RELE2);
  digitalWrite(RELE1, RLOW);
  digitalWrite(RELE2, RLOW);
  initPIN(STATUS);
  
  pinMode(BTN_ENABLE, INPUT);
  digitalWrite(BTN_ENABLE, HIGH); // activar pullup
  
  // arrancar puerto serie
#if ENABLE_XBEE
/*
   1  - + 3,3V
   2  - DATA OUT => TX = pin 0/RX
   3  - DATA IN  <= RX = pin 1/TX
   5  - Reset negada
   10 - GND
*/
  // reiniciar la Xbee al arranque
  pinMode(RST_XBEEPIN, OUTPUT);
  digitalWrite(RST_XBEEPIN, LOW); // Poner a 0
  delay(300);
  pinMode(RST_XBEEPIN, INPUT);
  digitalWrite(RST_XBEEPIN, LOW); // activar pullup
#endif  

  // software serial for modem
  if(settings.enable_modem==TRUE){
    pinMode(RXPIN, INPUT);
    pinMode(TXPIN, OUTPUT);
    mserial.begin(9600);
  }
  
  // software serial for debug ASCII
  pinMode(XRXPIN, INPUT);
  pinMode(XTXPIN, OUTPUT);
  debug.begin(9600);
  
  //sets the frequency for the specified pin
  InitTimersSafe(); 
  bool success = SetPinFrequencySafe(PWM_PIN, 1);
  
  if(success) {
    digitalWrite(STATUS, HIGH);
    delay(150);
    digitalWrite(STATUS, LOW);
    delay(150);
  }
  
  delay(500);
  
  // poner estado al ultimo conocido
  if(settings.last_status == 1) {
      emergencia(1);
  }
  else {
      emergencia(0);
  }
  
  // watchdog 4 segundos (bucle infinito el ardu se reinicia)
  wdt_enable(WDTO_4S);
}

void loop() {
  if( millis() < time ) {
    time_overflow++;
  }
  time = millis();
  uptime=(time_overflow*4294967) + time/1000;
  
#if ENABLE_XBEE
  if( xbee_started == FALSE && uptime > 5 ) {
     Serial.begin(57600);
     xbee.begin(Serial);
     xbee_started=TRUE;
     texto="***XBEE ON***"; sendFrameAscii(texto);
  }
#endif
  
  wdt_reset(); // avisar al WD que estamos vivos
  
  loopAlive();
  loopDisableRelay();

  if(settings.enable_modem==TRUE){
    loopReadModem();
  }

#if ENABLE_XBEE
  loopReadXBee();
#endif


  //btn_status=digitalRead(BTN_ENABLE);
  if(digitalRead(BTN_ENABLE) == 0) {
    emergencia(!settings.last_status);
    // esperar un poco para evitar dobles pulsaciones
    delay(500);
  }
 
  
  delay(100);
}





