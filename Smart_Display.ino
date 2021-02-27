#include <Wire.h>    // Libraries
#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
#include <TouchScreen.h>
#include <SPI.h>
#include <SdFat.h>

// Variables for picture rendering
#define PALETTEDEPTH   8     // support 256-colour Palette
#define BMPIMAGEOFFSET 54
#define BUFFPIXEL      20
#define NAMEMATCH ""
int x = 0;
int y = 0;

#define MINPRESSURE 200   // Define minimum and maximum pressures for the touchscreen
#define MAXPRESSURE 1000

#define gnd 18          // Define pins
#define vcc 19
#define buzzer 49

SdFatSoftSpi <12, 11, 13> SD; // Start software-emulated SPI for SD card (slow, yes, but sufficient)
#define SD_CS     10

#define DS3231_ADDRESS 0x68  // Define addresses
#define DS3231_TEMP 0x11

#define BLACK   0x0000    // Define colors
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF
#define CRIMSON 0x4000
#define REDCRIMSON 0xF000

// Variables to hold time
int prevMin = 0;
int Second = 0;             
int Minute = 0;
int Hour = 0;
int DoW = 0;
int Date = 0;
int Month = 0;
int Year = 0;

Adafruit_GFX_Button option_btn, close_btn, alarm_btn, alarm_off_btn, alarm_stop, alarm_snooze, International, Imperial, log_on, log_off;

MCUFRIEND_kbv tft;           
const int XP=8,XM=A2,YP=A3,YM=9; //240x320 ID=0x9341
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);
String nm = "sunset.bmp";

int pixel_x, pixel_y;



int Alarms[5] = {700, 730, 800, 830, 900};


// Variable to control views
int views = 1;
int Options = 0;
int ButtonsInit = 0;

// General variables
int alarms = 1;
int ring = 0;
int temp = 0;
int Log = 1;
int engaged = 0;
String TimeMode = "IMPL";


void setup() {
  // Comms
  Serial.begin(9600);   // Initiate serial communication with 9600 baud rate
  Wire.begin();         // Initiate SPI with the DS3231

  // DS3231
  pinMode(vcc, OUTPUT);   //Setting the pin modes for the defined pins
  pinMode(gnd, OUTPUT);

  // Power
  digitalWrite(vcc, HIGH);    // Provide power to the DS3231 without messy wiring
  digitalWrite(gnd, LOW);

  // Alarm buzzer
  pinMode(buzzer, OUTPUT);
  // TFT
  uint16_t ID = tft.readID();   // Read the TFT's ID and start it
  tft.begin(ID);
  tft.setRotation(1);           // Set up TFT in horizontal layout

  bool good = SD.begin(SD_CS);
    if (!good) {
        Serial.print(F("cannot start SD"));
        while (1);
    }
  dissplay();
}


