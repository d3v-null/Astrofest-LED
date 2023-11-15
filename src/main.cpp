#include <Arduino.h>
#include <FastLED.h>

#define NUM_LEDS 150
// #define DATA_PIN 5 // for esp32
#define DATA_PIN 21 // for nodemcu

// Define the array of leds
CRGB leds[NUM_LEDS];

void setup() {
  // put your setup code here, to run once:
  FastLED.addLeds<WS2812, DATA_PIN, RGB>(leds, NUM_LEDS);
}

int step = 0;

#define TRAIL_SIZE 5
#define COUNTRY_HOLD 10

int instruments[] = {
  68, // SKA Low (Murchison)
  45, // SKA Mid (The Karoo)
};
#define NUM_INSTRUMENTS (sizeof(instruments)/sizeof(instruments[0]))

int countries[] = {
  0, // Western Canada
  // 13 (iceland)
  // 19 (southern ireland ocean), next to uk
  // 20 (uk ocean)
  // 21 (france ocean)
  // 22 (spain ocean)
  // 23 (portugal ocean)
  24, // spain
  // 25 (southern france)
  26, // france
  27, // germany/switzerland
  // 28, (north italy)
  29, // italy
  // 30 (ionian sea)
  // 31 (mediterranean sea)
  // 32 Libya
  // 34 Chad
  // 44 (Namibia, partner)
  46, // South Africa (Cape Town)
  // 48 ()
  66, // Australia (Perth)
  // 68 (Murchison)
  // 69 (Nice)
  83, // Japan
  // 84 (TODO: drill)
  85, // Korea
  // 86 (Yellow Sea)
  // 87 (Shanghai)
  88, // China (Hangzhou)
  // 92 (Myanmar)
  // 93 (Bay of Bengal)
  97, // India
  // 100 (Pakistan)
  // 101 (Afghanistan)
  // 105 (Kazakhstan)
  // 111 (Belarus)
  // 112 (Lithuania)
  // 114 (Baltic Sea)
  117, // Sweden
  // 118 (Norway)
  // 120 (North Sea)
  121, // uk
};
#define NUM_COUNTRIES (sizeof(countries)/sizeof(countries[0]))

#define NUM_PACKETS 3
int packet_pos[NUM_PACKETS];
int packet_vel[NUM_PACKETS];
int packet_mag[NUM_PACKETS]; // 0 - respawn me, 1+ - data remaining

DEFINE_GRADIENT_PALETTE(aussrc) {
    0, 0x07, 0x00, 0x68, // navy #070068
  127, 0xE7, 0x00, 0x68, // magenta #E70068
  255, 0xEF, 0x7C, 0x38, // orange #EF7C38
};
CRGBPalette256 aussrc_palette = aussrc;
#define PALETTE(a) ColorFromPalette(aussrc_palette, a)

void loop() {
  // put your main code here, to run repeatedly:
  for (int i = 0; i < NUM_LEDS; i++) {
    bool ledIsCountry = false;
    for (int c = 0; c < NUM_COUNTRIES; c++) {
      if (i == countries[c]) {
        ledIsCountry = true;
        break;
      }
    }
    bool ledIsLink = (i == step);

    if(ledIsCountry) {
      if (i > step - (COUNTRY_HOLD+1) && i < step) {
        leds[i] = PALETTE(127);
      } else {
        leds[i] = PALETTE(255);
      }
    } else if (i > step - (TRAIL_SIZE+1) && i < step) {
      leds[i] = PALETTE(map((step - i) * (step - i), 0, TRAIL_SIZE*TRAIL_SIZE, 0, 127) );
    } else {
      leds[i] = PALETTE(0);
    }
  //  leds[i].setHue((hue + 0 * i) % 255);
  };
  step = (step + 1) % (2 * NUM_LEDS);
  delay(5);
  FastLED.show();
}