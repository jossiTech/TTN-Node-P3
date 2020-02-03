//for JT-P3 ###########################################
//                                                    #
// Arduino IDE: set to Arduino Nano with 328P         #
// v.2.0                                              #
//                                                    #
//In C:\Program Files (x86)\Arduino\hardware\arduino\avr\cores\arduino\HardwareSerial.h ==> do not work, instead:
//in C:\Users\micha\AppData\Local\Arduino15\packages\arduino\hardware\avr\1.8.2\cores\arduino\HardwareSerial.h
// change #define SERIAL_RX_BUFFER_SIZE 64 to #define SERIAL_RX_BUFFER_SIZE 100 
//for JT-P3 ###########################################

// necessary libraries#################################
  #include <lmic.h>
  #include <hal/hal.h>
  #include <CayenneLPP.h>
  #include <OneWire.h> 
  #include <DallasTemperature.h>
  #include <DHT.h>  
  #include <EEPROM.h>
// necessary libraries##################
  

  uint8_t devEUI[8];
  uint8_t appEUI[8];
  uint8_t appKey[16];


//prepare 1-Wire-Busses (16 for JT-P3)##################
  #define ONE_WIRE_BUS 16
  OneWire oneWire(ONE_WIRE_BUS);   
  DallasTemperature sensor(&oneWire);
//prepare 1-Wire-Busses (16 for JT-P3)##################

//prepare DHT-Sensor (14 for JT-P3)###################
  DHT dht(14, DHT22);
//prepare DHT-Sensor (JT-P3: (IDE Nano, 328p) Port 14, Typ DHT11############

// Support for CayenneLPP Dataformat####################
  CayenneLPP lpp(51);
// Support for CayenneLPP Dataformat####################


//Inital Target Temperature & Hysterese#################
  uint8_t targettemp = 20;
  uint8_t targethyst = 02;
//Inital Target Temperature & Hysterese#################

//OTAA (DevEUI=LSB, APPEUI=LSB, AppKey=MSB)###
  void os_getDevEui (u1_t* buf) { memcpy(buf, devEUI, 8);}
  void os_getArtEui (u1_t* buf) { memcpy(buf, appEUI, 8);}
  void os_getDevKey (u1_t* buf) { memcpy(buf, appKey, 16);}  
//OTAA (DevEUI=LSB, APPEUI=LSB, AppKey=MSB)###
  
//Interval for Sending...###############################
  const unsigned TX_INTERVAL = 60;
//Interval for Sending...###############################

// PIN-Mapping JT-P3 & RFM95W ##########################
  const lmic_pinmap lmic_pins = {
        .nss = 10,
        .rxtx = LMIC_UNUSED_PIN,
        .rst = 9,
        .dio = {2, 6, 7},};
  static osjob_t sendjob;
// PIN-Mapping JT-P3 & RFM95W ##########################

  
//Sensors still not read...
  bool validData = false;

// Status Relais  
  bool RelaisOn = false;  

//##########################################################################################################
void setup() {
  
// Start up serial####
   Serial.begin(9600);
   Serial.println(F("========================================="));Serial.println(F("JT-P3 v.1.0 FW 2.0     https://www.jossitech.de"));
// Start up serial###


//Prepare for Configuration###
Serial.println(F("Hit <C ENTER > to configure OTAA (DevEUI, AppEUI & AppKey) or wait 10 seconds for normal operation... "));

  //Check for 10sec, if to (C)onfig or to (S)how###############
    while (millis() < 10000) {
          char x = Serial.read();
          if (x=='C' || x=='c') {OTAA2EE();}
          if (x=='S') {showOTAA();}
    }
     //Read EEPROM and write the OTAA Variables
     writeEE2OTAA();
     //Show the OTAA Variables
     showOTAA();


    
// Start up the Dallas-library######################
  sensor.begin(); 
// Start up the Dallas-library######################


// Define Relais-Port (JT-P3: A1 ###################
  pinMode(A1, OUTPUT);
// At the beginning Ralais off...
	digitalWrite(A1, LOW);
// Define Relais-Port (JT-P3: A1 ###################
	
	
// Initialize dht-Sensor and wait a bit ##########
  dht.begin();
  delay(2000);
// Initialize dht-Sensor and wait a bit ##########
	
    
// LMIC init########################################
  os_init();
  LMIC_reset();
         //Timing für 32U4 anpassen
         //LMIC_setClockError(MAX_CLOCK_ERROR * 1 / 100);
  do_send(&sendjob);
// LMIC init########################################
}
//##########################################################################################################


