//////////////////////////////////////////////////
// GEIGER COUNTER / LOGGER V1.0
// (c) Ismas 2013
//
// License Creative Commons Attribution-NonCommercial-ShareAlike
// CC BY-NC-SA 
// Do whatever you want with it as long you cite me,
// not shell it and preserve this licence.
// This includes work of others, mainly libraries
// Credited where corresponds.
//////////////////////////////////////////////////

////////////////////////////
// CAPABILITES SELECTOR
////////////////////////////
// Uncomment to enable.
#define LCD  1// Display
#define RCK  1// Clock
#define TMP  1// Termometer
#define SER  1// Serial
#define EEP  1// Eeprom
//define SDC 1// SD Card
//define GPS 1// GPS
#define DSK 1 // EEPROM external memory
///////////////////////////////

///////////////////////////////
// Include block
// #ifdef directive does not work here
///////////////////////////////
// Nokia screen
#include "PCD8544.h"
// SD card R/W
//#include "SD.h"
//Internal EEPROM
#include "EEPROM.h"
//Real TIme CLock
#include <Wire.h>
#include <DS1307.h> // written by  mattt on the Arduino forum and modified by D. Sjunnesson
/////////////////////////////

//////////////////////////////
// PINS definition
// D pins unless indicated
/////////////////////////////
#define IRQPIN     2  // IRQ pin. Corresponds to IRQ 0
#define HVPIN      3  // PWM pin for HV. Not every pin can be PWM.
#define BUZPIN     5  // PWM pin for buzzer.
#define PCD1       8   //SCLK
#define PCD2       9   //MOSI
#define PCD3       10  //D/C
#define PCD4       12  //RST
#define PCD5       11  //SCE
#define LEDPIN     13 // Ping for led & buzzer activation
#define TEMPPIN    7  //Analog
#define BATPIN     6  //Analog 
#define SDAPIN     4  //Analog. Only for description purposes.
#define SCLPIN     5  // Analog. Only for description purposes.
////////////////////////

/////////////////////////////
// Other HW definitions
////////////////////////////
#define IRQ        0         // Matches to pin 2
//#define HVCONST   2.15     // PWM to high voltage ratio. Not sure of this
#define HVCONST   8.5        // PWM to high voltage ratio. Not sure of this
#define TEMPCONST 0.456054688 // mv to temperature ratio
#define BATCONST  10.15          // read to mvolts battery ratio
#define uSvCONST  0.0057      // SBM-20 Conversion CPM to uSv, 
#define EEMAX     1024        // eprom memory size in bytes. This is for internal nano
//#define EEMAX     128*1024  //eeprom memory size in bytes. This is for 27LC1025
#define DISK1     0x50        // External eeprom i2c id 
#define SERSPEED  19200       // Serial baudrate
//////////////////////////////////
// Hardware globals
///////////////////////////////////
#ifdef LCD
PCD8544 nokia =  PCD8544(PCD1, PCD2, PCD3, PCD4, PCD5);
#endif
////////////////////////////////////

///////////////////////////////////
// Variable Globals
/////////////////////////////////////
unsigned long i = 0;  // Is used to several long counters
char s[40];
#ifdef SDC
File  dataFile;
#endif
unsigned int  tcps;
unsigned int  TEMP = 0;
unsigned int  CPS  = 0;
unsigned int  CPMB[60];    //To store 60 sec values
byte          CPMpos = 0;
unsigned int  CPM = 0;
float         uSv = 0.0;
float         uSvB[60];  //To store 60 min values
byte          uSvpos = 0;
unsigned long EEpos = 3; //32 bits variable, only 24 bits addressing
unsigned int  BAT = 0;  // Battery level x1000;
char          hora[9];
char          fecha[11];
///////////////////////////////////////////

//////////////////////////////////////////////////
// Hardware functions
//////////////////////////////////////////////////

