/**
 *  Temp/Humid/Barom sender 
 *  http://www.smbaker/com/
 */

#include "tinyBME280.h"

#include <util/crc16.h>
#include <RCSwitch.h>
#include "sleep.h"

#define RADIOEN_PIN PB4
#define RADIOOUT_PIN PB3

// DO NOT EDIT THESE -- they will be overridden by patchid.py
#define MAX_SAME 20
#define SLEEP_TIME SLEEP_8S
#define SLEEP_REPEAT 4
#define ENABLE_RADIOEN 0
#define STEADY 0
// I HOPE YOU DIDN"T EDIT THOSE

#define DELTA_CHECK(x,y,thresh) (abs((x)-(y))>=(thresh))

#define adc_disable() (ADCSRA &= ~(1<<ADEN)) // disable ADC (before power-off)

struct ConfigBlock {
  uint32_t header;
  uint8_t id;
  uint8_t sleep_time;
  uint8_t sleep_repeat;
  uint8_t max_same;
  uint8_t enable_radioen;
  uint8_t steady;
};

class Averager {
 public:
  int16_t values[10];
  uint8_t count, index;
  
  void init() {
      count = 0;
      index = 0;
  }

  void add(int16_t value) {
      values[index % 10] = value;
      index ++;
      if (count < 10) {
        count++;
      }
  }

  int16_t get() {
    int32_t accum;
    accum = 0;
    for (int i=0; i<count; i++) {
      accum += values[i];
    }
    return accum/count;
  }
};

int lastHumidity, lastTemperature, lastPressure;
uint8_t sameCounter = 0;
uint8_t seq = 0;
uint32_t idq = 0xFEEDFA1F;

ConfigBlock config = {0xFEEDFACE, 0x1F, SLEEP_TIME, SLEEP_REPEAT, MAX_SAME, ENABLE_RADIOEN};

Averager avgHumidity, avgTemperature, avgPressure;


RCSwitch mySwitch = RCSwitch();

void setup()
{
    if (config.steady) {
      digitalWrite(RADIOEN_PIN, HIGH);
      digitalWrite(RADIOOUT_PIN, HIGH);
      while (1) ;
    }
  
    mySwitch.enableTransmit(RADIOOUT_PIN);
    mySwitch.setRepeatTransmit(3); // needs to be at least two or the receiver won't detect

    // Send a signal that we just booted
    if (config.enable_radioen) {
        digitalWrite(RADIOEN_PIN, HIGH);
        delay(1);
    }
    DWrite3(0,0x66, 0x84, 0x66);
    digitalWrite(RADIOEN_PIN,LOW);

    Wire.begin();
    BME280setup(MODE_SLEEP);

    // This doesn't seem to do anything.
    // Maybe because I don't use the ADC...
    adc_disable();

    lastHumidity = 0;
    lastTemperature = 0;
    lastPressure = 0;
}

#define W_ID 5
#define W_SEQ 4
#define W_T 10
#define W_H 10
#define W_P 12
#define W_CRC 8

#define O_ID   0
#define O_SEQ  O_ID + W_ID
#define O_T    O_SEQ + W_SEQ
#define O_H    O_T + W_T
#define O_P    O_H + W_H
#define O_CRC  O_P + W_P
#define O_TERM O_CRC + W_CRC

// Note: Requires modifications
//          arduino tx library - modify to support arbitrary length strings
//          pi rx library - change MAX_CHANGES from 67 to 129
void DWrite3(uint8_t seq, unsigned int t, unsigned int h, unsigned int p)
{
  char s[O_TERM + 1];
  unsigned int v;
  uint16_t crc;

 // id - 5 bits
  v = config.id & 0x1F;
  for (int i=0; i<W_ID; i++) {
    if (v&0x10) {
      s[i]='1';
    } else {
      s[i]='0';
    }
    v=v<<1;
  }

  // seq - 4 bits
  v=seq;
  for (int i=0; i<W_SEQ; i++) {
    if (v&0x08) {
      s[O_SEQ+i]='1';
    } else {
      s[O_SEQ+i]='0';
    }
    v=v<<1;
  }

  // temp - 10 bits
  v=t;
  for (int i=0; i<W_T; i++) {
    if (v&0x0200) {
      s[O_T+i]='1';
    } else {
      s[O_T+i]='0';
    }
    v=v<<1;
  }

  // humid - 10 bits
  v=h;
  for (int i=0; i<W_H; i++) {
    if (v&0x0200) {
      s[O_H+i]='1';
    } else {
      s[O_H+i]='0';
    }
    v=v<<1;
  }

  // barom - 12 bits
  v=p;
  for (int i=0; i<W_P; i++) {
    if (v&0x0800) {
      s[O_P+i]='1';
    } else {
      s[O_P+i]='0';
    }
    v=v<<1;
  }

  crc = 0;
  crc = _crc16_update(crc, config.id & 0x1F);
  crc = _crc16_update(crc, seq & 0x0F);
  crc = _crc16_update(crc, t>>8);
  crc = _crc16_update(crc, t&0xFF);
  crc = _crc16_update(crc, h>>8);
  crc = _crc16_update(crc, h&0xFF);
  crc = _crc16_update(crc, p>>8);
  crc = _crc16_update(crc, p&0xFF);

  // crc - 8 bits
  v=crc;
  for (int i=0; i<W_CRC; i++) {
    if (v&0x80) {
      s[O_CRC+i]='1';
    } else {
      s[O_CRC+i]='0';
    }
    v=v<<1;
  }

  s[O_TERM] = 0;
                 
  mySwitch.sendLong(s);
}

void loop()
{
    char str[64];
    int temp, humid, pres;
    uint8_t r;
    uint8_t changed;

    BME280ForceMeasurement();
    humid = BME280humidity();
    temp = BME280temperature();
    pres = BME280pressure()/10; // tenth of an hpa, otherwise we'll overflow

    avgHumidity.add(humid);
    avgTemperature.add(temp);
    avgPressure.add(pres);

    humid = avgHumidity.get() / 10; // tenth of a percent humid
    temp = avgTemperature.get() / 10; // tenth of a degree
    pres = avgPressure.get() / 10; // hpa

     if ((DELTA_CHECK(lastHumidity,humid,10)) ||
         (DELTA_CHECK(lastTemperature,temp,5)) ||
         (DELTA_CHECK(lastPressure,pres,2))) {

        lastHumidity = humid;
        lastTemperature = temp;
        lastPressure = pres;

        changed = 1;
    } else {
        changed = 0;
        sameCounter++;
    }

    if (changed || (sameCounter>config.max_same)) {
        sameCounter = 0;
        seq++;

        if (config.enable_radioen) {
            digitalWrite(RADIOEN_PIN, HIGH);
            delay(1);
        }

        DWrite3(seq, temp, humid, pres);

        digitalWrite(RADIOEN_PIN,LOW);
    }

    for (int i=0; i<config.sleep_repeat; i++) {
        powerDown(config.sleep_time);
    }

    // Add a random component to the sleep.
    // this will pick between 16ms, 32ms, 64ms, 128ms, 250ms, and nothing.
    r=random(6);
    if (r<5) {
      powerDown(r);
    }
}