//##########################################################################################################
void loop() {os_runloop_once();}
//##########################################################################################################

//##########################################################################################################
void onEvent (ev_t ev) {
    //Serial.println(F("DEBUG: onEvent"));
    switch(ev) {
        case EV_JOINED:
                        LMIC_setDrTxpow(DR_SF9,14);
                        // Disable link check validation (automatically enabled
                        // during join, but not supported by TTN at this time).
                        LMIC_setLinkCheckMode(0);
                        break;
       case EV_TXCOMPLETE:
                          if (LMIC.dataLen) {

                                              //processing downlink
                                               uint8_t result0 = LMIC.frame[LMIC.dataBeg + 0];//Serial.print(" #Result0: "); Serial.print(result0);
                                               uint8_t result1 = LMIC.frame[LMIC.dataBeg + 1];//Serial.print(" #Result1: "); Serial.print(result1);
                                               uint8_t result2 = LMIC.frame[LMIC.dataBeg + 2];//Serial.print(" #Result2: "); Serial.print(result2);
                                               uint8_t result3 = LMIC.frame[LMIC.dataBeg + 3];//Serial.print(" #Result3: "); Serial.println(result3);
                                               
                                               targettemp = result0;
                                               targethyst = result1;
                                               //if (result0 == 1)    {digitalWrite(A1, HIGH);RelaisOn=true;}              
                                               //if (result0 == 0)    {digitalWrite(A1, LOW);RelaisOn=false;}
                              
                                              delay(1000);}
                                              // Schedule next transmission
                                              os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
                                              break;
       default:
            break;
                }
}
//##########################################################################################################


//##########################################################################################################
void do_send(osjob_t* j){
    //Serial.println(F("DEBUG: do_send"));
    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {} 
      else {
             ReadSensors(); 
             if (validData == true) {
                                      // Prepare upstream data transmission at the next possible time.
                                      LMIC_setTxData2(1, lpp.getBuffer(), lpp.getSize(), 0);}
          
             else {  //No valid data, waiting for next iteration. Schedule next transmission
                     os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);}
           }
    // Next TX is scheduled after TX_COMPLETE event.
}
//##########################################################################################################


//##########################################################################################################
void ReadSensors() {
  validData = false;
  //Read Temp-Sensor Dallas 1-Wire
  sensor.requestTemperatures(); float temp=(sensor.getTempCByIndex(0));
  // Why "byIndex"?  You can have more than one DS18B20 on the same wire. Here are 2/4 wires. 0 refers to the first IC on the wire 


// Read DHT ######################################
  float humidity = dht.readHumidity();
  float tempDHT = dht.readTemperature();
// Read dht ######################################

  Serial.println();
  Serial.print(F("dht Humidity : ")); Serial.print(humidity);Serial.print(F("     dht Temperature : ")); Serial.println(tempDHT);
  Serial.print(F("1-Wire temp1 : ")); Serial.print(temp); Serial.print(F("    Min.: ")); Serial.print(targettemp);Serial.print(F("    Max.: ")); Serial.println(targettemp+targethyst);
  Serial.println();


//Data in Cayenne LPP-Format #######################
  lpp.reset();
  lpp.addRelativeHumidity(1, humidity);
  lpp.addTemperature(2, tempDHT);
  lpp.addTemperature(3, temp);
  lpp.addTemperature(4, targettemp);
  lpp.addTemperature(5, targettemp + targethyst);
//Data in Cayenne LPP-Format #######################


  if (temp < targettemp) {RelaisOn == true; digitalWrite(A1, HIGH); lpp.addDigitalOutput(6,1);}
  if (temp > targettemp + targethyst) {RelaisOn == false; digitalWrite(A1, LOW); lpp.addDigitalOutput(6,0);}
  
  validData = true;
  //Serial.println("DEBUG: End ReadSensors");
}
//##########################################################################################################



