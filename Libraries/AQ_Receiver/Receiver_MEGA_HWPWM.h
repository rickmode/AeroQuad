/*
  AeroQuad
  www.AeroQuad.com
  Copyright (c) 2012 Ted Carancho.  All rights reserved.
  An Open Source Arduino based multicopter.
 
  This program is free software: you can redistribute it and/or modify 
  it under the terms of the GNU General Public License as published by 
  the Free Software Foundation, either version 3 of the License, or 
  (at your option) any later version. 

  This program is distributed in the hope that it will be useful, 
  but WITHOUT ANY WARRANTY; without even the implied warranty of 
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
  GNU General Public License for more details. 

  You should have received a copy of the GNU General Public License 
  along with this program. If not, see <http://www.gnu.org/licenses/>. 
*/
#ifndef _AEROQUAD_RECEIVER_MEGA_HWPPM_H_
#define _AEROQUAD_RECEIVER_MEGA_HWPPM_H_

/*
  Reads PWM receiver using timer 5.
  Timer 5 is only available on Arduino Megas.
  Timer 5 is the same timer used by the Receiver_HWPPM code
  (which cannot be used at the same time as this of course).
*/

#if !defined (__AVR_ATmega1280__) && !defined(__AVR_ATmega2560__)
  #error Receiver_MEGA_HWPPM can only be used on ATmega1280 or AVR_ATmega2560
#endif


#include "Arduino.h"
#include "Receiver.h"

#define RISING_EDGE 1
#define FALLING_EDGE 0
#define MINONWIDTH 950
#define MAXONWIDTH 2075
#define MINOFFWIDTH 12000
#define MAXOFFWIDTH 24000

#include "pins_arduino.h"
#include <AQMath.h>
#include "GlobalDefined.h"

volatile static uint8_t PCintLast;

// Channel data
typedef struct {
  byte edge;
  uint16_t riseTime;
  uint16_t fallTime;
  uint16_t lastGoodWidth;
} tPinTimingData;

volatile static tPinTimingData pinData[MAX_NB_CHANNEL];

volatile static uint32_t isrCount = 0;

static uint8_t inputPort;

ISR(PCINT2_vect) {
  uint8_t bit;
  uint8_t curr;
  uint8_t mask;
  uint8_t pin;
  uint16_t currentTime;
  uint16_t time;

  ++isrCount;

  curr = *portInputRegister(inputPort);
  mask = curr ^ PCintLast;
  PCintLast = curr;

  // mask is pins that have changed. screen out non pcint pins.
  if ((mask &= PCMSK2) == 0) {
    return;
  }

  currentTime = TCNT5;

  // mask is pcint pins that have changed.
  for (uint8_t i=0; i < 8; i++) {
    bit = 0x01 << i;
    if (bit & mask) {
      pin = i;
      // for each pin changed, record time of change
      if (bit & PCintLast) {
        time = (currentTime - pinData[pin].fallTime)/2;
        pinData[pin].riseTime = currentTime;
        if ((time >= MINOFFWIDTH) && (time <= MAXOFFWIDTH))
          pinData[pin].edge = RISING_EDGE;
        else
          pinData[pin].edge = FALLING_EDGE; // invalid rising edge detected
      }
      else {
        time = (currentTime - pinData[pin].riseTime)/2;
        pinData[pin].fallTime = currentTime;
        if ((time >= MINONWIDTH) && (time <= MAXONWIDTH) && (pinData[pin].edge == RISING_EDGE)) {
          pinData[pin].lastGoodWidth = time;
          pinData[pin].edge = FALLING_EDGE;
        }
      }
    }
  }
}


#ifdef OLD_RECEIVER_PIN_ORDER
  // arduino pins 67, 65, 64, 66, 63, 62
  static byte receiverPin[6] = {5, 3, 2, 4, 1, 0}; // bit number of PORTK used for XAXIS, YAXIS, ZAXIS, THROTTLE, MODE, AUX
#else
  //arduino pins 63, 64, 65, 62, 66, 67
  static byte receiverPin[8] = {1, 2, 3, 0, 4, 5, 6, 7}; // bit number of PORTK used for XAXIS, YAXIS, ZAXIS, THROTTLE, MODE, AUX
#endif

void initializeReceiver(int nbChannel = 6) {

  initializeReceiverParam(nbChannel);
  
  // A8 is pin 62
  // All input is on the same port
  inputPort = digitalPinToPort(A8);

  // Configure timer HW, just run 0-ffff at 2MHz
  TCCR5A = 0;
  TCCR5B = (1<<CS51); //Prescaler set to 8, that give us a resolution of 0.5us, read page 134 of data sheet

  DDRK = 0;
  PORTK = 0;
  PCMSK2 |=(1<<lastReceiverChannel)-1;
  PCICR |= 0x1 << 2;

  for (byte channel = XAXIS; channel < lastReceiverChannel; channel++)
    pinData[receiverPin[channel]].edge = FALLING_EDGE;
}


int getRawChannelValue(byte channel) {
  byte pin = receiverPin[channel];
  uint8_t oldSREG = SREG;
  cli();
  // Get receiver value read by pin change interrupt handler
  uint16_t receiverRawValue = pinData[pin].lastGoodWidth;
  SREG = oldSREG;
  
  return receiverRawValue;
}

void setChannelValue(byte channel,int value) {
}  

#endif // _AEROQUAD_RECEIVER_MEGA_HWPPM_H_