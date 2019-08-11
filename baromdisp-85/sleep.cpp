#include <stdlib.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include "sleep.h"

// from https://www.re-innovation.co.uk/docs/sleep-modes-on-attiny85/

// Routines to set and claer bits (used in the sleep code)
#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

// 0=16ms, 1=32ms,2=64ms,3=128ms,4=250ms,5=500ms
// 6=1 sec,7=2 sec, 8=4 sec, 9= 8sec
int setupWatchdog1(int ii) {
  uint8_t bb;
  int ww;
  if (ii > 9 ) ii=9;
  bb=ii & 7;
  if (ii > 7) bb|= (1<<5);
  bb|= (1<<WDCE);
  ww=bb;
 
  MCUSR &= ~(1<<WDRF);
  // start timed sequence
  WDTCR |= (1<<WDCE) | (1<<WDE);
  // set new watchdog timeout value
  WDTCR = bb;
  WDTCR |= _BV(WDIE);
}

void disableWatchdog() {
  __asm__ __volatile__ (" wdr\n");
  MCUSR = 0x00;
  WDTCR |= (1<<WDCE) | (1<<WDE);
  WDTCR = 0x00;
}

// Enable the watchdog for the specified duration, then power down. When
// the watchdog fires, it will disable itself.
void powerDown(int ii) {
  setupWatchdog1(ii);
  
  cbi(ADCSRA,ADEN);                    // switch Analog to Digitalconverter OFF
 
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // sleep mode is set here
  sleep_enable();
 
  sleep_mode();                        // System actually sleeps here
 
  sleep_disable();                     // System continues execution here when watchdog timed out 
  
  sbi(ADCSRA,ADEN);                    // switch Analog to Digitalconverter ON
  
}

ISR (WDT_vect)
{
	// WDIE & WDIF is cleared in hardware upon entering this ISR
	disableWatchdog();
}
