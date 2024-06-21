#include <WiFi.h>

void setup() {
  Serial.begin(115200);
  delay(10);

  WiFi.begin("SPD-SLT","7K..");

  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }

  Serial.print("Conected...!!!");
  Serial.println(WiFi.localIP());

}

void loop() {
  // put your main code here, to run repeatedly:

}
