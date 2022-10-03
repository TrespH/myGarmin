/*MYGARMIN PROJECT
  By Matteo Pompilio - 5Binf
  Started  -- 2018, September
  Finished -- 2018, October
  Printed  -- 2018, November
  Upgraded -- 2019, January (Rechargable 9v Battery + Battery Level Indicator)
  Upgraded -- 2019, January (Bluetooth HC-06 module + Ionic tracking app)
  Upgraded -- 2019, February (4V - 24V Recharge module + Micro USB Port)
*/
#include <NMEAGPS.h>
#include <GPSport.h>
#include <SD.h>
#include <SPI.h>
#include <Adafruit_TFTLCD.h>
#include <Adafruit_GFX.h>
#include <TouchScreen.h>
#include <TimeLib.h>
#include <SoftwareSerial.h>

#define BUFFPIXEL 20

#define LCD_CS A3
#define LCD_CD A2
#define LCD_WR A1
#define LCD_RD A0
#define LCD_RESET A4

#define TS_MINX 130
#define TS_MINY 947
#define TS_MAXX 916
#define TS_MAXY 118

#define YP A2
#define XM A3
#define YM 8
#define XP 9

//CODICI COLORI IN RGB565 (https://ee-programming-notepad.blogspot.com/2016/10/16-bit-color-generator-picker.html)
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF
#define MYGREY  0xAD55
#define ORANGE  0xFD40

#define SD_CS 53
/*
  Pila Kennstone Li-ion 9v 800mAh:
  Tensione minima funzionamento = 6.50V/6.40V
  Tensione massima = 8.33V
  Durata in registrazione ≃ 2.5h
*/
#define battery_pin A15
#define BATTERY_MAX_VOLTAGE 8.33
#define BATTERY_MIN_VOLTAGE 6.20

#define  RX 15
#define  TX 14

NMEAGPS  nmea;
gps_fix  fixx;
Adafruit_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);
TouchScreen touch = TouchScreen(XP, YP, XM, YM, 300);
TSPoint p;
SoftwareSerial btSerial(RX, TX);

File myFile;
float speedd, latt, lon, dist = 0;
unsigned long oneSecond;
short xPoint, yPoint, alt, yearr, monthh, dayy, hourr, minutee, secondd, sat;
String newFileName, cronoTime, curTime, datee;
boolean check1, check2, check3, check4, isImageLoaded = false, dead = false;

void fatDateTime(uint16_t *date, uint16_t *time) { //TRASCRITTA (Settaggio data/ora salvataggio file)
  *date = FAT_DATE(yearr, monthh, dayy);
  *time = FAT_TIME(hourr, minutee, secondd);
}

void setup() {
  Serial.begin(9600);
  while (!Serial) ;
  pinMode(SD_CS, OUTPUT);
  gpsPort.begin(9600);
  btSerial.begin(9600);
  tft.reset();
  tft.begin(0x9341);
  tft.fillScreen(MYGREY);
  tft.setRotation(2);
  startPage();
}

void loop() {
  if (setTouchPoints()) {
    Serial.println("REGISTRAZIONE TERMINATA.");
    delay(500);
    startPage();
  }
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);
  if (nmea.available(gpsPort)) {
    fixx = nmea.read();
    if (fixx.valid.location && fixx.valid.altitude && fixx.valid.speed) {
      curTime = getDateTime(1);
      latt =  fixx.latitude();
      sat = fixx.satellites;
      lon =  fixx.longitude();
      alt = fixx.altitude();
      speedd = fixx.speed_kph();
      if (speedd < 2) speedd = 0;
      else dist += speedd / (0.0036 * (millis() - oneSecond));
      oneSecond = millis();
      cronoTime = getDateTime(2);
      printValues();
      String btData = String(latt, 6) + String(lon, 6);
      btSerial.print(btData);
    }
  }
}

