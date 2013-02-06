/*
  This example reads a PWM receiver using PinChangeInt and timer5.
  NOTE: Timer 5 is only available on Arduino Megas.
*/
#include <PinChangeInt.h>
#include <timer5.h>

#define LASTCHANNEL 7

const byte pin[] = {A8, A9, A10, A11, A12, A13, A14};    //for maximum efficency thise pins should be attached

volatile uint16_t time[] = {0,0,0,0,0,0,0};
volatile uint8_t trackedPin=0;

uint16_t safe_time(uint8_t channel)
{
  uint8_t sreg;
  uint32_t t;

  sreg = SREG; // save interrupt status registers
  cli(); // stop interrupts
  t = time[channel];
  SREG = sreg; // restore interrupts to prior status
  
  return t;
}

uint32_t fastTimer = 0;
uint32_t slowTimer = 0;

uint32_t maxReceiver[LASTCHANNEL];
uint32_t minReceiver[LASTCHANNEL];

void resetMinMaxReceiver()
{
  for (uint8_t c = 0; c < LASTCHANNEL; ++c)
  {
    maxReceiver[c] = 0;
    minReceiver[c] = 10000;
  }
}

void setup()
{
  Serial.begin(115200);

  timer5_init(2200);
  timer5_stop();
  timer5_reset();

  for (uint8_t c=0; c < LASTCHANNEL; c++)
    pinMode(pin[c], INPUT);

  PCintPort::attachInterrupt(pin[trackedPin], rise, RISING); // attach a PinChange Interrupt to our first pin

  resetMinMaxReceiver();
}

void printChannel(const uint8_t c, const uint32_t t)
{
  Serial.print(pin[c]);
  Serial.print(": ");
  Serial.print(t);
  if (maxReceiver[c] >= minReceiver[c])
  {
    Serial.print(" (");
    Serial.print(minReceiver[c]);
    Serial.print(",");
    Serial.print(maxReceiver[c]);
    const unsigned long diff = maxReceiver[c] - minReceiver[c];
    Serial.print(",");
    Serial.print(diff);
    Serial.print(")");
  }
  Serial.print(" ");
}

uint32_t cycleStart = 0;
uint8_t lastPin = trackedPin;
uint32_t duration = 0;

void loop() 
{
  const uint32_t now = millis();
  const uint8_t p = trackedPin;
  if (p != lastPin)
  {
    lastPin = p;
    if (p == 0)
    {
      duration = now - cycleStart;
      cycleStart = now;
    }
  }
  
  //if ((now - fastTimer) >= 50) // 20Hz
  if ((now - fastTimer) >= 100)
  {
    fastTimer = now;
   
    for (int c = 0; c < LASTCHANNEL; ++c)
    {
      const uint32_t t = safe_time(c);
      if (t < minReceiver[c])
        minReceiver[c] = t;
      if (t > maxReceiver[c])
        maxReceiver[c] = t;
        
      printChannel(c, t);
    }
    Serial.println();
  }
  
  if ((now - slowTimer) >= 2000)
  {
    slowTimer = now;
    resetMinMaxReceiver();
    Serial.print("\nMin/max reset - last cycle duration: ");
    Serial.print(duration);
    Serial.println("ms\n");
  }
}

void rise()
{
  timer5_reset();
  timer5_start();

  PCintPort::detachInterrupt(pin[trackedPin]);
  PCintPort::attachInterrupt(pin[trackedPin], fall, FALLING);
}

void fall()
{
  time[trackedPin] = timer5_isr_micros();
  timer5_stop();
  
  PCintPort::detachInterrupt(pin[trackedPin]);
  trackedPin++;
  trackedPin = trackedPin % LASTCHANNEL;
  PCintPort::attachInterrupt(pin[trackedPin], rise, RISING);
}