////////////////////////////////////////////
// SETUP
////////////////////////////////////////////
void setup(){
  // Set pins
  pinMode(HVPIN, OUTPUT);    
  pinMode(BUZPIN, OUTPUT);    
  pinMode(LEDPIN, OUTPUT);

  // RTC
#ifdef RCK
  // RTC HOUR SET
  // Uncomment and change to set clock
 
   RTC.stop();
   RTC.set(DS1307_SEC,00);     //set the seconds
   RTC.set(DS1307_MIN,50);     //set the minutes
   RTC.set(DS1307_HR,20);      //set the hours
   RTC.set(DS1307_DOW,3);      //set the day of the week
   RTC.set(DS1307_DATE,29);    //set the date
   RTC.set(DS1307_MTH,3);     //set the month
   RTC.set(DS1307_YR,13);      //set the year
  
  RTC.stop();
  delay(50);
  RTC.start();
  delay(50);
#endif

  // Serial  
#ifdef SER
  Serial.begin(SERSPEED);
#endif

#ifdef EEP
  // Eeprom initialization and dump
  //eepromdump();
  //reseteeprom();
#ifdef DSK
  // Read 24 bits actual position
  EEpos = (unsigned long)(readEEPROM(DISK1,0) *0x10000 + readEEPROM(DISK1,1) *0x100 + readEEPROM(DISK1,2)); //Posicion actual
#endif
#ifndef DSK
  // Read 24 bits actual position
  EEpos = (unsigned long)(EEPROM.read(0) *0x10000 + EEPROM.read(1) *0x100 + EEPROM.read(2)); //Posicion actual
#endif
  //Jump first 3 bytes when reset
  if (EEpos==0) EEpos=3;
#endif

#ifdef LCD
  //Configure Screen
  nokia.init();
  nokia.setContrast(40);
  //Invert screen if desired
  //nokia.command(PCD8544_DISPLAYCONTROL | PCD8544_DISPLAYINVERTED);
  //Start message
  nokia.clear();
  nokia.print("GEIGER logger\n Ismas 2013\n");
#ifdef LCD
  nokia.print("LCD ");
#endif
#ifdef RCK
  nokia.print("RTC ");
#endif
#ifdef TMP
  nokia.print("TMP ");
#endif
#ifdef SER
  nokia.print("SER ");
#endif
#ifdef SDC
  nokia.print("SDC ");
#endif
#ifdef EEP
  nokia.print("EEP ");
#endif
#ifdef GPS
  nokia.print("GPS ");
#endif
#ifdef DSK
  nokia.print("GPS ");
#endif
#ifdef EEP
  nokia.print("MEM:");
  nokia.print(EEpos);
  nokia.print("/");
  nokia.print(EEMAX);
#endif
  nokia.display();
  delay(3000);
#endif


#ifdef SDC
  /*
#ifdef SER
   Serial.print("Initializing SD card...");
   #endif
   // make sure that the default chip select pin is set to
   // output, even if you don't use it:
   pinMode(10, OUTPUT);
   
   #ifdef SER
   // see if the card is present and can be initialized:
   if (!SD.begin(10)) Serial.println("Card failed, or not present");
   else Serial.println("card initialized.");
   #endif
   // so you have to close this one before opening another.
   dataFile = SD.open("TEXT.TXT", FILE_WRITE);
   #ifdef SER
   if (!dataFile) {
   Serial.println("Impossible to open file");
   return;
   }
   #endif
   delay(1000);
   */
#endif

  // Clear CPM buffer
  for (i=0;i<59;i++) CPMB[i]=0;

  // Start GM tube
  setHV(400);

  // FAST PWM with OCRA
  /* 
   // This kills display
   TCCR2A = _BV(COM2A0) | _BV(COM2B1) | _BV(WGM21) | _BV(WGM20);
   TCCR2B = _BV(WGM22) | _BV(CS22);
   OCR2A = 18;
   OCR2B = 9;
   */

  analogWrite(BUZPIN, 16);    // Buzzer PWM
  digitalWrite(LEDPIN,HIGH); // Lights light

  // Start GM sensing interupt
  attachInterrupt(IRQ,irqattend,RISING);
}

