

#define NUM_LEDS 150        
#define BRIGHTNESS 230    

#define SOUND_R A2         // microphone
#define SOUND_R_FREQ A3    // microphone
#define BTN_PIN 3          // button for switch modes
#define LED_PIN 12         // LED-strip
#define POT_GND A0         // potentiometer

#define RAINBOW_SPEED 6    
#define RAINBOW_STEP 6     

#define MODE 0              // first mode
#define MAIN_LOOP 5        
#define SMOOTH 0.5         
#define SMOOTH_FREQ 0.8     
#define MAX_COEF 1.8        
#define MAX_COEF_FREQ 1.2  


#define MONO 1              
#define EXP 1.4             
#define POTENT 0           

int LOW_PASS = 100;         
int SPEKTR_LOW_PASS = 40;   
#define AUTO_LOW_PASS 0     
#define EEPROM_LOW_PASS 1   
#define LOW_PASS_ADD 13     
#define LOW_PASS_FREQ_ADD 3 


#define LOW_COLOR RED       
#define MID_COLOR GREEN     
#define HIGH_COLOR YELLOW   

#define BLUE     0x0000FF
#define RED      0xFF0000
#define GREEN    0x00ff00
#define CYAN     0x00FFFF
#define MAGENTA  0xFF00FF
#define YELLOW   0xFFFF00
#define WHITE    0xFFFFFF
#define BLACK    0x000000

#define STRIPE NUM_LEDS / 5

#define FHT_N 64         
#define LOG_OUT 1
#include <FHT.h>         
#include <EEPROMex.h>

#include "FastLED.h"
CRGB leds[NUM_LEDS];

#include "GyverButton.h"
GButton butt1(BTN_PIN);


DEFINE_GRADIENT_PALETTE(soundlevel_gp) {
  0,    0,    255,  0,  // green
  100,  255,  255,  0,  // yellow
  150,  255,  100,  0,  // orange
  200,  255,  50,   0,  // red
  255,  255,  0,    0   // red
};
CRGBPalette32 myPal = soundlevel_gp;

byte Rlenght, Llenght;
float RsoundLevel, RsoundLevel_f;
float LsoundLevel, LsoundLevel_f;

float averageLevel = 50;
int maxLevel = 100;
byte MAX_CH = NUM_LEDS / 2;
int hue;
unsigned long main_timer, hue_timer;
float averK = 0.006, k = SMOOTH, k_freq = SMOOTH_FREQ;
byte count;
float index = (float)255 / MAX_CH;   
boolean lowFlag;
byte low_pass;
int RcurrentLevel, LcurrentLevel;
int colorMusic[3];
float colorMusic_f[3], colorMusic_aver[3];
boolean colorMusicFlash[3];
byte this_mode = MODE;

#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))

void setup() {
  Serial.begin(9600);
  FastLED.addLeds<WS2811, LED_PIN, GRB>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(BRIGHTNESS);

  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);

  pinMode(POT_GND, OUTPUT);
  digitalWrite(POT_GND, LOW);
  butt1.setTimeout(900);

  if (POTENT) analogReference(EXTERNAL); // AREF
  else analogReference(INTERNAL); // 1,1V void Arduino
  

  sbi(ADCSRA, ADPS2);
  cbi(ADCSRA, ADPS1);
  sbi(ADCSRA, ADPS0);

  if (AUTO_LOW_PASS && !EEPROM_LOW_PASS) {         
    autoLowPass();
  }
  if (EEPROM_LOW_PASS) {                
    LOW_PASS = EEPROM.readInt(0);
    SPEKTR_LOW_PASS = EEPROM.readInt(2);
  }
}