void loop() {
  bool down = Touch_getXY();              // Check the touch position
  if (Options == 0){
    getTime();                              // Repeatedly update the time
    
    if (prevMin != Minute){                 // Render the display ONLY when the
      dissplay();                           // Arduino has started and refresh it per minute
      views = 1;
    }                                       
    prevMin = Minute;    
    
    tempUpdate();                           // Repeatedly update the temperature
    initOptions();                          // Bring up the buttons on first run or when switching menus
    
    option_btn.press(down && option_btn.contains(pixel_x, pixel_y));    // Send coordinates to Adafruit GFX library's button handler

    if (option_btn.justPressed()){          // Check if the options button is pressed
      option_btn.drawButton(true);
    }
    
    if (option_btn.justReleased()){
      option_btn.drawButton();              // If aforementioned button is pressed, clear the screen 
      Options = 1;                          // by drawing over it to render the options menu
      ButtonsInit = 0;
      nm = "sunset.bmp"; x = 0; y = 0;
      showBMP();
      options();
    }
    
    if (alarms == 1){                       // Check if alarms is enabled
       for (int x = 0; x < 5; x ++){        // Iterate over the Alarms array to read the alarm times
        if ((Alarms[x] - 1 == (Hour * 100 + Minute)) && engaged == 0){
          ring = 1;                         // Engage the buzzer one minute before the alarm
          initOptions();
          engaged = 1;
        }
        if (Options == 0 && ring == 0){
         tft.setCursor(160, 175);
         tft.setTextSize(2);
         tft.setTextColor(WHITE, CRIMSON);
         tft.println("No Upcoming");
         tft.setCursor(170, 195);
         tft.print("Alarms");
        }
        if (Alarms[x] == (Hour * 100 + Minute) && ring == 1){   // Play beep-beep when the alarm time matches the current time
          if ((Second % 2) == 0){
            digitalWrite(buzzer, HIGH);
          }
          else{
            digitalWrite(buzzer, LOW);
          }
        }
        
        down = Touch_getXY();               // Request touch coordinates
        alarm_stop.press(down && alarm_stop.contains(pixel_x, pixel_y));    // Send coordinates to Adafruit GFX library's button handler
        if (alarm_stop.justPressed()){
          alarm_btn.drawButton(true);
        }
        if (alarm_stop.justReleased()){
          alarm_btn.drawButton();
          ring = 0;
          ButtonsInit = 0;
          digitalWrite(buzzer, LOW);
          int engaged = 0;
          nm = "sunset.bmp"; x = 0; y = 0;
          showBMP();
          
          dissplay();
          
        }
      }  
    }
    if(Options == 0 && alarms == 0){
         tft.setCursor(160, 175);
         tft.setTextSize(2);
         tft.setTextColor(WHITE, CRIMSON);
         tft.println("Alarms");
         tft.setCursor(160, 195);
         tft.print("Disabled");
    }
  }
  else{
    options();
    initOptions();
    alarm_btn.press(down && alarm_btn.contains(pixel_x, pixel_y));
    alarm_off_btn.press(down && alarm_off_btn.contains(pixel_x, pixel_y));
    International.press(down && International.contains(pixel_x, pixel_y));
    Imperial.press(down && Imperial.contains(pixel_x, pixel_y));
    log_on.press(down && log_on.contains(pixel_x, pixel_y));
    log_off.press(down && log_off.contains(pixel_x, pixel_y));
    close_btn.press(down && close_btn.contains(pixel_x, pixel_y));

    if (alarm_btn.justPressed()){
      alarm_btn.drawButton(true);
    }
    if (alarm_off_btn.justPressed()){
      alarm_off_btn.drawButton(true);
    }
    if (close_btn.justPressed()){
      close_btn.drawButton(true);
      Serial.println("Exit button pressed");
    }
    if (International.justPressed()){
      International.drawButton(true);
    }
    if (Imperial.justPressed()){
      Imperial.drawButton(true);
    }
    if (log_on.justPressed()){
      log_on.drawButton(true);
    }
    if (log_off.justPressed()){
      log_off.drawButton(true);
    }
    
    if (alarm_btn.justReleased()){
      alarm_btn.drawButton();
      alarms = 1;
    }
    if (alarm_off_btn.justReleased()){
      alarm_off_btn.drawButton();
      alarms = 0;
    }
    if (International.justReleased()){
      International.drawButton();
      TimeMode = "INTL";
    }
    if (Imperial.justReleased()){
      Imperial.drawButton();
      TimeMode = "IMPL";
    }
    if (log_on.justReleased()){
      log_on.drawButton();
      Log = 1;
    }
    if (log_off.justReleased()){
      log_off.drawButton();
      Log = 0;
    }
    
    if (close_btn.justReleased()){
      close_btn.drawButton();
      Serial.println("Exit button pressed");
      Options = 0;
      ButtonsInit = 0;
      nm = "sunset.bmp";
      x = 0; y = 0;
      showBMP();
      nm = "temperat.bmp"; x = 240; y = 10;
      showBMP();
      dissplay();
    }
  }
}

