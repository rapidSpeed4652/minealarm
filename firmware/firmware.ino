//Written by rapidSpeed4652 :D
//Taken help from Gemini 3.8 Flash to learn

#include <Adafruit_GFX.h> // graphics library
#include <Adafruit_ST7789.h> // driver for the ST7789 screen
#include <SPI.h> 
#include <OneButton.h>

// Display Pins based on KiCad schematic
#define TFT_SCLK D9
#define TFT_MOSI D10
#define TFT_RST  D8
#define TFT_DC   D4
#define TFT_CS   D5
#define TFT_BL   D6

// Button Pins
#define BTN1_PIN D0
#define BTN2_PIN D1
#define BTN3_PIN D2
#define BTN4_PIN D3

// Buzzer Pin
#define BUZZER_PIN D7

OneButton btn1(BTN1_PIN, true);
OneButton btn2(BTN2_PIN, true);
OneButton btn3(BTN3_PIN, true);
OneButton btn4(BTN4_PIN, true);

// Fix setColRowStart() by exposing it via a subclass
class MyST7789 : public Adafruit_ST7789 {
public:
  MyST7789(int8_t cs, int8_t dc, int8_t mosi, int8_t sclk, int8_t rst)
    : Adafruit_ST7789(cs, dc, mosi, sclk, rst) {}
  void setOffsets(uint8_t col, uint8_t row) {
    _colstart = _colstart2 = col;
    _rowstart = _rowstart2 = row;
  }
};

