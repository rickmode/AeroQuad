/*
  This example reads a PWM receiver using PinChangeInt and timer5.
  NOTE: Timer 5 is only available on Arduino Megas.
*/
#include <PinChangeInt.h>
#include <timer5.h>

#define LASTCHANNEL 7

// AeroQuad shield has 8 PWM receiver inputs on pins A8 to A15
const byte pin[] = {A8, A9, A10, A11, A12, A13, A14, A15};

volatile uint16_t pulseWidth[LASTCHANNEL];
volatile uint8_t currentChannel = 0;

uint16_t maxPulseWidth[LASTCHANNEL];
uint16_t minPulseWidth[LASTCHANNEL];

volatile boolean lost[LASTCHANNEL];

void resetMinMax()
{
  for (uint8_t i = 0; i < LASTCHANNEL; ++i)
  {
    maxPulseWidth[i] = 0;
    minPulseWidth[i] = 65535;
  }
}

uint16_t getPulseWidth(uint8_t channel)
{
  uint8_t sreg;
  uint16_t pw;

  sreg = SREG; // save interrupt status registers
  cli(); // stop interrupts
  pw = pulseWidth[channel];
  SREG = sreg; // restore interrupts to prior status
  
  return pw;
}

void rise()
{
  timer5_stop(); // ***
  timer5_reset();
  timer5_start();
  
  PCintPort::detachInterrupt(pin[currentChannel]);
  PCintPort::attachInterrupt(pin[currentChannel], fall, FALLING);

  lost[currentChannel] = false; // ***
}

void fall()
{
  pulseWidth[currentChannel] = timer5_isr_micros();
  timer5_stop();
  
  PCintPort::detachInterrupt(pin[currentChannel]);
  currentChannel = (++currentChannel) % LASTCHANNEL;
  PCintPort::attachInterrupt(pin[currentChannel], rise, RISING);

  // start timer again to catch dead pin via timer overflow interrupt
  timer5_reset(); // ***
  timer5_start(); // ***
}

volatile boolean timerOverflow = false;

// timer overflow interrupt
// give up on current pin and move to next pin
ISR(TIMER5_OVF_vect)
{
  timerOverflow = true;
}

void handleTimerOverflow()
{
  uint8_t sreg = SREG;
  cli();

  lost[currentChannel] = true;
  pulseWidth[currentChannel] = 0;
  timer5_stop();
  
  PCintPort::detachInterrupt(pin[currentChannel]);
  currentChannel = (++currentChannel) % LASTCHANNEL;
  PCintPort::attachInterrupt(pin[currentChannel], rise, RISING);
  
  timer5_reset();
  timer5_start();
  
  timerOverflow = false;
  
  SREG = sreg;
}

void setup()
{
  Serial.begin(115200);
  Serial.println("setup start");

  resetMinMax();

  //timer5_init(2200);
  timer5_init(25000); //***
  timer5_stop();
  timer5_reset();
  timer5_start(); //***

  for (uint8_t i = 0; i < LASTCHANNEL; ++i)
  {
    pulseWidth[i] = 0;
    lost[i] = false;
    pinMode(pin[i], INPUT);
  }

  // enable overflow timer interrupt
  TIMSK5 = _BV(TOIE5); //***

  // attach a PinChange Interrupt to our first pin
  PCintPort::attachInterrupt(pin[currentChannel], rise, RISING);

  Serial.println("setup end");
}

void printChannel(const uint8_t i, const uint16_t pw)
{
  Serial.print(pin[i]);
  Serial.print(": ");
  if (lost[i])
  {
    Serial.print("LOST ");
  }
  else
  {
    Serial.print(pw);
    if (maxPulseWidth[i] >= minPulseWidth[i])
    {
      Serial.print(" (");
      Serial.print(minPulseWidth[i]);
      Serial.print(",");
      Serial.print(maxPulseWidth[i]);
      const uint32_t diff = maxPulseWidth[i] - minPulseWidth[i];
      Serial.print(",");
      Serial.print(diff);
      Serial.print(")");
    }
    Serial.print(" ");
  }
}

uint32_t fastTimer = 0;
uint32_t slowTimer = 0;

uint32_t cycleStart = 0;
uint8_t lastChannel = currentChannel;
uint32_t duration = 0;

void loop() 
{
  if (timerOverflow)
    handleTimerOverflow();
  
  const uint32_t now = millis();
  const uint8_t channel = currentChannel;
  if (channel != lastChannel)
  {
    lastChannel = channel;
    if (channel == 0)
    {
      duration = now - cycleStart;
      cycleStart = now;
    }
  }
  
  //if ((now - fastTimer) >= 50) // 20Hz
  if ((now - fastTimer) >= 100)
  {
    fastTimer = now;
   
    for (uint8_t i = 0; i < LASTCHANNEL; ++i)
    {
      const uint16_t pw = getPulseWidth(i);
      if (pw < minPulseWidth[i])
        minPulseWidth[i] = pw;
      if (pw > maxPulseWidth[i])
        maxPulseWidth[i] = pw;
        
      printChannel(i, pw);
    }
    Serial.println();
  }
  
  if ((now - slowTimer) >= 2000)
  {
    slowTimer = now;
    resetMinMax();
    Serial.print("\nMin/max reset - last cycle duration: ");
    Serial.print(duration);
    Serial.println("ms\n");
    Serial.print("clockSelectBits = ");
    Serial.println(clockSelectBits);
    Serial.print("scale = ");
    Serial.println(scale);
    Serial.print("F_CPU = ");
    Serial.println(F_CPU);
    Serial.print("ICR5 = ");
    Serial.println(ICR5);
    Serial.println();
  }
}