void dissplay(){
   if (Month == 0){
      nm = "sunset.bmp"; x = 0; y = 0;
      showBMP();        // Draw the background the first time it is loaded
      nm = "temperat.bmp"; x = 240; y = 10;
      showBMP();    // Draw the thermometer icon
   }
   else{
    logger();
   } 
   
   tft.drawRoundRect(10, 10, 225, 145, 15, RED);        // Draw the box for the clock's display

   tft.drawRoundRect(140, 165, 170, 65, 15,  RED);           // Draw the alarms "box"

   getTime();   // Obtain time from RTC

   // Print the time
   tft.setTextColor(WHITE, CRIMSON);
   tft.setCursor(20, 20);
   tft.setTextSize(6);
   
   if (TimeMode == "INTL"){
      if (Hour < 10){          // Ensure the hour is presented in two digits regardless of value
        tft.print("0");       // i.e. 1 is displayed as 01
        tft.print(Hour); 
      }
      else{
        tft.print(Hour);
      }

      tft.print(":");
   
      if (Minute < 10){        // Ensure the minute is presented in two digits regardless of value
        tft.print("0");       // i.e. 1 is displayed as 01
        tft.print(Minute); 
      }
      else{
        tft.print(Minute);
      } 
   }
   else{
      if (Hour < 12 && Hour > 0){
        if (Hour < 10){
          tft.print("0");
          tft.print(Hour);
        }
        else{
          tft.print(Hour);
        }
      }
      else if (Hour == 0){
        tft.print(Hour);
      }
      else{
        if ( (Hour - 12) < 10){
          tft.print("0");
          tft.print(Hour - 12);
        }
        else{
          tft.print(Hour - 12);
        }
      }
      
      tft.print(":");
   
      if (Minute < 10){        // Ensure the minute is presented in two digits regardless of value
        tft.print("0");       // i.e. 1 is displayed as 01
        tft.print(Minute); 
      }
      else{
        tft.print(Minute);
      } 
   }
   

   tft.drawRoundRect(240, 10, 70, 145, 15, RED);  //Draw the box for the temperature display

   // Print the date, month, year and day
   tft.setCursor(20, 100);                        // Display the time accordingly
   tft.setTextColor(WHITE, CRIMSON);
   tft.setTextSize(2);
   tft.print(Date);
   tft.print(" ");
   tft.print(Months(Month));
   tft.print(", ");
   tft.print(2000 + Year - 48);
   tft.setCursor(20, 130);
   tft.print(days(DoW));   
}

// Update the temperature
void tempUpdate(){                                            
   int ht = round((float(temp)/float(100))*float(40));
   tft.fillRoundRect(240+33, 65-ht, 5, ht,2,REDCRIMSON);             // Draw the thermometer's "liquid level" dynamically
   tft.setTextSize(2);
   tft.setTextColor(WHITE);
   tft.setCursor(250, 90);
   tft.print("Temp");                                         // Display the temperature in degrees celsius
   tft.setCursor(250, 112);
   tft.setTextSize(2);
   tft.setTextColor(WHITE, BLACK);
   tft.print(temp);
   tft.print(" C");
}

void options(){                                    // Options menu
  if (views == 1){
    nm="sunset.bmp"; x = 0; y = 0;
    showBMP();     // Draw the background when first loaded
    views = 0;
  }
  else{
    tft.setTextColor(WHITE);                       // Draw the title bar
    tft.setCursor(20, 20);
    tft.setTextSize(4);
    tft.print("OPTIONS");
    tft.drawLine(10, 53, 310, 53, WHITE);
    tft.setTextSize(2);
    tft.setCursor(20, 65);
    tft.print("Alarms: ");                         // Display the options (only alarms in this case, more can be added later)
    tft.drawLine(215, 60, 215, 220, WHITE);

    if (alarms == 0){
      tft.setTextColor(RED, BLACK);                // Display the status of the alarms 
      tft.print("Disabled");
    }
    else{
      tft.setTextColor(GREEN, BLACK);
      tft.print("Enabled ");
    }

    tft.setCursor(20, 125);
    tft.setTextColor(WHITE, BLACK);
    tft.print("Mode: ");
    if (TimeMode == "INTL"){
      tft.print("Int'l");
    }
    else{
      tft.print("AM/PM");
    }

    tft.setCursor(20, 185);
    tft.setTextColor(WHITE, BLACK);
    tft.print("Logs:");

     if (Log == 0){
      tft.setTextColor(RED, BLACK);                // Display the status of the alarms 
      tft.print("Disabled");
    }
    else{
      tft.setTextColor(GREEN, BLACK);
      tft.print("Enabled ");
    }
  }
}

