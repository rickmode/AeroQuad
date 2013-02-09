/*
  Simple test of reading receiver PWM input using pulseIn
*/
#define XAXIS 0
#define YAXIS 1
#define ZAXIS 2
#define THROTTLE 3
#define MODE 4
#define AUX1 5
#define AUX2 6
#define AUX3 7
#define AUX4 8
#define AUX5 9

#define LASTCHANNEL 7

// pins used for XAXIS, YAXIS, ZAXIS, THROTTLE, MODE, AUX
//static int receiverPin[6] = {2, 5, 6, 4, 7, 8};
// bit number of PORTK used for XAXIS, YAXIS, ZAXIS, THROTTLE, MODE, AUX
//static byte receiverPin[8] = {1, 2, 3, 0, 4, 5, 6, 7};

// bit number of PORTK used for XAXIS, YAXIS, ZAXIS, THROTTLE, MODE, AUX
static byte receiverPin[] = {A8+1, A8+2, A8+3, A8+0, A8+4, A8+5, A8+6, A8+7};

uint32_t pulseWidth[LASTCHANNEL];

uint32_t maxPulseWidth[LASTCHANNEL];
uint32_t minPulseWidth[LASTCHANNEL];

boolean lost[LASTCHANNEL];

#define MAX_CHANNEL (AUX5 + 1)

const char* channelName[MAX_CHANNEL];

#define MAX_PULSE_WIDTH 65535

void resetMinMax()
{
  for (uint8_t i = 0; i < LASTCHANNEL; ++i)
  {
    maxPulseWidth[i] = 0;
    minPulseWidth[i] = MAX_PULSE_WIDTH;
  }
}

void setup()
{
  resetMinMax();

  for (uint8_t i = 0; i < LASTCHANNEL; ++i)
  {
    pulseWidth[i] = 0;
    lost[i] = false;
    pinMode(receiverPin[i], INPUT);
  }

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

  Serial.begin(115200);
}

void printChannel(const uint8_t i)
{
  Serial.print(channelName[i]);
  if (lost[i])
    Serial.print(" LOST");
  Serial.print(": ");
  Serial.print(pulseWidth[i]);
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

uint32_t fastTimer = 0;
uint32_t slowTimer = 0;

void loop()
{
  const uint32_t now = millis();
  if ((now - fastTimer) >= 100)
  {
    fastTimer = now;

    for (uint8_t i = 0; i < LASTCHANNEL; ++i)
    {
      const uint32_t pw = pulseIn(receiverPin[i], HIGH, 25000);
      pulseWidth[i] = pw;
      if (pw == 0)
      {
        lost[i] = true;
        minPulseWidth[i] = MAX_PULSE_WIDTH;
        maxPulseWidth[i] = 0;
      }
      else
      {
        lost[i] = false;
        if (pw < minPulseWidth[i])
          minPulseWidth[i] = pw;
        if (pw > maxPulseWidth[i])
          maxPulseWidth[i] = pw;
      }
        
      printChannel(i);
    }
    const uint32_t duration = millis() - now;
    Serial.print(duration);
    Serial.println("ms");
  }

  if ((now - slowTimer) >= 3000)
  {
    slowTimer = now;
    resetMinMax();
    Serial.println("\nMin/max reset\n");
  }
}

