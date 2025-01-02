// TFT_eSPI_memory
//
// Example sketch which shows how to display an
// animated GIF image stored in FLASH memory

// Adapted by Bodmer for the TFT_eSPI Arduino library:
// https://github.com/Bodmer/TFT_eSPI
//
// To display a GIF from memory, a single callback function
// must be provided - GIFDRAW
// This function is called after each scan line is decoded
// and is passed the 8-bit pixels, RGB565 palette and info
// about how and where to display the line. The palette entries
// can be in little-endian or big-endian order; this is specified
// in the begin() method.
//
// The AnimatedGIF class doesn't allocate or free any memory, but the
// instance data occupies about 22.5K of RAM.

//========================USEFUL VARIABLES=============================
const char *ssid = "ASUS-RT-AX56U-2.4G";
const char *password = "tocino25";
int UTC = -8;                       //Set your time zone ex: france = UTC+2
uint16_t notification_volume = 25;  //0 to 30
#define DEBUG 0                     // Set to 0 to exclude
//=====================================================================

#include <TFT_eSPI.h>
#include "WiFiManager.h"
#include "NTPClient.h"
#include "DFRobotDFPlayerMini.h"
#include <Adafruit_AHTX0.h>
#include "FS.h"
#include <SPI.h>

#define REPEAT_CAL false
#define Light_green 0x35C2
#define Dark_green 0x0261
#define Time_color 0x04C0

// Load GIF library
#include <AnimatedGIF.h>
AnimatedGIF gif;

#include "images/INIT.h"
#include "images/STAT.h"
#include "images/RADIO.h"
#include "images/DATA_1.h"
#include "images/TIME.h"
#include "images/Bottom_layer_2.h"
#include "images/Date.h"
#include "images/INV.h"
#include "images/temperatureTemp_hum.h"
#include "images/RADIATION.h"
#include "images/temperatureTemp_hum_F.h"
#include "images/vaultboy.h"

#define INIT INIT
#define TIME TIME
#define STAT STAT
#define DATA_1 DATA_1
#define INV INV

#define IN_STAT 25
#define IN_INV 26
#define IN_DATA 27
#define IN_TIME 32
#define IN_MP3PLAYER 33
#define IN_RADIO 34
#define IN_RADIO1 35
#define IN_RADIO2 36

// PCM5102A DAC pins for internet radio
#define I2S_BCLK 12  // BCK pin
#define I2S_LRC 13   // LRCK pin
#define I2S_DOUT 14  // DIN pin

// Internet radio
#include <AudioFileSourceICYStream.h>
#include <AudioFileSourceBuffer.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputI2S.h>

// Forward declarations
void cleanupI2S();
void MDCallback(void *cbData, const char *type, bool isUnicode, const char *string);
bool setupAudio();
bool startRadio(uint8_t stationIndex);
void displaySongInfo();
void handleRadioStation(uint8_t stationIndex);
void show_hour();

// Variables
const byte RXD2 = 16;  // Connects to module's TX => 16
const byte TXD2 = 17;  // Connects to module's RX => 17

DFRobotDFPlayerMini myDFPlayer;
#define FPSerial Serial1
TFT_eSPI tft = TFT_eSPI();
Adafruit_AHTX0 aht;

int i = 0;
int a = 0;
uint16_t x = 0, y = 0;
int interupt = 1;
float t_far = 0;
int hh = 0;
int mm = 0;
int ss = 0;
int flag = 0;
int prev_hour = 0;
String localip;
bool enableHeater = false;
uint8_t loopCnt = 0;
const long utcOffsetInSeconds = 3600;  // Offset in second
uint32_t targetTime = 0;               // for next 1 second timeout

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds *UTC);

byte omm = 99, oss = 99;
byte xcolon = 0, xsecs = 0;
unsigned int colour = 0;

// Audio objects
AudioFileSourceICYStream *stream = nullptr;
AudioFileSourceBuffer *buff = nullptr;
AudioGeneratorMP3 *mp3 = nullptr;
AudioOutputI2S *out = nullptr;
uint8_t *preallocateBuffer = nullptr;
const int preallocateBufferSize = 64 * 1024;