void startPage() {
  check1 = false, check2 = false, check3 = false, check4 = false;

  while (!check1 || !check2 /*|| !check3 */) {
    Serial.println("---CHECKING---");

    //BATTERIA ------------------------
    tft.setTextSize(2);
    tft.setCursor(175, 236);
    tft.print(readBatteryVoltage(0));
    tft.setCursor(175, 256);
    tft.print(readBatteryVoltage(1));
    //---------------------------------

    btSerial.print("Waiting..");
    ////////////CHECK1: PRESENZA MODULO GPS
    if (!check1 || !check2) {
      if (nmea.available( gpsPort )) {
        check1 = true;
        fixx = nmea.read();
        Serial.println("GPS inizializzato -----------");

        ////////////CHECK2: VALIDITA' SEGNALE GPS
        if (fixx.valid.location && fixx.valid.date && fixx.valid.time) {
          check2 = true;
          //STAMPA DATA CORRENTE SU LCD
          tft.setTextSize(1);
          tft.setTextColor(WHITE, MYGREY);
          datee = getDateTime(0);
          tft.setCursor(145, 205);
          tft.print(datee);
          //STAMPA ORARIO CORRENTE SU LCD
          curTime = getDateTime(1);
          tft.setTextColor(WHITE, MYGREY);
          tft.setTextSize(2);
          tft.setCursor(50, 236);
          tft.print(curTime);
          Serial.println("GPS valido -----------");
        }
        else {
          Serial.print("2) GPS non Valido! (Day: ");
          Serial.print(fixx.dateTime.date);
          Serial.print(" - Min: ");
          Serial.print(fixx.dateTime.minutes);
          Serial.print(" - Lat: ");
          Serial.print(fixx.latitude());
          Serial.println(")");
        }
      }
      else Serial.println("1) Lettura GPS fallita!");
    }

    ////////////CHECK3: VALIDITA' E PRESENZA SD
    if (!check3) {
      if (SD.begin(SD_CS)) {
        check3 = true;
        if (!isImageLoaded) if (bmpDraw("GPS.bmp", 0, 0)) isImageLoaded = true;
        Serial.println("Card inizializzata -----------");
      }
      else Serial.println("3) Card assente!");
    }
    checkPrint();
    //STAMPA LIVELLO BATTERIA SU LCD
    String level = readBatteryVoltage(0);
    tft.setTextSize(2);
    tft.setCursor(175, 236);
    tft.print(level);
  }

  Serial.println("Pronto! Aspetto un segnale..");
  while (!setTouchPoints()) ;

  ////////////CHECK4: SCRITTURA SU SD
  newFileName = getDateTime(4);
  checkPrint();
  SdFile::dateTimeCallback(fatDateTime);
  myFile = SD.open(newFileName, O_WRITE | O_CREAT);
  if (myFile) {
    check4 = true;
    myFile.print("date,       time,     ");
    myFile.print("latitude,  sat, longitude, ");
    myFile.println("speed, altitude");
    myFile.close();

    File myFileTest = SD.open(newFileName);
    Serial.print("Verifica Lettura da ");
    Serial.println(myFileTest.name());
    if (myFileTest.read()) Serial.println("Scrittura verificata -----------");
    myFileTest.close();
  }
  else Serial.println("4) Apertura file fallita!");
  checkPrint();
  delay(500);
  dist = 0;
  setTime(0, 0, 0, dayy, monthh, yearr);
  Serial.println("REGISTRAZIONE AVVIATA.");
}

