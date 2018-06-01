/*
ESP32を使ったWifiテレメトリーのスケッチです。
ESP32がWifiアクセスポイントに接続しTCPサーバーになり、そこへMission Plannerで接続します。
コンパイルの前に次のものをインストールしてください。
・Python (Arduino core for ESP32 WiFi chipのインストールに必要)
・Arduino core for ESP32 WiFi chip (参考URL:https://www.mgo-tec.com/arduino-core-esp32-install)
mDNSを使用するためIPアドレス管理不要で、ドメイン名.localで接続可能。
ただし、AndroidはmDNSに対応していない模様。IPアドレスを固定する方法と併用しても良いかもしれません。
*/

#include <WiFi.h>
#include <ESPmDNS.h>

#define MAX_SRV_CLIENTS 1

//以下の接続するアクセスポイントのSSIDとパスワードは、使用する環境に合わせてください。
//const char* ssid = "********";
//const char* password = "********";
//const char* ssid = "MyPlace";
//const char* password = "Z6mWlNQOyO";
const char* ssid = "mission";
const char* password = "12345678";

WiFiServer wifiServer(23);
WiFiClient wifiServerClients[MAX_SRV_CLIENTS];

void setup()
{
//  Serial.begin(115200);
  Serial.begin(57600);
  delay(1000);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED)
  {
    for (uint8_t i = 0 ; WiFi.status() != WL_CONNECTED && i < 10 ; ++i)
    {
      delay(1000);
      Serial.print(".");
    }
    if(WiFi.status() != WL_CONNECTED)
    {
      WiFi.disconnect();
      WiFi.begin(ssid, password);
    }
  }

  Serial.println("\nConnected to the WiFi network");
  Serial.println(WiFi.localIP());

  if (!MDNS.begin("esp32")) //mDNSドメイン名。適宜覚えやすい好きなドメイン名に書き換えてください。
  {
    Serial.println("Error setting up MDNS responder!");
    while (1)
    {
      delay(1000);
    }
    Serial.println("mDNS responder not started");
  } else {
    Serial.println("mDNS responder started");
  }
  wifiServer.begin();
  wifiServer.setNoDelay(true);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Disconnected from the WiFi network");
    delay(500);
    setup();
  }
  uint8_t i;
  //check if there are any new clients
  if (wifiServer.hasClient())
  {
    for (i = 0; i < MAX_SRV_CLIENTS; i++)
    {
      //find free/disconnected spot
      if (!wifiServerClients[i] || !wifiServerClients[i].connected())
      {
        if (wifiServerClients[i]) wifiServerClients[i].stop();
        wifiServerClients[i] = wifiServer.available();
        //Serial1.print("New client: "); Serial1.print(i);
        continue;
      }
    }
    //no free/disconnected spot so reject
    WiFiClient wifiServerClient = wifiServer.available();
    wifiServerClient.stop();
  }
  //check clients for data
  for (i = 0; i < MAX_SRV_CLIENTS; i++)
  {
    if (wifiServerClients[i] && wifiServerClients[i].connected())
    {
      if (wifiServerClients[i].available())
      {
        //get data from the telnet client and push it to the UART
        while (wifiServerClients[i].available()) Serial.write(wifiServerClients[i].read());
      }
    }
  }
  //check UART for data
  if (Serial.available())
  {
    size_t len = Serial.available();
    uint8_t sbuf[len];
    Serial.readBytes(sbuf, len);
    //push UART data to all connected telnet clients
    for (i = 0; i < MAX_SRV_CLIENTS; i++)
    {
      if (wifiServerClients[i] && wifiServerClients[i].connected())
      {
        wifiServerClients[i].write(sbuf, len);
        //delay(1);
      }
    }
  }
}
