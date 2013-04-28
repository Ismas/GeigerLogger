//////////////////////////////////////////////////
// GEIGER COUNTER / LOGGER V1.0.3
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
//#define LCT  1// Text Display HD8448
#define LCG  1// Graphic Display PCD8544
#define RCK  1// Clock
#define TMP  1// Termometer
#define SER  1// Serial
#define EEP  1// Eeprom
//define SDC 1// SD Card
//define GPS 1// GPS
#define DSK 1 // EEPROM external memory
///////////////////////////////
//#define SETHOUR 1
///////////////////////////////

///////////////////////////////
// Include block
// #ifdef directive does not work here
///////////////////////////////
// TIme libraries
#include "Time.h"
// Nokia screen
#include "PCD8544.h"
// SD card R/W
//#include "SD.h"
//Internal EEPROM
#include "EEPROM.h"
//Real TIme CLock
#include <Wire.h>
#include <DS1307.h> // written by  mattt on the Arduino forum and modified by D. Sjunnesson
// LCD text screen
#include <LiquidCrystal.h>
/////////////////////////////

//////////////////////////////
// PINS definition
// D pins unless indicated
/////////////////////////////
#define IRQPIN     2  // IRQ pin. Corresponds to IRQ 0
#define HVPIN      3  // PWM pin for HV. Not every pin can be PWM.
#define BUZPIN     5  // PWM pin for buzzer.
#define PCD0       7  // RS
#define PCD1       8   //SCLK - E
#define PCD2       9   //MOSI - D7
#define PCD3       10  //D/C - D6
#define PCD4       12  //RST - D5
#define PCD5       11  //SCE - D4
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
#define HVCONST   8.0        // PWM to high voltage ratio. Not sure of this
//#define TEMPCONST 0.456054688 // mv to temperature ratio theorical
#define TEMPCONST 0.38       // mv to temperature ratio adjusted
#define BATCONST  9.8          // read to mvolts battery ratio
//#define uSvCONST  0.0057      // SBM-20 Conversion CPM to uSv, PER HOUR
#define uSvCONST  0.000095      // SBM-20 Conversion CPM to uSv, PER MINUTE
//#define EEMAX     1024          // eprom memory size in bytes. This is for internal nano
//#define EEMAX     128*1024   //eeprom memory size in bytes. This is for 27LC1025
#define EEMAX     65535        //eeprom memory size in bytes. This is for 64KB
#define EEHEADERSIZE  3        // Header size on memory for information.
#define DISK1     0x50        // External eeprom i2c id 
#define SERSPEED  19200       // Serial baudrate
#define TUBEVOLT  380         // Tube voltage
#define LOOPADJUST            // Enable to display main loop time on serial
#define LOOP      48          // Main loop adjust to last 1000ms
#define  KRAD      0,017452406 // Radians to degrees
//////////////////////////////////
// Hardware globals
///////////////////////////////////
#ifdef LCG
PCD8544 nokia =  PCD8544(PCD1, PCD2, PCD3, PCD4, PCD5);
#endif
#ifdef LCT
//LiquidCrystal lcd(RS-pin, E, D4, D5, D6, D7);
LiquidCrystal lcd(7,8,12,11,10,9);
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
char          DISKWRITE = ' ';
///////////////////////////////////////////
// Graphics
///////////////////////////////////////////
static unsigned char PROGMEM diskicon[] = { 
  B00011000,
  B00111100, 
  B01011010, 
  B10011001,
  B10011001,
  B10001001,
  B01000010,  
  B00111100
};
static unsigned char PROGMEM diskicon2[] = { 
  B00011000,
  B00011000, 
  B01011010, 
  B11011011,
  B11011011,
  B11011011,
  B01001010,  
  B00111100
};
static unsigned char PROGMEM batteryicon[] = { 
  B11111100,
  B10000111, 
  B10110101, 
  B10110101, 
  B10000111, 
  B11111100,
  B00000000,
  B00000000
};
static unsigned char PROGMEM thermoicon[] = { 
  B00000000,
  B00000000,
  B01100000,
  B10011110, 
  B10000001, 
  B10011110, 
  B01100000,
  B00000000,
  B00000000
};
static unsigned char PROGMEM warnicon[] = { 
  B11000000,
  B10110000,
  B10001100,
  B10110011,
  B10110011,
  B10001100,
  B10110000,
  B11000000,
};
static unsigned char PROGMEM okicon[] = { 
  B00011000,
  B00110000,
  B01100000,
  B00110000, 
  B00011000, 
  B00001100, 
  B00000110,
  B00000000
};
///////////////////////////////////////////

