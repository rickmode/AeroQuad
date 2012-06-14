/*
  Copyright (c) 2011 ala42.  All rights reserved.

  STM32 receiver class by ala42 using time input capture
  for use with AeroQuad software and Maple library
  V 1.0 Oct 15 2011
  V 1.1 Jan 22 2012	class free version for AeroQuad 3.0 compatibility

  Define the pin numbers used for the receiver in receiverPin[]

  Timer and timer channels are accessed using the Maple PIN_MAP array.
  Make sure libmaple and this receiver class are compiled using the
  same structure alignment mode. When in doubt, change the stm32_pin_info
  declaration in wirish_types.h to align the size to a multiple of 4 byte
  by adding a filler byte at the end of the structure declaration.
*/

#ifndef _AEROQUAD_RECEIVER_STM32PPM_H_
#define _AEROQUAD_RECEIVER_STM32PPM_H_

#if defined(AeroQuadSTM32)

#include "Receiver.h"
#include "wirish.h"

//#define STM32_TIMER_DEBUG // enable debug messages

///////////////////////////////////////////////////////////////////////////////
// configuration part starts here
// definition of pins used for PWM receiver input

#ifdef BOARD_aeroquad32
static byte receiverPin = Port2Pin('D', 12);
#endif

#ifdef BOARD_aeroquad32mini
static byte receiverPin = 2; // PB7
#endif

#ifdef BOARD_freeflight
static byte receiverPin = Port2Pin('A',  0);
#endif

#ifdef BOARD_discovery_f4
static byte receiverPin = Port2Pin('E',  9);
#endif


#define SERIAL_SUM_PPM_1         1,2,3,0,4,5,6,7 // PITCH,YAW,THR,ROLL... For Graupner/Spektrum
#define SERIAL_SUM_PPM_2         0,1,3,2,4,5,6,7 // ROLL,PITCH,THR,YAW... For Robe/Hitec/Futaba
#define SERIAL_SUM_PPM_3         1,0,3,2,4,5,6,7 // PITCH,ROLL,THR,YAW... For some Hitec/Sanwa/Others

#if defined (SKETCH_SERIAL_SUM_PPM)
  #define SERIAL_SUM_PPM SKETCH_SERIAL_SUM_PPM
#else
  #define SERIAL_SUM_PPM SERIAL_SUM_PPM_1
#endif

static byte ReceiverChannelMap[] = {SERIAL_SUM_PPM};



///////////////////////////////////////////////////////////////////////////////
// implementation part starts here.
// forward declaration, array is defined at the end of this file
extern voidFuncPtr FrqChange();

typedef struct {
  timer_dev   *TimerDev;
  timer_gen_reg_map *TimerRegs;
  __io uint32	*Timer_ccr;
  int			Low;
  int			High;
  uint16		HighTime;
  uint16		RiseTime;
  uint16		LastChange;
  int			Channel;
  int			TimerChannel;
  int			PolarityMask;
  int			Valid;
  int			Debug;
} tFrqData;

volatile tFrqData FrqData;

void FrqInit(int aDefault, timer_dev *aTimer, int aTimerChannel)
{
  aTimerChannel--;  // transform timer channel numbering from 1-4 to 0-3

  FrqData.Channel      = 0;
  FrqData.Valid        = false;
  
  FrqData.TimerDev     = aTimer;
  timer_gen_reg_map *timer = aTimer->regs.gen;
  FrqData.TimerRegs    = timer;
  
  FrqData.Timer_ccr    = &timer->CCR1 + aTimerChannel;
  FrqData.Debug        = false;
  FrqData.HighTime     = aDefault;
  FrqData.TimerChannel = aTimerChannel;
  
  int TimerEnable = (1 << (4*aTimerChannel));
  FrqData.PolarityMask = TimerEnable << 1;
  
  uint32 clock_speed = rcc_dev_timer_clk_speed(FrqData.TimerDev->clk_id);
  timer->PSC	= (clock_speed/1000000)-1;
  timer->ARR	= 0xffff;
  timer->CR1	= 0;
  timer->DIER &= ~(1);
  
  timer->CCER &= ~TimerEnable; // Disable timer
  timer->CCER &= ~(FrqData.PolarityMask);
  
#ifdef STM32_TIMER_DEBUG
  Serial.print("  clk ");
  Serial.print(clock_speed/1000000, 10);
  Serial.print("MHz ");
  
  Serial.print(" CCMR0 ");
  Serial.print(timer->CCMR1, 16);
#endif
  
  volatile uint32 *mr;
  if(aTimerChannel < 2) {
    mr = &(timer->CCMR1);
  } else {
    mr = &(timer->CCMR2);
  }
  *mr &= ~(0xFF << (8*(aTimerChannel&1)));	// prescaler 1
  *mr |= 0x61 << (8*(aTimerChannel&1));		// 0x61 -> 6=filter, 1=inputs 1,2,3,4
  
  timer->CCER |= TimerEnable; // Enable
  timer->CR1 = 1;
  
#ifdef STM32_TIMER_DEBUG
  Serial.print(" CCER ");
  Serial.print(timer->CCER, 16);
  Serial.print(" CCMR1 ");
  Serial.print(timer->CCMR1, 16);
  Serial.println();
#endif
}


