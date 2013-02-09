#include <avr/io.h>
#include <avr/interrupt.h>

#include "timer5.h"

#define RESOLUTION 65536    // This timer is 16 bit

uint8_t clockSelectBits = 0;
uint8_t scale = 0;

void timer5_init(long microseconds)
{
  TCCR5A = 0; // clear control register A 

  /*
  TCCR5B = _BV(WGM53); // set mode as phase and frequency correct pwm, stop the timer
  // the counter runs backwards after TOP, interrupt is at BOTTOM so divide microseconds by 2 
  long cycles = (F_CPU * microseconds) / 2000000;
  */

  TCCR5B = _BV(WGM53) | _BV(WGM52) | _BV(WGM51); // mode 14 - Fast PWM, Top is ICR5, TOV5 set on ICR5
  long cycles = (F_CPU * microseconds) / 1000000;
  
  if (cycles < RESOLUTION)
    clockSelectBits = _BV(CS50);              // no prescale, full xtal
  else if((cycles >>= 3) < RESOLUTION)
    clockSelectBits = _BV(CS51);              // prescale by /8
  else if((cycles >>= 3) < RESOLUTION)
    clockSelectBits = _BV(CS51) | _BV(CS50);  // prescale by /64
  else if((cycles >>= 2) < RESOLUTION)
    clockSelectBits = _BV(CS52);              // prescale by /256
  else if((cycles >>= 2) < RESOLUTION)
    clockSelectBits = _BV(CS52) | _BV(CS50);  // prescale by /1024
  else
  {
    cycles = RESOLUTION - 1;
    clockSelectBits = _BV(CS52) | _BV(CS50);  // request was out of bounds, set as maximum
  }
  
  ICR5 = cycles;  // ICR is TOP
  TCCR5B &= ~(_BV(CS50) | _BV(CS51) | _BV(CS52));
  TCCR5B |= clockSelectBits; // reset clock select register

  switch (clockSelectBits)
  {
    case 1: // no prescalse
      scale = 0;
      break;
    case 2: // x8 prescale
      scale = 3;
      break;
    case 3: // x64
      scale = 6;
      break;
    case 4: // x256
      scale = 8;
      break;
    case 5: // x1024
      scale = 10;
      break;
  }
}

void timer5_start()
{
  TCCR5B |= clockSelectBits;
}

void timer5_stop()
{
  TCCR5B &= ~(_BV(CS50) | _BV(CS51) | _BV(CS52)); // clears all clock selects bits
}

void timer5_reset()
{
  TCNT5 = 0;
}

uint32_t timer5_micros()
{
  uint8_t sreg;
  uint32_t tmp;
  uint32_t count;

  sreg = SREG; // save interrupt flag
  cli(); // stop interrupts

  tmp = TCNT5;
  do
  {
    count = TCNT5;
  } while (count == tmp);

  tmp = ( (count > tmp) ? tmp : (uint32_t)(ICR5-count)+(uint32_t)ICR5);

  SREG = sreg; // restore interrupt flag
  return ((tmp*1000UL)/(F_CPU /1000UL))<<scale;
}

uint32_t timer5_isr_micros()
{
  uint32_t tmp;
  uint32_t count;

  tmp = TCNT5;
  do
  {
    count = TCNT5;
  } while (count == tmp);

  tmp = ( (count > tmp) ? tmp : (uint32_t)(ICR5-count)+(uint32_t)ICR5);
  return ((tmp*1000UL)/(F_CPU /1000UL))<<scale;
}
