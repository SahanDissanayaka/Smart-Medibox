#include <WiFi.h>
#include <PubSubClient.h>
#include <DHTesp.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP32Servo.h>

#define DHT_Pin 15
#define Buzzer 12
#define left_LDR 13
#define right_LDR 4
#define servo_pin 16

int left_LDR_value=0;
int right_LDR_value=0;
int max_LDR_value=0;
float intensity_value;
bool custom_mode = false;

float angle;
int offset = 30;
float ctrlfactor= 0.75;
float d;

char tempAr[6];
DHTesp dhtsensor;

bool isScheduleledON = false;
unsigned long scheduledOnTime;

WiFiClient espClient;
PubSubClient mqttClient(espClient);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

Servo Myservo;

void setup() {
  Serial.begin(115200);

  setupWifi();
  setupMqtt();

  dhtsensor.setup(DHT_Pin,DHTesp::DHT22);

  timeClient.begin();
  timeClient.setTimeOffset(5.5*3600);

  pinMode(Buzzer,OUTPUT);
  digitalWrite(Buzzer,LOW);
  
  pinMode(left_LDR, INPUT);
  pinMode(right_LDR, INPUT);
  
  Myservo.attach(servo_pin);

}

void loop() {

  if (!mqttClient.connected()){
    connectToBroker();
  }

  mqttClient.loop();

  updateTemperature();

  mqttClient.publish("ENTC-TEMP-SPD",tempAr);

  check_intensity();

  checkSchedule();

  set_angle();

  delay(100);

}

// Setup Wifi Connection
void setupWifi(){
  WiFi.begin("Wokwi-GUEST","");

  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }

  Serial.print("Conected...!!!");
  Serial.println(WiFi.localIP());

  
}

// Update Temperature
void updateTemperature(){
  TempAndHumidity data = dhtsensor.getTempAndHumidity();
  String(data.temperature,2).toCharArray(tempAr,6);
}

// Setup Mqtt
void setupMqtt(){
  mqttClient.setServer("test.mosquitto.org",1883);
  mqttClient.setCallback(receiveCallback);
}

// Setup function to connect to broker
void connectToBroker(){
  while(!mqttClient.connected()){
    Serial.print("Attempting MQTT conection...");
    if(mqttClient.connect("ESP32-123456")){
      Serial.println("Connected!");
      // Subscribing
      mqttClient.subscribe("ENTC-ON-OFF-SPD");
      mqttClient.subscribe("ENTC-SCH-ON-SPD");
      mqttClient.subscribe("SPD-MEDIBOX-MENU");
      mqttClient.subscribe("SPD-MEDIBOX-MINANGLE-OUT");
      mqttClient.subscribe("SPD-MEDIBOX-CTRLFACTOR-OUT");
    }else{
      Serial.print("failed ");
      Serial.print(mqttClient.state());
      delay(5000);
    }
  }
  
}

// Function to turn on and off the Buzzer 
void buzzerOn(bool on){
  if(on){
    tone(Buzzer,256);
  }else{
    noTone(Buzzer);
  }
}

// Function which calls when subcribed data arrived
void receiveCallback(char* topic,byte* payload,unsigned int length){
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]");

  char payloadCharAr[length];
  for(int i=0;i<length;i++){
    Serial.print((char)payload[i]);
    payloadCharAr[i] = (char)payload[i];
  }

  Serial.println();

  if(strcmp(topic,"ENTC-ON-OFF-SPD")==0){// main-switch
    buzzerOn(payloadCharAr[0] == '1');
  }else if(strcmp(topic,"ENTC-SCH-ON-SPD")==0){// schedule-switch
    if(payloadCharAr[0] == 'N'){
      isScheduleledON = false;
    }else{
      isScheduleledON = true;
      scheduledOnTime = atol(payloadCharAr);
    }
  }else if(strcmp(topic,"SPD-MEDIBOX-MENU")==0){// tablet-menu
    if(payloadCharAr[0] == '1'){
      handle_sliders(1);
    }else if(payloadCharAr[0] == '2'){
      handle_sliders(2);
    }else if(payloadCharAr[0] == '3'){
      handle_sliders(3);
    }else if(payloadCharAr[0] == '4'){
      handle_sliders(4);
    }else if(payloadCharAr[0] == '5'){
      handle_sliders(5);
    }
  }else if(custom_mode && strcmp(topic,"SPD-MEDIBOX-MINANGLE-OUT")==0){// minimum angle slider
    offset = atoi(payloadCharAr);
    handle_sliders(4);
  }else if(custom_mode && strcmp(topic,"SPD-MEDIBOX-CTRLFACTOR-OUT")==0){// control factor slider
    ctrlfactor = atof(payloadCharAr);
    handle_sliders(4);
  }
}

