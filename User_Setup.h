// =====================================================================
//   TFT_eSPI – User_Setup.h fuer ESP32-2424S012
//   (ESP32-C3 + 1.28" round GC9A01, 240x240)
//
//   Diese Datei in den TFT_eSPI-Library-Ordner kopieren und die
//   bestehende User_Setup.h ersetzen, oder per User_Setup_Select.h
//   einbinden.
//
//   Standardpfad (Arduino):
//     ~/Documents/Arduino/libraries/TFT_eSPI/User_Setup.h
//
//   Ergaenzend in User_Setup_Select.h sicherstellen:
//     #include <User_Setup.h>
//   (alle anderen Setups dort auskommentieren)
// =====================================================================

#define USER_SETUP_INFO  "ESP32-2424S012 GC9A01"

// ---------- Display-Treiber ----------
#define GC9A01_DRIVER

#define TFT_WIDTH   240
#define TFT_HEIGHT  240

// Auf einigen Boards ist die Farbe invertiert.  Falls das Logo
// negativ aussieht, diese Zeile aktivieren:
// #define TFT_INVERSION_OFF
// #define TFT_INVERSION_ON

// ---------- Pinbelegung ESP32-2424S012 ----------
#define TFT_MOSI   7
#define TFT_SCLK   6
#define TFT_CS    10
#define TFT_DC     2
#define TFT_RST   -1     // RST liegt auf EN / nicht separat verdrahtet
#define TFT_BL     3     // Backlight – im Sketch separat geschaltet
#define TFT_BACKLIGHT_ON  HIGH

// ---------- Schriften (nur was wir brauchen, spart Flash) ----------
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_GFXFF
#define SMOOTH_FONT

// ---------- SPI-Frequenz ----------
#define SPI_FREQUENCY        40000000
#define SPI_READ_FREQUENCY   20000000
