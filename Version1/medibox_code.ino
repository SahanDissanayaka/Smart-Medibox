#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHTesp.h>
#include <WiFi.h>

#define screen_width 128
#define screen_height 64
#define oled_reset -1
#define screen_address 0x3C

#define Buzzer 5
#define LED_1 15
#define PB_CANCEL 34
#define PB_OK 32
#define PB_UP 33
#define PB_DOWN 35
#define DHTPIN 12

#define NTP_SERVER "pool.ntp.org"
#define UTC_OFFSET 19800
#define UTC_OFFSET_DST 0

DHTesp dhtsensor;
Adafruit_SSD1306 display(screen_width, screen_height, &Wire, oled_reset);

int days = 0, hours = 0, minutes = 0, seconds = 0;
int months = 0, years = 0;

unsigned long timeNow, timeLast = 0;

bool alarm_enabled = true;
int n_alarms = 2;
int alarm_hours[] = { 0, 1 };
int alarm_minutes[] = { 1, 10 };
bool alarm_triggered[] = { false, false };

int n_notes = 8;
int C = 262;
int D = 294;
int E = 330;
int F = 349;
int G = 392;
int A = 440;
int B = 494;
int C_H = 523;
int notes[] = { C, D, E, F, G, A, B, C_H };

int current_mode = 0;
int max_modes = 4;
String modes[] = { "1 - Set Time", "2 - Set Alarm 1", "3 - Set Alarm 2", "4 - Disable Alarms" };

void setup() {
  Serial.begin(9600);
  pinMode(Buzzer, OUTPUT);
  pinMode(LED_1, OUTPUT);
  pinMode(PB_CANCEL,INPUT);
  pinMode(PB_OK,INPUT);
  pinMode(PB_UP,INPUT);
  pinMode(PB_DOWN,INPUT);

  dhtsensor.setup(DHTPIN,DHTesp::DHT22);

  if (!display.begin(SSD1306_SWITCHCAPVCC, screen_address)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (true) {}
  }

  display.display();
  delay(2000);

  WiFi.begin("Wokwi-GUEST","",6);
  while (WiFi.status() != WL_CONNECTED){
    delay(250);
    display.clearDisplay();
    print_line("Connecting to WiFi",0,20,2);
  }

  display.clearDisplay();
  print_line("Connected to WIFI",0,0,2);

  configTime(UTC_OFFSET,UTC_OFFSET_DST,NTP_SERVER);

  display.clearDisplay();

  print_line("Welcome to Medibox!", 0, 20, 2);
  display.clearDisplay();
}

void loop() {
  //put your main code here, to run repeatedly:
  update_time_with_check_alarm();
  if (!digitalRead(PB_OK)) {
    delay(200);
    go_to_menu();
  }
  check_temp();
  //print_time_now();
}

void print_line(String text, int column, int row, int text_size) {

  display.setTextSize(text_size);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(column, row);
  display.println(text);

  display.display();
  //delay(2000);
}

void print_time_now() {
  display.clearDisplay();
  print_line(String(years), 0, 0, 2);
  print_line("/", 50, 0, 2);
  print_line(String(months), 70, 0, 2);
  print_line("/", 90, 0, 2);
  print_line(String(days), 110, 0, 2);
  //print_line(":", 80, 0, 2);
  print_line(String(hours), 0, 20, 2);
  print_line(":", 20, 20, 2);
  print_line(String(minutes), 40, 20, 2);
  print_line(":", 60, 20, 2);
  print_line(String(seconds), 80, 20, 2);
  //print_line(":", 120, 0, 2);

}

// void update_time() {
//   timeNow = millis() / 1000;
//   seconds = timeNow - timeLast;

//   if (seconds >= 60) {
//     minutes += 1;
//     timeLast += 60;
//   }

//   if (minutes == 60) {
//     hours += 1;
//     minutes = 0;
//   }

//   if (hours == 24) {
//     days += 1;
//     hours = 0;
//   }
// }

void ring_alarm() {
  display.clearDisplay();
  print_line("MEDICINE TIME!", 0, 0, 2);

  digitalWrite(LED_1, HIGH);

  bool break_happened = false;

  while (!break_happened && digitalRead(PB_CANCEL)) {
    for (int i = 0; i < n_notes; i++) {
      if (!digitalRead(PB_CANCEL)) {
        delay(200);
        break_happened = true;
        break;
      }
      tone(Buzzer, notes[i]);
      delay(500);
      noTone(Buzzer);
      delay(2);
    }
  }


  digitalWrite(LED_1, LOW);
  display.clearDisplay();
}

void update_time_with_check_alarm() {
  update_time();
  print_time_now();

  if (alarm_enabled) {
    for (int i = 0; i < n_alarms; i++) {
      if (!alarm_triggered[i] && alarm_hours[i] == hours && alarm_minutes[i] == minutes) {
        ring_alarm();
        alarm_triggered[i] = true;
      }
    }
  }
}

int wait_for_button_press() {
  while (true) {
    if (!digitalRead(PB_UP)) {
      delay(200);
      return 1;
    } else if (!digitalRead(PB_DOWN)) {
      delay(200);
      return 2;
    } else if (!digitalRead(PB_OK)) {
      delay(200);
      return 3;
    } else if (!digitalRead(PB_CANCEL)) {
      delay(200);
      return 4;
    }
  }
}

