#include <EEPROM.h>
#define CONFIG_VERSION "004"
#define CONFIG_START 32

struct StoreStruct {
  boolean last_status;
  unsigned int frequency; // frecuencia en Hz
  boolean enable_modem;
  byte token_period; // Tiempo en minutos antes de que se reinicie por no recibir un "Estoy vivo"
  char version_of_program[4];
} settings = {  
  FALSE,
  200,
  FALSE,
  60, //una hora
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

//http://www.instructables.com/id/two-ways-to-reset-arduino-in-software/step2/using-just-software/
// esto reinicia el ARDUINO
void(* resetFunc) (void) = 0;//declare reset function at address 0

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
      SetPinFrequencySafe(PWM_PIN, settings.frequency);
      
      digitalWrite(RELE1, RHIGH);
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
      digitalWrite(RELE1, RLOW);
      digitalWrite(STATUS, LOW);
      pwmWrite(PWM_PIN, 0);
      SetPinFrequencySafe(PWM_PIN, 1);

      //guardar estado
      if(settings.last_status != 0) {
        settings.last_status=0;
        saveConfig();
      }
      
      texto="***DESACTIVADO "; texto+=settings.frequency; texto+="(Hz)***";
      sendFrameAscii(texto);
      
    }
}




void loopAlive() {
  if (iamalive==0) {
    iamalive=uptime+(settings.token_period*60); // minutos
    //iamalive=uptime+60;
    return;
  }
  if( uptime > iamalive ){ //Si detectamos que no se han recibido paquetes "Estoy vivo" en el tiempo determinado se reinicia el arduino
      texto="*** REINICIANDO iamalive uptime="; texto+=uptime; texto+=" iamalive="; texto+=iamalive; texto+=" ***"; sendFrameAscii(texto);
      delay(200);
      resetFunc();
      return;    
  }
}

void loopDisableRelay(){
  if (rele_time==0) {
    return;
  }
  if( uptime > rele_time ){ //si han pasado 10 segundos desactivamos botonera
      texto="*** RELE2 OFF ***"; sendFrameAscii(texto);
      digitalWrite(RELE2, RLOW);
      rele_time=0;    
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