void loop() {
  butt1.tick();  
  if (butt1.isSingle())                   
    if (++this_mode > 4) this_mode = 0;   

  if (butt1.isHolded()) {     
    digitalWrite(13, HIGH);   
    FastLED.setBrightness(0); 
    FastLED.clear();          
    FastLED.show();           
    delay(500);               
    autoLowPass();            
    delay(500);              
    FastLED.setBrightness(BRIGHTNESS);  
    digitalWrite(13, LOW);    
  }

  
  if (millis() - hue_timer > RAINBOW_SPEED) {
    if (++hue >= 255) hue = 0;
    hue_timer = millis();
  }


  if (millis() - main_timer > MAIN_LOOP) {
    
    RsoundLevel = 0;
    LsoundLevel = 0;

    
    if (this_mode == 0 || this_mode == 1) {
      for (byte i = 0; i < 100; i ++) {                                
        RcurrentLevel = analogRead(SOUND_R);                           
        if (!MONO) LcurrentLevel = analogRead(SOUND_L);                 

        if (RsoundLevel < RcurrentLevel) RsoundLevel = RcurrentLevel;   
        if (!MONO) if (LsoundLevel < LcurrentLevel) LsoundLevel = LcurrentLevel;   
      }

      
      RsoundLevel = map(RsoundLevel, LOW_PASS, 1023, 0, 500);
      if (!MONO)LsoundLevel = map(LsoundLevel, LOW_PASS, 1023, 0, 500);


      RsoundLevel = constrain(RsoundLevel, 0, 500);
      if (!MONO)LsoundLevel = constrain(LsoundLevel, 0, 500);

      RsoundLevel = pow(RsoundLevel, EXP);
      if (!MONO)LsoundLevel = pow(LsoundLevel, EXP);

 
      RsoundLevel_f = RsoundLevel * k + RsoundLevel_f * (1 - k);
      if (!MONO)LsoundLevel_f = LsoundLevel * k + LsoundLevel_f * (1 - k);

      if (MONO) LsoundLevel_f = RsoundLevel_f; 

   
      if (RsoundLevel_f > 15 && LsoundLevel_f > 15) {

       
        averageLevel = (float)(RsoundLevel_f + LsoundLevel_f) / 2 * averK + averageLevel * (1 - averK);


        maxLevel = (float)averageLevel * MAX_COEF;


        Rlenght = map(RsoundLevel_f, 0, maxLevel, 0, MAX_CH);
        Llenght = map(LsoundLevel_f, 0, maxLevel, 0, MAX_CH);

       
        Rlenght = constrain(Rlenght, 0, MAX_CH);
        Llenght = constrain(Llenght, 0, MAX_CH);

        animation();      
      }
    }

    if (this_mode == 2 || this_mode == 3 || this_mode == 4) {
      analyzeAudio();
      colorMusic[0] = 0;
      colorMusic[1] = 0;
      colorMusic[2] = 0;
     
      for (byte i = 3; i < 6; i++) {
        if (fht_log_out[i] > SPEKTR_LOW_PASS) {
          if (fht_log_out[i] > colorMusic[0]) colorMusic[0] = fht_log_out[i];
        }
      }
      
      for (byte i = 6; i < 11; i++) {
        if (fht_log_out[i] > SPEKTR_LOW_PASS) {
          if (fht_log_out[i] > colorMusic[1]) colorMusic[1] = fht_log_out[i];
        }
      }
     
      for (byte i = 11; i < 31; i++) {
        if (fht_log_out[i] > SPEKTR_LOW_PASS) {
          if (fht_log_out[i] > colorMusic[2]) colorMusic[2] = fht_log_out[i];
        }
      }
      for (byte i = 0; i < 3; i++) {
        colorMusic_aver[i] = colorMusic[i] * averK + colorMusic_aver[i] * (1 - averK); 
        colorMusic_f[i] = colorMusic[i] * k_freq + colorMusic_f[i] * (1 - k_freq);     
        if (colorMusic_f[i] > ((float)colorMusic_aver[i] * MAX_COEF_FREQ))
          colorMusicFlash[i] = 1;
        else
          colorMusicFlash[i] = 0;
      }
      animation();
    }

    FastLED.show();           
    FastLED.clear();          
    main_timer = millis();    
  }
}

