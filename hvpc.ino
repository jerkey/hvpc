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

#define ACVOLTAGETARGET 120 // how many volts we want to generate
#define ACFREQUENCY 60 // target frequency in Hertz (cycles per second)
#define ACMINOFFTIME 1000 // milliseconds minimum offTime before turn AC on
#define ACSTARTVOLTAGE MINVOLTAGE + 30 // minimum turn-on voltage
#define MINVOLTAGE 120 * 1.414 * 0.90 // minimum operation voltage
#define MAXVOLTAGE 300 * 0.85 // based on capacitor maximum voltage?
#define RELAYHYSTVOLTS 30 // how many volts below MAXVOLTAGE relay cancels
#define RELAYMINTIME 2000 // how many milliseconds minimum relay on time
#define X-NEG 7
#define Y-NEG 8
#define X-POS 9  // high = off, low = on
#define Y-POS 10 // these are capacitively coupled
#define RELAYPIN 2 // a 3PDT relay which disconnects 3-winding generator

float voltage; // voltage of high-voltage DC rail
unsigned long timeNow, offTime, lastRelayTime = 0; // offTime is when AC last turned off
boolean lastPolarity = true; // which polarity was AC last cycle
boolean outputEnabled = false; // whether we are allowed to deliver AC output

void setup() {
  Serial.begin(BAUDRATE);
  Serial.println(versionStr);
  pinMode(X-NEG,OUTPUT);
  pinMode(Y-NEG,OUTPUT);
  digitalWrite(X-POS,HIGH); // high = off, low = on
  digitalWrite(Y-POS,HIGH); // high = off, low = on
  pinMode(X-POS,OUTPUT);
  pinMode(Y-POS,OUTPUT);
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

void setupInterruptHandler() { // configure interrupt-based FET control
  look at ACFREQUENCY and use it to setup frequency for first handler
  setup one interrupt handler to trigger 120x per second
    this handler turns on the FETs (opposite polarity each time)
    and sets the timer for the other interrupt handler to be called
    based on the correct amount of on-time for the present voltage
  setup another interrupt handler to trigger when another timer expires
    this handler just turns all the FETs off when it's called.
}

void polarityIntHandler() { // this is called 120 times per second for 60 Hertz
  if (outputEnabled) {
    if (lastPolarity) {
      digitalWrite(Y-POS,LOW);  // activate Y-positive
      digitalWrite(X-NEG,HIGH); // activate X-negative
      lastPolarity = false; // make it opposite what it was
    } else { // lastPolarity was false
      digitalWrite(X-POS,LOW);  // activate X-positive
      digitalWrite(Y-NEG,HIGH); // activate Y-negative
      lastPolarity = true; // make it opposite what it was
    }
  }

  now set the interrupt handler for offTimeIntHandlers()
    to happen the correct amount of time after now
    based on the voltage versus the target voltage
}

void offTimeIntHandlers() { // this is called by the settable timer interrupt
  digitalWrite(X-POS,HIGH);
  digitalWrite(Y-POS,HIGH);
  digitalWrite(X-NEG,LOW);
  digitalWrite(Y-NEG,LOW);
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
