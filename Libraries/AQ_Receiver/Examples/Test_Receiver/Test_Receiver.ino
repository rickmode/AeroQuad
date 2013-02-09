/*
  AeroQuad v3.1 - January 2013
  www.AeroQuad.com
  Copyright (c) 2011 Ted Carancho.  All rights reserved.
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

#include <AQMath.h>
#include <GlobalDefined.h>

//Choose how many channels you have available (6, 8 or 10)
#define LASTCHANNEL 7

// Uncomment only one of the following receiver types depending on the type you are using
// see http://aeroquad.com/showwiki.php?title=Connecting+the+receiver+to+your+AeroQuad+board
  
//PPM receivers
//#include <Receiver_PPM.h>   // for Arduino boards using a PPM receiver
//#include <Receiver_HWPPM.h> // for AeroQuad shield v1.x with Arduino Mega and shield v2.x with hardware mod                       

//PWM receivers
//#include <Receiver_MEGA_HWPWM.h>  // for AeroQuad shield v1.x with Arduino Mega and shield v2.x using a standard PWM receiver
#include <PinChangeInt.h> // required by Receiver_MEGA_Timer.h
#include <Receiver_MEGA_Timer.h>
//#include <Receiver_MEGA.h>   // for AeroQuad shield v1.x with Arduino Mega and shield v2.x using a standard PWM receiver
//#include <Receiver_328p.h> // for AeroQuad shield v1.x with Arduino Due/Uno and mini shield v1.0 using a standard PWM receiver

//Futaba sBus
//#define sBus // for sBus receiver

// -------------  End of configuration ----------------- //


#if defined(sBus)
  #include <Receiver_SBUS.h>
#endif

uint16_t maxPulseWidth[LASTCHANNEL];
uint16_t minPulseWidth[LASTCHANNEL];

#define MAX_CHANNEL (AUX5 + 1)

const char* channelName[MAX_CHANNEL];

void resetMinMax()
{
  for (uint8_t i = 0; i < LASTCHANNEL; ++i)
  {
    maxPulseWidth[i] = 0;
    minPulseWidth[i] = 65535;
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Receiver library test");

  initializeReceiver(LASTCHANNEL);   
  receiverXmitFactor = 1.0;
  
  resetMinMax();

  channelName[XAXIS] = "Roll";
  channelName[YAXIS] = "Pitch";
  channelName[ZAXIS] = "Yaw";
  channelName[THROTTLE] = "Throttle";
  channelName[MODE] = "Mode";
  channelName[AUX1] = "Aux1";
  channelName[AUX2] = "Aux2";
  channelName[AUX3] = "Aux3";
  channelName[AUX4] = "Aux4";
  channelName[AUX5] = "Aux5";

  Serial.println("setup done");
}

void printChannel(const uint8_t i)
{
  Serial.print(channelName[i]);
  Serial.print(": ");
  Serial.print(receiverCommand[i]);
  if (minPulseWidth >= 0)
  {
    Serial.print(" (");
    Serial.print(minPulseWidth[i]);
    Serial.print(",");
    Serial.print(maxPulseWidth[i]);
    const int diff = maxPulseWidth[i] - minPulseWidth[i];
    Serial.print(",");
    Serial.print(diff);
    Serial.print(")");
  }
  Serial.print(" ");
}

#define RESET_SECS 5
#define MILLIS_PER_SEC 1000
#define RESET_MILLIS (RESET_SECS * 1000)

uint32_t fastTimer = 0;
uint32_t slowTimer = 0;

void loop() 
{
  const uint32_t now = millis();

  //if ((now - fastTimer) >= 50) // 20Hz
  if ((now - fastTimer) >= 100)
  {
    fastTimer = now;
   
    #if defined(sBus)
      readSBUS();
    #endif

    // readReceiver();
    
    for (uint8_t i = 0; i < LASTCHANNEL; ++i)
    {
      // const int pw = receiverCommand[i];
      const int pw = receiverCommand[i] = getRawChannelValue(i);
      if (pw < minPulseWidth[i])
        minPulseWidth[i] = pw;
      if (pw > maxPulseWidth[i])
        maxPulseWidth[i] = pw;
        
      printChannel(i);
    }
    Serial.println();
  }
  
  if ((now - slowTimer) >= RESET_MILLIS)
  {
    slowTimer = now;
    resetMinMax();
    Serial.println("\nMin/max reset\n");
  }
}

