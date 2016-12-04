// =====================================
// Midi Master Clock
// Modified code for Arduino Mega 2560, Sparkfun MIDI shield and 
// Arducam Liquid Crystal display.
// Original at: https://github.com/DieterVDW/arduino-midi-clock
// 12/16/2016
// Source: https://github.com/freonirons409/midi_master_clock
// Author: Aaron Irons (hey@aaronirons.net)
// Website: https://www.aaronirons.net
// =====================================

#include <TimerOne.h>
#include <LiquidCrystal.h>

LiquidCrystal lcd(49, 45, 35, 33, 31, 29);

#define TAP_PIN 2
#define TAP_PIN_POLARITY RISING

#define MINIMUM_TAPS 3
#define EXIT_MARGIN 150 // If no tap after 150% of last tap interval -> measure and set

#define DIMMER_INPUT_PIN A0

#define DIMMER_CHANGE_MARGIN 2
//#define DIMMER_CHANGE_PIN A1
#define DEAD_ZONE 50
#define CHANGE_THRESHOLD 5000
#define RATE_DIVISOR 30

#define BLINK_OUTPUT_PIN 7
#define BLINK_PIN_POLARITY 0  // 0 = POSITIVE, 255 - NEGATIVE
#define BLINK_TIME 4 // How long to keep LED lit in CLOCK counts (so range is [0,24])

#define SYNC_OUTPUT_PIN 9 // Can be used to drive sync analog sequencer (Korg Monotribe etc ...)
#define SYNC_PIN_POLARITY 0 // 0 = POSITIVE, 255 - NEGATIVE

#define START_STOP_INPUT_PIN 3
#define START_STOP_PIN_POLARITY 0 // 0 = POSITIVE, 1024 = NEGATIVE

#define MIDI_START 0xFA
#define MIDI_STOP 0xFC

#define DEBOUNCE_INTERVAL 500L // Milliseconds

#define EEPROM_ADDRESS 0 // Where to save BPM
#ifdef EEPROM_ADDRESS
#include <EEPROM.h>
#endif

#define MIDI_FORWARD

#define MIDI_TIMING_CLOCK 0xF8
#define CLOCKS_PER_BEAT 24
#define MINIMUM_BPM 40 // Used for debouncing
#define MAXIMUM_BPM 300 // Used for debouncing

long intervalMicroSeconds;
int bpm;

boolean initialized = false;
long minimumTapInterval = 60L * 1000 * 1000 / MAXIMUM_BPM;
long maximumTapInterval = 60L * 1000 * 1000 / MINIMUM_BPM;

int tapButton = 0;
int startStopPressed = 0;
volatile long firstTapTime = 0;
volatile long lastTapTime = 0;
volatile long timesTapped = 0;

volatile int blinkCount = 0;

int lastDimmerValue = 0;

boolean playing = false;
long lastStartStopTime = 0;

#ifdef DIMMER_CHANGE_PIN
long changeValue = 0;
#endif

