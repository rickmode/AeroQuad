/*
  This example reads a PWM receiver using PinChangeInt and timer5.
  NOTE: Timer 5 is only available on Arduino Megas.
*/
#include <PinChangeInt.h>

const uint8_t clockMode = 0; // clock mode 0
#define PRESCALING 8
const uint8_t clockSelectBits = _BV(CS51); // 8x prescaling

inline void timer5_init()
{
  // initialize to mode 0: count from 0 to 0xFFFF the restart at 0
  //   overflow flag TOV5 set when clock restarts at 0
  TCCR5A = 0;
  TCCR5B = clockMode;
}

inline void timer5_start()
{
  // start by setting desired clock select bits
  TCCR5B |= clockSelectBits;
}

inline void timer5_stop()
{
  // stop by clearing all clock select bits
  TCCR5B &= ~(_BV(CS50) | _BV(CS51) | _BV(CS52));
}

inline void timer5_reset()
{
  TCNT5 = 0;
}

#define MICROS_PER_SECOND (1000UL * 1000UL)
#define MICROS_PER_TIC (F_CPU / PRESCALING / MICROS_PER_SECOND)

inline uint16_t timer5_micros()
{
  return TCNT5 / MICROS_PER_TIC;
}

inline void timer5_enable_overflow_interrupt()
{
  TIMSK5 = _BV(TOIE5);
}

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
  uint8_t sreg = SREG; // save interrupt status registers
  cli(); // stop interrupts
  uint16_t pw = pulseWidth[channel];
  SREG = sreg; // restore interrupts to prior status
  
  return pw;
}

void rise()
{
  timer5_stop();
  timer5_reset();
  timer5_start();
  
  PCintPort::detachInterrupt(pin[currentChannel]);
  PCintPort::attachInterrupt(pin[currentChannel], fall, FALLING);

  lost[currentChannel] = false;
}

void fall()
{
  pulseWidth[currentChannel] = timer5_micros();
  timer5_stop();
  
  PCintPort::detachInterrupt(pin[currentChannel]);
  currentChannel = (++currentChannel) % LASTCHANNEL;
  PCintPort::attachInterrupt(pin[currentChannel], rise, RISING);

  // start timer again to catch dead pin via timer overflow interrupt
  timer5_reset();
  timer5_start();
}

// timer overflow interrupt
// give up on current pin and move to next pin
ISR(TIMER5_OVF_vect)
{
  lost[currentChannel] = true;
  pulseWidth[currentChannel] = 0;
  timer5_stop();
  
  PCintPort::detachInterrupt(pin[currentChannel]);
  currentChannel = (++currentChannel) % LASTCHANNEL;
  PCintPort::attachInterrupt(pin[currentChannel], rise, RISING);
  
  timer5_reset();
  timer5_start();
}

void setup()
{
  Serial.begin(115200);
  Serial.println("setup start");

  resetMinMax();

  timer5_init();
  timer5_reset();
  timer5_enable_overflow_interrupt();
  timer5_start();

  for (uint8_t i = 0; i < LASTCHANNEL; ++i)
  {
    pulseWidth[i] = 0;
    lost[i] = false;
    pinMode(pin[i], INPUT);
  }

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
    Serial.print("Micros per tic: ");
    Serial.println(MICROS_PER_TIC);
    Serial.print("Max micros: ");
    Serial.println(MICROS_PER_TIC * 65536);
    Serial.println();
  }
}

