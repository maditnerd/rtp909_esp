#include <Arduino.h>
#include "ESP8266WiFi.h"
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <i2s.h>
#include <i2s_reg.h>
#include <pgmspace.h>
#include "AppleMidi.h"
#include <Ticker.h>

//Store data in ICACHE https://gist.github.com/sticilface/e54016485fcccd10950e93ddcd4461a3
#define PROGMEM   ICACHE_RODATA_ATTR

// Samples
#include "BD16.h"
#include "CP16.h"
#include "CR16.h"
#include "HH16.h"
#include "HT16.h"
#include "LT16.h"
#include "MT16.h"
#include "OH16.h"
#include "RD16.h"
#include "RS16.h"
#include "SD16.h"

extern "C" {
#include "user_interface.h"
}

#define GPIO_IN ((volatile uint32_t*) 0x60000318)    // register contains gpio pin values of ESP8266 in read mode + some extra values (need to be truncated)

char ssid[] = ""; //  your network SSID (name)
char pass[] = "";    // your network password (use for WPA, or use as key for WEP)

APPLEMIDI_CREATE_INSTANCE(WiFiUDP, AppleMIDI); // see definition in AppleMidi_Defs.h

// Forward declaration
void OnAppleMidiConnected(uint32_t ssrc, char* name);
void OnAppleMidiDisconnected(uint32_t ssrc);
void OnAppleMidiNoteOn(byte channel, byte note, byte velocity);
void OnAppleMidiNoteOff(byte channel, byte note, byte velocity);

uint32_t i2sACC;
uint8_t i2sCNT=32;
uint16_t DAC=0x8000;
uint16_t err;

uint32_t samplecounter=100;
uint32_t TRIG0, TRIG1, TRIG2, TRIG3, TRIG4, TRIG5, TRIG6, TRIG7, TRIG8, TRIG9, TRIG10;
uint32_t OLDTRIG0,OLDTRIG1,OLDTRIG2,OLDTRIG3,OLDTRIG4,OLDTRIG5,OLDTRIG6,OLDTRIG7,OLDTRIG8,OLDTRIG9,OLDTRIG10;

uint16_t SYNTH909() {
  int32_t DRUMTOTAL=0;
  if (BD16CNT<BD16LEN) DRUMTOTAL+=(pgm_read_word_near(BD16 + BD16CNT++)^32768)-32768;
  if (CP16CNT<CP16LEN) DRUMTOTAL+=(pgm_read_word_near(CP16 + CP16CNT++)^32768)-32768;
  if (CR16CNT<CR16LEN) DRUMTOTAL+=(pgm_read_word_near(CR16 + CR16CNT++)^32768)-32768;
  if (HH16CNT<HH16LEN) DRUMTOTAL+=(pgm_read_word_near(HH16 + HH16CNT++)^32768)-32768;
  if (HT16CNT<HT16LEN) DRUMTOTAL+=(pgm_read_word_near(HT16 + HT16CNT++)^32768)-32768;
  if (LT16CNT<LT16LEN) DRUMTOTAL+=(pgm_read_word_near(LT16 + LT16CNT++)^32768)-32768;
  if (MT16CNT<MT16LEN) DRUMTOTAL+=(pgm_read_word_near(MT16 + MT16CNT++)^32768)-32768;
  if (OH16CNT<OH16LEN) DRUMTOTAL+=(pgm_read_word_near(OH16 + OH16CNT++)^32768)-32768;
  if (RD16CNT<RD16LEN) DRUMTOTAL+=(pgm_read_word_near(RD16 + RD16CNT++)^32768)-32768;
  if (RS16CNT<RS16LEN) DRUMTOTAL+=(pgm_read_word_near(RS16 + RS16CNT++)^32768)-32768;
  if (SD16CNT<SD16LEN) DRUMTOTAL+=(pgm_read_word_near(SD16 + SD16CNT++)^32768)-32768;
  if  (DRUMTOTAL>32767) DRUMTOTAL=32767;
  if  (DRUMTOTAL<-32767) DRUMTOTAL=-32767;
  DRUMTOTAL+=32768;
  return DRUMTOTAL;
}

void ICACHE_RAM_ATTR onTimerISR(){
  
  while (!(i2s_is_full())) { //Don't block the ISR
    
    DAC=SYNTH909();

    //----------------- Pulse Density Modulated 16-bit I2S DAC --------------------
     for (uint8_t i=0;i<32;i++) { 
      i2sACC=i2sACC<<1;
      if(DAC >= err) {
        i2sACC|=1;
        err += 0xFFFF-DAC;
      }
        else
      {
        err -= DAC;
      }
     }
     bool flag=i2s_write_sample(i2sACC);
    //-----------------------------------------------------------------------

  }
  
  timer1_write(2000);//Next in 2mS
}


void setup() {
  //WiFi.forceSleepBegin();             
  //delay(1);                               
  system_update_cpu_freq(160);

  //Serial.begin(9600);
  
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  //Serial.print(F("IP address is ")); 
  //Serial.println(WiFi.localIP()); 


  AppleMIDI.begin("ESP909"); // 'ESP909' will show up as the session name

  AppleMIDI.OnReceiveNoteOn(OnAppleMidiNoteOn);
  
  i2s_begin();
  i2s_set_rate(44100);
  timer1_attachInterrupt(onTimerISR); //Attach our sampling ISR
  timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
  timer1_write(2000); //Service at 2mS intervall 
}

void loop() {
 AppleMIDI.run();
}



void OnAppleMidiNoteOn(byte channel, byte note, byte velocity) {

/* Triggers
Bass Drum MIDI-35
Bass Drum MIDI-36
Rim Shot MIDI-37
Snare Drum MIDI-38
Hand Clap MIDI-39
Snare Drum MIDI-40
Low Tom MIDI-41
Closed Hat MIDI-42
Low Tom MIDI-43
Closed Hat MIDI-44
Mid Tom MIDI-45
Open Hat MIDI-46
Mid Tom MIDI-47
Hi Tom MIDI-48
Crash Cymbal MIDI-49
Hi Tom MIDI-50
Ride Cymbal MIDI-51
*/

  if (channel==10) {
    if(note==35) BD16CNT=0;
    if(note==36) BD16CNT=0;
    if(note==37) RS16CNT=0;
    if(note==38) SD16CNT=0;
    if(note==39) CP16CNT=0;
    if(note==40) SD16CNT=0;
    if(note==41) LT16CNT=0;
    if(note==42) HH16CNT=0;
    if(note==43) LT16CNT=0;
    if(note==44) HH16CNT=0;
    if(note==45) MT16CNT=0;
    if(note==46) OH16CNT=0;
    if(note==47) MT16CNT=0;
    if(note==48) HT16CNT=0;
    if(note==49) CR16CNT=0;
    if(note==50) HT16CNT=0;  
    if(note==51) RD16CNT=0;
  }
}