char currentTitle[64] = "";
char currentArtist[64] = "";
bool newSongInfo = false;

struct RadioStation {
  const char *name;
  const char *url;
};

// Radio stations
const RadioStation stations[] = {
  { "60s and 70s Hits", "http://strm112.1.fm/60s_70s_mobile_mp3" },
  { "70s Greatest Hits", "http://hydra.cdnstream.com/1823_128" },
  { "Q102 80s", "http://wg.cdn.tibus.net/Q10280s" }
};

void handleRadioStation(uint8_t stationIndex, uint8_t rotaryPin) {
  flag = 1;
  bool radioStarted = false;

  tft.fillScreen(TFT_BLACK);
  tft.drawBitmap(35, 300, Bottom_layer_2Bottom_layer_2, 380, 22, Dark_green);
  tft.drawBitmap(35, 300, myBitmapDate, 380, 22, Light_green);

  // Show initial radio GIF
  if (gif.open((uint8_t *)RADIO, sizeof(RADIO), GIFDraw)) {
    tft.startWrite();
    gif.playFrame(true, NULL);
    gif.close();
    tft.endWrite();
  }

  // Stop DFPlayer
  myDFPlayer.pause();

  // Add debug output for radio startup
#if DEBUG
  Serial.println("Starting radio playback...");
#endif

  while (digitalRead(rotaryPin) == false) {
    if (!radioStarted) {
#if DEBUG
      Serial.printf("Attempting to start station %d\n", stationIndex);
#endif
      radioStarted = startRadio(stationIndex);
      delay(100);
    }

    if (radioStarted && mp3) {
      // Multiple mp3->loop() calls for smoother playback
      for (int i = 0; i < 4; i++) {
        if (mp3->isRunning()) {
          if (!mp3->loop()) {
            break;
          }
        }
        yield();
      }

      // Update song info if available
      if (newSongInfo) {
        displaySongInfo(stationIndex);
        newSongInfo = false;
      }

      // Restart if stream stops
      if (!mp3->isRunning()) {
#if DEBUG
        Serial.println("MP3 stopped, restarting...");
#endif
        cleanupI2S();
        radioStarted = startRadio(stationIndex);
      }
    }
    yield();
  }

  // Cleanup when exiting
  if (radioStarted) {
    cleanupI2S();
    radioStarted = false;
  }
}

void MDCallback(void *cbData, const char *type, bool isUnicode, const char *string) {
#if DEBUG
  Serial.printf("Metadata: %s = %s\n", type, string);
#endif

  if (strcmp(type, "StreamTitle") == 0 || strcmp(type, "Title") == 0) {
    // Search for the separator " - "
    const char *separator = strstr(string, " - ");

    if (separator) {
      // Calculate lengths
      size_t artistLen = separator - string;

      // Copy artist (everything before " - ")
      strncpy(currentArtist, string, min(artistLen, sizeof(currentArtist) - 1));
      currentArtist[min(artistLen, sizeof(currentArtist) - 1)] = '\0';

      // Copy title (everything after " - ")
      const char *titleStart = separator + 3;  // Skip " - "
      strncpy(currentTitle, titleStart, sizeof(currentTitle) - 1);
      currentTitle[sizeof(currentTitle) - 1] = '\0';
    } else {
      // If no separator found, put entire string in title
      strncpy(currentTitle, string, sizeof(currentTitle) - 1);
      currentTitle[sizeof(currentTitle) - 1] = '\0';
      currentArtist[0] = '\0';  // Clear artist
    }

    newSongInfo = true;
  }
}

void cleanupI2S() {
  if (mp3) {
    mp3->stop();
    delete mp3;
    mp3 = nullptr;
  }

  if (buff) {
    delete buff;
    buff = nullptr;
  }

  if (stream) {
    delete stream;
    stream = nullptr;
  }
}

