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

// Wiederverbinden mit Backoff: 2s, 4, 8, 16, 32, dann alle 60s. Bewusst KEIN
// enger Takt -- ein Verbindungsaufbau braucht mit Auth und Assoziation 1-3 s,
// und wer alle 500 ms neu anfaengt, bricht jeden laufenden Versuch ab und
// kommt nie ins Netz zurueck.
#define WIFI_RETRY_START_MS   2000UL
#define WIFI_RETRY_MAX_MS    60000UL

TFT_eSPI    tft = TFT_eSPI();
WebServer   server(80);
Preferences prefs; // Speicher-Objekt

bool          showingQR    = false;
unsigned long qrShownAt    = 0;
String        serialInput  = "";

// WLAN-Zustand. Die Zugangsdaten liegen im RAM, damit der Reconnect sie nicht
// bei jedem Versuch aus dem NVS holen muss.
String   wifiSSID          = "";
String   wifiPass          = "";
bool     netServicesUp     = false;   // mDNS + HTTP-Server laufen
uint32_t wifiLostSince     = 0;       // 0 = Verbindung steht
uint32_t wifiNextTry       = 0;
uint8_t  wifiTries         = 0;

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

// --- NETZ ---

// Statuspunkt am unteren Rand: gelb = versucht, gruen = verbunden, rot = weg.
void showNetState(uint16_t colour) {
  tft.fillCircle(120, 235, 2, colour);
}

// mDNS und HTTP-Server hochziehen. Idempotent, damit der Aufruf aus wifiTick()
// nach jedem Reconnect gefahrlos ist -- vorher wurden die Dienste NUR beim Boot
// gestartet, ein spaeter auftauchendes WLAN blieb also ohne HTTP-Server.
void startNetServices() {
  if (netServicesUp) return;

  MDNS.begin(MDNS_HOST);
  server.on("/qr", HTTP_POST, []() {
    if (server.hasArg("url")) drawQR(server.arg("url"));
    server.send(200, "text/plain", "OK");
  });
  server.begin();
  netServicesUp = true;

  Serial.print(F("HTTP-Server laeuft. IP: "));
  Serial.println(WiFi.localIP());
  Serial.print(F("Host: http://"));
  Serial.print(MDNS_HOST);
  Serial.println(F(".local"));
}

// Wird aus loop() bei jedem Durchlauf aufgerufen und haelt die Verbindung.
void wifiTick() {
  if (wifiSSID == "") return;   // ohne Zugangsdaten gibt es nichts zu halten

  if (WiFi.status() == WL_CONNECTED) {
    if (wifiLostSince != 0) {
      Serial.printf("[WiFi] wieder verbunden nach %lu s\n",
                    (unsigned long)((millis() - wifiLostSince) / 1000));
      wifiLostSince = 0;
      wifiTries     = 0;
      showNetState(TFT_GREEN);
    }
    startNetServices();
    return;
  }

  uint32_t now = millis();

  if (wifiLostSince == 0) {
    wifiLostSince = now;
    wifiNextTry   = now;        // erster Versuch sofort
    wifiTries     = 0;
    Serial.println(F("[WiFi] Verbindung verloren"));
    showNetState(TFT_RED);
  }

  if ((int32_t)(now - wifiNextTry) < 0) return;   // Backoff noch nicht abgelaufen

  uint32_t backoff = WIFI_RETRY_START_MS << (wifiTries < 5 ? wifiTries : 5);
  if (backoff > WIFI_RETRY_MAX_MS) backoff = WIFI_RETRY_MAX_MS;
  wifiNextTry = now + backoff;
  if (wifiTries < 200) wifiTries++;

  Serial.printf("[WiFi] Versuch %u, naechster in %lu s\n",
                wifiTries, (unsigned long)(backoff / 1000));
  showNetState(TFT_YELLOW);
  WiFi.disconnect();
  WiFi.begin(wifiSSID.c_str(), wifiPass.c_str());
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
  wifiSSID = prefs.getString("ssid", "");
  wifiPass = prefs.getString("pass", "");
  prefs.end();

  if (wifiSSID != "") {
    Serial.print(F("Verbinde mit gespeicherten Daten: "));
    Serial.println(wifiSSID);
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);   // erste Verteidigungslinie des Kerns
    WiFi.begin(wifiSSID.c_str(), wifiPass.c_str());

    // Kurzer Verbindungsversuch (10 Sek). Klappt er nicht, ist das kein
    // Beinbruch mehr -- wifiTick() in loop() versucht es weiter.
    unsigned long startTry = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTry < 10000) {
      delay(500); Serial.print(".");
      showNetState(TFT_YELLOW);
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
      showNetState(TFT_GREEN);
      startNetServices();
    } else {
      showNetState(TFT_RED);
      Serial.println(F("Noch keine Verbindung. Wird im Hintergrund weiter versucht."));
    }
  } else {
    Serial.println(F("Keine WLAN-Daten vorhanden. Per USB setzen: WLAN:SSID,Passwort"));
  }
}

void loop() {
  wifiTick();   // haelt die Verbindung, startet Dienste nach dem ersten Connect

  if (netServicesUp && WiFi.status() == WL_CONNECTED) server.handleClient();

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
    } else if (input.equalsIgnoreCase("logo")) {
      // Der Mac-Client (QR-Manager) bietet "Zurueck zum Logo" an und sendet
      // genau dieses Wort. Ohne diesen Zweig wurde daraus ein QR-Code mit dem
      // Text "logo".
      showingQR = false;
      drawLogo();
      Serial.println(F("Logo."));
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