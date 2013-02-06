
#ifndef _TIMER_5_H_
#define _TIMER_5_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
  Use timer 5 as a simple count up timer for the desired maximum duration 
  with the highest resolution possible for this timer.
*/

// intialize timer with maximum needed duration
void timer5_init(long microseconds);

// start the timer
void timer5_start();

// stop the timer
void timer5_stop();

// reset the timer (resets timer's count)
void timer5_reset();

// get time in microseconds
uint32_t timer5_micros();

// get time in microseconds from within an ISR
// (where interrupts are disabled)
// this variation does not disable interrupts while
// reading the timer count
uint32_t timer5_isr_micros();

#ifdef __cplusplus
}
#endif

#endif // _TIMER_5_H_