bool setupAudio() {
  if (!preallocateBuffer) {
    preallocateBuffer = (uint8_t *)malloc(preallocateBufferSize);
    if (!preallocateBuffer) {
#if DEBUG
      Serial.println("Failed to allocate buffer");
#endif
      return false;
    }
  }

  if (!out) {
    out = new AudioOutputI2S();
    out->SetPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    out->SetGain(0.5);
    out->SetOutputModeMono(true);
  }

  return true;
}

bool startRadio(uint8_t stationIndex) {
#if DEBUG
  Serial.println("Starting radio...");
  Serial.printf("Attempting to play station: %s\n", stations[stationIndex].name);
#endif

  // Bounds check for station index
  if (stationIndex >= sizeof(stations) / sizeof(stations[0])) {
#if DEBUG
    Serial.println("Invalid station index, defaulting to first station");
#endif
    stationIndex = 0;
  }

  cleanupI2S();

  if (!setupAudio()) return false;

  // Use the URL from our stations array
  stream = new AudioFileSourceICYStream(stations[stationIndex].url);
  if (!stream) {
#if DEBUG
    Serial.println("Failed to create stream");
#endif
    return false;
  }
  stream->RegisterMetadataCB(MDCallback, NULL);

  buff = new AudioFileSourceBuffer(stream, preallocateBuffer, preallocateBufferSize);
  if (!buff) {
#if DEBUG
    Serial.println("Failed to create buffer");
#endif
    cleanupI2S();
    return false;
  }

  mp3 = new AudioGeneratorMP3();
  if (!mp3) {
#if DEBUG
    Serial.println("Failed to create MP3 decoder");
#endif
    cleanupI2S();
    return false;
  }

  if (!mp3->begin(buff, out)) {
#if DEBUG
    Serial.println("Failed to start MP3 decoder");
#endif
    cleanupI2S();
    return false;
  }

#if DEBUG
  Serial.printf("Radio started successfully - playing %s\n", stations[stationIndex].name);
#endif

  return true;
}

void setup() {

  pinMode(IN_RADIO, INPUT_PULLUP);
  pinMode(IN_RADIO1, INPUT_PULLUP);
  pinMode(IN_RADIO2, INPUT_PULLUP);
  pinMode(IN_MP3PLAYER, INPUT_PULLUP);
  pinMode(IN_STAT, INPUT_PULLUP);
  pinMode(IN_DATA, INPUT_PULLUP);
  pinMode(IN_INV, INPUT_PULLUP);
  pinMode(IN_TIME, INPUT_PULLUP);

  Serial.begin(115200);
  delay(1000);

  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(Light_green, TFT_BLACK);
  tft.drawString("Network connection in progress", 10, 20, 4);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
#if DEBUG
    Serial.print(".");
#endif
    tft.drawString(".", 10 + a, 40, 4);
    a = a + 5;
    if (a > 100) {
      tft.fillScreen(TFT_BLACK);
      tft.drawString("ERROR", 180, 20, 4);
      tft.drawString("Check wifi SSID and PASSWORD", 10, 60, 4);
    }
  }

#if DEBUG
  Serial.println("\nConnected to the WiFi network");
  Serial.print("Local ESP32 IP: ");
  Serial.println(WiFi.localIP());
#endif

  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(Light_green, TFT_BLACK);
  localip = WiFi.localIP().toString();
  tft.drawString(localip, 10, 20, 4);

  timeClient.begin();

  FPSerial.begin(9600, SERIAL_8N1, RXD2, TXD2);
  delay(1000);

#if DEBUG
  Serial.println();
  Serial.println(F("DFRobot DFPlayer Mini Demo"));
  Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));
#endif

  tft.drawString("Initializing DFPlayer...", 10, 20, 4);
  tft.fillScreen(TFT_BLACK);

  if (!myDFPlayer.begin(FPSerial, /*isACK = */ true, /*doReset = */ true)) {  //Use serial to communicate with mp3.
#if DEBUG
    Serial.println(F("Unable to begin DFplayer"));
#endif
    tft.drawString("Unable to begin DFplayer", 10, 20, 4);

#if DEBUG
    Serial.println(F("1.Recheck the connection!"));
#endif
    tft.drawString("1. Recheck the connection", 10, 50, 4);

#if DEBUG
    Serial.println(F("2.Insert the SD card!"));
#endif

    tft.drawString("2. Insert the SD card", 10, 80, 4);
    tft.drawString("3. Format SD card in FAT32", 10, 110, 4);

    while (true) {
      delay(0);
    }
  }

