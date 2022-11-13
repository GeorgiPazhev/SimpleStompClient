#include <WiFi.h>
#include<Stomp.h>

const char *ssid = "Your_WIFI_SSID";
const char *pass = "********";

StompClient *stomp;

WiFiClient client;

void setup() 
{
  Serial.begin(115200);
  Serial.print("Attempting to connect to WPA SSID: ");
  Serial.println(ssid);
  while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
    // failed, retry
    Serial.print(".");
    delay(5000);
  }
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  stomp = new StompClient(&client, ACK_AUTO);
  
  while(!stomp->connect("192.168.0.141", 61616, "1.2"))
  {
    Serial.println("Not connected!");
    delay(5000);
  }
  delay(5000);
  stomp->setMessageCallback([](std::map<String,String> &p1, String &p2)
  {
    Serial.println("Message body:");
    Serial.println(p2);
    Serial.println("Message headers:");
    for (std::map<String,String>::iterator it=p1.begin(); it!=p1.end(); ++it)
    {
      Serial.print("header-key: ");
      Serial.println(it->first);
      Serial.print("header-value: ");
      Serial.println(it->second);
    }
  });
  stomp->subscribe("/test/queue1", "esp32_1");
}

void loop() {
  stomp->loop();

}