//////////////////////////////////////////////////
// Hardware functions
//////////////////////////////////////////////////

////////////////////////////////////////////////
// Set system time from RTC
///////////////////////////////////////////////
void setsystemtime(){
  // Set "system" time
  setTime(RTC.get(DS1307_HR,true), RTC.get(DS1307_MIN,false), RTC.get(DS1307_SEC,false), RTC.get(DS1307_DATE,false), RTC.get(DS1307_MTH,false),RTC.get(DS1307_YR,false));
}

////////////////////////////////////////////////
// EEPROM FUNCTIONS
////////////////////////////////////////////////

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

#ifdef EEP
void reseteeprom() {
  unsigned long li;
  
  Serial.println("Resetting EEPROM");
  for (li=0;li<EEMAX;i++) {
#ifdef DSK
    writeEEPROM(DISK1, li, 0);
#endif
#ifndef DSK
    EEPROM.write(li,0);
#endif
#ifdef LCT
  lcd.setCursor(0, 0);
  lcd.println(" FORMAT ");
#endif
    if (i%256==0) {
      Serial.print(li);
      Serial.print("/"); 
      Serial.println(EEMAX);
#ifdef LCT
      lcd.setCursor(0, 1);
      lcd.print(li*100/EEMAX);
      lcd.print("%      ");
#endif
    }
    digitalWrite(LEDPIN,LOW);
    analogWrite(BUZPIN,0);
  }  
  // Reset counter
  EEpos=3;
}

void eepromdump() {
  unsigned long li;
#ifdef LCT
  lcd.setCursor(0, 0);
  lcd.println("DUMPING ");
#endif
  for (li=0; li<EEMAX; li++) {
#ifndef DSK
    sprintf(s,"%02X ",EEPROM.read(li));
#endif
#ifdef DSK
    sprintf(s,"%02X ",readEEPROM(DISK1, li));
#endif
    Serial.print(s);
#ifdef LCT
  if (li%256==0) {
    lcd.setCursor(0, 1);
    lcd.print(li*100/EEMAX);
    lcd.print("%      ");
  }
#endif
  }     
}

void eepromdumpformatted() {
  unsigned long li,lp,lcpm, epoch;

  // Read last position stored
#ifdef DSK
  lp = readEEPROM(DISK1,0) *0x10000 + readEEPROM(DISK1,1) *0x100 + readEEPROM(DISK1,2);
#endif
#ifndef DSK
  lp = EEPROM.read(0) *0x10000 + EEPROM.read(1) *0x100 + EEPROM.read(2);
#endif
#ifdef LCT
  lcd.setCursor(0, 0);
  lcd.println("DUMPINNG ");
#endif

  // Print a header
  Serial.print("Last position: ");
  Serial.println(lp);
  //Serial.println("   Epoch         Date           CPM");
  Serial.println("   Date           CPM");
  // Run the disk
  for (li=3; li<lp; li+=7) {
#ifdef DSK
    lcpm  = (unsigned)readEEPROM(DISK1,li) *0x10000 + (unsigned)readEEPROM(DISK1,li+1) *0x100 + (unsigned)readEEPROM(DISK1,li+2);
    epoch = (unsigned)readEEPROM(DISK1,li+3) *0x1000000 + (unsigned)readEEPROM(DISK1,li+4) *0x10000 + (unsigned)readEEPROM(DISK1,li+5) *0x100 + (unsigned)readEEPROM(DISK1,li+6);    
#endif
#ifndef DSK
    lcpm  = (unsigned)EEPROM.read(li) *0x10000 + (unsigned)EEPROM.read(li+1) *0x100 + (unsigned)EEPROM.read(li+2);
    epoch = (unsigned)EEPROM.read(li+3) * 0x1000000 + (unsigned)EEPROM.read(li+4) *0x10000 + (unsigned)EEPROM.read(li+5) *0x100 + (unsigned)EEPROM.read(li+6);    
#endif
    // Display it
    //Serial.print(epoch);
    //Serial.print(" ");
    // Get human date from epoch
    setTime(epoch);
    sprintf(hora,"%02d:%02d.%02d", hour(), minute(), second());
    sprintf(fecha,"%02d/%02d/%02d",day(),month(),year());
    Serial.print(fecha);
    Serial.print(" ");
    Serial.print(hora);
    Serial.print(" ");
    Serial.println(lcpm);
#ifdef LCT
    if (li%256==0) {
      lcd.setCursor(0, 1);
      lcd.print(li*100/EEMAX);
      lcd.print("%     ");
    }
#endif

  }     
  // Fix system time
  setsystemtime();
}
#endif

