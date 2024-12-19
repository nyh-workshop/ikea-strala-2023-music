#include <Arduino.h>
#include <hardware/pll.h>
#include <hardware/vreg.h>

#ifdef ESP32
void setup() {
  Serial.begin(115200);
  Serial.printf("ERROR - ESP32 does not support LittleFS\n");
}
void loop() {}
#else
#if defined(ARDUINO_ARCH_RP2040)
#define WIFI_OFF
class __x {
public:
  __x(){};
  void mode(){};
};
__x WiFi;
#else
#include <ESP8266WiFi.h>
#endif
#include <AudioOutputI2S.h>
#include <AudioGeneratorMIDI.h>
#include <AudioFileSourceLittleFS.h>

#define MAX98357_SD 2

AudioFileSourceLittleFS *sf2;
AudioFileSourceLittleFS *mid;
AudioOutputI2S *dac;
AudioGeneratorMIDI *midi;
Dir dir;
File f;
bool isPlayedAllSongs = false;
const char *soundfont = "soundfont/DX7_Guitar_1.sf2";

void goToSleepUntilNextPwrOn() {
  rp2040.idleOtherCore();
  //set_sys_clock_pll(756000000, 7, 6);
  set_sys_clock_48mhz();
  pll_deinit(pll_usb);
  vreg_set_voltage(VREG_VOLTAGE_0_90);
}

String getNextMidiLocationAtRoot() {
  String midiFileLocation;
  //File midiFile = root.openNextFile();
  
  bool isNextFile = dir.next();

  if(!isNextFile)
    return "NoMidiFile";

  midiFileLocation = '/' + dir.fileName();

  //midiFileLocation = '/' + String(midiFile.name());

  int found = midiFileLocation.indexOf(".mid");

  //if (!midiFile || found == -1)
  if (found == -1)
    return "NoMidiFile";

  Serial.printf("midi file: %s\n", midiFileLocation.c_str());

  //midiFile.close();

  return midiFileLocation;
}

void playNextMidiFile(String aMidiFileLocation) {

  if (aMidiFileLocation == "NoMidiFile")
  {
    Serial.printf("Done playing all songs!\n");
    isPlayedAllSongs = true;
    return;
  }

  sf2 = new AudioFileSourceLittleFS(soundfont);
  mid = new AudioFileSourceLittleFS(aMidiFileLocation.c_str());

  dac = new AudioOutputI2S();
  midi = new AudioGeneratorMIDI();
  midi->SetSoundfont(sf2);
  midi->SetSampleRate(22050);
  Serial.printf("BEGIN...\n");
  midi->begin(mid, dac);
}

void donePlayingMidiFile() {
  delete mid;
  delete midi;
  delete sf2;
  delete dac;
}

void setup() {
  WiFi.mode(WIFI_OFF);

  Serial.begin(115200);
  Serial.println("Starting up...\n");

  pinMode(MAX98357_SD, INPUT);

  LittleFS.begin();
  //root = LittleFS.open("/", "r");
  dir = LittleFS.openDir("/");
  dir.rewind();

  delay(2000);

  audioLogger = &Serial;

  playNextMidiFile(getNextMidiLocationAtRoot());
}

void loop() {
  if (!isPlayedAllSongs) {
    if (midi->isRunning()) {
      if (!midi->loop()) {
        midi->stop();
        Serial.printf("Preparing next song!\n");
        donePlayingMidiFile();
        playNextMidiFile(getNextMidiLocationAtRoot());
      }
    }
  } else {
    Serial.printf("Play music done!\n");
    delay(2000);
    pinMode(MAX98357_SD, OUTPUT);
    digitalWrite(MAX98357_SD, 1);
    goToSleepUntilNextPwrOn();
    while(1)
    {
      delay(1000);
    }

  }
}

#endif