MyST7789 tft(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

enum ClockState
  {
    STATE_NORMAL,
    STATE_SET_TIME,
    STATE_SET_ALARM,
    STATE_ALARM_RINGING
  };

  ClockState currentState = STATE_NORMAL;

int hours = 12;
int minutes = 0;
int seconds = 0;
bool isPM = false;

int alarmHours = 7;
int alarmMinutes = 30;
int alarmSeconds = 0;
bool alarmEnabled = false;
bool alarmIsPM = false;

bool isSnoozed = false;
int snoozeHours = 0;
int snoozeMinutes = 0;
bool snoozeIsPM = false;

int editCursor = 0;

unsigned long lastTick = 0;
unsigned long lastBeep = 0;
bool beepState = false;

void adjustClockTime(int a)
{
  if(editCursor == 0)
  {
    hours += a;

    if (hours > 12) hours = 1;
    if (hours < 1) hours = 12;
  }
  else
  {
    minutes += a;

    if (minutes > 59) minutes = 0;
    if (minutes < 0) minutes = 59;
  }
}

void adjustClockTime(int a)
{
  if(editCuror == 0)
  {
    alarmHours += a;

    if (alarmHours > 12) alarmHours = 1;
    if (alarmHours < 1) alarmHours = 12;
  }
  else
  {
    alarmMinutes += a;

    if (alarmMinutes > 59) alarmMinutes = 0;
    if (alarmMinutes < 1) alarmMinutes = 59;
  }
}

void snoozeAlarm()
{
  digitalWrite(BUZZER_PIN, LOW);

  snoozeMinutes += 5;
  snoozeHours = hours;
  snoozeIsPM = isPM;

  if(snoozeMinutes > 60)
  {
    snoozeMinutes -= 60;

    if(snoozeHours == 11)
    {
      snoozeHours = 12;
      snoozeIsPM = !snoozeIsPM;
    }
    else if(snoozeHours == 12)
    {
      snoozeHours = 1;
    }
    else
    {
      snoozeHours++;
    }
  }

  isSnoozed = true;
  currentState = STATE_NORMAL;
}

void dismissAlarm()
{
  digitalWrite(BUZZER_PIN, LOW;

  isSnoozed = false;
  currentState = STATE_NORMAL;
}

void printTwoDigits(int number) {
  if (number < 10) tft.print("0");
  tft.print(number);
}

void renderNormalClock() {
  // Main Time (Hours:Minutes:Seconds)
  tft.setTextSize(4);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setCursor(10, 10);

  printTwoDigits(hours);
  tft.print(":");
  printTwoDigits(minutes);
  tft.print(":");
  printTwoDigits(seconds);

  // AM / PM Indicator
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
  tft.setCursor(215, 12);
  tft.print(isPM ? "PM" : "AM");

  // Alarm Footer Status
  tft.setCursor(10, 52);
  tft.setTextSize(2);
  if (alarmEnabled) {
    tft.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
    tft.print("ALM ON  ");
    printTwoDigits(alarmHours);
    tft.print(":");
    printTwoDigits(alarmMinutes);
    tft.print(alarmIsPM ? " PM" : " AM");
  } else {
    tft.setTextColor(ST77XX_RED, ST77XX_BLACK);
    tft.print("ALM OFF          "); // Trailing spaces clear old time text
  }
}

void renderSetTimeScreen() {
  // Header banner
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
  tft.setCursor(10, 6);
  tft.print("SET TIME         ");

  // Time value with highlight on active field
  tft.setTextSize(4);
  tft.setCursor(10, 32);

  // Hours: highlighted in Yellow if editCursor == 0
  tft.setTextColor((editCursor == 0) ? ST77XX_YELLOW : ST77XX_WHITE, ST77XX_BLACK);
  printTwoDigits(hours);

  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.print(":");

  // Minutes: highlighted in Yellow if editCursor == 1
  tft.setTextColor((editCursor == 1) ? ST77XX_YELLOW : ST77XX_WHITE, ST77XX_BLACK);
  printTwoDigits(minutes);

  // AM / PM
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_MAGENTA, ST77XX_BLACK);
  tft.setCursor(160, 38);
  tft.print(isPM ? "PM " : "AM ");
}

void renderSetAlarmScreen() {
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_ORANGE, ST77XX_BLACK);
  tft.setCursor(10, 6);
  tft.print("SET ALARM        ");

  tft.setTextSize(4);
  tft.setCursor(10, 32);

  tft.setTextColor((editCursor == 0) ? ST77XX_YELLOW : ST77XX_WHITE, ST77XX_BLACK);
  printTwoDigits(alarmHours);

  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.print(":");

  tft.setTextColor((editCursor == 1) ? ST77XX_YELLOW : ST77XX_WHITE, ST77XX_BLACK);
  printTwoDigits(alarmMinutes);

  tft.setTextSize(2);
  tft.setTextColor(ST77XX_MAGENTA, ST77XX_BLACK);
  tft.setCursor(160, 38);
  tft.print(alarmIsPM ? "PM " : "AM ");
}

void renderAlarmRingingScreen() {
  tft.setTextSize(4);
  tft.setTextColor(ST77XX_RED, ST77XX_BLACK);
  tft.setCursor(20, 22);
  tft.print("! WAKE UP !");
}

