#include <WiFiManager.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

WiFiUDP udp;
NTPClient timeClient(udp);

void setup() {
  Serial.begin(115200);

  WiFiManager wm;

  bool res = wm.autoConnect("My ESP32","12345678");
  if(!res){
    Serial.println("Failed");
    ESP.restart();
  }else{
    Serial.println("Done!!!");
  }
  timeClient.begin();

}

void loop() {
  timeClient.update();
  Serial.println(timeClient.getFormattedTime());
  delay(1000);

}