void InitFrqMeasurement()
{
#ifdef STM32_TIMER_DEBUG
  Serial.println("InitFrqMeasurement");
#endif
  int pin = receiverPin;
  timer_dev *timer_num = PIN_MAP[pin].timer_device;
  if(timer_num == NULL) {
#ifdef STM32_TIMER_DEBUG
    Serial.print("InitFrqMeasurement: invalid PWM input ");
    Serial.print(pin);
    Serial.println();
#endif
  } else {
#ifdef STM32F2
    gpio_set_mode(PIN_MAP[pin].gpio_device, PIN_MAP[pin].gpio_bit, GPIO_AF_INPUT_PD);
#else
    pinMode(pin, INPUT_PULLDOWN);
#endif
    
#ifdef STM32_TIMER_DEBUG
    timer_gen_reg_map *timer = PIN_MAP[pin].timer_device->regs.gen;
    Serial.print("pin ");
    Serial.print(pin);
    Serial.print(" timerbase ");
    Serial.print((int32)timer,16);
    Serial.println();
#endif
    FrqInit(1500, timer_num, PIN_MAP[pin].timer_channel);
    
    timer_attach_interrupt(timer_num, PIN_MAP[pin].timer_channel, FrqChange);
  }
  
#ifdef STM32_TIMER_DEBUG
  Serial.println("InitFrqMeasurement done");
#endif
}


void PWMInvertPolarity()
{
  FrqData.TimerRegs->CCER ^= FrqData.PolarityMask; // invert polarity
}


void FrqChange()
{
  timer_gen_reg_map *timer = FrqData.TimerRegs;
  uint16_t c = *(FrqData.Timer_ccr);
  bool rising = (timer->CCER & FrqData.PolarityMask) == 0;
  
  if(FrqData.Valid) {
    if(FrqData.Debug && FrqData.Channel == 6) {
      Serial.print(FrqData.Channel);
      Serial.print(rising ? " r " : " f ");
      Serial.print((int)c);
      Serial.print(" ");
      uint16_t delta = c - FrqData.LastChange;
      Serial.print((int)delta);
      Serial.println();
      FrqData.LastChange = c;
    }
    
    if(rising) {
      // rising edge, store start time
      FrqData.RiseTime = c;
      //Serial.print("  r ");
      //Serial.println(FrqData.RiseTime, 10);
    } else {
      uint16_t highTime = c - FrqData.RiseTime;
      if(highTime > 900 && highTime < 2100) {
	FrqData.HighTime = highTime;
      } else {
	FrqData.Valid = false;
      }
      //Serial.print("  f ");
      //Serial.println(FrqData.HighTime, 10);
    }
  } else {
    if(rising) {
      // rising edge, store start time
      FrqData.RiseTime = c;
      FrqData.Valid = true;
    }
  }
  
  PWMInvertPolarity();
}


///////////////////////////////////////////////////////////////////////////////
// interface part starts here

void initializeReceiver(int nbChannel = 8) {

  initializeReceiverParam(nbChannel);
  InitFrqMeasurement();
}


int getRawChannelValue(const byte channel) {
	int chan = ReceiverChannelMap[channel];
	return 1500;
}


void setChannelValue(byte channel,int value) {
}

#endif

#endif
