#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <TFT_eSPI.h>
#include <unoqrcode.h> 
#include <Preferences.h> // Neu: Für dauerhaftes Speichern

#include "logo.h" 

// --- KONFIGURATION ---
const char* MDNS_HOST     = "esp-qr";
const int PIN_BL          = 3;
const unsigned long QR_DURATION_MS = 30000; 

TFT_eSPI    tft = TFT_eSPI();
WebServer   server(80);
Preferences prefs; // Speicher-Objekt

bool          showingQR    = false;
unsigned long qrShownAt    = 0;
String        serialInput  = "";

// --- SPEICHER-FUNKTIONEN ---

void saveWiFi(String ssid, String pass) {
  prefs.begin("wifi-store", false);
  prefs.putString("ssid", ssid);
  prefs.putString("pass", pass);
  prefs.end();
  Serial.println(F("WLAN-Daten gespeichert. Starte neu..."));
  delay(1000);
  ESP.restart(); // Neustart um mit neuen Daten zu verbinden
}

// --- DISPLAY & QR (unverändert) ---

void drawLogo() {
  tft.fillScreen(TFT_BLACK);
  for (int y = 0; y < LOGO_HEIGHT; y++) {
    tft.pushImage(0, y, LOGO_WIDTH, 1, &logo_data[y * LOGO_WIDTH]);
    if (y % 15 == 0) yield(); 
  }
}

void drawQR(const String& url) {
  const int boxSize = 180;
  const int boxX = 30, boxY = 30; 
  tft.fillScreen(TFT_BLACK);
  tft.fillRect(boxX, boxY, boxSize, boxSize, TFT_WHITE);
  uint8_t version = (url.length() > 70) ? 9 : 4;
  int size = qrcode_getBufferSize(version);
  uint8_t* qrcodeData = (uint8_t*)malloc(size);
  if (!qrcodeData) return;
  QRCode qrcode;
  if (qrcode_initText(&qrcode, qrcodeData, version, 0, url.c_str()) == 0) {
    int scale = boxSize / qrcode.size;
    int offset = (boxSize - (qrcode.size * scale)) / 2;
    for (uint8_t y = 0; y < qrcode.size; y++) {
      for (uint8_t x = 0; x < qrcode.size; x++) {
        if (qrcode_getModule(&qrcode, x, y)) {
          tft.fillRect(boxX+offset + x*scale, boxY+offset + y*scale, scale, scale, TFT_BLACK);
        }
      }
    }
    showingQR = true;
    qrShownAt = millis();
  }
  free(qrcodeData);
}

// --- SETUP ---

void setup() {
  Serial.begin(115200);
  Serial.setRxBufferSize(1024);
  delay(2000);

  pinMode(PIN_BL, OUTPUT);
  digitalWrite(PIN_BL, LOW);
  tft.init();
  tft.setRotation(0);
  tft.setSwapBytes(true);
  drawLogo();
  digitalWrite(PIN_BL, HIGH);

  // WLAN-Daten aus Speicher lesen
  prefs.begin("wifi-store", true);
  String storedSSID = prefs.getString("ssid", "");
  String storedPass = prefs.getString("pass", "");
  prefs.end();

  if (storedSSID != "") {
    Serial.print(F("Verbinde mit gespeicherten Daten: "));
    Serial.println(storedSSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(storedSSID.c_str(), storedPass.c_str());

    // Kurzer Verbindungsversuch (10 Sek)
    unsigned long startTry = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTry < 10000) {
      delay(500); Serial.print(".");
      tft.fillCircle(120, 235, 2, TFT_YELLOW);
    }

    if (WiFi.status() == WL_CONNECTED) {
      tft.fillCircle(120, 235, 2, TFT_GREEN);
      Serial.println(WiFi.localIP());
      MDNS.begin(MDNS_HOST);
      server.on("/qr", HTTP_POST, [](){
        if(server.hasArg("url")) drawQR(server.arg("url"));
        server.send(200, "text/plain", "OK");
      });
      server.begin();
    } else {
      tft.fillCircle(120, 235, 2, TFT_RED);
      Serial.println(F("\nVerbindung fehlgeschlagen. Warte auf USB-Befehle."));
    }
  } else {
    Serial.println(F("Keine WLAN-Daten vorhanden. Bitte USB nutzen."));
  }
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) server.handleClient();

  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input.startsWith("WLAN:")) {
      // Format: WLAN:SSID,Passwort
      int commaIndex = input.indexOf(',');
      if (commaIndex > 5) {
        String newSSID = input.substring(5, commaIndex);
        String newPass = input.substring(commaIndex + 1);
        saveWiFi(newSSID, newPass);
      }
    } else if (input.length() > 0) {
      drawQR(input);
    }
  }

  if (showingQR && (millis() - qrShownAt > QR_DURATION_MS)) {
    showingQR = false;
    drawLogo();
  }
  yield();
}