#if DEBUG
  Serial.println(F("DFPlayer Mini online."));
#endif
  tft.drawString("DFPlayer Mini online", 10, 20, 4);

  myDFPlayer.volume(notification_volume);
  myDFPlayer.setTimeOut(500);

  // Initialize AHT sensor
  if (!aht.begin()) {
#if DEBUG
    Serial.println("Couldn't find AHT sensor!");
#endif
    tft.fillScreen(TFT_BLACK);
    tft.drawString("Couldn't find AHT sensor", 10, 20, 4);
    tft.drawString("Recheck the connection", 10, 50, 4);
    while (1) delay(1000);
  }

#if DEBUG
  Serial.println("AHT sensor initialized");
#endif

  gif.begin(BIG_ENDIAN_PIXELS);

  delay(1000);
  myDFPlayer.playMp3Folder(1);  //Play the first mp3

  // Initialize audio system
  if (!setupAudio()) {
#if DEBUG
    Serial.println("Failed to setup audio");
#endif
    return;
  }

  if (gif.open((uint8_t *)INIT, sizeof(INIT), GIFDraw)) {
    tft.startWrite();  // The TFT chip select is locked low
    while (gif.playFrame(true, NULL)) {
      yield();
    }

    gif.close();
    tft.endWrite();  // Release TFT chip select for other SPI devices
  }
}

