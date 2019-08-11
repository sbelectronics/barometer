/**
 * Temp/Humid/Barom Display and/or transmitter
 * http://www.smbkaer.com/
 */

#include <SPI.h>
#include "fepd2in13.h"
#include "epdpaint.h"
#include <Adafruit_Sensor.h>

#ifdef BME680
#include "Adafruit_BME680.h"
#else
#include "Adafruit_BME280.h"
#endif

#include "LowPower.h"
#include <util/crc16.h>
#include <RCSwitch.h>

#define RADIOEN_PIN 4
#define RADIOOUT_PIN 1


/**
  * Due to RAM not enough in Arduino UNO, a frame buffer is not allowed.
  * In this case, a smaller image buffer is allocated and you have to
  * update a partial display several times.
  * 1 byte = 8 pixels, therefore you have to set 8*N pixels at a time.
  */
Epd epd;

#ifdef BME680
Adafruit_BME680 bme; // I2C
#else
Adafruit_BME280 bme;
#endif

#define WINDOW_W 48
#define WINDOW_H 144

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

unsigned char image[WINDOW_W * WINDOW_H / 8];
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
  
    //Serial.begin(1200);
  
    // put your setup code here, to run once:
    //Serial.begin(9600);
/*    if (epd.Init() != 0) {
        //Serial.print("e-Paper init failed\n");
        return;
    }*/

    if (!bme.begin()) {
        //Serial.println(F("Could not find a valid BME680 sensor, check wiring!"));
        return;
    }

#ifdef BME680
#else
    bme.setSampling(Adafruit_BME280::MODE_FORCED,
                    Adafruit_BME280::SAMPLING_X2,   // temperature
                    Adafruit_BME280::SAMPLING_X2, // pressure
                    Adafruit_BME280::SAMPLING_X2,   // humidity
                    Adafruit_BME280::FILTER_OFF );
#endif

    lastHumidity = 0;
    lastTemperature = 0;
    lastPressure = 0;
}

unsigned char counter = 0;
uint16_t crc;

void SWrite(unsigned char x)
{
    mySwitch.send(x, 24);   
    //Serial.write(x);
    _crc16_update(crc, x);
}

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
    Paint paint(image, WINDOW_W, WINDOW_H);
    char str[64];
    int temp, humid, pres;

    #ifdef BME680
    bme.setTemperatureOversampling(BME680_OS_8X);
    bme.setHumidityOversampling(BME680_OS_2X);
    bme.setPressureOversampling(BME680_OS_4X);
    //bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
    bme.setGasHeater(0, 0); // disable heater

    bme.beginReading();

    if (!bme.endReading()) {
       //Serial.println(F("Failed to complete reading :("));
       return;
    }

    // convert to integers with the desired precisions 
    humid = int(bme.humidity); 
    temp = int(bme.temperature*10); // tenths
    pres = int(bme.pressure/100);
    
    #else

    bme.takeForcedMeasurement();
    humid = int(bme.readHumidity());
    temp = int(bme.readTemperature()*10); // tenths
    pres = int(bme.readPressure()/100); 
    
    #endif

    avgHumidity.add(humid);
    avgTemperature.add(temp);
    avgPressure.add(pres);

    humid = avgHumidity.get();
    temp = avgTemperature.get();
    pres = avgPressure.get();

     if ((lastHumidity != humid) ||
        (lastTemperature != temp) ||
        (lastPressure != pres)) {

#ifdef EPAPER_DISPLAY
        paint.Clear(1);
        paint.SetRotate(ROTATE_90);

        //sprintf(str, "Humid: %d%%", int(humid));
        sprintf(str, "%d H: %d%%", counter, humid);
        paint.DrawStringAt(0, 0, str, &Font16, 0);

        sprintf(str, "Temp: %d.%d *C", int(temp/10), temp%10);
        paint.DrawStringAt(0, 16, str, &Font16, 0);

        sprintf(str, "Pres: %d hPA", pres);
        paint.DrawStringAt(0, 32, str, &Font16, 0);

        epd.Init();
        epd.SetFrameRAM(image, WINDOW_W/8, WINDOW_H);
        epd.Sleep();
#endif

        lastHumidity = humid;
        lastTemperature = temp;
        lastPressure = pres;

        counter++;
    }

    digitalWrite(RADIOEN_PIN, HIGH);
    delay(1);
    /*crc = 0;
    SWrite(0x66);
    SWrite(0x84);
    SWrite(humid >> 8);
    SWrite(humid & 0xFF);
    SWrite(temp >> 8);
    SWrite(temp & 0xFF);
    SWrite(pres >> 8);
    SWrite(pres & 0xFF);
    SWrite(crc >> 8);
    SWrite(crc & 0xFF);  // wrong!
    delay(1);*/

    DWrite(0, 6684);
    DWrite(1, humid);
    DWrite(2, temp);
    DWrite(3, pres);
    
    digitalWrite(RADIOEN_PIN,LOW);

    LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_OFF);
}
