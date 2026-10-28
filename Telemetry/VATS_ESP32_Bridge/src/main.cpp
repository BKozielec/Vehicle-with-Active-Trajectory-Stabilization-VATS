#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#define LED_PIN 2

//const char* AP_SSID = "VATS_Vehicle_AP";
//const char* AP_PASS = "12345678";

const char* WIFI_SSID = "NETAMO.PL_2451";
const char* WIFI_PASS = "12771714";

const uint16_t UDP_RX_PORT = 5006;
const uint16_t UDP_TX_PORT = 5005;

WiFiUDP udp;
//IPAddress pcIP(192, 168, 4, 2);
IPAddress pcIP;
bool pcConnected = false;

char incomingPacket[512];

int lastMode = -1;
int lastSpeedRpm = -1;
char lastDir = ' ';
int lastAngle = -1;

unsigned long lastUdpPacketTime = 0;
const unsigned long UDP_TIMEOUT_MS = 1000;
bool estopTriggered = false;

int getJsonValue(String json, String key) {
  int keyIndex = json.indexOf("\"" + key + "\":");
  if (keyIndex == -1) return 0;
  int startIndex = keyIndex + key.length() + 3;
  int endIndex = json.indexOf(',', startIndex);
  if (endIndex == -1) endIndex = json.indexOf('}', startIndex);
  return json.substring(startIndex, endIndex).toInt();
}

void triggerEstop(const char* reason) {
  if(!estopTriggered) {
    estopTriggered = true;
    pcConnected = false;
    lastSpeedRpm = 0;
    lastDir = 'S';


    Serial2.print("ESTOP\n");
    Serial.println("\n==================================================");
    Serial.printf("!!! EMERGENCE STOP: %s !!!\n", reason);
    Serial.println("[UART -> STM32]: ESTOP");
    Serial.println("==================================================\n");
  }
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, 16, 17);// RX2=GPIO16, TX2=GPIO17 do STM32

  //WiFi.softAP(AP_SSID, AP_PASS);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print("Łączenie z WiFi");
while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nPołączono z Wi-Fi!");
  Serial.print("Adres IP ESP32 w sieci domowej: ");
  Serial.println(WiFi.localIP());

  udp.begin(UDP_RX_PORT); 
Serial.println("Oczekiwanie na uruchomienie aplikacji Python...\n");}

void loop() {
  unsigned long currentMillis = millis();

  int packetSize = udp.parsePacket();
  if (packetSize > 0) {
    pcIP = udp.remoteIP();
    lastUdpPacketTime = currentMillis;

    if (!pcConnected) {
      pcConnected = true;
      estopTriggered = false;
      Serial.println("\n==================================================");
      Serial.printf("[SYSTEM] ANCHOR POINT: Połączono z aplikacją Python!\n");
      Serial.printf("[SYSTEM] Klient IP: %s\n", pcIP.toString().c_str());
      Serial.println("==================================================\n");
    }

    int len = udp.read(incomingPacket, sizeof(incomingPacket) - 1);
    if (len > 0) {
      incomingPacket[len] = '\0';
      String jsonStr = String(incomingPacket);

      if (getJsonValue(jsonStr, "estop") == 1){
        triggerEstop("Aplikacja została zamknięta");
        return;
      }

      int mode = getJsonValue(jsonStr, "mode");
      int speedPercent = getJsonValue(jsonStr, "speed");
      int angle = getJsonValue(jsonStr, "angle");

      if (mode != lastMode) {
        lastMode = mode;
        Serial2.print(mode == 0 ? "MODE:MAN\n" : "MODE:AUTO\n");
        Serial.printf("[UART -> STM32]: %s", mode == 0 ? "MODE:MAN\n" : "MODE:AUTO\n");
      }

      char currentDir = (speedPercent > 0) ? 'F' : ((speedPercent < 0) ? 'B' : 'S');
      int targetRpm = map(abs(speedPercent), 0, 100, 0, 300);

      if (currentDir != lastDir) {
        lastDir = currentDir;
        Serial2.printf("DIR:%c\n", currentDir);
        Serial.printf("[UART -> STM32]: DIR:%c\n", currentDir);
      }

      if (targetRpm != lastSpeedRpm) {
        lastSpeedRpm = targetRpm;
        Serial2.printf("S:%d\n", targetRpm);
        Serial.printf("[UART -> STM32]: S:%d\n", targetRpm);
      }

      if (angle != lastAngle){
        lastAngle = angle;
        Serial2.printf("SERVO:%d\n", angle);
        Serial.printf("[UART -> STM32]: SERVO:%d\n", angle);
      }

      if (Serial2.available() == 0) {
        String dummyTelemetry = "{\"yaw_ref\":0.0,\"yaw_meas\":0.0,\"rpm_l\":0,\"rpm_r\":0,\"t_offset\":0}";
        udp.beginPacket(pcIP, UDP_TX_PORT);
        udp.print(dummyTelemetry);
        udp.endPacket();
      }
    }
  }

  if (pcConnected && (currentMillis - lastUdpPacketTime > UDP_TIMEOUT_MS)) {
    triggerEstop("Utrata pakietów UDP / Utrata zasięgu Wi-Fi (>1s)");
  }

  if (Serial2.available() > 0) {
    String stm32Telemetry = Serial2.readStringUntil('\n');
    stm32Telemetry.trim();

    if (stm32Telemetry.length() > 0 && pcConnected){
    udp.beginPacket(pcIP, UDP_TX_PORT);
    udp.print(stm32Telemetry);
    udp.endPacket();
    }
  }
}