//##########################################################################################################
void OTAA2EE() {
     
                  Serial.print(F("DevEUI (lsb):"));
                  Ser2EEPROM(1);

                  Serial.println();
                  Serial.print(F("AppEUI (lsb):"));
                  Ser2EEPROM(2);
                  
                  Serial.println();
                  Serial.print(F("AppKey(msb):"));
                  Ser2EEPROM(3);
}
//##########################################################################################################



//##########################################################################################################
void Ser2EEPROM(int j){
      char CHR1, CHR2;
      int h=0;
      int x=0;
      int hb, lb;      
      bool complete = false;
      

      
                 delay(500);Serial.end();Serial.begin(9600);delay(500);
                  while (!Serial.available());
                  while(complete==false) {
                        if (Serial.available()) {
                              // Breaks Loop!!! Don´t use===> Serial.write(Serial.read());
                              CHR1=Serial.read(); delay(20); 
                              CHR2=Serial.read(); delay(20);
                              if (CHR1=='}'|| CHR2=='}'){complete=true;}
                              if (x==h*3+2) {calcValue(h, CHR1, CHR2, j); h++;}
                              x++;
                        }
                  }
}
//##########################################################################################################



//##########################################################################################################
void calcValue(int i, char hb, char lb, int j) {
  byte value;
  switch (hb)  {
            case '0': value=0;break;
            case '1': value=16;break;
            case '2': value=32;break;
            case '3': value=48;break;
            case '4': value=64;break;
            case '5': value=80;break;
            case '6': value=96;break;
            case '7': value=112;break;
            case '8': value=128;break;
            case '9': value=144;break;
            case 'A': value=160;break;
            case 'B': value=176;break;
            case 'C': value=192;break;
            case 'D': value=208;break;
            case 'E': value=224;break;
            case 'F': value=240;break;
            default: break; } 
              
            switch (lb)  {
            case '0': value=value + 0;break;
            case '1': value=value + 1;break;
            case '2': value=value + 2;break;
            case '3': value=value + 3;break;
            case '4': value=value + 4;break;
            case '5': value=value + 5;break;
            case '6': value=value + 6;break;
            case '7': value=value + 7;break;
            case '8': value=value + 8;break;
            case '9': value=value + 9;break;
            case 'A': value=value + 10;break;
            case 'B': value=value + 11;break;
            case 'C': value=value + 12;break;
            case 'D': value=value + 13;break;
            case 'E': value=value + 14;break;
            case 'F': value=value + 15;break;
            default: break; } 
             
            if (j==1){devEUI[i] = value;EEPROM.write(i, devEUI[i]);}
            if (j==2){appEUI[i] = value;EEPROM.write(i+8, appEUI[i]);}
            if (j==3){appKey[i] = value;EEPROM.write(i+16, appKey[i]);}
   
}
//##########################################################################################################


//##########################################################################################################
 void showOTAA() {

  Serial.println("OTAA:");
  Serial.print("devEUI :"); for (int i = 0; i < 8; i = i + 1) { Serial.print(devEUI[i], HEX);Serial.print(":");}Serial.println();
  Serial.print("appEUI :"); for (int i = 0; i < 8; i = i + 1) { Serial.print(appEUI[i], HEX);Serial.print(":");}Serial.println();
  Serial.print("appKey :"); for (int i = 0; i < 16; i = i + 1) { Serial.print(appKey[i], HEX);Serial.print(":");}Serial.println();
 }
//##########################################################################################################


//##########################################################################################################
 void writeEE2OTAA() {
   int j=0;

  for (int i = 0; i < 8; i = i + 1) { devEUI[j]=EEPROM.read(i);j++;} j=0;
  for (int i = 8; i < 16; i = i + 1) { appEUI[j]=EEPROM.read(i);j++;} j=0;
  for (int i = 16; i < 32; i = i + 1) { appKey[j]=EEPROM.read(i);j++;} j=0;
 }
//##########################################################################################################