//////////////////////////////////////////////////

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


#ifdef LCG
/*
/////////////////////////////////////////////////
// Starting animation
/////////////////////////////////////////////////
void getnuclear(){
  float t, t2;
  int j;
  //nokia.fillcircle( 42, 24, 24, 1);
  //nokia.fillcircle( 42, 24, 8, 0);
  //nokia.fillcircle( 42, 24, 6, 1);
  for (i=0;i<60;i++) {
      t= (float)i*KRAD;
      nokia.drawline(42-(8*sin(t)),24-(8*cos(t)),42-(84*sin(t)),24-(48*cos(t)),1);
      sprintf(s,"x1 %d y1 %d x2 %d y2 %d",42-(8*sin(t)),24-(8*cos(t)),42-(84*sin(t)),24-(48*cos(t)));
      Serial.println(s);
  }
  for (j=0;j<24;j++) {
    for (i=0;i<24;i++) {
      t= (i+j)*KRAD;
      nokia.drawline(42,24,42*sin(t), 24*cos(t),0);
      t= (i+120+j)*KRAD;
      nokia.drawline(42,24,42*sin(t), 24*cos(t),0);
      t= (i+240+j)*KRAD;
      nokia.drawline(42,24,42*sin(t), 24*cos(t),0);
    }

  nokia.display();
  delay(10000);
} 
*/
#endif

/////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////
////////////////////////////////////////////
// SETUP
////////////////////////////////////////////
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
#ifdef SETHOUR 
  RTC.stop();
  RTC.set(DS1307_SEC,01);     //set the seconds
  RTC.set(DS1307_MIN,37);     //set the minutes
  RTC.set(DS1307_HR,17);      //set the hours
  RTC.set(DS1307_DOW,1);      //set the day of the week
  RTC.set(DS1307_DATE,28);    //set the date
  RTC.set(DS1307_MTH,4);     //set the month
  RTC.set(DS1307_YR,13);      //set the year
#endif

  RTC.stop();
  delay(50);
  setsystemtime();
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

#ifdef LCG
  //Configure Screen
  nokia.init();
  nokia.setContrast(60);
  //Invert screen if desired
  //nokia.command(PCD8544_DISPLAYCONTROL | PCD8544_DISPLAYINVERTED);
  //Start message
  nokia.clear();
  nokia.println("GEIGER logger\n Ismas 2013\n");
#ifdef LCG
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
  delay(1000);
  //getnuclear();

#endif

#ifdef LCT
  lcd.begin(8, 2);
  lcd.setCursor(0, 0);
  lcd.println(" Geiger ");
  lcd.setCursor(0, 1);
  lcd.print("dosimetr");
  delay(1000);
  lcd.setCursor(0, 0);
  lcd.println("(C)Ismas");
  lcd.setCursor(0, 1);
  lcd.print("  2013  ");
  delay(1000);
#endif

#ifdef SDC
  //SD initialization