// Get the time from RTC
void getTime(){
  Wire.beginTransmission(DS3231_ADDRESS);    // Start the transmission at 0x68
  Wire.write(0);                             // Signal the RTC to provide time
  Wire.endTransmission();                    // End the transmission

  Wire.requestFrom(DS3231_ADDRESS, 7);
  
  Second = bcdToDec(Wire.read());    // Receive the seconds
  
  Minute = bcdToDec(Wire.read());    // Receive the minute
  
  Hour = bcdToDec(Wire.read());      // Receive the hour

  DoW = bcdToDec(Wire.read());       // Receive the day of week

  Date = bcdToDec(Wire.read());      // Receive the date of month

  Month = bcdToDec(Wire.read());     // Receive the month

  Year = bcdToDec(Wire.read());      // Receive the year

  Wire.beginTransmission(DS3231_ADDRESS);   // Request RTC to send temperature
  Wire.write(DS3231_TEMP);
  Wire.endTransmission();
  Wire.requestFrom(DS3231_ADDRESS, 1);

  temp = bcdToDec(Wire.read());   // Receive the temperature
}

// Convert binary coded decimal value to a decimal number (for I2C)
byte bcdToDec(byte val){
  return( (val/16*10) + (val%16) );  // converts BCD to Decimal
}

String days(int dayofweek){   // Convert number-coded week days to strings
  String days;                // (i.e. day "1" of week to "Sunday")
  switch(dayofweek){
    case 1: days = "Sunday"; return days; break;  
    case 2: days = "Monday"; return days; break;
    case 3: days = "Tuesday"; return days; break;
    case 4: days = "Wednesday"; return days; break;
    case 5: days = "Thursday"; return days; break;
    case 6: days = "Friday"; return days; break;
    case 7: days = "Saturday"; return days; break;
  }
}


String Months(int numberofmonth){   // Convert number-coded months to strings
  String Month;                     // (i.e. month "1" of year X to "January"
  switch(numberofmonth){
    case 1: Month = "January"; return Month; break;
    case 2: Month = "February"; return Month; break;
    case 3: Month = "March"; return Month; break;
    case 4: Month = "April"; return Month; break;
    case 5: Month = "May"; return Month; break;
    case 6: Month = "June"; return Month; break;
    case 7: Month = "July";return Month;break;
    case 8: Month = "August"; return Month; break;
    case 9: Month = "September"; return Month; break;
    case 10: Month = "October"; return Month; break;
    case 11: Month = "November"; return Month; break;
    case 12: Month = "December"; return Month; break;
  }
}

// Touchscreen
bool Touch_getXY(void)
{
    TSPoint p = ts.getPoint();
    pinMode(YP, OUTPUT);      // Set up the power lines for the resistive touch panel
    pinMode(XM, OUTPUT);
    digitalWrite(YP, HIGH);   // Provide power to the aforementioned panel
    digitalWrite(XM, HIGH);
    
    bool pressed = (p.z > MINPRESSURE && p.z < MAXPRESSURE);  // Figure out the position of the touch position
    if (pressed) 
    {
        pixel_x =  round((float(p.y) / float(742)) * 320) ;
        pixel_x = round( (float(pixel_x - 55)/float(321))*320 );
        pixel_y =  round((float(855-p.x)/float(707)) * 240);
    }

    return pressed;
}