void loop() {
  static unsigned long lastTimeUpdate = 0;
  const unsigned long TIME_UPDATE_INTERVAL = 10000;  // milliseconds to update time

  static bool radioStarted = false;
  static unsigned long lastCheck = 0;

  // Update time only every X seconds
  unsigned long currentMillis = millis();
  if (currentMillis - lastTimeUpdate >= TIME_UPDATE_INTERVAL) {
    lastTimeUpdate = currentMillis;
    timeClient.update();
#if DEBUG
    Serial.print("Time: ");
    Serial.println(timeClient.getFormattedTime());
#endif

    unsigned long epochTime = timeClient.getEpochTime();
    struct tm *ptm = gmtime((time_t *)&epochTime);
    int currentYear = ptm->tm_year + 1900;
#if DEBUG
    Serial.print("Year: ");
    Serial.println(currentYear);
#endif
    int monthDay = ptm->tm_mday;
#if DEBUG
    Serial.print("Month day: ");
    Serial.println(monthDay);
#endif

    int currentMonth = ptm->tm_mon + 1;
#if DEBUG
    Serial.print("Month: ");
    Serial.println(currentMonth);
#endif
  }

  if (digitalRead(IN_STAT) == false) {
    flag = 1;
    myDFPlayer.playMp3Folder(random(2, 5));
    while (digitalRead(IN_STAT) == false) {
      if (gif.open((uint8_t *)STAT, sizeof(STAT), GIFDraw)) {
#if DEBUG
        Serial.printf("Successfully opened GIF; Canvas size = %d x %d\n", gif.getCanvasWidth(), gif.getCanvasHeight());
#endif
        tft.startWrite();  // The TFT chip select is locked low
        while (gif.playFrame(true, NULL)) {
          yield();
        }
        gif.close();
        tft.endWrite();  // Release TFT chip select for other SPI devices
      }
    }
  }

  if (digitalRead(IN_INV) == false) {
    flag = 1;
    myDFPlayer.playMp3Folder(random(2, 5));
    while (digitalRead(IN_INV) == false) {
      if (gif.open((uint8_t *)INV, sizeof(INV), GIFDraw)) {
        tft.startWrite();  // The TFT chip select is locked low
        while (gif.playFrame(true, NULL)) {
          yield();
        }
        gif.close();
        tft.endWrite();  // Release TFT chip select for other SPI devices
      }
    }
  }

  if (digitalRead(IN_DATA) == false) {
    flag = 1;
    myDFPlayer.playMp3Folder(random(2, 5));
    tft.fillScreen(TFT_BLACK);
    tft.drawBitmap(35, 300, Bottom_layer_2Bottom_layer_2, 380, 22, Dark_green);
    tft.drawBitmap(35, 300, myBitmapDate, 380, 22, Light_green);
    tft.drawBitmap(35, 80, temperatureTemp_hum_F, 408, 29, Light_green);
    tft.drawBitmap(200, 200, RadiationRadiation, 62, 61, Light_green);

    while (digitalRead(IN_DATA) == false) {

      sensors_event_t humidity, temp;
      aht.getEvent(&humidity, &temp);
      float t = temp.temperature;
      float h = humidity.relative_humidity;

      if (gif.open((uint8_t *)DATA_1, sizeof(DATA_1), GIFDraw)) {
#if DEBUG
        Serial.printf("Successfully opened GIF; Canvas size = %d x %d\n", gif.getCanvasWidth(), gif.getCanvasHeight());
#endif
        tft.startWrite();  // The TFT chip select is locked low
        while (gif.playFrame(true, NULL)) {
          yield();
        }
        gif.close();
        tft.endWrite();  // Release TFT chip select for other SPI devices
      }
      //show_hour();
      tft.setTextColor(Time_color, TFT_BLACK);
      t_far = (t * 1.8) + 32;
      tft.drawFloat(t_far, 2, 60, 135, 7);
      tft.drawFloat(h, 2, 258, 135, 7);
    }
  }

  if (digitalRead(IN_TIME) == false) {
    myDFPlayer.playMp3Folder(random(2, 5));
    tft.fillScreen(TFT_BLACK);
    tft.drawBitmap(35, 300, Bottom_layer_2Bottom_layer_2, 380, 22, Dark_green);
    tft.drawBitmap(35, 300, myBitmapDate, 380, 22, Light_green);

    if (gif.open((uint8_t *)TIME, sizeof(TIME), GIFDraw)) {
#if DEBUG
      Serial.printf("Successfully opened GIF; Canvas size = %d x %d\n", gif.getCanvasWidth(), gif.getCanvasHeight());
#endif
      tft.startWrite();  // The TFT chip select is locked low
      while (gif.playFrame(true, NULL)) {
        yield();
      }
      gif.close();
      tft.endWrite();  // Release TFT chip select for other SPI devices
    }

    while (digitalRead(IN_TIME) == false) {
      if (gif.open((uint8_t *)vaultboy, sizeof(vaultboy), GIFDraw)) {
        tft.startWrite();  // The TFT chip select is locked low
        while (gif.playFrame(true, NULL)) {
          yield();
        }
        gif.close();
        tft.endWrite();  // Release TFT chip select for other SPI devices
      }
      show_hour();
    }
  }

  if (digitalRead(IN_MP3PLAYER) == false) {
    flag = 1;
#if DEBUG
    Serial.println("Start DF Mini Player");
#endif
    myDFPlayer.playMp3Folder(random(2, 5));
    delay(500);
    myDFPlayer.playMp3Folder(random(5, 10));
    tft.fillScreen(TFT_BLACK);
    tft.drawBitmap(35, 300, Bottom_layer_2Bottom_layer_2, 380, 22, Dark_green);
    tft.drawBitmap(35, 300, myBitmapDate, 380, 22, Light_green);

    while (digitalRead(IN_MP3PLAYER) == false) {
      if (gif.open((uint8_t *)RADIO, sizeof(RADIO), GIFDraw)) {
        tft.startWrite();  // The TFT chip select is locked low
        while (gif.playFrame(true, NULL)) {
          yield();
        }
        gif.close();
        tft.endWrite();  // Release TFT chip select for other SPI devices
      }
    }
  }

  if (digitalRead(IN_RADIO) == false) {
    handleRadioStation(0, IN_RADIO);
  }

  if (digitalRead(IN_RADIO1) == false) {
    handleRadioStation(1, IN_RADIO1);
  }

  if (digitalRead(IN_RADIO2) == false) {
    handleRadioStation(2, IN_RADIO2);
  }
}