void printValues() { //Stampa valori su LCD e MicroSD
  String output = datee + ", " +
                  getDateTime(3) + ", " +
                  String(latt, 6) + ", " +
                  String(sat) + ",   " +
                  String(lon, 6) + ", " +
                  String(speedd, 1) + ",   " +
                  String(alt);
  //Serial.print( output );
  myFile = SD.open(newFileName, O_WRITE | O_APPEND);
  if (myFile) {
    myFile.println( output );
    myFile.close();
    //Serial.println(" -OK '" + newFileName + "'-");
  }
  else Serial.println(" -ERR '" + newFileName + "'-");
  //LATITUDINE ------------------------
  tft.setCursor(17, 32);
  tft.setTextColor(BLACK, WHITE);
  tft.setTextSize(2);
  tft.setRotation(2);
  tft.print(latt, 2);
  //SATELLITI ------------------------
  tft.setCursor(109, 32);
  if (sat < 10) tft.print("0");
  tft.print(sat);
  //LONGITUDINE ------------------------
  tft.setCursor(162, 32);
  tft.print(lon, 2);
  //VELOCITA' ------------------------
  tft.setTextSize(3);
  tft.setCursor(33, 84);
  if (speedd < 10) tft.print("0");
  tft.print(speedd, 1);
  if (speedd >= 100) {
    tft.setCursor(38, 84);
    tft.print(speedd, 1);
  }
  //ALTITUDINE ------------------------
  tft.setCursor(140, 84);
  if (alt < 10) tft.print("000");
  else if (alt < 100) tft.print("00");
  else if (alt < 1000) tft.print("0");
  tft.print(alt);
  //DISTANZA ------------------------
  if (dist >= 1000) {
    float distKm = dist / 1000;
    tft.setCursor(20, 145);
    if (distKm < 10) tft.print("00");
    else if (distKm < 100) tft.print("0");
    tft.print(distKm, 1);
  }
  else {
    tft.setCursor(41, 145);
    if (dist < 10) tft.print("00");
    else if (dist < 100) tft.print("0");
    tft.print((int)dist);
  }
  //TEMPO DA START ------------------------
  tft.setTextSize(2);
  tft.setCursor(133, 148);
  tft.print(cronoTime);
  //ORARIO ------------------------
  tft.setTextColor(WHITE, MYGREY);
  tft.setCursor(50, 236);
  tft.print(curTime);
  //BATTERIA ------------------------
  tft.setTextSize(2);
  tft.setCursor(175, 236);
  tft.print(readBatteryVoltage(0));
  //tft.setCursor(175, 256);
  //tft.print(readBatteryVoltage(1));
  //---------------------------------
}

float calcDist(float flat1, float flon1, float flat2, float flon2) { //TRASCRITTA (Distanza in metri tra due coordinate geografiche)
  float dist_calc = 0, dist_calc2 = 0, diflat = 0, diflon = 0;
  diflat = radians(flat2 - flat1);
  flat1 = radians(flat1);
  flat2 = radians(flat2);
  diflon = radians((flon2) - (flon1));
  dist_calc = (sin(diflat / 2.0) * sin(diflat / 2.0));
  dist_calc2 = cos(flat1);
  dist_calc2 *= cos(flat2);
  dist_calc2 *= sin(diflon / 2.0);
  dist_calc2 *= sin(diflon / 2.0);
  dist_calc += dist_calc2;
  dist_calc = (2 * atan2(sqrt(dist_calc), sqrt(1.0 - dist_calc)));
  dist_calc *= 6371000.0;
  return dist_calc;
}

boolean setTouchPoints() { //Gestione touch pulsante START/STOP
  p = touch.getPoint();
  if (p.z != 1) {
    xPoint = -1;
    yPoint = -1;
    return false;
  }
  return true;
}

