
int processInput(byte *rcv);


byte CRC(byte *bytes, byte msize) {
    byte crc=0;
    int i=0;
    for(i=0; i<msize; i++) {
        crc+=bytes[i];
    }
    return crc;
}


void sendFrame(byte *msg, int msize) {
  mserial.write(0x23);
  mserial.write(msize);
  mserial.write(msg, msize);
  mserial.write( CRC(msg, msize) );
  delay(20);
}


void sendFrameAscii(String txt) {
  Serial.println(txt);
  delay(20);
}

boolean checkCRC(byte *rcv, int last) {
    byte crc=0, i=0;
    for(i=0; i<last; i++) {
        crc+=rcv[i];
    }
    
    if ( rcv[last] != crc ) {
        texto="ERROR CRC, last="; texto+=last; sendFrameAscii(texto);
        return FALSE;
    }
    return TRUE;
}


void loopReadSerial() {
  buffer[0] = '\0';
  counter=0;
  /*
  if( Serial.available() > 10 ) {
    delay(20);
  }
  */
  
  while (Serial.available() > 0) {
    buffer[counter] = Serial.read();
    counter++;
    buffer[counter] = '\0';
    if (counter >= MAX ) {break;}
  }
  
  if (counter > 0) {
    // checkCRC
    if ( ! checkCRC(buffer, counter-1) ) {
        return;
    }
    processInput(buffer);
  }
}



int processInput(byte *rcv) {
    //texto="=>";
    //texto+=buffer[0];
    //sendFrameAscii(texto);
    /*
    int i=0;
    texto="<==";
    for(i=0; i<mcounter; i++) {
      texto+=" ";
      texto+=intToHex(rcv[i]);
    }
    sendFrameAscii(texto);
    */
  
    if(rcv[2] == 0x31) {
      // frecuencia
      if((int)rcv[1] == 2) {
        unsigned int freq;
        freq=(unsigned int)(rcv[3] *256 + rcv[4]);
        if(freq > 0 && freq < 10000) {
            settings.frequency=freq;
            SetPinFrequencySafe(PWM_PIN, settings.frequency);
        }
      }
      emergencia(1);
      return 1;
    }
    
    else if(rcv[2] == 0x32) {
      emergencia(0);
      return 0;
    }
    
    else if(rcv[2] == 0x33) {
      
      if(settings.last_status == 1) {
          texto="***ACTIVADO***";
      }
      else {
          texto="***DESACTIVADO***";
      }
      sendFrameAscii(texto);
      return settings.last_status;
    }
    
    else if(rcv[2] == 0x34) {
      int waitsecs=10;
      if((int)rcv[1] == 2) {
        waitsecs=(int)rcv[3];
      }
      // desactivar anemo
      emergencia(0);
      // resetear automata
      digitalWrite(RELE2, HIGH);
      digitalWrite(RELE3, HIGH);
      digitalWrite(RELE4, HIGH);
      timetoreset2=uptime+waitsecs;
      texto="***RESET2 ";
      texto+=waitsecs;
      texto+=" segs*** ";
      sendFrameAscii(texto);
      return 4;
    }
    
    else if(rcv[2] == 0x35) {
      // frecuencia
      unsigned int freq;
      freq=(unsigned int)(rcv[3] *256 + rcv[4]);
      if(freq > 0 && freq < 10000) {
          settings.frequency=freq;
          SetPinFrequencySafe(PWM_PIN, settings.frequency);
      }
      return 5;
    }
    
    else if(rcv[2] == 0x36) {
      saveConfig();
      return 6;
    }
    
    else if(rcv[2] == 0x37) {
      texto=__DATE__; texto+=" "; texto+=__TIME__;
      sendFrameAscii(texto);
      mserial.print(texto);
      return 99;
    }
    
    else if(rcv[2] == 0x38) {
      resetSettings();
      return 99;
    }
    
    else if(rcv[2] == 0x39) { // 0x39 + [ segs to wait : default 10secs ]
      int waitsecs=10;
      if((int)rcv[1] == 2) {
        waitsecs=(int)rcv[3];
      }
      // desactivar anemo
      emergencia(0);
      // resetear automata
      // 1 pulsacion larga (10 segs)
      timetoreset1=uptime+waitsecs;
      digitalWrite(RELE2, HIGH);
      
      texto="***RESET1 ";
      texto+=waitsecs;
      texto+=" segs*** ";
      sendFrameAscii(texto);
      return 9;
    }
    
    /*
    else {
      texto="*** MSG desconocido "; texto+=rcv[2]; texto+="***";
      sendFrameAscii(texto);
      
      byte errorDesconocido[] = {0xAA, 0x01}; sendFrame(errorDesconocido, 2);
    }
    */
    
    
    // valor por defecto
    return 99;
}


void loopReadModem() {
  if (mserial.available() > 0) {
    mbuffer[0] = '\0';
    mcounter=0;
    while (mserial.available() > 0) {
      mbuffer[mcounter] = mserial.read();
      mcounter++;
      mbuffer[mcounter] = '\0';
      if (mcounter >= MAX ) {break;}
    }
    
    digitalWrite(13, !digitalRead(13));
  
    if(mcounter > 0) {
        if ( ! checkCRC(mbuffer, mcounter-1) ) {
            return;
        }
        if ( mbuffer[0] != 0x40 ) {
            return;
        }
        
        //digitalWrite(13, !digitalRead(13));
        
        result=processInput(mbuffer);
        if(result == 99) {
            return;
        }
        
        int temp=0;
        byte message[6] = {};
        
        message[0] = (byte)result;
        message[1] = (byte)settings.last_status;
        
        temp = (int)settings.frequency;
        message[2] = temp >> 8 & 0xff;
        message[3] = temp      & 0xff;
        
        // temperature
        temp = (int)(GetTemp()*10);
        message[4] = temp >> 8 & 0xff;
        message[5] = temp      & 0xff;
        
        sendFrame(message, 6);
        
        int i=0;
        texto="==>";
        for(i=0; i<6; i++) {
          texto+=" ";
          texto+=intToHex(message[i]);
        }
        sendFrameAscii(texto);
    }
  }
}