void initOptions(){     // Initialization for the buttons
  if (ButtonsInit == 0 && Options == 1){
    alarm_btn.initButton(     &tft, 270, 70, 50, 20, WHITE, WHITE, BLACK, "ON", 2);
    alarm_off_btn.initButton( &tft, 270, 95, 50, 20, WHITE, WHITE, BLACK, "OFF", 2);
    International.initButton( &tft, 270, 130, 50, 20, WHITE, WHITE, BLACK, "INTL", 2);
    Imperial.initButton(      &tft, 270, 155, 50, 20, WHITE, WHITE, BLACK, "IMP", 2);
    log_on.initButton(        &tft, 270, 190, 50, 20, WHITE, WHITE, BLACK, "ON", 2);
    log_off.initButton(       &tft, 270, 215, 50, 20, WHITE, WHITE, BLACK, "OFF", 2);
    
    close_btn.initButton(&tft, 270, 35, 60, 25, RED, RED, WHITE, "Exit", 2);
    
    alarm_btn.drawButton(false);
    alarm_off_btn.drawButton(false);
    International.drawButton(false);
    Imperial.drawButton(false);
    log_on.drawButton(false);
    log_off.drawButton(false);
    
    close_btn.drawButton(false);
    ButtonsInit = 1;
  }
  if (ButtonsInit == 0 && Options == 0){
    option_btn.initButton(&tft,  60, 200, 100, 40, WHITE, YELLOW, BLACK, "Options", 2);
    option_btn.drawButton(false);
    ButtonsInit = 1;
  }
  if (ring == 1){
      alarm_stop.initButton(&tft,  225, 198, 170, 65, RED, RED, WHITE, "STOP", 3);
      alarm_stop.drawButton(false);
  }
}

void logger(){
  if (Log == 1){
    File logData;
    String folderName = "templogs/";
    String fileName = String(Date) + "-" + String(Month) + "-" + "20" + String(Year - 48) + ".csv";
    String line = String(Hour) + ":" + String(Minute) + "," + String(temp) + "\r\n";
  
    logData = SD.open(folderName + fileName, FILE_WRITE);

    if ( (Hour * 100 + Minute) ==  0){
      logData.print(F("Time of Day, Temperature\r\n"));
      logData.print(line);
    }
    else{
      logData.print(line); 
    }

    logData.close();
  }
}

// Drawing images
uint16_t read16(File& f) {
    uint16_t result;         // read little-endian
    f.read(&result, sizeof(result));
    return result;
}

uint32_t read32(File& f) {
    uint32_t result;
    f.read(&result, sizeof(result));
    return result;
}

