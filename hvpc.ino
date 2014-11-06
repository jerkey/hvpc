/* High Voltage Pedal Charger by jake 2014-10-29

   regulates a modified sinewave from high voltage DC
   to simulate the output of a 120 VAC 60Hz inverter
   by switching output polarity between forward, backward,
   and zero volts.

   pins 7 and 8 control N-FETs which connect AC socket
   legs X and Y (respectively) to ground, which is also
   the minus side of a high-voltage DC supply generated
   from pedal power.

   through MOSFET drivers and capacitively coupled,
   pins 9 and 10 control P-FETs which connect AC socket
   legs X and Y (respectively) to the positive rail of
   the high-voltage DC supply, which is connected through
   a voltage divider to ADC input A?
*/

char versionStr[] = "High Voltage Pedal Charger v1.0";

#define BAUDRATE 57600
#define ACVOLTAGETARGET 120 // how many volts we want to generate
#define ACFREQUENCY 60 // target frequency in Hertz (cycles per second)
#define ACMINOFFTIME 1000 // milliseconds minimum offTime before turn AC on
#define ACSTARTVOLTAGE MINVOLTAGE + 30 // minimum turn-on voltage
#define MINVOLTAGE 120 * 1.414 * 0.90 // minimum operation voltage
#define MAXVOLTAGE 300 * 0.85 // based on capacitor maximum voltage?
#define RELAYHYSTVOLTS 30 // how many volts below MAXVOLTAGE relay cancels
#define RELAYMINTIME 2000 // how many milliseconds minimum relay on time
#define X_NEG 7
#define Y_NEG 8
#define X_POS 9  // high = off, low = on
#define Y_POS 10 // these are capacitively coupled
#define RELAYPIN 2 // a 3PDT relay which disconnects 3-winding generator
#define VOLTPIN A0 // voltage divider of high voltage DC
#define VOLTCOEFF 13.179  // larger number interprets as lower voltage
#define AVG_CYCLES 50 // average measured values over this many samples

float voltage; // voltage of high-voltage DC rail
unsigned long timeNow, offTime, lastRelayTime = 0; // offTime is when AC last turned off
boolean lastPolarity = true; // which polarity was AC last cycle
boolean outputEnabled = false; // whether we are allowed to deliver AC output

void setup() {
  Serial.begin(BAUDRATE);
  Serial.println(versionStr);
  pinMode(X_NEG,OUTPUT);
  pinMode(Y_NEG,OUTPUT);
  digitalWrite(X_POS,HIGH); // high = off, low = on
  digitalWrite(Y_POS,HIGH); // high = off, low = on
  pinMode(X_POS,OUTPUT);
  pinMode(Y_POS,OUTPUT);
  setupInterruptHandler(); // configure interrupt-based FET control
}

void loop() {
  timeNow = millis();
  getVoltage();
  doSafety();
  if ((voltage > ACSTARTVOLTAGE) && (timeNow - offTime > ACMINOFFTIME)) outputEnabled = true;
  if ((voltage < MINVOLTAGE) && (outputEnabled)) {
    outputEnabled = false;
    offTime = timeNow;
  }
}

// ISR(TIMER1_COMPB_vect)
// ISR(TIMER1_COMPA_vect)

void setupInterruptHandler() { // configure interrupt-based FET control
  byte wgm_mode = 14; // PAGE 136
  byte clock_select = 0b010; // CLKio/8 so CS12:10=010  PAGE 137
  PRR &= 0b11110111;  // make sure timer1 is on PAGE 45
  TCCR1A = 0b00000000 + (wgm_mode & 0b11); // OC1A/OC1B disconnected. TCCR1A PAGE 134
  TCCR1B = ((wgm_mode & 0b1100) << 1) + clock_select; // TCCR1B PAGE 136
  // note there is a master clock prescaler, usually off, see PAGE 38
  ICR1 = 16000000 / 8 / (ACFREQUENCY * 2); // two interrupts per cycle
  // 8MHz divided by 30000 = 135.5 Hz @ TIMER1_OVF_vect
  // 8MHz divided by 128 = 31.8 KHz = total 18.6 KHz cycle
  OCR1A = 5000;  // higher value = lower duty cycle
//  OCR1B = divisor / 2;  // put the same value into both registers
  TIMSK1 = 0b00000001;  // bit 0 = OVF1   bit 1 = OCF1A   PAGE 139

  // setup another interrupt handler to trigger when another timer expires
  //   this handler just turns all the FETs off when it's called.
}

ISR(TIMER1_OVF_vect) { // this is called 120 times per second for 60 Hertz
  if (outputEnabled) {
    if (lastPolarity) { // this handler turns on the FETs (opposite polarity each time)
      digitalWrite(Y_POS,LOW);  // activate Y_positive
      digitalWrite(X_NEG,HIGH); // activate X_negative
      lastPolarity = false; // make it opposite what it was
    } else { // lastPolarity was false
      digitalWrite(X_POS,LOW);  // activate X_positive
      digitalWrite(Y_NEG,HIGH); // activate Y_negative
      lastPolarity = true; // make it opposite what it was
    }
  }

  // now set the interrupt handler for offTimeIntHandlers()
  //   to happen the correct amount of time after now
  //   based on the voltage versus the target voltage
}

void offTimeIntHandlers() { // this is called by the settable timer interrupt
  digitalWrite(X_POS,HIGH);
  digitalWrite(Y_POS,HIGH);
  digitalWrite(X_NEG,LOW);
  digitalWrite(Y_NEG,LOW);
}

void getVoltage() {
  static float voltsAdc = analogRead(VOLTPIN);
  static float voltsAdcAvg = average(voltsAdc, voltsAdcAvg);
  voltage = voltsAdcAvg / VOLTCOEFF;
}

float average(float val, float avg) {
  if(avg == 0)
    avg = val;
  return (val + (avg * (AVG_CYCLES - 1))) / AVG_CYCLES;
}

void doSafety() {
  if (voltage > MAXVOLTAGE) {
    if (!digitalRead(RELAYPIN)) lastRelayTime = timeNow; // record when relay turned on
    digitalWrite(RELAYPIN,HIGH);
  }
  if ((voltage < (MAXVOLTAGE - RELAYHYSTVOLTS)) && (timeNow - lastRelayTime > RELAYMINTIME)) {
    digitalWrite(RELAYPIN,LOW);
  }
}
