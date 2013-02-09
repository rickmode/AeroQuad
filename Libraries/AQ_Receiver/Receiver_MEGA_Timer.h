#ifndef _AEROQUAD_RECEIVER_MEGA_TIMER_H_
#define _AEROQUAD_RECEIVER_MEGA_TIMER_H_

/*
  Reads PWM receiver using PinChangeInt and timer 5.
  Timer 5 is only available on Arduino Megas.
  Timer 5 is the same timer used by the Receiver_HWPPM code
  (which cannot be used at the same time as this of course).
*/

#if !defined (__AVR_ATmega1280__) && !defined(__AVR_ATmega2560__)
  #error Receiver_MEGA_Timer can only be used on ATmega1280 or AVR_ATmega2560
#endif

#include "Receiver.h"
#include <PinChangeInt.h>

const uint8_t clockMode = 0; // clock mode 0
#define PRESCALING 8
const uint8_t clockSelectBits = _BV(CS51); // 8x prescaling

inline void timer5_init() {
  // initialize to mode 0: count from 0 to 0xFFFF the restart at 0
  //   overflow flag TOV5 set when clock restarts at 0
  TCCR5A = 0;
  TCCR5B = clockMode;
}

inline void timer5_start() {
  // start by setting desired clock select bits
  TCCR5B |= clockSelectBits;
}

inline void timer5_stop() {
  // stop by clearing all clock select bits
  TCCR5B &= ~(_BV(CS50) | _BV(CS51) | _BV(CS52));
}

inline void timer5_reset() {
  TCNT5 = 0;
}

#define MICROS_PER_SECOND (1000UL * 1000UL)
#define MICROS_PER_TIC (F_CPU / PRESCALING / MICROS_PER_SECOND)

inline uint16_t timer5_micros() {
  return TCNT5 / MICROS_PER_TIC;
}

inline void timer5_enable_overflow_interrupt() {
  TIMSK5 = _BV(TOIE5);
}

// receiver pins are A8 (62) to A15 (70)

#ifdef OLD_RECEIVER_PIN_ORDER
  // arduino pins 67, 65, 64, 66, 63, 62
  static byte receiverPin[6] = {A8+5, A8+3, A8+2, A8+4, A8+1, A8+0}; // pins for XAXIS, YAXIS, ZAXIS, THROTTLE, MODE, AUX
#else
  //arduino pins 63, 64, 65, 62, 66, 67
  static byte receiverPin[8] = {A8+1, A8+2, A8+3, A8+0, A8+4, A8+5, A8+6, A8+7}; // pins for XAXIS, YAXIS, ZAXIS, THROTTLE, MODE, AUX
#endif


volatile uint32_t channelPulse[MAX_NB_CHANNEL];
volatile uint8_t currentChannel = 0;

void channelRise();
void channelFall();

void channelRise() {
  timer5_stop();
  timer5_reset();
  timer5_start();

  PCintPort::detachInterrupt(receiverPin[currentChannel]);
  PCintPort::attachInterrupt(receiverPin[currentChannel], channelFall, FALLING);
}

void channelFall() {
  channelPulse[currentChannel] = timer5_micros();
  timer5_stop();
  
  PCintPort::detachInterrupt(receiverPin[currentChannel]);
  currentChannel = (++currentChannel) % lastReceiverChannel;
  PCintPort::attachInterrupt(receiverPin[currentChannel], channelRise, RISING);

  // start timer again to catch dead pin via timer overflow interrupt
  timer5_reset();
  timer5_start();
}

// timer overflow interrupt
// give up on current pin and move to next pin
ISR(TIMER5_OVF_vect) {
  channelPulse[currentChannel] = 0;
  timer5_stop();
  
  PCintPort::detachInterrupt(receiverPin[currentChannel]);
  currentChannel = (++currentChannel) % LASTCHANNEL;
  PCintPort::attachInterrupt(receiverPin[currentChannel], channelRise, RISING);
  
  timer5_reset();
  timer5_start();
}

void initializeReceiver(int nbChannel) {
  initializeReceiverParam(nbChannel);
  
  for (uint8_t i = 0; i < MAX_NB_CHANNEL; ++i)
    channelPulse[i] = 0;

  timer5_init();
  timer5_reset();
  timer5_enable_overflow_interrupt();
  timer5_start();

  for (uint8_t i = 0; i < nbChannel; ++i)
    pinMode(receiverPin[i], INPUT);

  PCintPort::attachInterrupt(receiverPin[currentChannel], channelRise, RISING); // attach a PinChange Interrupt to our first pin
}

int getRawChannelValue(byte channel) {
  uint8_t sreg = SREG; // save interrupt status registers
  cli(); // stop interrupts
  uint32_t t = channelPulse[channel];
  SREG = sreg; // restore interrupts to prior status
  return t;
}

#endif // _AEROQUAD_RECEIVER_MEGA_TIMER_H_
