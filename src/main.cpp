#include <Arduino.h>
#include <FastLED.h>

#define NUM_LEDS 122
// #define DATA_PIN 5 // for esp32
#define DATA_PIN 21 // for nodemcu

// Array of led color values
CRGB leds[NUM_LEDS];

// Array of led temperature values
uint8_t temps[NUM_LEDS];

// Packets of data flow through the map
// - position is the led index
// - velocity is the direction to move
#define NUM_PACKETS 10
int packet_pos[NUM_PACKETS];
int packet_vel[NUM_PACKETS]; // 0 - respawn me, +-1 - move left/right

void setup() {
  // put your setup code here, to run once:
  FastLED.addLeds<WS2812, DATA_PIN, RGB>(leds, NUM_LEDS);

  // initialize temperatures and colors
  for (int i = 0; i < NUM_LEDS; i++) {
    temps[i] = 0;
    leds[i] = CRGB::Black;
  }

  // initialize packet state
  for (int p = 0; p < NUM_PACKETS; p++) {
    packet_pos[p] = 0;
    packet_vel[p] = 0;
  }
}

int step = 0;
#define COUNTRY_FLASH 2 // number of steps to flash a hot country (1=off, 2+=flash frames)
#define INSTRUMENT_FLASH 2 // number of steps to flash a hot instrument (1=off, 2+=flash frames)
#define P_SPAWN 10 // probability of packet spawning at cold instrument (percent)
#define P_INSTRUMENT 50 // probability of instrument absorbing packet (percent)
#define P_COUNTRY 10 // probability of instrument absorbing packet (percent)
#define P_JUNCTION 50 // probability of junction being active (percent)

#define L_CANADA 0
// 13 (iceland)
#define L_IRELAND 19 // UK towards Canada
#define L_CELTIC 20 // UK towards Spain
// 21 (france ocean)
// 22 (spain ocean)
// 23 (portugal ocean),
#define L_SPAIN 24
// 25 (southern france)
#define L_FRANCE 26
#define L_SWITZERLAND 27
// 28, (north italy)
#define L_ITALY 29
// 30 (ionian sea)
// 31 (mediterranean sea)
// 32 Libya
// 34 Chad
// 44 (Namibia, partner)
#define L_SKA_MID 45
#define L_CAPETOWN 46
#define L_PERTH 66
#define L_SKA_LOW 68
#define L_JAPAN 83
// 84 (TODO: drill)
#define L_KOREA 85
// 86 (Yellow Sea)
// 87 (Shanghai)
#define L_CHINA 88
  // 92 (Myanmar)
  // 93 (Bay of Bengal)
#define L_INDIA 97
  // 100 (Pakistan)
  // 101 (Afghanistan)
  // 105 (Kazakhstan)
  // 111 (Belarus)
  // 112 (Lithuania)
  // 114 (Baltic Sea)
#define L_SWEDEN 117
  // 118 (Norway)
  // 120 (North Sea)
#define L_UK 121

// member countries
int countries[] = {
  L_CANADA,
  L_SPAIN,
  L_FRANCE,
  L_SWITZERLAND,
  L_ITALY,
  L_CAPETOWN,
  L_PERTH,
  L_JAPAN,
  L_KOREA,
  L_CHINA,
  L_INDIA,
  L_SWEDEN,
  L_UK,
};
#define NUM_COUNTRIES (sizeof(countries)/sizeof(countries[0]))

int instruments[] = {
  L_SKA_LOW,
  L_SKA_MID,
};
#define NUM_INSTRUMENTS (sizeof(instruments)/sizeof(instruments[0]))

struct junction {
  int from_pos;
  int from_vel;
  int to_pos;
  int to_vel;
};
// array of junctions
junction junctions[] = {
  { L_CANADA, -1, L_JAPAN, +1 }, // pacific crossing, west
  { L_CANADA, -1, L_JAPAN, -1 }, // pacific crossing, south
  { L_UK,     +1, L_IRELAND, -1}, // atlantic crossing, west
  { L_UK,     +1, L_CELTIC, +1}, // atlantic crossing, south
};
#define NUM_JUNCTIONS sizeof(junctions)/sizeof(junctions[0])


DEFINE_GRADIENT_PALETTE(aussrc) {
    0, 0x07, 0x00, 0x68, // navy #070068
  127, 0xE7, 0x00, 0x68, // magenta #E70068
  255, 0xEF, 0x7C, 0x38, // orange #EF7C38
};
CRGBPalette256 aussrc_palette = aussrc;
#define PALETTE(a) ColorFromPalette(aussrc_palette, a)