void setup() {
  //  Set MIDI baud rate:
  Serial.begin(31250);
  //Serial.begin(74880);

  // Set pin modes
#ifdef BLINK_OUTPUT_PIN
  pinMode(BLINK_OUTPUT_PIN, OUTPUT);
#endif
#ifdef SYNC_OUTPUT_PIN
  pinMode(SYNC_OUTPUT_PIN, OUTPUT);
#endif
#ifdef DIMMER_INPUT_PIN
  pinMode(DIMMER_INPUT_PIN, INPUT);
#endif
#ifdef START_STOP_INPUT_PIN
  pinMode(START_STOP_INPUT_PIN, INPUT);
#endif

  pinMode(TAP_PIN, INPUT_PULLUP);
  pinMode(START_STOP_INPUT_PIN, INPUT_PULLUP);
  pinMode(4, INPUT_PULLUP);
  pinMode(51, OUTPUT); //input on LCD, receiving pot pin value
  pinMode(53, OUTPUT); //LCD pin 2, power supply for logic operating - 5.0v
  pinMode(49, OUTPUT);
  pinMode(47, OUTPUT);
  pinMode(45, OUTPUT);
  pinMode(35, OUTPUT);
  pinMode(33, OUTPUT);
  pinMode(31, OUTPUT);
  pinMode(29, OUTPUT);
  pinMode(27, OUTPUT); //LCD pin 15, power supply for backlight - 5.0v
  pinMode(25, OUTPUT); //LCD pin 16, backlight grnd
  pinMode(A8, OUTPUT); //Ground for Display pot
  pinMode(A10, OUTPUT); //5v for Display pot

  digitalWrite(53, HIGH);
  digitalWrite(47, LOW);
  digitalWrite(35, HIGH);
  digitalWrite(33, HIGH);
  digitalWrite(31, HIGH);
  digitalWrite(29, HIGH);
  digitalWrite(27, HIGH);
  digitalWrite(25, LOW);
  analogWrite(A8, 0);
  analogWrite(A10, 255);

  //init LCD
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.print("MIDI CLOCK! v666");
  
  // Get the saved BPM value from 2 stored bytes: MSB LSB
  bpm = EEPROM.read(EEPROM_ADDRESS) << 8;
  bpm += EEPROM.read(EEPROM_ADDRESS + 1);
  if (bpm < MINIMUM_BPM || bpm > MAXIMUM_BPM) {
    bpm = 120;
  }
  
  // Interrupt for catching tap events
  attachInterrupt(digitalPinToInterrupt(TAP_PIN), tapInput, TAP_PIN_POLARITY);

  // Attach the interrupt to send the MIDI clock and start the timer
  Timer1.initialize(intervalMicroSeconds);
  Timer1.setPeriod(calculateIntervalMicroSecs(bpm));
  Timer1.attachInterrupt(sendClockPulse);

  // Initialize dimmer value
#ifdef DIMMER_INPUT_PIN
  // Initialize dimmer value
  lastDimmerValue = analogRead(DIMMER_INPUT_PIN);
#endif
  
  lcd.setCursor(0, 1);
  lcd.print("BPM:");
  lcd.setCursor(5, 1);
  setDisplayValue(bpm);
  //Serial.write(MIDI_STOP);
}

void loop() {  
  long now = micros();

#ifdef TAP_PIN
  /*
   * Handle tapping of the tap tempo button
   */
  if (timesTapped > 0 && timesTapped < MINIMUM_TAPS && (now - lastTapTime) > maximumTapInterval) {
    // Single taps, not enough to calculate a BPM -> ignore!
    //Serial.println("Ignoring lone taps!");
    timesTapped = 0;
  } else if (timesTapped >= MINIMUM_TAPS) {
    long avgTapInterval = (lastTapTime - firstTapTime) / (timesTapped-1);
    if ((now - lastTapTime) > (avgTapInterval * EXIT_MARGIN / 100)) {
      bpm = 60L * 1000 * 1000 / avgTapInterval;
      updateBpm(now);
      blinkCount = ((now - lastTapTime) * 24 / avgTapInterval) % CLOCKS_PER_BEAT;
      timesTapped = 0;
    }
  }
#endif

#ifdef DIMMER_INPUT_PIN
  /*
   * Handle change of the dimmer input
   */
  int curDimValue = analogRead(DIMMER_INPUT_PIN);
  if (curDimValue > lastDimmerValue + DIMMER_CHANGE_MARGIN
      || curDimValue < lastDimmerValue - DIMMER_CHANGE_MARGIN) {
    bpm = map(curDimValue, 0, 1024, MINIMUM_BPM, MAXIMUM_BPM);
    updateBpm(now);
    lastDimmerValue = curDimValue;
  }
#endif

#ifdef DIMMER_CHANGE_PIN
  int curDimValue = analogRead(DIMMER_CHANGE_PIN);
  if (bpm > MINIMUM_BPM && curDimValue < (512 - DEAD_ZONE)) {
    int val = (512 - DEAD_ZONE - curDimValue) / RATE_DIVISOR;
    changeValue += val * val;
  } else if (bpm < MAXIMUM_BPM && curDimValue > (512 + DEAD_ZONE)) {
    int val = (curDimValue - 512 - DEAD_ZONE) / RATE_DIVISOR;
    changeValue += val * val;
  } else {
    changeValue = 0;
  }
  if (changeValue > CHANGE_THRESHOLD) {
    bpm += curDimValue < 512 ? -1 : 1;
    updateBpm(now);
    changeValue = 0;
  }
#endif

#ifdef START_STOP_INPUT_PIN
  /*
   * Check for start/stop button pressed
   */
  startStopPressed = digitalRead(START_STOP_INPUT_PIN);
  
  if (startStopPressed == 0 && (lastStartStopTime + (DEBOUNCE_INTERVAL * 1000)) < now) {
    startOrStop();
    lastStartStopTime = now;
  }
#endif
#ifdef MIDI_FORWARD
  /*
   * Forward received serial data
   */
  while (Serial.available()) {
    int b = Serial.read();
    Serial.write(b);
  }
#endif

}

