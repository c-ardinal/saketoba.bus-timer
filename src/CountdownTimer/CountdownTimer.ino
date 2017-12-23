#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include <Ticker.h>
#include "MCP23017.h"
#include "Timetable.h"

#define TIMETABLE_JSON "/timetable.json"
#define WIFI_CONF      "/wifi.conf"
#define SERV_PASS      "saketoba"

#define MODE_SW        12
#define SW_OFF         1
#define SW_ON          0

#define EXPANDER_ADDR  0x20
#define SCL_PIN        4
#define SDA_PIN        5

String json = "";
String wifiSsid = "";
String wifiPass = "";

ESP8266WebServer server(80);
Ticker seg7ClockEvent;
Ticker modeButtonEvent;
Ticker seg7LedEvent;
MCP23017 seg7Led(EXPANDER_ADDR, SDA_PIN, SCL_PIN);
Timetable timetable;

seg7LedId_t raw1Data = {_HYPHEN, _HYPHEN, _HYPHEN, _HYPHEN};
seg7LedId_t raw2Data = {_HYPHEN, _HYPHEN, _HYPHEN, _HYPHEN};


/* モード切り替え用 */
enum State {
  CLOCK,
  COUNTDOWN,
  WIFI_SETTING
} state = CLOCK;


/* システム稼働準備 */
void setup() {
  // IO関連初期化
  Serial.begin(74880);
  seg7Led.init();
  pinMode(MODE_SW, INPUT);
  Serial.println();
  if (!SPIFFS.begin()) {
    Serial.println(">>Failed to mount file system.");
    return;
  }

  // Wi-Fi設定ファイルが存在しないか，モード切り替えスイッチ押しっぱ機動えWi-Fi接続設定モードに突入
  if (!SPIFFS.exists(WIFI_CONF) || digitalRead(MODE_SW) == SW_ON) {
    seg7LedEvent.attach_ms(10, displayLed);
    initWifiSetupServer();
    return;
  }

  // Wi-Fi設定ファイルの読み込み
  File wcf = SPIFFS.open(WIFI_CONF, "r");
  wifiSsid = wcf.readStringUntil('\n');
  wifiPass = wcf.readStringUntil('\n');
  wcf.close();
  wifiSsid.trim();
  wifiPass.trim();
  Serial.println("[SSID]: " + wifiSsid);
  Serial.print("[PASS]: ");
  for (int i = 0; i < wifiPass.length(); i++) {
    Serial.print("*");
  }
  Serial.println();

  // JSONファイルの読み込み
  if (SPIFFS.exists(TIMETABLE_JSON)) {
    File f = SPIFFS.open(TIMETABLE_JSON, "r");
    json = f.readString();
    f.close();
  }

  // 7segLED表示イベントのアタッチ
  seg7LedEvent.attach_ms(10, displayLed);

  // Wi-Fi接続試行
  Serial.println("[MODE]: NORMAL_BOOT");
  Serial.print(">> Wi-Fi connecting");
  WiFi.begin(wifiSsid.c_str(), wifiPass.c_str());
  uint8_t connectRetry = 0;
  while (1) {
    Serial.print('.');
    delay(500);
    connectRetry++;
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("ok");
      Serial.print("[IP ADDR]: ");
      Serial.println(WiFi.localIP());
      server.on("/", HTTP_GET, timetableConfGetHandler);
      server.on("/", HTTP_POST, timetableConfPostHandler);
      server.begin();
      break;
    }
    if (connectRetry >= 20) {
      Serial.println("failed");
      initWifiSetupServer();
      return;
    }
  }

  seg7ClockEvent.attach_ms(100, displayTimes);
  modeButtonEvent.attach_ms(100, modeSwitch);
  timetable.setJson(&json);
}


void loop() {
  server.handleClient();
}


/* 7seg表示 */
void displayLed() {
  seg7Led.writeToAll(raw1Data, raw2Data);
}


/* モード切替 */
void modeSwitch() {
  static uint8_t sw_before = SW_OFF, sw_after = SW_OFF;
  sw_before = sw_after;
  sw_after = digitalRead(MODE_SW);
  if (sw_before == SW_OFF && sw_after == SW_ON) {
    if (state == CLOCK) {
      Serial.println("[MODE]: COUNTDOWN_MODE");
      state = COUNTDOWN;
    } else {
      Serial.println("[MODE]: CLOCK_MODE");
      state = CLOCK;
    }
  }
}