//////////////////////////////////////////////////
// Interrupt function
//////////////////////////////////////////////////
void irqattend(){
  CPS++; // Increments Counts Per Second
  analogWrite(BUZPIN, 16);   // Buzzer PWM
  digitalWrite(LEDPIN,HIGH); // Lights light
}

//////////////////////////////////////////////////
// Set PWM voltage control
//////////////////////////////////////////////////
void setHV(int volts){
  analogWrite(HVPIN, (unsigned int)((float)volts/HVCONST));  
}

//////////////////////////////////////////////////
// Read temperature
//////////////////////////////////////////////////
void gettemp() {
  TEMP = int(analogRead(TEMPPIN) * TEMPCONST); 
}

//////////////////////////////////////////////////
// Read battery
//////////////////////////////////////////////////
void getbatt() {
  BAT = int(analogRead(BATPIN) * BATCONST); 
}

////////////////////////////////////////////////
// EEPROM FUNCTIONS
////////////////////////////////////////////////
#ifdef EEP
void reseteeprom() {
  Serial.println("Resetting EEPROM");
  for (i=0;i<EEMAX;i++) {
#ifdef DSK
    writeEEPROM(DISK1, i, 0);
#endif
#ifndef DSK
    EEPROM.write(i,0);
#endif
    Serial.print(i);Serial.print("/"); Serial.println(EEMAX);
  }
  // Reset counter
 EEpos=3;
}

void eepromdump() {
  for (i=0; i<EEMAX; i++) {
#ifndef DSK
    sprintf(s,"%02X ",EEPROM.read(i));
#endif
#ifdef DSK
    sprintf(s,"%02X ",readEEPROM(DISK1, i));
#endif
    Serial.print(s);
  }     
}

void eepromdumpformatted() {
  unsigned long t1, t2, t3, t4;
  Serial.print("Position: ");
  Serial.println((unsigned long)(readEEPROM(DISK1,0) *0x10000 + readEEPROM(DISK1,1) *0x100 + readEEPROM(DISK1,2)) );
  Serial.println("Date  Lon  Lat CPM");
  for (i=3; i<EEMAX; i+=15) {
    // TODO TODO TODO Fix this
    /*
#ifdef DSK
    sprintf(s,"%04l %04l %04l %04l\n",
      (unsigned long)(readEEPROM(DISK1,i) *0x1000000 + readEEPROM(DISK1,i+1) *0x10000 + readEEPROM(DISK1,i+2) *0x100 + readEEPROM(DISK1,i+3)),
      (unsigned long)(readEEPROM(DISK1,i+4) *0x1000000 + readEEPROM(DISK1,i+5) *0x10000 + readEEPROM(DISK1,i+6) *0x100 + readEEPROM(DISK1,i+7)), 
      (unsigned long)(readEEPROM(DISK1,i+8) *0x1000000 + readEEPROM(DISK1,i+9) *0x10000 + readEEPROM(DISK1,i+10) *0x100 + readEEPROM(DISK1,i+11)), 
      (unsigned long)(readEEPROM(DISK1,i+12) *0x10000 + readEEPROM(DISK1,i+13) *0x100 + readEEPROM(DISK1,i+14)) 
     ); 
#endif
#ifndef DSK
  EEpos = (unsigned long)(EEPROM.read(0) *0x10000 + EEPROM.read(1) *0x100 + EEPROM.read(2)); //Posicion actual
#endif
*/
    Serial.print(s);
  }     
}
#endif

