#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <Keypad.h>
#include <string.h>


/**
* Gauden pausua esaten digu.
* @param integer
*/
int pausua = 0;
int programa = 1;
char tenperatura[10], denbora[10];
int programak[10][2];
int keycounter = 0;
int t,d,i,p,tenp_orain,tenp_helburua, denbora_helburua;

// tenperatura aldatzen den jakiteko
int tenp_aldaketa;

unsigned long denbora_orain;
unsigned long denbora_hasiera;


// LCD
LiquidCrystal_I2C lcd(0x27,16,2);

// TENPERATURA
OneWire  ds(2);

// KEYPAD
const byte ROWS = 4; //four rows
const byte COLS = 3; //three columns
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
byte rowPins[ROWS] = {9, 8, 7, 6}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {5, 4, 3}; //connect to the column pinouts of the keypad
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

// relea
int Rele1 = 10;
int Rele2 = 11;
int Rele3 = 12;
int Rele4 = 13;


void setup()
{
  lcd.init();
  lcd.backlight();
  
  pinMode(Rele1, OUTPUT);
  pinMode(Rele2, OUTPUT); 
  pinMode(Rele3, OUTPUT); 
  pinMode(Rele4, OUTPUT); 
}

void loop() {
  
  char key = keypad.getKey();
  
  switch (pausua) {
    
    case 9:
      // programa amaitu ala ez?
      if (programa == p) {
        pausua = 0;
        
        lcd.clear();
        lcd.print("Programa amaitua.");
        delay(5000);
        
      } else {
        programa += 1;
        pausua = 6;
      }
      
      break;
    
    case 8:
      // programak ekutzatzen, denbora
      tenp_orain = getTenperatura();
      tenp_helburua = programak[programa][0];
      
      denbora_orain = (millis() - denbora_hasiera) / 1000 / 60;
      denbora_helburua = programak[programa][1];
      
      setBeroa(tenp_helburua, tenp_orain);
      
      if (denbora_orain >= denbora_helburua) {
        pausua = 9;
      }
      
      lcdIdazten(programa, tenp_helburua, denbora_helburua, tenp_orain, denbora_helburua - denbora_orain);
      
      delay(2000);
      
      break;
      
    case 7:
      // programa exekutatzen, tenperatura bila
      tenp_orain = getTenperatura();
      tenp_helburua = programak[programa][0];
      denbora_helburua = programak[programa][1];
      
      lcdIdazten(programa, tenp_helburua, denbora_helburua, tenp_orain, denbora_helburua);
      
      setBeroa(tenp_helburua, tenp_orain);
      
      if (tenp_orain >= tenp_helburua) {
        denbora_hasiera = millis();
        pausua = 8;
      }
      
      // ez bada helburua lortzen 
      // 30 minututan programa bukatzen da
      denbora_orain = (millis() - denbora_hasiera) / 1000 / 60;
      if (denbora_orain >= 1) {
         pausua = 0;
         endBeroa();
         
         lcd.clear();
         lcd.print("Programa ezeztatu egin da.");
         lcd.setCursor(0, 1);
         lcd.print("Ez da tenperatura lortzen.");
         delay(5000);
      }
      
      delay(2000);
      
      break;
      
    case 6:
      // exekutzio hasten
      lcdExekutatzen();
      denbora_hasiera = millis();
      pausua = 7;
    
      break;
    
    case 5:
      p = programa;
      programa = 1;
      pausua = 6;
      break;
    
    case 4:
      // beste programa bat
      if (key == '#') {
        programak[programa][0] = parseTenperatura(tenperatura);
        programak[programa][1] = parseDenbora(denbora);
        pausua = 1;
        break;
      }
      
      // programazioa amaitu
      if (key == '*') {
        programak[programa][0] = parseTenperatura(tenperatura);
        programak[programa][1] = parseDenbora(denbora);
        pausua = 5;
        break;
      }
      
      if (key != NO_KEY){
        denbora[keycounter] = key;
        keycounter++;
        lcd.print(key);
      } 
      
      break;
    
    case 3:
      // programaren hasiera, datuen sartzeko zai
      keycounter = 0;
      
      lcd.clear();
      lcd.print("P");
      lcd.print(programa);
      lcd.print(": Minutuak?");
      lcd.setCursor(0, 1);
      
      pausua = 4;
      
      break;
    
    case 2:
      // Hurrengo pausuar joan
      if (key == '#') {
        pausua = 3;
        break;
      }
      
      // programa reseteatu
      if (key == '*') {
          lcd.clear();
          lcd.print("Ezabatzen...");
          delay(500);
          pausua = 0;
          break;
      }
      
      if (key != NO_KEY){
        tenperatura[keycounter] = key;
        keycounter++;
        lcd.print(key);
      } 
      
      break;
      
    case 1:
      // programaren hasiera, datuen sartzeko zai
      programa++;
      keycounter = 0;
      
      lcd.clear();
      lcd.print("P");
      lcd.print(programa);
      lcd.print(": Tenperatura?");
      
      lcd.setCursor(0, 1);
      
      pausua = 2;
      break;
    
    case 0:
    default:
      programa = 0;
      pausua = 1;
      keycounter = 0;
  }
}

