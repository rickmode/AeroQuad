#include <avr/io.h>
#include <avr/interrupt.h>

#include "timer5.h"

#define RESOLUTION 65536    // This timer is 16 bit

uint8_t clockSelectBits = 0;
uint8_t scale = 0;

void timer5_init(long microseconds)
{
  TCCR5A = 0; // clear control register A 
  TCCR5B = _BV(WGM13); // set mode as phase and frequency correct pwm, stop the timer

  // the counter runs backwards after TOP, interrupt is at BOTTOM so divide microseconds by 2 
  long cycles = (F_CPU * microseconds) / 2000000;
  
  if (cycles < RESOLUTION)
    clockSelectBits = _BV(CS10);              // no prescale, full xtal
  else if((cycles >>= 3) < RESOLUTION)
    clockSelectBits = _BV(CS11);              // prescale by /8
  else if((cycles >>= 3) < RESOLUTION)
    clockSelectBits = _BV(CS11) | _BV(CS10);  // prescale by /64
  else if((cycles >>= 2) < RESOLUTION)
    clockSelectBits = _BV(CS12);              // prescale by /256
  else if((cycles >>= 2) < RESOLUTION)
    clockSelectBits = _BV(CS12) | _BV(CS10);  // prescale by /1024
  else
    cycles = RESOLUTION - 1, clockSelectBits = _BV(CS12) | _BV(CS10);  // request was out of bounds, set as maximum
  
  ICR5 = cycles;  // ICR is TOP in p & f correct pwm mode
  TCCR5B &= ~(_BV(CS10) | _BV(CS11) | _BV(CS12));
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
  TCCR5B &= ~(_BV(CS10) | _BV(CS11) | _BV(CS12)); // clears all clock selects bits
}

void timer5_reset()
{
  TCNT5 = 0;
}

uint16_t timer5_safe_count()
{
  uint8_t sreg;
  uint16_t t;

  sreg = SREG; // save interrupt status registers
  cli(); // stop interrupts
  t = TCNT5;
  SREG = sreg; // restore interrupts to prior status
  
  return t;
}

uint32_t timer5_micros()
{
  uint32_t tmp;
  uint32_t count;

  tmp = timer5_safe_count();
  do
  {
    count = timer5_safe_count();
  } while (count == tmp);

  tmp = ( (count > tmp) ? tmp : (uint32_t)(ICR5-count)+(uint32_t)ICR5);
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
