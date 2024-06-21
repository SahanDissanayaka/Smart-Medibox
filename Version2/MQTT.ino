#include <PubSubClient.h>
#include <WiFi.h>
#include <DHTesp.h>

#define DHT_Pin 15
char tempAr[6];
DHTesp dhtsensor;

WiFiClient espClient;
PubSubClient mqttClient(espClient);


void setup() {
  Serial.begin(115200);
  setupWifi();
  setupMqtt();

  dhtsensor.setup(DHT_Pin,DHTesp::DHT22);
  pinMode(LED_BUILTIN,OUTPUT);
  digitalWrite(LED_BUILDIN, LOW);
}

void loop() {
  if (!mqttClient.connected()){
    connectToBroker();
  }

  mqttClient.loop();
  updateTemperature();
  Serial.println(tempAr);

  mqttClient.publish("ENTC-TEMP-SPD",tempAr);
  delay(1000);

}

void setupWifi(){
  WiFi.begin("Wokwi-GUEST","");

  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }

  Serial.print("Conected...!!!");
  Serial.println(WiFi.localIP());

  
}

void setupMqtt(){
  mqttClient.setServer("test.mosquitto.org",1883);
  mqttClient.setCallback(receiveCallback);
}

void connectToBroker(){
  while(!mqttClient.connected()){
    Serial.print("Attempting MQTT conection...");
    if(mqttClient.connect("ESP32-123456")){
      Serial.println("Connected!");
      mqttClient.subscribe("ENTC-ON-OFF-SPD");
    }else{
      Serial.print("failed ");
      Serial.print(mqttClient.state());
      delay(5000);
    }
  }
  
}

void updateTemperature(){
  TempAndHumidity data = dhtsensor.getTempAndHumidity();
  String(data.temperature,2).toCharArray(tempAr,6);
}

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

  if(strcmp(topic,"ENTC-ON-OFF-SPD")==0){
    if(payloadCharAr[0] == '1'){
      digitalWrite(LED_BUILDIN, HIGH);
    }else{
      digitalWrite(LED_BUILDIN, LOW);
    }
  }
}



































