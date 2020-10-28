/*

 The LiquidCrystal library works with all LCD displays that are compatible 
 with the Hitachi HD44780 driver. There are many of them out there, and you
 can usually tell them by the 16-pin interface.

LCD Pins
 * 1-     VSS to ground
 * 2-     VDD/VCC to +5v
 * 3-     V0 to ground 
 *           *Can use 10K potentiometer to set contrast:
 *           *ends to +5V and ground
 *           *VO pin to wiper (pot pin 2)
 * 4-     RS to digital pin 7
 * 5-     R/W to ground
 * 6-     E (Enable) digital pin 6
 * 7-10-  D0-D3 to none
 * 11-    D4 to digital pin 5
 * 12-    D5 to digital pin 4
 * 13-    D6 to digital pin 3
 * 14-    D7 to digital pin 12
 * 15-    A to +5v
 * 16-    K to ground for full LED backlight
 * RGB LCDs:
 * 16-    Red to digital PWM pin 11 (255=backlight off, 0=on)
 * 17-    Grn to digital PWM pin 10 (255=backlight off, 0=on)
 * 18-    Blu to digital PWM pin 9  (255=backlight off, 0=on)

 */

// include the library code:
#include <LiquidCrystal.h>
#include <LcdBarGraph.h>

//CONSTANTS
//Hook RPM sensor to pin 2 for interrupt
#define RPM_SENSOR 2
//Flywheel to pedal ratio (2*pi*radius or pi*diameter)
#define FLYWHEEL_RATIO 4.5
//Flywheel circumference INCHES
#define FLYWHEEL_CIRC 57.305
//debounce time in milliseconds
//Set slightly above max RPM of 200RPM = 3.33 pedal rev per sec *4.5 flywheel ratio = 15 flywheel spins/sec
//1000 millis in a sec / 15 flywheel per sec = 66.66 millis between wheel spins
#define DEBOUNCE_TIME 70
// interval at which to blink LED (milliseconds)
#define BLINK_DURATION 5
//How often to update LCD (milliseconds)
#define LCD_INTERVAL 1000
//How long to wait after last pedal to auto-pause (milliseconds)
#define PAUSE_DELAY 2000
//LCD Width in columns
#define LCD_WIDTH 20
//LCD Height in columns
#define LCD_HEIGHT 4
//Pins for the RGB LCD backlight
#define REDLITE 11
#define GREENLITE 10
#define BLUELITE 9

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(7, 6, 5, 4, 3, 12);
//LcdBarGraph params (lcdName, columns wide, lcd starting column, lcd line)
LcdBarGraph pwrBar(&lcd, LCD_WIDTH-2, 2, 0);
String timestring = "00:00";
String pausetime = "PAUSE";
int inter_pin;
const int ledPin =  13;// the number of the LED pin
int ledState = LOW; 
int sensorValue = 0;
int len = 0;
int len_rt = 0;
int gear = 0;
int rpm = 0;
float lcd_updates = 0.0;
int cur_pwr = 0;
float avg_pwr = 0.0;
float rps;
float mph;
float total_mi;
float total_in;
float avg_mph = 0.0;
float elapsedSecs;
unsigned long avg_mph_total = 0;
unsigned long cur_pwr_total = 0;
unsigned long avg_pwr_total = 0;
float total_pwr = 0.0;
//changed the unsigned longs above this line 10/15/20 from ints due to negative values
//I think I quickly exceeded the limit on ints of ~32000
unsigned long last_spin_time = 0;
unsigned long previousMillisLED = 0;     // will store last time LED was updated          
unsigned long previousMillisLCD = 0;     // will store last time LCD was updated
unsigned long startMillis;
unsigned long currentMillis;
unsigned long elapsedMillis;
unsigned long pausedMillis = 0;
bool pause_flash = 0;
byte xbar[] = {
  B11111,
  B00000,
  B10001,
  B01010,
  B00100,
  B01010,
  B10001,
  B00000
};
byte sigma[] = {
  B11110,
  B10000,
  B01000,
  B00100,
  B01000,
  B10000,
  B11110,
  B00000
};