////////////////////////////////////////////////
// External EEPROM FUNCINTIONS
// TAKEN FROM http://www.hobbytronics.co.uk/arduino-external-eeprom
////////////////////////////////////////////////
#ifdef DSK
void writeEEPROM(int deviceaddress, unsigned int eeaddress, byte data ) {
  Wire.beginTransmission(deviceaddress);
  Wire.write((int)(eeaddress >> 8));   // MSB
  Wire.write((int)(eeaddress & 0xFF)); // LSB
  Wire.write(data);
  Wire.endTransmission();

  delay(5);
}

byte readEEPROM(int deviceaddress, unsigned int eeaddress ) {
  byte rdata = 0xFF;

  Wire.beginTransmission(deviceaddress);
  Wire.write((int)(eeaddress >> 8));   // MSB
  Wire.write((int)(eeaddress & 0xFF)); // LSB
  Wire.endTransmission();

  Wire.requestFrom(deviceaddress,1);

  //if (Wire.available()) rdata = Wire.receive();
  if (Wire.available()) rdata = Wire.read();

  return rdata;
}
#endif
//////////////////////////////////////////////////


////////////////////////////////////////////////
// Display function
////////////////////////////////////////////////
void muestra(){
  unsigned int ent = 0;
  unsigned int dec = 0;

  // Fixed point uSv;
  ent = (int)uSv;
  dec = (int)((uSv - (float)ent) * 10000);
#ifdef LCD
  // LCD DISPLAY COMPOSITION
  nokia.clear();
  nokia.println(fecha);
  nokia.println(hora);  
  sprintf(s,"T %d   B %d", TEMP, BAT);
  nokia.println(s);
  sprintf(s,"CPS %d",CPS);
  nokia.println(s);
  sprintf(s,"CPM %d",CPM);
  nokia.println(s);  
  sprintf(s,"uSv/h %d.%d",ent,dec);
  nokia.println(s);
  nokia.display();
#endif
#ifdef SER
  Serial.print(fecha);   
  Serial.print(" ");  
  Serial.print(hora);
  Serial.print(" Tc:");  
  Serial.print(TEMP);
  Serial.print(" CPS:"); 
  Serial.print(CPS);
  Serial.print(" CPM:"); 
  Serial.print(CPM);
  Serial.print(" uSv/h:");  
  Serial.print(ent); 
  Serial.print("."); 
  Serial.println(dec);
#endif  
#ifdef SDC  
  dataFile.println(s);
  dataFile.flush(); 
  dataFile.close();
#endif
}

void info() {
#ifdef SER
  Serial.print("Date: ");
  Serial.print(fecha);
  Serial.print(" ");
  Serial.println(hora);
  Serial.print("Temp: ");
  Serial.println(TEMP);
  Serial.print("Batt: ");
  Serial.println(BAT);
  Serial.print("Memory: ");
  Serial.println(EEMAX);
  Serial.print("Position: ");
  Serial.println(EEpos);
  Serial.print("SBM constant: ");
  Serial.println(uSvCONST);
  
#endif
}

//////////////////////////////////////////////////

//////////////////////////////////////////////////
// Count & log functions
//////////////////////////////////////////////////