// led index is a country
bool ledIsCountry(int l) {
  for (int c = 0; c < NUM_COUNTRIES; c++) {
    if (l == countries[c]) {
      return true;
    }
  }
  return false;
}

// led index is an instrument
bool ledIsInstrument(int l) {
  for (int c = 0; c < NUM_INSTRUMENTS; c++) {
    if (l == instruments[c]) {
      return true;
    }
  }
  return false;
}

// led index has temp > 0
bool ledIsHot(int l) {
  return (temps[l] > 0);
}

// packet is falling off the end of the map
bool packetIsTerminal(int p) {
  return (
    (packet_pos[p] <= 0 && packet_vel[p] < 0)
    || (packet_pos[p] >= NUM_LEDS - 1 && packet_vel[p] > 0));
}

// packet matches with colder junction in forward direction
bool packetJunctionForward(int p, int j) {
  return (
    (packet_pos[p] == junctions[j].from_pos)
    && (packet_vel[p] == junctions[j].from_vel)
    && (temps[junctions[j].to_pos] <= temps[junctions[j].from_pos])
  );
}

// packet matches with colder junction in reverse direction
bool packetJunctionReverse(int p, int j) {
  return (
    (packet_pos[p] == junctions[j].to_pos)
    && (packet_vel[p] == -1 * junctions[j].to_vel)
    && (temps[junctions[j].from_pos] <= temps[junctions[j].to_pos])
  );
}

void killPacket(int p) {
  packet_vel[p] = 0;
}

bool junctionPass = false;

void loop() {
  // put your main code here, to run repeatedly:

  // decrement all temperatures
  for (int l = 0; l < NUM_LEDS; l++) {
    if (temps[l] > 0) {
      temps[l] = temps[l] / 2;
    }
  }

  // update packet states
  for (int p = 0; p < NUM_PACKETS; p++) {
    if (packet_vel[p] == 0) {
      // respawn packets with zero velocity at one of the cold instruments
      for (int i = 0; i < NUM_INSTRUMENTS; i++) {
        if (!ledIsHot(instruments[i]) && random(100) < P_SPAWN) {
          packet_pos[p] = instruments[i];
          packet_vel[p] = random(2) * 2 - 1;
          temps[packet_pos[p]] = 127;
          break;
        }
      }
    } else {
      // check for junction
      junctionPass = false;
      for (int j = 0; j < NUM_JUNCTIONS; j++) {
        if (packetJunctionForward(p, j) && random(100) < P_JUNCTION) {
          packet_pos[p] = junctions[j].to_pos;
          packet_vel[p] = junctions[j].to_vel;
          junctionPass = true;
          break;
        } else if (packetJunctionReverse(p, j) && random(100) < P_JUNCTION) {
          packet_pos[p] = junctions[j].from_pos;
          packet_vel[p] = -1 * junctions[j].from_vel;
          junctionPass = true;
          break;
        }
      }
      // move packets that haven't passed through a junction
      if (!junctionPass) {
        packet_pos[p] += packet_vel[p];
      }
      // increment temperature
      temps[packet_pos[p]] = 127;
      // kill packet if it's terminal (always), or if it's in a country or instrument (randomly)
      if (
        (packetIsTerminal(p))
        || (ledIsCountry(packet_pos[p]) && random(100) < P_COUNTRY)
        || (ledIsInstrument(packet_pos[p]) && random(100) < P_INSTRUMENT)
      ) {
        killPacket(p); continue;
      }
    }
  }

  for (int i = 0; i < NUM_LEDS; i++) {
    if(ledIsCountry(i)) {
      if (ledIsHot(i)) {
        if (step % COUNTRY_FLASH < COUNTRY_FLASH / 2) {
          leds[i] = PALETTE(255);
        } else {
          leds[i] = PALETTE(191);
        }
      } else {
        leds[i] = PALETTE(127);
      }
    } else if (ledIsInstrument(i)) {
      if (ledIsHot(i)) {
        if (step % INSTRUMENT_FLASH < INSTRUMENT_FLASH / 2) {
          leds[i] = PALETTE(127);
        } else {
          leds[i] = PALETTE(191);
        }
      } else {
        leds[i] = PALETTE(255);
      }
    } else {
      leds[i] = PALETTE(temps[i]);
    }
  };
  step = (step + 1) % (2 * NUM_LEDS);
  // Perth - Geraldton = 414km = 0.00138 light seconds
  delay(50);
  FastLED.show();
}