void tapInput() {
  long now = micros();
  if (now - lastTapTime < minimumTapInterval) {
    return; // Debounce
  }

  if (timesTapped == 0) {
    firstTapTime = now;
  }

  timesTapped++;
  lastTapTime = now;
}

void startOrStop() {
  if (!playing) {
    Serial.write(MIDI_START);
  } else {
    Serial.write(MIDI_STOP);
  }
  playing = !playing;
}

void sendClockPulse() {
  // Write the timing clock byte
  Serial.write(MIDI_TIMING_CLOCK);

  blinkCount = (blinkCount+1) % CLOCKS_PER_BEAT;
  if (blinkCount == 0) {
    // Turn led on
#ifdef BLINK_OUTPUT_PIN
    analogWrite(BLINK_OUTPUT_PIN, 255 - BLINK_PIN_POLARITY);
#endif

#ifdef SYNC_OUTPUT_PIN
    // Set sync pin to HIGH
    analogWrite(SYNC_OUTPUT_PIN, 255 - SYNC_PIN_POLARITY);
#endif
  } else {
#ifdef SYNC_OUTPUT_PIN
    if (blinkCount == 1) {
      // Set sync pin to LOW
      analogWrite(SYNC_OUTPUT_PIN, 0 + SYNC_PIN_POLARITY);
    }
#endif
#ifdef BLINK_OUTPUT_PIN
    if (blinkCount == BLINK_TIME) {
      // Turn led on
      analogWrite(BLINK_OUTPUT_PIN, 0 + BLINK_PIN_POLARITY);
    }
#endif
  }
}

void updateBpm(long now) {
  // Update the timer
  long interval = calculateIntervalMicroSecs(bpm);
  Timer1.setPeriod(interval);

  // Save the BPM
#ifdef EEPROM_ADDRESS
  EEPROM.write(EEPROM_ADDRESS, bpm - 40);
  //EEPROM.write(EEPROM_ADDRESS + 1, bpm % 256);
#endif

  lcd.setCursor(0, 1);
  lcd.print("BPM:");
  setDisplayValue(bpm);
}

void setDisplayValue(int value) {
//  value >= 1000 ? lcd.print(value / 1000) : 0x00;
//  value >= 100 ? lcd.print((value / 100) % 10) : 0x00;
  lcd.setCursor(5, 1);
  lcd.print(value);
  if(value < 100) {
    lcd.setCursor(7, 1);
    lcd.print(" ");
  }
}


long calculateIntervalMicroSecs(int bpm) {
  // Take care about overflows!
  return 60L * 1000 * 1000 / bpm / CLOCKS_PER_BEAT;
}
