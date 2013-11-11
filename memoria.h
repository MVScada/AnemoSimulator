#include <EEPROM.h>
#define CONFIG_VERSION "001"
#define CONFIG_START 32

struct StoreStruct {
  boolean last_status;
  unsigned int frequency; // frecuencia en Hz
  char version_of_program[4];
} settings = {  
  0,
  200,
  CONFIG_VERSION
};


void saveConfig() {
  for (unsigned int t=0; t<sizeof(settings); t++) { // writes to EEPROM
    EEPROM.write(CONFIG_START + t, *((char*)&settings + t));
  }
}

void loadConfig() {
  if (EEPROM.read(CONFIG_START + sizeof(settings) - 2) == settings.version_of_program[2] &&
      EEPROM.read(CONFIG_START + sizeof(settings) - 3) == settings.version_of_program[1] &&
      EEPROM.read(CONFIG_START + sizeof(settings) - 4) == settings.version_of_program[0]) {
    for (unsigned int t=0; t<sizeof(settings); t++)
      *((char*)&settings + t) = EEPROM.read(CONFIG_START + t);
  } else {
    saveConfig();
  }
}

void(* resetFunc) (void) = 0;

void resetSettings() {
  int i=0;
  for (i=0; i<512; i++) {
    EEPROM.write(i, 0);
  }
  
  delay(500);
  resetFunc();
}

char* intToHex(byte num) {
  static char hexval[4];
  snprintf(hexval, 3, "%02x", num );
  return hexval;
}

void initPIN(int num) {
  if(num < 1) return;
  pinMode(num, OUTPUT);
  digitalWrite(num, LOW);
}


void sendFrameAscii(String message);
void sendFrame(byte *message, int msize);

void emergencia(int estado) {
    if (estado == 1) {
      pwmWrite(PWM_PIN, PWM_LENGHT);
      
      digitalWrite(RELE1, HIGH);
      digitalWrite(STATUS, HIGH);
      
      //guardar estado
      if(settings.last_status != 1) {
        settings.last_status=1;
        saveConfig();
      }
      
      texto="***ACTIVADO "; texto+=settings.frequency; texto+="(Hz)***";
      sendFrameAscii(texto);
    }
    else {
      //desactivar PWM
      digitalWrite(RELE1, LOW);
      digitalWrite(STATUS, LOW);
      pwmWrite(PWM_PIN, 0);

      //guardar estado
      if(settings.last_status != 0) {
        settings.last_status=0;
        saveConfig();
      }
      
      texto="***DESACTIVADO "; texto+=settings.frequency; texto+="(Hz)***";
      sendFrameAscii(texto);
      
    }
}


void timers() {
  if( millis() < time ) {
    time_overflow++;
  }
  time = millis();
  uptime=(time_overflow*4294967) + time/1000;
  
  every1secs=0;
  if ( (uptime-last1secs) >= 1 ) {
    last1secs=uptime;
    every1secs=1;
  }
}


double GetTemp(void) {
  unsigned int wADC;
  double t;

  // The internal temperature has to be used 
  // with the internal reference of 1.1V.
  // Channel 8 can not be selected with 
  // the analogRead function yet.

  // This code is not valid for the Arduino Mega,
  // and the Arduino Mega 2560.

#ifdef THIS_MIGHT_BE_VALID_IN_THE_FUTURE
  analogReference (INTERNAL);
  delay(20);            // wait for voltages to become stable.
  wADC = analogRead(8); // Channel 8 is temperature sensor.
#else
  // Set the internal reference and mux.
  ADMUX = (_BV(REFS1) | _BV(REFS0) | _BV(MUX3));
  ADCSRA |= _BV(ADEN);  // enable the ADC

  delay(20);            // wait for voltages to become stable.

  ADCSRA |= _BV(ADSC);  // Start the ADC

  // Detect end-of-conversion
  while (bit_is_set(ADCSRA,ADSC));

  // Reading register "ADCW" takes care of how to read ADCL and ADCH.
#if defined (__AVR_ATmega32U4__)
  wADC = ADC;      // For Arduino Leonardo
#else
  wADC = ADCW;     // 'ADCW' is preferred over 'ADC'
#endif
#endif

  // The offset of 337.0 could be wrong. It is just an indication.
  t = (wADC - 337.0 ) / 1.22;

  return (t);
}