uint8_t showBMP()
{
    digitalWrite(buzzer, LOW);
    File bmpFile;
    int bmpWidth, bmpHeight;    // W+H in pixels
    uint8_t bmpDepth;           // Bit depth (currently must be 24, 16, 8, 4, 1)
    uint32_t bmpImageoffset;    // Start of image data in file
    uint32_t rowSize;           // Not always = bmpWidth; may have padding
    uint8_t sdbuffer[3 * BUFFPIXEL];    // pixel in buffer (R+G+B per pixel)
    uint16_t lcdbuffer[(1 << PALETTEDEPTH) + BUFFPIXEL], *palette = NULL;
    uint8_t bitmask, bitshift;
    boolean flip = true;        // BMP is stored bottom-to-top
    int w, h, row, col, lcdbufsiz = (1 << PALETTEDEPTH) + BUFFPIXEL, buffidx;
    uint32_t pos;               // seek position
    boolean is565 = false;      //

    uint16_t bmpID;
    uint16_t n;                 // blocks read
    uint8_t ret;

    if ((x >= tft.width()) || (y >= tft.height()))
        return 1;               // off screen

    bmpFile = SD.open(nm);      // Parse BMP header

    Serial.print(SD.open(nm));
    bmpID = read16(bmpFile);    // BMP signature
    (void) read32(bmpFile);     // Read & ignore file size
    (void) read32(bmpFile);     // Read & ignore creator bytes
    bmpImageoffset = read32(bmpFile);       // Start of image data
    (void) read32(bmpFile);     // Read & ignore DIB header size
    bmpWidth = read32(bmpFile);
    bmpHeight = read32(bmpFile);
    n = read16(bmpFile);        // # planes -- must be '1'
    bmpDepth = read16(bmpFile); // bits per pixel
    pos = read32(bmpFile);      // format
    if (bmpID != 0x4D42) ret = 2; // bad ID
    else if (n != 1) ret = 3;   // too many planes
    else if (pos != 0 && pos != 3) ret = 4; // format: 0 = uncompressed, 3 = 565
    else if (bmpDepth < 16 && bmpDepth > PALETTEDEPTH) ret = 5; // palette 
    else {
        bool first = true;
        is565 = (pos == 3);               // ?already in 16-bit format
        // BMP rows are padded (if needed) to 4-byte boundary
        rowSize = (bmpWidth * bmpDepth / 8 + 3) & ~3;
        if (bmpHeight < 0) {              // If negative, image is in top-down order.
            bmpHeight = -bmpHeight;
            flip = false;
        }

        w = bmpWidth;
        h = bmpHeight;
        if ((x + w) >= tft.width())       // Crop area to be loaded
            w = tft.width() - x;
        if ((y + h) >= tft.height())      //
            h = tft.height() - y;

        if (bmpDepth <= PALETTEDEPTH) {   // these modes have separate palette
            bmpFile.seek(BMPIMAGEOFFSET); //palette is always @ 54
            bitmask = 0xFF;
            if (bmpDepth < 8)
                bitmask >>= bmpDepth;
            bitshift = 8 - bmpDepth;
            n = 1 << bmpDepth;
            lcdbufsiz -= n;
            palette = lcdbuffer + lcdbufsiz;
            for (col = 0; col < n; col++) {
                pos = read32(bmpFile);    //map palette to 5-6-5
                palette[col] = ((pos & 0x0000F8) >> 3) | ((pos & 0x00FC00) >> 5) | ((pos & 0xF80000) >> 8);
            }
        }

        // Set TFT address window to clipped image bounds
        tft.setAddrWindow(x, y, x + w - 1, y + h - 1);
        for (row = 0; row < h; row++) { // For each scanline...
            // Seek to start of scan line.  It might seem labor-
            // intensive to be doing this on every line, but this
            // method covers a lot of gritty details like cropping
            // and scanline padding.  Also, the seek only takes
            // place if the file position actually needs to change
            // (avoids a lot of cluster math in SD library).
            uint8_t r, g, b, *sdptr;
            int lcdidx, lcdleft;
            if (flip)   // Bitmap is stored bottom-to-top order (normal BMP)
                pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize;
            else        // Bitmap is stored top-to-bottom
                pos = bmpImageoffset + row * rowSize;
            if (bmpFile.position() != pos) { // Need seek?
                bmpFile.seek(pos);
                buffidx = sizeof(sdbuffer); // Force buffer reload
            }

            for (col = 0; col < w; ) {  //pixels in row
                lcdleft = w - col;
                if (lcdleft > lcdbufsiz) lcdleft = lcdbufsiz;
                for (lcdidx = 0; lcdidx < lcdleft; lcdidx++) { // buffer at a time
                    uint16_t color;
                    // Time to read more pixel data?
                    if (buffidx >= sizeof(sdbuffer)) { // Indeed
                        bmpFile.read(sdbuffer, sizeof(sdbuffer));
                        buffidx = 0; // Set index to beginning
                        r = 0;
                    }
                    switch (bmpDepth) {          // Convert pixel from BMP to TFT format
                        case 24:
                            b = sdbuffer[buffidx++];
                            g = sdbuffer[buffidx++];
                            r = sdbuffer[buffidx++];
                            color = tft.color565(r, g, b);
                            break;
                        case 16:
                            b = sdbuffer[buffidx++];
                            r = sdbuffer[buffidx++];
                            if (is565)
                                color = (r << 8) | (b);
                            else
                                color = (r << 9) | ((b & 0xE0) << 1) | (b & 0x1F);
                            break;
                        case 1:
                        case 4:
                        case 8:
                            if (r == 0)
                                b = sdbuffer[buffidx++], r = 8;
                            color = palette[(b >> bitshift) & bitmask];
                            r -= bmpDepth;
                            b <<= bmpDepth;
                            break;
                    }
                    lcdbuffer[lcdidx] = color;

                }
                tft.pushColors(lcdbuffer, lcdidx, first);
                first = false;
                col += lcdidx;
            }           // end cols
        }               // end rows
        tft.setAddrWindow(0, 0, tft.width() - 1, tft.height() - 1); //restore full screen
        ret = 0;        // good render
    }
    bmpFile.close();
    return (ret);
}
