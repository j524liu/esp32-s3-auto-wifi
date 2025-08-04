#include <Arduino.h>

#include <SPI.h>

#include <FS.h>
#include <SPIFFS.h>

#include <Preferences.h>

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>

const char * ap_ssid = "ESP32-S3-AP";
const char * ap_pwd = "12345678";

const char * prefs_name = "wifi_info";

WebServer server;

Preferences prefs;

void nvs_init();
void spiffs_init();
void handleRoot();
void handleSubmit();
void begin_server();
void WiFi_AP();
bool check_nvs();
bool connectWiFi(String ssid, String pwd, uint8_t timeout);

void nvs_init()
{
  prefs.begin(prefs_name, false);
}

void spiffs_init()
{
  // 初始化 SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS 初始化失败");
    return;
  }
}

void handleRoot()
{
  fs::File file = SPIFFS.open("/index.html", "r");
  server.streamFile(file, "text/html");
  file.close();
}

void handleSubmit()
{
  if(server.method() == HTTP_POST)
  {
    String ssid = server.arg("ssid");
    String pwd = server.arg("pwd");
    if(connectWiFi(ssid, pwd, 5))
    {
      prefs.putString("ssid", ssid);
      prefs.putString("pwd", pwd);
      prefs.end();
      server.close();
    }
    else
    {
      Serial.println("Cannot Establish WiFi Connection, Try Again!");
    }
  }
}

void begin_server()
{
  server.begin();
  if (MDNS.begin("esp32")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);
  server.on("/post", HTTP_POST, []() { handleSubmit(); });
}

void WiFi_AP()
{
  WiFi.mode(WIFI_MODE_AP);
  WiFi.softAP(ap_ssid, ap_pwd);
}

bool check_nvs()
{
  if(prefs.isKey("ssid") && prefs.isKey("pwd"))
  {
    return true;
  }
  else
  {
    return false;
  }
}

bool connectWiFi(String ssid, String pwd, uint8_t timeout)
{
  WiFi.mode(WIFI_MODE_STA);
  WiFi.begin(ssid, pwd);
  uint16_t wait_cnt = 0;
  while(!WiFi.isConnected())
  {
    delay(10);
    wait_cnt ++;
    if(wait_cnt >= (timeout * 100))
    {
      return false;
    }
  }
  return true;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(1000);
  Serial.println("NVS Init Begin");
  nvs_init();
  Serial.println("NVS Init Finish\nSPIFFS Init Begin");
  spiffs_init();
  Serial.println("SPIFFS Init Finish");
  Serial.println("NVS Have Keys?");
  if(check_nvs())
  {
    Serial.println("YES");
    String ssid = prefs.getString("ssid");
    String pwd = prefs.getString("pwd");
    if(connectWiFi(ssid, pwd, 5))
    {
      Serial.println("WiFi Connected!");
      prefs.end();
    }
    else{
      Serial.println("Cannot Establish WiFi Connection, NVS Data Wrong!");
      Serial.println("Starting softAP");
      WiFi_AP();
      Serial.println("softAP Started");
      Serial.println("LOCAL IP: " + WiFi.softAPIP().toString());
      Serial.println("Start Server");
      begin_server();
      Serial.println("Server Started");
    }
  }
  else
  {
    Serial.println("Cannot Establish WiFi Connection, NVS Don't Have Data!");
    Serial.println("Starting softAP");
    WiFi_AP();
    Serial.println("softAP Started");
    Serial.println("LOCAL IP: " + WiFi.softAPIP().toString());
    Serial.println("Start Server");
    begin_server();
    Serial.println("Server Started");
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  server.handleClient();
  delay(50);
}