void animation() {

  switch (this_mode) {
    case 0:
      count = 0;
      for (int i = (MAX_CH - 1); i > ((MAX_CH - 1) - Rlenght); i--) {
        leds[i] = ColorFromPalette(myPal, (count * index));  
        count++;
      }
      count = 0;
      for (int i = (MAX_CH); i < (MAX_CH + Llenght); i++ ) {
        leds[i] = ColorFromPalette(myPal, (count * index)); 
        count++;
      }
      break;
    case 1:
      count = 0;
      for (int i = (MAX_CH - 1); i > ((MAX_CH - 1) - Rlenght); i--) {
        leds[i] = ColorFromPalette(RainbowColors_p, (count * index) / 2 - hue);  
        count++;
      }
      count = 0;
      for (int i = (MAX_CH); i < (MAX_CH + Llenght); i++ ) {
        leds[i] = ColorFromPalette(RainbowColors_p, (count * index) / 2 - hue); /
        count++;
      }
      break;
    case 2:
      for (int i = 0; i < NUM_LEDS; i++) {
        if (i < STRIPE)          leds[i] = colorMusicFlash[2] * HIGH_COLOR;
        else if (i < STRIPE * 2) leds[i] = colorMusicFlash[1] * MID_COLOR;
        else if (i < STRIPE * 3) leds[i] = colorMusicFlash[0] * LOW_COLOR;
        else if (i < STRIPE * 4) leds[i] = colorMusicFlash[1] * MID_COLOR;
        else if (i < STRIPE * 5) leds[i] = colorMusicFlash[2] * HIGH_COLOR;
      }
      break;
    case 3:
      for (int i = 0; i < NUM_LEDS; i++) {
        if (i < NUM_LEDS / 3)          leds[i] = colorMusicFlash[2] * HIGH_COLOR;
        else if (i < NUM_LEDS * 2 / 3) leds[i] = colorMusicFlash[1] * MID_COLOR;
        else if (i < NUM_LEDS)         leds[i] = colorMusicFlash[0] * LOW_COLOR;
      }
      break;
    case 4:
      uint32_t this_color;
      if (colorMusicFlash[2]) this_color = HIGH_COLOR;
      else if (colorMusicFlash[1]) this_color = MID_COLOR;
      else if (colorMusicFlash[0]) this_color = LOW_COLOR;
      else this_color = BLACK;
      for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = this_color;
      }
      break;
  }
}

void autoLowPass() {

  delay(10);                                
  int thisMax = 0;                          
  int thisLevel;
  for (byte i = 0; i < 200; i++) {
    thisLevel = analogRead(SOUND_R);        
    if (thisLevel > thisMax)                
      thisMax = thisLevel;                 
    delay(4);                               
  }
  LOW_PASS = thisMax + LOW_PASS_ADD;        

  
  thisMax = 0;
  for (byte i = 0; i < 100; i++) {          
    analyzeAudio();                       
    for (byte j = 2; j < 32; j++) {       
      thisLevel = fht_log_out[j];
      if (thisLevel > thisMax)             
        thisMax = thisLevel;               
    }
    delay(4);                              
  }
  SPEKTR_LOW_PASS = thisMax + LOW_PASS_FREQ_ADD;  

  if (EEPROM_LOW_PASS && !AUTO_LOW_PASS) {
    EEPROM.updateInt(0, LOW_PASS);
    EEPROM.updateInt(2, SPEKTR_LOW_PASS);
  }
}

void analyzeAudio() {
  for (int i = 0 ; i < FHT_N ; i++) {
    int sample = analogRead(SOUND_R_FREQ);
    fht_input[i] = sample; /
  }
  fht_window();  
  fht_reorder(); 
  fht_run();    
  fht_mag_log(); 
}