/**
* Tenperatura sensoretik informazioa eskuratu
* @return float
*/
float getTenperatura() {
  
  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  float celsius;
  
  ds.search(addr);
  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);        // start conversion, with parasite power on at the end
  
  delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.
  
  present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE);         // Read Scratchpad

  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
  }
  
  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }
  celsius = (float)raw / 16.0;
  
  return celsius;
}

/**
* Tenperaturarako sartutako datua int bihurtu
* @param char tenperatura
* @return int
*/
int parseTenperatura (char *tenperatura) {
  int t = atoi(tenperatura);
  if (t > 105) {
     t = 105; 
  } else if (t < 30) {
     t = 30; 
  }
  return t;
}

/**
* Denborarako teklatuarekin sartutako balioa int bihurtu
* @param char denbora
* @return int
*/
int parseDenbora (char *denbora) {
  int d = atoi(denbora);
  if (d > 999) {
    d = 999;
  } else if (d < 1) {
    d = 1; 
  }
  return d;
}

/**
* Programa exekutatzen denean ikusten dena lcdan
*/
int lcdExekutatzen () {
  lcd.clear();
  lcd.print("Pro     C    m");
  lcd.setCursor(0, 1);
  lcd.print("Exe.    C    m");
  
  return 0;
}

int lcdIdazten (int programa, int pt, int pd, int et, int ed) {
  
  lcd.setCursor(3, 0);
  lcd.print(programa);
  
  if (pt >= 10 && pt < 100 ) {
    lcd.setCursor(6, 0);
    lcd.print(pt);
  } else if ( pt >= 100 ){
    lcd.setCursor(5, 0);
    lcd.print(pt);
  }
  
  if (pd < 10) {
    lcd.setCursor(12, 0);
    lcd.print(pd);
  } else if (pd >= 10 && pd < 100 ) {
    lcd.setCursor(11, 0);
    lcd.print(pd);
  } else if ( pd >= 100 ){
    lcd.setCursor(10, 0);
    lcd.print(pd);
  }
  
  if (et > 10 && et < 100 ) {
    lcd.setCursor(6, 1);
    lcd.print(et);
  } else if ( et >= 100 ){
    lcd.setCursor(5, 1);
    lcd.print(et);
  }
  
  if (ed < 10) {
    lcd.setCursor(12, 1);
    lcd.print(ed);
  } else if (ed >= 10 && ed < 100 ) {
    lcd.setCursor(11, 1);
    lcd.print(ed);
  } else if ( ed >= 100 ){
    lcd.setCursor(10, 1);
    lcd.print(ed);
  }
}

/**
* Sua itzali
*/
int endBeroa() {
  digitalWrite(Rele1, LOW);
  digitalWrite(Rele2, LOW);
  digitalWrite(Rele3, LOW);
  digitalWrite(Rele4, LOW);
  
  return 0;
}

/**
* Bero eman behar duen eta nola erabakitzen duen kodea
*/
int setBeroa (int t_helburua, int t_orain) {
  
  // helburua berdindu edo gainditu bada
  // ez da surik ematen
  if (t_orain >= t_helburua) {
    digitalWrite(Rele1, LOW);
    digitalWrite(Rele2, LOW);
    digitalWrite(Rele3, LOW);
    digitalWrite(Rele4, LOW);
    return 0;
  }
  
  digitalWrite(Rele1, HIGH);
  
  // temperatura direfentzia 10º baino gehiago
  // sua topera
  if (t_orain + 10 < t_helburua) {
    digitalWrite(Rele1, HIGH);
    digitalWrite(Rele2, HIGH);
    digitalWrite(Rele3, HIGH);
    digitalWrite(Rele4, HIGH);
    
    return 0;
    
  // tenperatura diferentzia txikia bada
  // minimoan jarri
  } else if (t_orain + 5 < t_helburua) {
    digitalWrite(Rele1, HIGH);
    digitalWrite(Rele2, LOW);
    digitalWrite(Rele3, LOW);
    digitalWrite(Rele4, LOW);
    
    return 0;
  }
  
  return 0;
}
