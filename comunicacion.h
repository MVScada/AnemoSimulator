
int processInput(byte *rcv);
boolean XBeeSend(byte *bytes, byte msize);


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
  debug.println(txt);
  delay(20);
}

boolean checkCRC(byte *rcv, int last) {
    byte crc=0, i=0;
    for(i=0; i<last; i++) {
        crc+=rcv[i];
    }
    
    if ( rcv[last] != crc ) {
        texto="*** ERROR CRC, last="; texto+=last; texto+=" ***";sendFrameAscii(texto);
        return FALSE;
    }
    return TRUE;
}



int processInput(byte *rcv) {
    
    if(rcv[2] == 0x31) {
      // frecuencia
      if((int)rcv[1] == 3) {
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
      digitalWrite(RELE2, RHIGH);
      rele_time=uptime+waitsecs;
      texto="*** EMERG OFF RELE2 ON ***"; sendFrameAscii(texto);
      return 4;
    }
    
    else if(rcv[2] == 0x35) {
      // frecuencia
      if((int)rcv[1] == 3) {
        unsigned int freq;
        freq=(unsigned int)(rcv[3] *256 + rcv[4]);
        if(freq > 0 && freq < 10000) {
            settings.frequency=freq;
            SetPinFrequencySafe(PWM_PIN, settings.frequency);
            texto="***CHANGE FREQ ";
            texto+=freq;
            texto+=" Hz*** ";
            sendFrameAscii(texto);
        }
        return 5;
      }
      return 99;
    }
    
    else if(rcv[2] == 0x36) {
      saveConfig();
      return 6;
    }
    
    else if(rcv[2] == 0x37) {
      // devuelve fecha y hora de compilacion
      texto=__DATE__; texto+=" "; texto+=__TIME__;
      sendFrameAscii(texto);
      if (settings.enable_modem == TRUE) {
        mserial.print(texto);
      }
      #if ENABLE_XBEE
        byte message[22] = {};
        message[0] = 0xff;
        message[1] = 0x9d;
        for (int i=0;i<20;i++){
          message[i+2] = (byte)texto[i];
        }
        XBeeSend(message, 22);
        
      #endif
      
      return 99;
    }
    
    else if(rcv[2] == 0x38) {
      // borra la EEPROM
      resetSettings();
      return 99;
    }
    
    else if(rcv[2] == 0x39) { // reiniciar
      texto="*** REINICIANDO ***"; sendFrameAscii(texto);
      delay(200);
      resetFunc();
      return 9;
    }
    
    else if(rcv[2] == 0x40) { // 0x40 para activar/desactivar el modem
       if((int)rcv[1] == 2) {
         if(rcv[3] == 0x00) {
           settings.enable_modem=FALSE;
         }
         else if(rcv[3] == 0x01){
           settings.enable_modem=TRUE;
         }
       }
       else {
         settings.enable_modem = !settings.enable_modem; //Si no se le manda parametro alterna entre activado/desactivado
       }
       texto="Modem_enable = "; texto+=settings.enable_modem;
       sendFrameAscii(texto);
       saveConfig();
       return 10;
    }
    
    else if(rcv[2] == 0x41) { // Sigo vivo: Si no le llega un paquete de Sigo vivo en un tiempo determinado se reinicia el Arduino
      iamalive=uptime+(settings.token_period*60);
      texto="*** IAMALIVE TOKEN RECEIVED***"; sendFrameAscii(texto);
      return 11;
    }
    else if(rcv[2] == 0x42) { // Cambia el tiempo en el que se reinicia sin recibir un paquete aimalive/*
      if(rcv[1] == 2){
        settings.token_period = rcv[3];
        texto="*** IAMALIVE TOKEN TIME CAMBIADO A ";texto+=settings.token_period;texto+=" MINUTOS"; sendFrameAscii(texto);
      }
      else{
        texto="*** IAMALIVE TOKEN TIME ES ";texto+=settings.token_period;texto+=" MINUTOS"; sendFrameAscii(texto);
      }
      return 12;
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
    
    //digitalWrite(13, !digitalRead(13));
    //flashLed(13, 2, 50);
  
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

#if ENABLE_XBEE
boolean XBeeSend(byte *bytes, byte msize) {
    ZBTxRequest zbTx = ZBTxRequest(CoordinatorAddr64, bytes, msize);
    ZBTxStatusResponse txStatus = ZBTxStatusResponse();
    xbee.send(zbTx);
    // after sending a tx request, we expect a status response
    // wait up to half second for the status response
    if (xbee.readPacket(500)) {
      // got a response!
  
      // should be a znet tx status            	
      if (xbee.getResponse().getApiId() == ZB_TX_STATUS_RESPONSE) {
        xbee.getResponse().getZBTxStatusResponse(txStatus);
  
        // get the delivery status, the fifth byte
        if (txStatus.getDeliveryStatus() == SUCCESS) {
          // success.  time to celebrate
          //flashLed(13, 5, 50);
          //texto=" Enviado OK";
          return TRUE;
        } else {
          // the remote XBee did not receive our packet. is it powered on?
          //flashLed(13, 3, 500);
          //texto=" No enviado";
          return FALSE;
        }
      }
    }
    else if (xbee.getResponse().isError()) {
      //texto="Error de envio";
      //nss.print("Error reading packet.  Error code: ");  
      //nss.println(xbee.getResponse().getErrorCode());
      return FALSE;
    }
    else {
      // local XBee did not provide a timely TX Status Response -- should not happen
      //flashLed(13, 2, 50);
      return FALSE;
    }
  return TRUE;
}



void loopReadXBee() {
  xbee.readPacket();
  if(! xbee.getResponse().isAvailable() ) {
    return;
  }

  ZBRxResponse rx = ZBRxResponse();
  xbee.getResponse().getZBRxResponse(rx);
  mcounter=rx.getDataLength();
  //texto="XBEE <== ";
  for (int i = 0; i < mcounter; i++) {
          //texto+=" ";
          //texto+=intToHex(rx.getData()[i]);
          mbuffer[i]=rx.getData()[i];
  }
    
  //sendFrameAscii(texto);
  digitalWrite(13, !digitalRead(13));
  iamalive=uptime+(settings.token_period*60); // minutos
    
  if ( mbuffer[0] != 0x40 ) {
    return;
  }
    
  if ( ! checkCRC(mbuffer, mcounter-1) ) {
        texto="XBee <==";
        for (int i = 0; i < mcounter; i++) {
              texto+=" ";
              texto+=intToHex(rx.getData()[i]);
        }
        texto+=" => "; texto+=mcounter;
        texto+=" CRC ERROR";
        sendFrameAscii(texto);
        return;
  }
    
    
  result=processInput(mbuffer);
  if(result == 99) {
    return;
  }
    
  // respuesta
  int temp=0;
  byte message[10] = {};
    
  message[0]=0xff; // net
  message[1]=0x9c; // tipo
        
  message[2] = (byte)result;
  message[3] = (byte)settings.last_status;
  message[4] = (byte)settings.enable_modem;
  message[5] = (byte)settings.token_period;
        
  temp = (int)settings.frequency;
  message[6] = temp >> 8 & 0xff;
  message[7] = temp      & 0xff;
        
  // temperature
  temp = (int)(GetTemp()*10);
  message[8] = temp >> 8 & 0xff;
  message[9] = temp      & 0xff;
    
    
  XBeeSend(message, 10);
}




#endif