void go_to_menu() {
  while (true) {
    display.clearDisplay();
    print_line(modes[current_mode], 0, 0, 2);

    int pressed = wait_for_button_press();
    if (pressed == 1) {
      delay(200);
      current_mode += 1;
      current_mode = current_mode % max_modes;
    } else if (pressed == 2) {
      delay(200);
      current_mode -= 1;
      if (current_mode < 0) {
        current_mode = 3;
      }

    } else if (pressed == 3) {
      delay(200);
      run_mode(current_mode);
    } else if (pressed == 4) {
      delay(200);
      break;
    }
  }
}

void set_time() {

  int temp_hour = hours;
  while (true) {
    display.clearDisplay();
    print_line("Enter hour: " + String(temp_hour), 0, 0, 2);

    int pressed = wait_for_button_press();
    if (pressed == 1) {
      delay(200);
      temp_hour += 1;
      temp_hour = temp_hour % 24;
    } else if (pressed == 2) {
      delay(200);
      temp_hour -= 1;
      if (temp_hour == 0) {
        temp_hour = 24;
      }

    } else if (pressed = 3) {
      delay(200);
      hours = temp_hour;
      break;
    } else if (pressed == 4) {
      delay(200);
      break;
    }
  }

  int temp_minute = minutes;
  while (true) {
    display.clearDisplay();
    print_line("Enter minute: " + String(temp_minute), 0, 0, 2);

    int pressed = wait_for_button_press();
    if (pressed == 1) {
      delay(200);
      temp_minute += 1;
      temp_minute = temp_minute % 60;
    } else if (pressed == 2) {
      delay(200);
      temp_minute -= 1;
      if (temp_minute < 0) {
        temp_minute = 59;
      }

    } else if (pressed == 3) {
      delay(200);
      minutes = temp_minute;
      break;
    } else if (pressed == 4) {
      delay(200);
      break;
    }
  }

  display.clearDisplay();
  print_line("Time is set",0,0,2);
  delay(1000);
}

void set_alarm(int alarm){
  int temp_hour = alarm_hours[alarm];
  while (true) {
    display.clearDisplay();
    print_line("Enter hour: " + String(temp_hour), 0, 0, 2);

    int pressed = wait_for_button_press();
    if (pressed == 1) {
      delay(200);
      temp_hour += 1;
      temp_hour = temp_hour % 24;
    } else if (pressed == 2) {
      delay(200);
      temp_hour -= 1;
      if (temp_hour == 0) {
        temp_hour = 24;
      }

    } else if (pressed == 3) {
      delay(200);
      alarm_hours[alarm] = temp_hour;
      break;
    } else if (pressed == 4) {
      delay(200);
      break;
    }
  }

  int temp_minute = alarm_minutes[alarm];
  while (true) {
    display.clearDisplay();
    print_line("Enter minute: " + String(temp_minute), 0, 0, 2);

    int pressed = wait_for_button_press();
    if (pressed == 1) {
      delay(200);
      temp_minute += 1;
      temp_minute = temp_minute % 60;
    } else if (pressed == 2) {
      delay(200);
      temp_minute -= 1;
      if (temp_minute < 0) {
        temp_minute = 59;
      }

    } else if (pressed == 3) {
      delay(200);
      alarm_minutes[alarm] = temp_minute;
      break;
    } else if (pressed == 4) {
      delay(200);
      break;
    }
  }

  display.clearDisplay();
  print_line("Alarm is set",0,0,2);
  delay(1000);

}

void run_mode(int mode) {
  if (mode == 0) {
    set_time();
  }
  else if (mode == 1 || mode == 2){
    set_alarm(mode-1);
  }
  else if (mode == 3){
    alarm_enabled = false;
    display.clearDisplay();
    print_line("All alarms are disabled",0,0,2);
    delay(1000);
  }
}

void check_temp(){
  TempAndHumidity data = dhtsensor.getTempAndHumidity();
  if(data.temperature > 35){
    display.clearDisplay();
    print_line("TEMP HIGH",0,40,1);
  }
  else if(data.temperature < 25){
    display.clearDisplay();
    print_line("TEMP LOW",0,40,1);
  }
  if(data.humidity > 40){
    display.clearDisplay();
    print_line("HUMIDITY HIGH",0,50,1);
  }
  else if(data.humidity < 25){
    display.clearDisplay();
    print_line("HUMIDITY HIGH",0,50,1);
  }

}

void update_time(){
  struct tm timeinfo;
  getLocalTime(&timeinfo);

  char timeHour[3];
  strftime(timeHour,3,"%H",&timeinfo);
  hours = atoi(timeHour);

  char timeMinute[3];
  strftime(timeMinute,3,"%M",&timeinfo);
  minutes = atoi(timeMinute);

  char timeSecond[3];
  strftime(timeSecond,3,"%S",&timeinfo);
  seconds = atoi(timeSecond);

  char timeDay[3];
  strftime(timeDay,3,"%d",&timeinfo);
  days = atoi(timeDay);

  char timeMonth[3];
  strftime(timeMonth,3,"%m",&timeinfo);
  months = atoi(timeMonth);

  char timeYear[5];
  strftime(timeYear,5,"%Y",&timeinfo);
  years = atoi(timeYear);
}
