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

#define MINVOLTAGE 120 * 1.414 * 0.90 // minimum turn-on voltage
#define MAXVOLTAGE 300 * 0.85 // based on capacitor maximum voltage?
#define RELAYHYSTVOLTS 30 // how many volts below MAXVOLTAGE relay cancels
#define RELAYMINTIME 2000 // how many milliseconds minimum relay on time
#define X-NEG 7
#define Y-NEG 8
#define X-POS 9  // high = off, low = on
#define Y-POS 10 // these are capacitively coupled
#define RELAYPIN 2 // a 3PDT relay which disconnects 3-winding generator

float hvdc; // voltage of high-voltage DC rail
unsigned long timeNow, lastRelayTime = 0;

void setup() {
  Serial.begin(BAUDRATE);
  Serial.println(versionStr);
  pinMode(X-NEG,OUTPUT);
  pinMode(Y-NEG,OUTPUT);
  digitalWrite(X-POS,HIGH); // high = off, low = on
  digitalWrite(Y-POS,HIGH); // high = off, low = on
  pinMode(X-POS,OUTPUT);
  pinMode(Y-POS,OUTPUT);
}

void loop() {
  timeNow = millis();
  getVoltage();
  doSafety();
  if hvdc > minVoltage
}

void doSafety() {
  if (hvdc > MAXVOLTAGE) {
    if (!digitalRead(RELAYPIN)) lastRelayTime = timeNow; // record when relay turned on
    digitalWrite(RELAYPIN,HIGH);
  }
  if ((hvdc < (MAXVOLTAGE - RELAYHYSTVOLTS)) && (timeNow - lastRelayTime > RELAYMINTIME)) {
    digitalWrite(RELAYPIN,LOW);
  }
}
