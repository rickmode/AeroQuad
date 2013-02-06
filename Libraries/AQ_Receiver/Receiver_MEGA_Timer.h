#ifndef _AEROQUAD_RECEIVER_MEGA_TIMER_H_
#define _AEROQUAD_RECEIVER_MEGA_TIMER_H_

/*
  Reads PWM receiver using PinChangeInt and timer5.
  Timer 5 is only available on Arduino Megas.
  Timer 5 is the same timer used by the Receiver_HWPPM code
  (which cannot be used at the same time as this of course).
*/

#if !defined (__AVR_ATmega1280__) && !defined(__AVR_ATmega2560__)
  #error Receiver_MEGA_Timer can only be used on ATmega1280 or AVR_ATmega2560
#endif

#include "Receiver.h"
#include <PinChangeInt.h>
#include <timer5.h>


// receiver pins are A8 (62) to A15 (70)

#ifdef OLD_RECEIVER_PIN_ORDER
  // arduino pins 67, 65, 64, 66, 63, 62
  static byte receiverPin[6] = {A8+5, A8+3, A8+2, A8+4, A8+1, A8+0}; // pins for XAXIS, YAXIS, ZAXIS, THROTTLE, MODE, AUX
#else
  //arduino pins 63, 64, 65, 62, 66, 67
  static byte receiverPin[8] = {A8+1, A8+2, A8+3, A8+0, A8+4, A8+5, A8+6, A8+7}; // pins for XAXIS, YAXIS, ZAXIS, THROTTLE, MODE, AUX
#endif


volatile uint32_t channelPulse[MAX_NB_CHANNEL];
volatile uint8_t trackedPin = 0;

void channelRise();
void channelFall();


void channelRise()
{
  timer5_reset();
  timer5_start();

  PCintPort::detachInterrupt(receiverPin[trackedPin]);
  PCintPort::attachInterrupt(receiverPin[trackedPin], channelFall, FALLING);
}

void channelFall()
{
  channelPulse[trackedPin] = timer5_isr_micros();
  timer5_stop();
  
  PCintPort::detachInterrupt(receiverPin[trackedPin]);
  trackedPin = (++trackedPin) % lastReceiverChannel;
  PCintPort::attachInterrupt(receiverPin[trackedPin], channelRise, RISING);
}

void initializeReceiver(int nbChannel)
{
  initializeReceiverParam(nbChannel);
  
  for (uint8_t i; i < MAX_NB_CHANNEL; ++i)
    channelPulse[i] = 0;

  timer5_init(2200);
  timer5_stop();
  timer5_reset();

  for (uint8_t i=0; i < nbChannel; ++i)
    pinMode(receiverPin[i], INPUT);

  PCintPort::attachInterrupt(receiverPin[trackedPin], channelRise, RISING); // attach a PinChange Interrupt to our first pin
}

int getRawChannelValue(byte channel)
{
  uint8_t sreg;
  uint32_t t;

  sreg = SREG; // save interrupt status registers
  cli(); // stop interrupts
  t = channelPulse[channel];
  SREG = sreg; // restore interrupts to prior status
  
  return t;
}

void setChannelValue(byte channel,int value)
{
}  

#endif // _AEROQUAD_RECEIVER_MEGA_TIMER_H_