#endif

  // Clear CPM buffer
  for (i=0;i<59;i++) CPMB[i]=0;

  // Start GM tube
  setHV(TUBEVOLT);

  // FAST PWM with OCRA
  /* 
   // This kills display
   TCCR2A = _BV(COM2A0) | _BV(COM2B1) | _BV(WGM21) | _BV(WGM20);
   TCCR2B = _BV(WGM22) | _BV(CS22);
   OCR2A = 18;
   OCR2B = 9;
   */

  // Start test flash
  analogWrite(BUZPIN, 16);      // Buzzer PWM
  digitalWrite(LEDPIN,HIGH);   // Lights light

  // Start GM sensing interupt
  attachInterrupt(IRQ,irqattend,RISING);
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
// Display function
////////////////////////////////////////////////
void muestra(){
  unsigned int ent = 0;
  unsigned int dec = 0;
  char susv[6];
  char minpassed = ' ';
  char hourpassed = ' ';
  char w;
  
  // Fixed point uSv;
  ent = (int)uSv;
  dec = (int)((uSv - (float)ent) * 1000);
  //String uSv
  sprintf(susv,"%d.%d%d%d",ent, dec/100, (dec % 100)/10,dec % 10);

  //Not confiable warning
  if (millis()<60000) minpassed='*';
  if (millis()<3600000) hourpassed='*';

  // Get hour
#ifdef RCK
  // Gest hour directly from RTC - obsolete
  //sprintf(hora,"%02d:%02d.%02d", RTC.get(DS1307_HR,true),RTC.get(DS1307_MIN,false),RTC.get(DS1307_SEC,false));
  //sprintf(fecha,"%02d/%02d/%02d",RTC.get(DS1307_DATE,false),RTC.get(DS1307_MTH,false),RTC.get(DS1307_YR,false));
  //Get hour from system time
  //sprintf(hora,"%02d:%02d.%02d", hour(), minute(), second());
  //sprintf(fecha,"%02d/%02d/%02d",day(),month(),year());
  // Get hour from system time, compact
  sprintf(hora,"%02d:%02d", hour(), minute(), second());
  sprintf(fecha,"%02d%02d%02d",year(),month(),day());
#endif
#ifdef LCG
  // LCD graphic DISPLAY COMPOSITION
  nokia.clear();
  nokia.drawbitmap(0, 0, thermoicon, 8, 8, 1);
  sprintf(s,"%d", TEMP);
  nokia.drawstring(9,0,s);
  nokia.drawbitmap(24, 0, batteryicon, 8, 8, 1);
  sprintf(s,"%d.%d", BAT/1000, (BAT%1000)/100);
  nokia.drawstring(32,0,s);
#ifdef EEP
  // Storage icon
  if (DISKWRITE!=' ') {
    DISKWRITE=' ';
    nokia.drawbitmap(56, 0, diskicon2, 8, 8, 1);
  } else {
    nokia.drawbitmap(56, 0, diskicon, 8, 8, 1);
  };
#endif
  // Warning signs
  // If danger (dose>10 times background) invert screen
  if ( (CPM>350) || (ent>2)) {
    nokia.command(PCD8544_DISPLAYCONTROL | PCD8544_DISPLAYINVERTED);  
  }
  // If high (dose>2 times background) mark icon
  if ((CPS>10) || (CPM>70) || (dec>450)) {
    nokia.drawbitmap(70, 0, warnicon, 8, 8, 1);
  } else {
    nokia.drawbitmap(70, 0, okicon, 8, 8, 1);
  }
  // Screen text
  sprintf(s,"%s %s", fecha, hora);
  nokia.drawstring(0,9,s);
  sprintf(s,"CPS %d CPM %d%c",CPS, CPM, minpassed);
  nokia.drawstring(0,21,s);
  sprintf(s,"uSv/h %s%c\n",susv,hourpassed);
  nokia.drawstring(0,29,s);
  // Graphic bars
  // Normal level line
  nokia.drawline(9, 37,9, 48, 1);
  // Graphic CPS bar
  w = 8*CPS; if (w>84) w = 84;
  nokia.fillrect(0,37,w, 3,1);
  // Graphic CPM bar
  w = CPM/4; if (w>84) w = 84;
  nokia.fillrect(0,41, w, 3,1);
 // Graphic uSv/h bar
   w = ent*30 + (dec/30) ; if (w>84) w = 84;
  nokia.fillrect(0,45, w, 3,1);
  // Show it
  nokia.display();
#endif
#ifdef LCT
  // Text display composition
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(CPS);
  lcd.print(" ");
  lcd.print(CPM);
  lcd.print(minpassed);
  lcd.setCursor(0, 1);
  lcd.print(susv);
  lcd.print(hourpassed);
  lcd.print(BAT/100);
#endif
#ifdef SER
  // Epoch displayed for debuggin purposes
  //Serial.print(now());
  //Serial.print(" ");
  Serial.print(fecha);   
  Serial.print(" ");  
  Serial.print(hora);
  Serial.print(" Tc:");  
  Serial.print(TEMP);
  Serial.print(" CPS:"); 
  Serial.print(CPS);
  Serial.print(" CPM:"); 
  Serial.print(CPM);
  Serial.print(minpassed);
  Serial.print(" uSv/h:");  
  Serial.print(susv); 
  Serial.println(hourpassed);
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
  Serial.print("Temp (C): ");
  Serial.println(TEMP);
  Serial.print("Batt (mv): ");
  Serial.println(BAT);
  Serial.print("Memory (bytes): ");
  Serial.println(EEMAX);
  Serial.print("Position (byte): ");
  Serial.println(EEpos);
  Serial.print("Tube supply (V): ");
  Serial.println(TUBEVOLT);
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
  // Internal EEPROM - only 24 bits are saved
  // Write CPM
  t = CPM & 0x00FFFFFF;
  Serial.print("Stored: ");
  Serial.print(t);
#ifdef DSK
  writeEEPROM(DISK1, EEpos++, (t&0x00FF0000)>>16); 
  writeEEPROM(DISK1, EEpos++, (t&0x0000FF00)>>8); 
  writeEEPROM(DISK1, EEpos++, (t&0x000000FF)); 
#endif
#ifndef DSK
  EEPROM.write(EEpos++, (t&0x00FF0000)>>16); 
  EEPROM.write(EEpos++, (t&0x0000FF00)>>8); 
  EEPROM.write(EEpos++, (t&0x000000FF)); 
#endif  
  // Write Epoch (Linux time from 1/1/1970)
  t = now();
  Serial.print(" Epoch: ");
  Serial.print(t);
#ifdef DSK
  writeEEPROM(DISK1, EEpos++, (t&0xFF000000)>>24); 
  writeEEPROM(DISK1, EEpos++, (t&0x00FF0000)>>16); 
  writeEEPROM(DISK1, EEpos++, (t&0x0000FF00)>>8); 
  writeEEPROM(DISK1, EEpos++, (t&0x000000FF)); 
#endif
#ifndef DSK
  EEPROM.write(EEpos++, (t&0xFF000000)>>24); 
  EEPROM.write(EEpos++, (t&0x00FF0000)>>16); 
  EEPROM.write(EEpos++, (t&0x0000FF00)>>8); 
  EEPROM.write(EEpos++, (t&0x000000FF)); 
#endif  
  // write new pos - only 24 bits are saved
  t = EEpos & 0x00FFFFFF;
  Serial.print(" Position: ");
  Serial.println(t);
#ifdef DSK
  writeEEPROM(DISK1, 0, (t&0x00FF0000)>>16); 
  writeEEPROM(DISK1, 1, (t&0x0000FF00)>>8); 
  writeEEPROM(DISK1, 2, (t&0x000000FF)); 
#endif
#ifndef DSK
  EEPROM.write(0, (t&0x00FF0000)>>16); 
  EEPROM.write(1, (t&0x0000FF00)>>8); 
  EEPROM.write(2, (t&0x000000FF)); 
#endif  
  // Cicle memory
  EEpos %= EEMAX;
  // Jump header
  if (EEpos < EEHEADERSIZE) EEpos = EEHEADERSIZE;
#endif

  DISKWRITE = '*';
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
  calculaCPM();
  calculauSv();
  // RESET CPS - Is now done at calculaCPM(); 
  //CPS = 0;  
}