// setup() runs ONCE when the board powers on
void setup() {
  Serial.begin(115200); // lets the board talk to your computer

  pinMode(TFT_BL, OUTPUT); // Set the backlight pin mode, or just wire it to 3.3V
  digitalWrite(TFT_BL, LOW); // Turns the backlight ON, for some reason this screen is active Low, so setting it to LOW is really HIGH

  tft.init(76, 284); // Our panel size (portrait)
  tft.setOffsets(82, 18); // Offsets for the weird resolution
  tft.invertDisplay(false); // Invert the colors (This display is flipped from normal)
  tft.setRotation(1); // Landscape, if it's upside down use 3!
  Serial.println("TFT Initialized!");

  tft.fillScreen(ST77XX_BLACK); // clear the screen

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(6);
  tft.setCursor(0,0); // Where the text is drawn, 0,0 is top left
  tft.print(42); // Show whatever you want! Draws from the top left of the text/number/shape

  void tickClock()
  {
    seconds++;
    if(seconds >= 60)
    {
      seconds = 0;
      minutes++;

      if(minutes >= 60)
      {
        minutes = 0; 

        if(hours == 11)
        {
          hours = 12;
          isPM = !isPM;
        }
        else if(hours == 12)
        {
          hours = 1;
        }
        else
        {
          hours++;
        }
      }
    }

    if(alarmEnabled && (hours == alarmHours) && (minutes == alarmMinutes) && (isPm == alarmIsPM))
    {
      currentState = STATE_ALARM_RINGING;
    }

    if(isSnoozed && (hours == snoozedHours) && (minutes == snoozedMinutes) && (snoozedIsPM == isPM))
    {
      isSnoozed = false;
      currentState = STATE_ALARM_RINGING;
    }
  }

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  btn1.attachLongPressStart([]()
  {
    if(currentState == STATE_NORMAL)
    {
      currentState = STATE_SET_TIME;
      editCursor = 0;
    }
  });
  btn1.attachClick([]()
  {
    if(currentState == STATE_SET_ALARM)
    {
      adjustAlarmTime(+1);
    }
    else if(currentState == STATE_SET_TIME)
    {
      if(editCursor < 2)
      {
        editCursor += 1;
      }
      else
      {
        editCursor = 0;
      }
    }
  });

  btn2.attachClick([]()
  {
    if(currentState == STATE_SET_ALARM)
    {
      adjustAlarmTime(-1);
    }
    else if(currentState == STATE_SET_TIME)
    {
      adjustClockTime(+1);
    }
    else if(currentState == STATE_ALARM_RINGING)
    {
      snoozeAlarm();
    }
  });
  btn2.attachLongPressStart([]()
  {
    if(currentState == STATE_NORMAL)
    {
      alarmEnabled = !alarmEnabled;
    }
  });

  btn3.attachClick([]()
  {
    if(currentState == STATE_SET_ALARM)
    {
      alarmPM = !alarmPM;
    }
    else if(currentState == STATE_SET_TIME)
    {
      adjustClockTime(-1);
    }
    else if(currentState == STATE_ALARM_RINGING)
    {
      dismissAlarm();
    }
  });
  btn3.attachLongPressStart([]()
  {
    if(currentState == STATE_SET_ALARM || currentState == STATE_SET_TIME)
    {
      currentState = STATE_NORMAL;
    }
  });

  btn4.attachLongPressStart([]()
  {
    if(currentState == STATE_NORMAL)
    {
      currentState = STATE_SET_ALARM;
      editCursor = 0;
    }
  });
  btn4.attachClick([]()
  {
    if(currentState == STATE_SET_TIME)
    {
      isPM = !isPM;
    }
    else if(currentState == STATE_SET_ALARM)
    {
      if(editCursor < 2)
      {
        editCursor += 1;
      }
      else
      {
        editCursor = 0;
      }
    }
  });
}

// loop() runs OVER and OVER, forever
void loop() {
  // add your code here!
  unsigned long currentMillis = millis();

  btn1.tick();
  btn2.tick();
  btn3.tick();
  btn4.tick();

  if(currentMillis - lastTick >= 1000)
  {
    lastTick += 1000;
    tickClock();
  }if (currentMillis - lastBeep >= 250) {
        lastBeep = currentMillis;
        beepState = !beepState;
        digitalWrite(BUZZER_PIN, beepState ? HIGH : LOW);
      }
      renderAlarmRingingScreen();

  switch(CurrentState)
  {
    case STATE_NORMAL:
      renderNormalClock();
      break;
    
    case STATE_SET_TIME:
      renderSetTimeScreen();
      break;
    
    case STATE_SET_ALARM:
      renderSetAlarmScreen();
      break;
    
    case STATE_ALARM_RINGING:
      if (currentMillis - lastBeep >= 250) {
        lastBeep = currentMillis;
        beepState = !beepState;
        digitalWrite(BUZZER_PIN, beepState ? HIGH : LOW);
      }
      renderAlarmRingingScreen();
      break;
  }  
}