// Function to display the current song info
void displaySongInfo(uint8_t stationIndex) {
#if DEBUG
  Serial.print("Drawing station name: ");
  Serial.println(stations[stationIndex].name);
#endif

  tft.setTextColor(Light_green, TFT_BLACK);
  tft.setTextSize(2);

  // Clear
  tft.fillRect(10, 70, 270, 100, TFT_BLACK);

  // Display station
  tft.drawString("STATION:", 35, 70, 1.5);
  tft.drawString(stations[stationIndex].name, 35, 85, 2);

  // Clear the song info area
  tft.fillRect(35, 190, 480, 100, TFT_BLACK);

  // Display artist
  if (strlen(currentArtist) > 0) {
    tft.drawString("ARTIST:", 35, 190, 1.5);
    tft.drawString(currentArtist, 35, 205, 2);
  }

  // Display title
  if (strlen(currentTitle) > 0) {
    tft.drawString("SONG:", 35, 245, 1.5);
    tft.drawString(currentTitle, 35, 260, 2);
  }
}

void show_hour() {
  tft.setTextSize(2);
  mm = timeClient.getMinutes();
  ss = timeClient.getSeconds();

  if (timeClient.getHours() == 0) {
    hh = 12;
  } else if (timeClient.getHours() == 12) {
    hh = timeClient.getHours();
  } else if (timeClient.getHours() >= 13) {
    hh = timeClient.getHours() - 12;
  } else {
    hh = timeClient.getHours();
  }

  if (timeClient.getHours() != prev_hour) {
    tft.fillRect(140, 210, 200, 50, TFT_BLACK);
  }

  int hour24 = timeClient.getHours();

  // Time periods display
  tft.setTextColor(TFT_BLACK, TFT_GREEN);
  tft.setTextSize(2);

  if (hour24 >= 5 && hour24 < 12) {
    tft.drawString("   MORNING   ", 190, 220, 2);
  } else if (hour24 >= 12 && hour24 < 17) {
    tft.drawString("   AFTERNOON   ", 170, 220, 2);
  } else if (hour24 >= 17 && hour24 < 21) {
    tft.drawString("   EVENING   ", 190, 220, 2);
  } else {
    tft.drawString("    NIGHT    ", 190, 220, 2);
  }

  // Update digital time
  static int baseX = 160;  // Base position that never changes
  int xpos = baseX;        // Working position that gets updated
  int ypos = 90;

  if (omm != mm || flag == 1) {  // Redraw hours and minutes time every minute
    // Clear the entire time display area
    tft.fillRect(baseX - 10, ypos - 10, 200, 60, TFT_BLACK);

    omm = mm;
    xpos = baseX;  // Reset xpos to base position

    // Draw hours and minutes
    tft.setTextColor(Time_color, TFT_BLACK);

    // Draw hours (with space for single digit)
    if (hh < 10) xpos += tft.drawChar(' ', xpos, ypos, 7);
    xpos += tft.drawNumber(hh, xpos, ypos, 7);

    // Save colon position before drawing it
    xcolon = xpos;
    xpos += tft.drawChar(':', xpos, ypos - 8, 7);

    // Draw minutes (with leading zero)
    if (mm < 10) {
      xpos += tft.drawChar('0', xpos, ypos, 7);  // Add the leading zero
    }
    xpos += tft.drawNumber(mm, xpos, ypos, 7);

    xsecs = xpos;
    flag = 0;
  }

  if (oss != ss) {  // Redraw seconds time every second
    oss = ss;

    // Only clear and redraw the actual colon area at the saved position
    tft.fillRect(xcolon, ypos - 8, 12, 48, TFT_BLACK);

    if (ss % 2) {
      tft.setTextColor(0x39C4, TFT_BLACK);
      tft.drawChar(':', xcolon, ypos - 8, 7);
      tft.setTextColor(Time_color, TFT_BLACK);
    } else {
      tft.setTextColor(Time_color, TFT_BLACK);
      tft.drawChar(':', xcolon, ypos - 8, 7);
    }
  }

  tft.setTextSize(1);
  prev_hour = timeClient.getHours();
}