// To get time
unsigned long getTime(){
  timeClient.update();
  return timeClient.getEpochTime();

}

// To check the schedule
void checkSchedule(){
  if(isScheduleledON){
    unsigned long currentTime = getTime();
    if(currentTime > scheduledOnTime){
      buzzerOn(true);
      isScheduleledON = false;
      mqttClient.publish("ENTC-MAIN-ON-OFF-SPD","1");
      mqttClient.publish("ENTC-SCHE-ON-OFF-SPD","0");
    }
  }
}

// To check the light intensity  
void check_intensity(){
  int max_side;
  left_LDR_value = analogRead(left_LDR);
  right_LDR_value = analogRead(right_LDR);
  if(left_LDR_value >= right_LDR_value){
    max_LDR_value = left_LDR_value;
    max_side = 0;
  }else{
    max_LDR_value = right_LDR_value;
    max_side = 1;
  }
  intensity_value = max_LDR_value/4095.0;
  char i[6];
  String(intensity_value,2).toCharArray(i,6);
  mqttClient.publish("SPD-MEDIBOX-LDR-VALUES",i);// Publishing max LDR value
  //Publishing the maximum LDR value occured side
  if(max_side){
    mqttClient.publish("SPD-MEDIBOX-LDR-SIDE","R");
    d = 0.5;
  }else{
    mqttClient.publish("SPD-MEDIBOX-LDR-SIDE","L");
    d = 1.5;
  }

}

// To handle sliders 
void handle_sliders(int mode){
  if(mode == 1){
    mqttClient.publish("SPD-MEDIBOX-MINANGLE","40");
    mqttClient.publish("SPD-MEDIBOX-CTRLFACTOR","0.3");
    delay(2000);
    mqttClient.publish("SPD-MEDIBOX-MINANGLE","disable");
    mqttClient.publish("SPD-MEDIBOX-CTRLFACTOR","disable");
    offset = 30;ctrlfactor = 0.3;
    custom_mode = false;
  }else if(mode == 2){
    mqttClient.publish("SPD-MEDIBOX-MINANGLE","60");
    mqttClient.publish("SPD-MEDIBOX-CTRLFACTOR","0.4");
    delay(2000);
    mqttClient.publish("SPD-MEDIBOX-MINANGLE","disable");
    mqttClient.publish("SPD-MEDIBOX-CTRLFACTOR","disable");
    offset = 60;ctrlfactor = 0.4;
    custom_mode = false;
  }else if(mode == 3){
    mqttClient.publish("SPD-MEDIBOX-MINANGLE","90");
    mqttClient.publish("SPD-MEDIBOX-CTRLFACTOR","0.5");
    delay(2000);
    mqttClient.publish("SPD-MEDIBOX-MINANGLE","disable");
    mqttClient.publish("SPD-MEDIBOX-CTRLFACTOR","disable");
    offset = 90;ctrlfactor = 0.5;
    custom_mode = false;
  }else if(mode == 4){
    mqttClient.publish("SPD-MEDIBOX-MINANGLE","0");
    mqttClient.publish("SPD-MEDIBOX-CTRLFACTOR","0");
    custom_mode = true;
  }else if(mode == 5){
    mqttClient.publish("SPD-MEDIBOX-MINANGLE","30");
    mqttClient.publish("SPD-MEDIBOX-CTRLFACTOR","0.75");
    delay(2000);
    mqttClient.publish("SPD-MEDIBOX-MINANGLE","disable");
    mqttClient.publish("SPD-MEDIBOX-CTRLFACTOR","disable");
    offset = 30;ctrlfactor = 0.75;
    custom_mode = false;
  }
  
}

// To do the angle calculation and turn the servo motor  
void set_angle(){
  float cal_angle = offset*d + (180-offset)*intensity_value*ctrlfactor;
  if(cal_angle > 180.0){
    angle = 180.0;
  }else{
    angle = cal_angle;
  }
  Serial.println(angle);
  Myservo.write(angle);
}