void disklog() {
  unsigned long t;
#ifdef EEP
    // Store current CPM in EEPROM
    // Only 16 bits
#ifndef DSK
    // Internal EEPROM - only 24 bits are saved
    t = CPM & 0x00FFFFFF;
    Serial.print("Stored: ");
    Serial.print(t);
    EEPROM.write(EEpos, t / 0x10000); 
    EEpos++;
    t %= 0x10000;
    EEPROM.write(EEpos, t / 0x100); 
    EEpos++;
    t %= 0x100;
    EEPROM.write(EEpos, t); 
    EEpos++;    
    // write new pos - only 24 bits are saved
    t = EEpos & 0x00FFFFFF;
    Serial.print(" Position: ");
    Serial.println(t);
    EEPROM.write(0, t / 0x10000); 
    t %= 0x10000;
    EEPROM.write(1, t / 0x100); 
    t %= 0x100;
    EEPROM.write(2, t); 
#endif
#ifdef DSK
    // External EEPROM - only 24 bits are used
    t = CPM & 0x00FFFFFF;
    Serial.print("Stored: ");
    Serial.print(t);
    writeEEPROM(DISK1, EEpos, t / 0x10000); 
    EEpos++;
    t %= 0x10000;
    writeEEPROM(DISK1, EEpos, t / 0x100); 
    EEpos++;
    t %= 0x100;
    writeEEPROM(DISK1, EEpos, t); 
    EEpos++;
    // write new pos - only 24 bits are saved
    t = EEpos & 0x00FFFFFF;
    Serial.print(" Position: ");
    Serial.println(t);
    writeEEPROM(DISK1, 0, t / 0x10000); 
    t %= 0x10000;
    writeEEPROM(DISK1,1, t / 0x100); 
    t %= 0x100;
    writeEEPROM(DISK1,2, t); 

#endif
    // Cicle memory
    EEpos %= EEMAX;
#endif
}

void calculaCPM(){
  // Buffer rotativo para almacenar los ultimos 60 segs  
  // Almacenamos el valor en el buffer
  CPMB[CPMpos] = CPS;
  //Reset CPS inmediatelly. This improves accuracy,
  CPS = 0;
  // Calculamos la suma. Esto se puede optimizar restando el ultimo
  CPM = 0;
  for (i=0;i<59;i++) CPM += CPMB[i];
  // Incrementamos posicion
  CPMpos++;
  // Actuamos por minuto
  if (CPMpos == 60) {    
    // Actualizamos la posicion de uSv
    uSvpos++;
    uSvpos %= 60;
    disklog();      
    CPMpos = 0;
  }
}

void calculauSv(){
  // Calcumaos el uSv
  // idem que CPM
  // Calculamos uSv instantaneo y lo guardamos en el minuto actual.
  uSvB[uSvpos] = (float)CPM * uSvCONST;
  // Calculamos la hora completa
  uSv = 0.0;
  for (i=0;i<59;i++) uSv += uSvB[i];
}
//////////////////////////////////////////////////

//////////////////////////////////////////////////
// Loop functions
//////////////////////////////////////////////////
void everysecond(){
  // Show it
  muestra();
  // Get new data
  gettemp();
  getbatt();
  // Get hour
#ifdef RCK
  sprintf(hora,"%02d:%02d.%02d", RTC.get(DS1307_HR,true),RTC.get(DS1307_MIN,false),RTC.get(DS1307_SEC,false));
  sprintf(fecha,"%02d/%02d/%02d",RTC.get(DS1307_DATE,false),RTC.get(DS1307_MTH,false),RTC.get(DS1307_YR,false));
#endif
  calculaCPM();
  calculauSv();
  // RESET CPS - Is now done at calculaCPM(); 
  //CPS = 0;  
}

//////////////////////////////////////////////////
// Main
//////////////////////////////////////////////////
void loop(){
  // A second passed
  everysecond();
  // Make 100ms long pings
  for (i=0;i<10;i++) {
    delay(95);
    digitalWrite(LEDPIN,LOW);
    analogWrite(BUZPIN,0);
  }
  // Attend to serial
  if (Serial.available()) {
    switch (Serial.read()) {
      case 'D': eepromdump(); break;
      case 'L': eepromdumpformatted(); break;
      case 'R': reseteeprom(); break;
      case 'I': info(); break;
      case 'T': digitalWrite(LEDPIN,HIGH);
                analogWrite(BUZPIN,16);
                break;
      case 10 :
      case 13 : break;
      default: Serial.println("# ?: Help | I: Info | L: Show log | T: Test tick | D: EEPROM data dump | R: Reset EEPROM (care!) | ");
    }
  }  
}
//////////////////////////////////////////////////
// END
//////////////////////////////////////////////////