void checkPrint() { //Gestione delle 'x' rosse/verdi
  tft.setTextSize(1);
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);

  if (check1) tft.setTextColor(GREEN, MYGREY);
  else tft.setTextColor(RED, MYGREY);
  tft.setCursor(52, 205);
  tft.print("X");

  if (check2) {
    tft.setTextSize(1);
    tft.setTextColor(WHITE, MYGREY);
    tft.setCursor(139, 220);
    tft.print(newFileName);
    Serial.print(newFileName);
    tft.setTextColor(GREEN, MYGREY);
  }
  else tft.setTextColor(RED, MYGREY);
  tft.setCursor(70, 205);
  tft.print("X");

  if (check3) tft.setTextColor(GREEN, MYGREY);
  else tft.setTextColor(RED, MYGREY);
  tft.setCursor(52, 219);
  tft.print("X");

  if (check4) {
    tft.setTextSize(3);
    tft.setCursor(67, 280);
    tft.setTextColor(WHITE, GREEN);
    tft.print("-STOP-");
    tft.setTextSize(1);
    tft.setTextColor(GREEN, MYGREY);
  }
  else {
    tft.setTextSize(3);
    tft.setCursor(67, 280);
    if (check1 && check2 && check3) {
      tft.setTextColor(WHITE, GREEN);
      tft.print("START!");
    }
    else {
      tft.setTextColor(RED, GREEN);
      tft.print("WAIT..");
    }
    tft.setTextSize(1);
    tft.setTextColor(RED, MYGREY);
  }
  tft.setCursor(70, 219);
  tft.print("X");
}

String getDateTime(int mode) { //Creazione stringhe data/ora
  // 0: return data (y/m/d)
  // 1: return ora corrente (h:m)
  // 2: return ora cronometro (h:m:s)
  // 3: return ora corrente (h:m:s)
  // 4: return ora corrente (h_m_s)
  String stringa;
  switch (mode) {
    case 0: yearr = 2000 + fixx.dateTime.year;
      monthh = fixx.dateTime.month;
      dayy = fixx.dateTime.date;
      stringa = String(yearr) + "/";
      if (monthh > 9) stringa += String(monthh) + "/";
      else stringa += "0" + String(monthh) + "/";
      if (dayy > 9) stringa += String(dayy);
      else stringa += "0" + String(dayy);
      break;
    case 1: hourr = fixx.dateTime.hours + 1;
      minutee = fixx.dateTime.minutes;
      secondd = fixx.dateTime.seconds;
      if (hourr > 9) stringa += String(hourr) + ":";
      if (minutee > 9) stringa += String(minutee);
      else stringa += "0" + String(minutee);
      break;
    case 2: stringa += String(hour()) + ":";
      if (minute() > 9) stringa += String(minute()) + ":";
      else stringa += "0" + String(minute()) + ":";
      if (second() > 9) stringa += String(second());
      else stringa += "0" + String(second());
      break;
    case 3: hourr = fixx.dateTime.hours + 1;
      minutee = fixx.dateTime.minutes;
      secondd = fixx.dateTime.seconds;
      if (hourr > 9) stringa += String(hourr) + ":";
      if (minutee > 9) stringa += String(minutee) + ":";
      else stringa += "0" + String(minutee) + ":";
      if (secondd > 9) stringa += String(secondd);
      else stringa += "0" + String(secondd);
      break;
    case 4: hourr = fixx.dateTime.hours + 1;
      minutee = fixx.dateTime.minutes;
      secondd = fixx.dateTime.seconds;
      if (hourr > 9) stringa += String(hourr) + "_";
      if (minutee > 9) stringa += String(minutee) + "_";
      else stringa += "0" + String(minutee) + "_";
      if (secondd > 9) stringa += String(secondd) + ".txt";
      else stringa += "0" + String(secondd) + ".txt";
      break;
  }
  return stringa;
}

String readBatteryVoltage(int i) { //Lettura valore batteria
  int sensorValue = analogRead(battery_pin);
  float voltage = sensorValue * (5.00 / 1023.00) * 2;
  int level = ((voltage - BATTERY_MIN_VOLTAGE) * 100) / (BATTERY_MAX_VOLTAGE - BATTERY_MIN_VOLTAGE);

  if (level >= 100) level = 99;
  if (level <= 0) level = 0;
  if (voltage <= BATTERY_MIN_VOLTAGE) {
    dead = true;
    tft.setTextColor(RED, MYGREY);
  }
  if (dead && i == 0) return "00%";
  String level_str = String(level) + "%";
  if (level < 10) level_str = "0" + String(level);

  Serial.print("Voltaggio: ");
  Serial.print(voltage);
  Serial.print(" - Livello: ");
  Serial.println(level);

  if (level >= 50 && level < 100) tft.setTextColor(GREEN, MYGREY);
  else if (level >= 20 && level < 50) tft.setTextColor(ORANGE, MYGREY);
  else tft.setTextColor(RED, MYGREY);
  if (i == 0) return level_str;
  else return String(voltage);
}