void setup() {
  pinMode(RPM_SENSOR, INPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(REDLITE, OUTPUT);
  pinMode(GREENLITE, OUTPUT);
  pinMode(BLUELITE, OUTPUT);
  inter_pin = digitalPinToInterrupt(RPM_SENSOR);
  attachInterrupt(inter_pin, handle_spin, RISING);
  analogWrite(GREENLITE, 0);
  analogWrite(REDLITE, 255);
  analogWrite(BLUELITE, 255);
  //Serial.begin(115200);
  currentMillis = millis();
  lcd.createChar(0, xbar);
  lcd.createChar(5, sigma);
  //analogWrite(8, CONTRAST);
  // set up the LCD's number of columns and rows:
  lcd.begin(LCD_WIDTH, LCD_HEIGHT);

  //TO-DO print WELCOME message
  //Serial.println("hello");
}

void loop() {
  //blink w/o delay code  
  // check to see if it's time to blink the LED
  currentMillis = millis();
  // check for spin since last led blink finished
  if (last_spin_time > (previousMillisLED + BLINK_DURATION)) {
    // save the last time LED blinked (now)
    previousMillisLED = currentMillis;
    ledState = HIGH;
  }
  //last led blink finished and no more recent spin
  else if (currentMillis > last_spin_time + BLINK_DURATION){
    ledState = LOW;
  }
  // set the LED with the ledState of the variable:
  digitalWrite(ledPin, ledState);
  
  // update LCD on LCD_INTERVAL interval
  if (currentMillis >= previousMillisLCD + LCD_INTERVAL) {
    previousMillisLCD = currentMillis;
    update_lcd(); 
  }
}

void handle_spin() {
  unsigned long diff = millis() - last_spin_time;
  if (diff > DEBOUNCE_TIME) {
    last_spin_time = millis();
    rps = 1000.000 / diff;
    //divide by 4.5 since wheel spins 4.5x/pedal revolution
    rpm = ((rps * 60.0)/FLYWHEEL_RATIO);
    mph = (FLYWHEEL_CIRC * FLYWHEEL_RATIO * rpm * 60)/63360;
    //Serial.print("RPM:");
    //Serial.println(rpm);   
    }
}
 
void update_lcd() {
  //update time if pedal detected in PAUSE_DELAY interval, else flash
  //alternating elapsed time and "PAUSED"
  if (currentMillis - last_spin_time < PAUSE_DELAY) {
    pause_flash = 0;
    update_time_mph_pwr();
  }
  else {
    pwrBar.drawValue(0, 99);
    flash_pause(); 
  }
  
  sensorValue = analogRead(A0);
  //Serial.println(sensorValue);
  gear= map(sensorValue, 0, 611, 0, 100);

  //line1
  // set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0):
  lcd.setCursor(0, 0);
  if (cur_pwr < 10){
    lcd.print(cur_pwr);
    lcd.print(" ");
    pwrBar.drawValue(cur_pwr, 99);
  }
  else if (cur_pwr <= 99){
    lcd.print(cur_pwr);
    pwrBar.drawValue(cur_pwr, 99);
  }
  else {
    lcd.print("99");
    pwrBar.drawValue(99, 99);
  }
  
  //line2
  lcd.setCursor(0, 1);
  lcd.print(avg_pwr,1);
  len = String(avg_pwr,1).length();
  lcd.setCursor(len, 1);
  lcd.write(byte(0)); //xbar
  lcd.print("/");
  lcd.print(total_pwr,1);
  lcd.write(byte(5));//sigma
  len = len + String(total_pwr,1).length() +3; //+3 for "xbar/" and sigma
  len_rt = LCD_WIDTH - String(rpm).length() -3; //-3 for "rpm"
  //print spaces
  for (int i = len; i < len_rt; i++) {
    lcd.print(" ");
  }
  lcd.setCursor(len_rt, 1);
  lcd.print(rpm);
  lcd.print("rpm");
  
  //line3
  lcd.setCursor(0, 2);
  lcd.print(avg_mph,1);
  lcd.write(byte(0)); //xbar
  lcd.print("mph");
  len = String(avg_mph,1).length() + 4; //+4 for xbar+"mph"
  len_rt = LCD_WIDTH - String(mph, 1).length() - 3; //-3 for "mph"
  //print spaces
  for (int i = len; i < len_rt; i++) {
    lcd.print(" ");
  }
  lcd.print(mph,1);
  lcd.print("mph");  

  //line4
  lcd.setCursor(0, 3);
  lcd.print(total_mi,1);
  lcd.print("mi");
  len = String(total_mi,1).length() + 2; //2 for "mi"
  len_rt = LCD_WIDTH - (LCD_WIDTH/2) - timestring.length() + 3; //+3 for ":ss" piece
  //print spaces
  for (int i = len; i < len_rt; i++) {
    lcd.print(" ");
    len++;
  }
  lcd.print(timestring);
  len = len + timestring.length(); //-1 to remove last increment made by for loop
  len_rt = LCD_WIDTH - String(gear).length() - 1; //-1 for "%"
  for (int i = len; i < len_rt; i++) {
    lcd.print(" ");
  }
  lcd.print(gear);
  lcd.print("%");

}

void update_time_mph_pwr() {
  pause_flash = 0;
  elapsedMillis = (last_spin_time - startMillis - pausedMillis);
  elapsedSecs = elapsedMillis/1000;
  unsigned long durSS = (elapsedMillis/1000)%60;    //Seconds
  unsigned long durMM = (elapsedMillis/(60000))%60; //Minutes
    
  if (durMM < 10)
  {
    timestring ="0" + String(durMM) + ":";
  }
  else{
    timestring =String(durMM) + ":";
  }
  
  if (durSS < 10)
  {
    timestring = timestring + "0" + String(durSS);
  }
  else{
    timestring = timestring + String(durSS);
  }
  //keep track of how many times this was called so we know 
  //how much to devide the totals by since values are added to the total
  //each time this is called
  lcd_updates++; 
  avg_mph_total = avg_mph_total + mph;
  avg_mph = avg_mph_total/lcd_updates;
  //using avg_mph to calculate total instead of actual flywheel spins
  //since we don't capture 100% of the flywheel spins,
  //using avg_mph may be more accurate
  total_mi = avg_mph * (elapsedSecs/3600);

  cur_pwr = (rpm * gear)/100;
  avg_pwr_total = avg_pwr_total + cur_pwr;
  avg_pwr = avg_pwr_total/lcd_updates;
  //kilojoules = watts X seconds / 1000
  //total_pwr = (avg_pwr * elapsedSecs)/1000; //old formlua, potential for total_pwr to drop if avg_pwr does
  //avg_pwr_total = watt seconds = joules when LCD updates every 1000ms
  //use (1000/LCD_INTERVAL) in case LCD_INTERVAL is different than 1000ms
  total_pwr = (avg_pwr_total/(1000.0/LCD_INTERVAL))/1000.0;
}

void flash_pause() {
  //will flash every other LCD update
  if (pause_flash) {
    pause_flash = !pause_flash;
    rpm = 0;
    mph = 0;
    cur_pwr = 0;
    timestring = pausetime;
    pausedMillis = pausedMillis + LCD_INTERVAL;
  }
  else {
    pause_flash = !pause_flash;
    rpm = 0;
    mph = 0;
    cur_pwr = 0;
    pausetime = timestring;
    timestring = "PAUSE";
    pausedMillis = pausedMillis + LCD_INTERVAL;
  }
}

 
