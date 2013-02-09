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
#include <Receiver_MEGA_HWPWM.h>  // for AeroQuad shield v1.x with Arduino Mega and shield v2.x using a standard PWM receiver
//#include <Receiver_MEGA.h>   // for AeroQuad shield v1.x with Arduino Mega and shield v2.x using a standard PWM receiver
//#include <Receiver_328p.h> // for AeroQuad shield v1.x with Arduino Due/Uno and mini shield v1.0 using a standard PWM receiver

//Futaba sBus
//#define sBus // for sBus receiver

// -------------  End of configuration ----------------- //


#if defined(sBus)
  #include <Receiver_SBUS.h>
#endif

unsigned long fastTimer = 0;
unsigned long slowTimer = 0;

int maxReceiver[LASTCHANNEL];
int minReceiver[LASTCHANNEL];

#define MAX_CHANNEL (AUX5 + 1)

const char* channelName[MAX_CHANNEL];

void resetMinMaxChannel()
{
  for (int i = 0; i < LASTCHANNEL; ++i)
  {
    maxReceiver[i] = -1;
    minReceiver[i] = 10000;
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Receiver library test");

  initializeReceiver(LASTCHANNEL);   
  receiverXmitFactor = 1.0;
  
  resetMinMaxChannel();

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

void printChannel(const int i)
{
  Serial.print(channelName[i]);
  Serial.print(": ");
  Serial.print(receiverCommand[i]);
  if (minReceiver >= 0)
  {
    Serial.print(" (");
    Serial.print(minReceiver[i]);
    Serial.print(",");
    Serial.print(maxReceiver[i]);
    const int diff = maxReceiver[i] - minReceiver[i];
    Serial.print(",");
    Serial.print(diff);
    Serial.print(")");
  }
  Serial.print(" ");
}

#define RESET_SECS 5
#define MILLIS_PER_SEC 1000
#define RESET_MILLIS (RESET_SECS * 1000)

void loop() 
{
  const unsigned long now = millis();

  //if ((now - fastTimer) >= 50) // 20Hz
  if ((now - fastTimer) >= 100)
  {
    fastTimer = now;
   
    #if defined(sBus)
      readSBUS();
    #endif

    // readReceiver();
    
    for (int i = 0; i < LASTCHANNEL; ++i)
    {
      // const int v = receiverCommand[i];
      const int v = receiverCommand[i] = getRawChannelValue(i);
      if (v < minReceiver[i])
        minReceiver[i] = v;
      if (v > maxReceiver[i])
        maxReceiver[i] = v;
        
      printChannel(i);
    }
    Serial.println();
  }
  
  if ((now - slowTimer) >= RESET_MILLIS)
  {
    uint8_t sreg = SREG;
    cli();
    uint32_t isrs = isrCount;
    isrCount = 0;
    SREG = sreg;
    
    uint32_t freq = isrs / RESET_SECS;
    
    slowTimer = now;
    resetMinMaxChannel();
    Serial.println("\nMin/max reset");
    Serial.print("Interrupts: ");
    Serial.print(isrs);
    Serial.print(", freq: ");
    Serial.print(freq);
    Serial.println("\n");
  }
}