boolean bmpDraw(char *filename, int x, int y) { //TRASCRITTA (Gestione stampa file .bmp di sfondo)
  File bmpFile; int w, h, row, col, bmpWidth, bmpHeight;
  uint8_t  sdbuffer[3 * BUFFPIXEL], lcdidx = 0, r, g, b, buffidx = sizeof(sdbuffer), bmpDepth;
  uint16_t lcdbuffer[BUFFPIXEL]; boolean  first = true, flip = true, goodBmp = false;
  uint32_t rowSize, bmpImageoffset, pos = 0, startTime = millis();
  if ((x >= tft.width()) || (y >= tft.height())) return;
  Serial.println(); Serial.print("Loading image '"); Serial.print(filename); Serial.println('\'');
  if ((bmpFile = SD.open(filename)) == NULL) {
    Serial.println("File not found");
    return false;
  }
  if (read16(bmpFile) == 0x4D42) {
    Serial.print(F("File size: ")); Serial.println(read32(bmpFile));
    (void)read32(bmpFile); bmpImageoffset = read32(bmpFile);
    Serial.print(F("Image Offset: ")); Serial.println(bmpImageoffset, DEC);
    Serial.print(F("Header size: ")); Serial.println(read32(bmpFile));
    bmpWidth  = read32(bmpFile); bmpHeight = read32(bmpFile);
    if (read16(bmpFile) == 1) {
      bmpDepth = read16(bmpFile);
      Serial.print(F("Bit Depth: ")); Serial.println(bmpDepth);
      if ((bmpDepth == 24) && (read32(bmpFile) == 0)) {
        goodBmp = true;
        Serial.print(F("Image size: ")); Serial.print(bmpWidth); Serial.print('x'); Serial.println(bmpHeight);
        rowSize = (bmpWidth * 3 + 3) & ~3;
        if (bmpHeight < 0) {
          bmpHeight = -bmpHeight;
          flip = false;
        }
        w = bmpWidth; h = bmpHeight;
        if ((x + w - 1) >= tft.width())  w = tft.width()  - x;
        if ((y + h - 1) >= tft.height()) h = tft.height() - y;
        tft.setAddrWindow(x, y, x + w - 1, y + h - 1);
        for (row = 0; row < h; row++) {
          if (flip) pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize;
          else pos = bmpImageoffset + row * rowSize;
          if (bmpFile.position() != pos) {
            bmpFile.seek(pos);
            buffidx = sizeof(sdbuffer);
          }
          for (col = 0; col < w; col++) {
            if (buffidx >= sizeof(sdbuffer)) {
              if (lcdidx > 0) {
                tft.pushColors(lcdbuffer, lcdidx, first);
                lcdidx = 0;
                first  = false;
              }
              bmpFile.read(sdbuffer, sizeof(sdbuffer)); buffidx = 0;
            }
            b = sdbuffer[buffidx++]; g = sdbuffer[buffidx++]; r = sdbuffer[buffidx++];
            lcdbuffer[lcdidx++] = tft.color565(r, g, b);
          }
        }
        if (lcdidx > 0) tft.pushColors(lcdbuffer, lcdidx, first);
        Serial.print(F("Loaded in "));
        Serial.print(millis() - startTime);
        Serial.println(" ms");
      }
    }
  } bmpFile.close();
  if (!goodBmp) {
    Serial.println("BMP format not recognized.");
    return false;
  } return true;
}

uint16_t read16(File &f) { //TRASCRITTA
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t read32(File &f) { //TRASCRITTA
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}
