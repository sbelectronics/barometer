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

#define adc_disable() (ADCSRA &= ~(1<<ADEN)) // disable ADC (before power-off)

class Averager {
 public:
  int values[10];
  unsigned char count, index;
  
  void init() {
      count = 0;
      index = 0;
  }

  void add(int value) {
      values[index % 10] = value;
      index ++;
      if (count < 10) {
        count++;
      }
  }

  int get() {
    int accum;
    accum = 0;
    for (int i=0; i<count; i++) {
      accum += values[i];
    }
    return accum/count;
  }
};

int lastHumidity, lastTemperature, lastPressure;
unsigned char id = 1;

Averager avgHumidity, avgTemperature, avgPressure;


RCSwitch mySwitch = RCSwitch();

void setup()
{
    mySwitch.enableTransmit(RADIOOUT_PIN);

    digitalWrite(RADIOEN_PIN, HIGH);
    delay(1);
    DWrite(0,0x8466);
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

unsigned char counter = 0;
uint16_t crc;

void DWrite(unsigned int var, unsigned int x)
{
  char s[33];
  unsigned int v;

 // id - 4 bits
  v = id;
  for (int i=0; i<4; i++) {
    if (v&0x08) {
      s[i]='1';
    } else {
      s[i]='0';
    }
    v=v<<1;
  }

  // variable - 4 bits
  v=var;
  for (int i=0; i<4; i++) {
    if (v&0x08) {
      s[4+i]='1';
    } else {
      s[4+i]='0';
    }
    v=v<<1;
  }

  // value - 16 bits
  v=x;
  for (int i=0; i<16; i++) {
    if (v&0x8000) {
      s[8+i]='1';
    } else {
      s[8+i]='0';
    }
    v=v<<1;
  }

  crc = 0;
  crc = _crc16_update(crc, 0);
  crc = _crc16_update(crc, var);
  crc = _crc16_update(crc, x>>8);
  crc = _crc16_update(crc, x&0xFF);

  // crc - 8 bits
  v=crc;
  for (int i=0; i<8; i++) {
    if (v&0x80) {
      s[24+i]='1';
    } else {
      s[24+i]='0';
    }
    v=v<<1;
  }

  s[32] = 0;
                 
  mySwitch.send(s);
}

void loop()
{
    char str[64];
    int temp, humid, pres;

    BME280ForceMeasurement();
    humid = BME280humidity()/100;
    temp = BME280temperature()/10;
    pres = BME280pressure()/100;

    avgHumidity.add(humid);
    avgTemperature.add(temp);
    avgPressure.add(pres);

    humid = avgHumidity.get();
    temp = avgTemperature.get();
    pres = avgPressure.get();

     if ((lastHumidity != humid) ||
        (lastTemperature != temp) ||
        (lastPressure != pres)) {

        lastHumidity = humid;
        lastTemperature = temp;
        lastPressure = pres;

        counter++;
    }

    digitalWrite(RADIOEN_PIN, HIGH);
    delay(1);

    DWrite(0, 6684);
    DWrite(1, humid);
    DWrite(2, temp);
    DWrite(3, pres);
    
    digitalWrite(RADIOEN_PIN,LOW);

    powerDown(SLEEP_4S);
}