//////////////////////////////////////////////////
// Main
//////////////////////////////////////////////////
void loop(){
  unsigned long tt;
  // A second passed

  // Loop time adjust
  // It must last as close to 1000ms as possible
  // to adjust, enable LOOPADJUST 
#ifdef LOOPADJUST
  tt = millis();
#endif
  everysecond();
  digitalWrite(LEDPIN,LOW);
  analogWrite(BUZPIN,0);

  // Make 20ms long pings
  // THIS MUST BE ADJUSTED FOR EVERY SCREEN
  // 49 for LCT
  // 47 for LCG
  for (i=0;i<LOOP;i++) {
    delay(19);
    digitalWrite(LEDPIN,LOW);
    analogWrite(BUZPIN,0);
  }

  // Attend to serial
  if (Serial.available()) {
    switch (Serial.read()) {
    case 'D': 
      eepromdump(); 
      break;
    case 'L': 
      eepromdumpformatted(); 
      break;
    case 'R': 
      reseteeprom(); 
      break;
    case 'I': 
      info(); 
      break;
    case 'T': 
      digitalWrite(LEDPIN,HIGH);
      analogWrite(BUZPIN,16);
      break;
    case 10 :
    case 13 : 
      break;
    default: 
      Serial.println("# ?: Help | I: Info | L: Show log | T: Test tick | D: EEPROM data dump | R: Reset EEPROM (care!) | ");
    }
  }  
  digitalWrite(LEDPIN,LOW);
  analogWrite(BUZPIN,0);
#ifdef LOOPADJUST
  // Time control - enable to display time & adjust loop duration
  tt = millis() - tt; 
  Serial.println(tt);
#endif
}
//////////////////////////////////////////////////
// END
//////////////////////////////////////////////////