/* 現在時刻を表示 */
void displayTimes() {
  NowTime_t nt = timetable.getNowTime();
  raw1Data = convertTenToOne(nt.Hour, nt.Min);

  NextTime_t tt = timetable.getNextTimetable();
  if (state == CLOCK) {
    raw2Data = convertTenToOne(tt.nextHour, tt.nextMin);
  } else {
    raw2Data = convertTenToOne(tt.remMin, tt.remSec);
  }
}


/* 時刻を7segで表示できるように分解 */
seg7LedId_t convertTenToOne(uint8_t left, uint8_t right) {
  uint8_t segData[10] = {_0, _1, _2, _3, _4, _5, _6, _7, _8, _9};

  int8_t leftTen = left / 10;
  int8_t leftOne = left - (leftTen * 10);
  int8_t rightTen = right / 10;
  int8_t rightOne = right - (rightTen * 10);

  seg7LedId_t result = {segData[leftTen], segData[leftOne], segData[rightTen], segData[rightOne]};
  return result;
}


/* HTMLの生成を行う */
String generateHtml(String body) {
  String html = "<!DOCTYPE><html lang='ja'>";

  html += "<head>";
  html += "<meta http-equiv='Content-Type' content='text/html; charset=utf-8'/>";
  html += "</head>";

  html += "<body>" + body + "</body>";

  html += "</html>";

  return html;
}


/* Wi-Fi設定用モードへの切り替え */
void initWifiSetupServer() {
  Serial.println("[MODE]: WIFI_SETTING");
  state = WIFI_SETTING;
  raw1Data = {_W, _I, _F, _I};
  raw2Data = {_NULL, _S, _E, _T};

  byte macAddr[6];
  String servSsid = "ESP_";
  String servPass = SERV_PASS;

  WiFi.macAddress(macAddr);
  for (int i = 0; i < 6; i++) {
    servSsid += String(macAddr[i], HEX);
  }
  WiFi.softAP(servSsid.c_str(), servPass.c_str());
  WiFi.softAPConfig(IPAddress(192, 168, 100, 1), IPAddress(192, 168, 100, 1), IPAddress(255, 255, 255, 0));
  server.on("/", HTTP_GET, wifiConfGetHandler);
  server.on("/", HTTP_POST, wifiConfPostHandler);
  server.begin();

  Serial.println("[SSID]: " + servSsid);
  Serial.println("[PASS]: " + servPass);
  Serial.print("[IP ADDR]: ");
  Serial.println(WiFi.softAPIP());
}


/* Wi-Fi設定用GETハンドラ */
void wifiConfGetHandler() {

  int accessPoints = WiFi.scanNetworks();

  String body = "";
  body += "<h1>Wi-Fi Preference</h1>";
  body += "<form method='post'>";
  body += "[SSID] : <select name='ssid' size='1'>";
  for (int i = 0; i < accessPoints; i++) {
    body += "<option value='" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + "</option>";
  }
  body += "</select><br>";
  body += "[Password] : <input type='password' name='pass'><br>";
  body += "<button type='submit'>登録</button><br>";
  body += "</form>";
  server.send(200, "text/html", generateHtml(body));
}


/* Wi-Fi設定用POSTハンドラ */
void wifiConfPostHandler() {
  wifiSsid = server.arg("ssid");
  wifiPass = server.arg("pass");

  File f = SPIFFS.open(WIFI_CONF, "w");
  f.println(wifiSsid);
  f.println(wifiPass);
  f.close();

  String body = "";
  body += "<h1>Wi-Fi Preference</h1>";
  body += "[SSID] : " + wifiSsid + "<br>";
  body += "please system restart.";
  server.send(200, "text/html", generateHtml(body));
}


/* 時刻表設定用GETハンドラ */
void timetableConfGetHandler() {
  String body = "";
  body += "<h1>Timetable Preference</h1>";
  body += "[Timetable JSON] : <br>";
  body += "<form method='post'>";
  body += "<textarea name='json' rows='40' cols='80'>" + json + "</textarea><br>";
  body += "<button type='submit'>登録</button><br>";
  body += "</form>";
  server.send(200, "text/html", generateHtml(body));
}


/* 時刻表設定用POSTハンドラ */
void timetableConfPostHandler() {
  String json = server.arg("json");
  json.trim();

  String body = "";
  body += "<h1>Timetable Preference</h1>";
  body += "[Timetable JSON] : <br>";
  body += json;

  File f = SPIFFS.open(TIMETABLE_JSON, "w");
  f.println(json);
  f.close();

  timetable.setJson(&json);

  server.send(200, "text/html", generateHtml(body));
}

