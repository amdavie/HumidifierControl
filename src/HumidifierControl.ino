#include <ArducamSSD1306.h> // Modification of Adafruit_SSD1306 for ESP8266 compatibility
#include <Adafruit_GFX.h>   // Needs a little change in original Adafruit library (See README.txt file)
#include <Wire.h>           // For I2C comm, but needed for not getting compile error

#define OLED_RESET  LED_BUILTIN  // Pin 15 -RESET digital signal

ArducamSSD1306 display(OLED_RESET); // FOR I2C

const int X1 = 3;  // input (float switch), open = tank full
const int X2 = D6;  // input (float switch), closed = start tank fill
const int X3 = D5;  // input, closed = start heater
const int Y1 = D3;  // output, relay (NO), closed opens valve to fill
const int Y2 = D4;  // output, relay (NO), closed opens valve to drain
const int Y3 = D7;  // output, relay (NO), closed powers on heater

const long drainDelay = 3000;           // T1 - milliseconds to delay before draining
const long drainTime = 8000;            // T2 - milliseconds to drain
const long fillTankCycleThreshold = 5;  // N - number of on/off fill cycles before during humidify cycle before running drain cycle
const long humidifySleepTime = 1000;    // milliseconds to sleep between humidify cycles

bool isDrainCycleRunning = false;
unsigned long drainDelayTimerStart = 0;
bool isWaitingForDrainDelayTimer = false;
unsigned long drainTimerStart = 0;
bool isWaitingForDrainTimer = false;
int fillTankCycle = 0;
unsigned long humidifySleepTimerStart = 0;
bool isWaitingForHumidifySleepTimer = false;

void setup() {
  Serial.begin(115200);
  initPins();
  initDisplay();
  initDrainCycle();
}

void loop() {
  if (isDrainCycleRunning) {
    drainCycle();
  } else {
    humidifyCycle();
  }
}

void initDrainCycle() {
  fillTankOff();
  heaterOff();
  isDrainCycleRunning = true;
  drainDelayTimerStart = millis();
  isWaitingForDrainDelayTimer = true;
  setStatus("Drain cycle - delay");
}

void drainCycle() {
  if (isWaitingForDrainDelayTimer) {
    bool isDrainDelayFinished = millis() - drainDelayTimerStart >= drainDelay;
    if (isDrainDelayFinished) {
      drainTankOn();
      isWaitingForDrainDelayTimer = false;
      drainTimerStart = millis();
      isWaitingForDrainTimer = true;
      setStatus("Drain cycle - drain");
    }
  } else if (isWaitingForDrainTimer){
    bool isDrainTimerFinished = millis() - drainTimerStart >= drainTime;
    if (isDrainTimerFinished) {
      drainTankOff();
      isWaitingForDrainTimer = false;
      fillTankOn();
    }
  } else {
    if (isTankFull()) {
      fillTankOff();
    }

    isDrainCycleRunning = false;
    fillTankCycle = 0;
  }
}

void humidifyCycle() {
  setStatus("Humidify cycle (n=" + String(fillTankCycle) + ")");
  if (isWaitingForHumidifySleepTimer) {
    bool isHumidifySleepTimerFinished = millis() - humidifySleepTimerStart >= humidifySleepTime;
    isWaitingForHumidifySleepTimer = !isHumidifySleepTimerFinished;
  } else {
    drainTankOff();
    if (isReadyToStartHeater()) {
      heaterOn();
    } else {
      heaterOff();
    }
    if (!isTankFull() && isReadyToFillTank()) {
      fillTankOn();
    }
    if (isTankFull()) {
      fillTankOff();
    }
  
    if (fillTankCycle >= fillTankCycleThreshold) {
      initDrainCycle();
    }

    humidifySleepTimerStart = millis();
    isWaitingForHumidifySleepTimer = true;
  }
}

bool isTankFull() {
  return digitalRead(X1) == HIGH;
}

bool isReadyToFillTank() {
  return digitalRead(X2) == LOW;
}

bool isReadyToStartHeater() {
  return digitalRead(X3) == LOW;
}

void fillTankOff() {
  if (digitalRead(Y1) == LOW) {
    fillTankCycle++;
  }
  
  digitalWrite(Y1, HIGH);  // open Y1

  Serial.println("FILL OFF");
  displayLine("FILL   OFF", 1);
}

void fillTankOn() {
  digitalWrite(Y1, LOW);  // close Y1

  Serial.println("FILL ON");
  displayLine("FILL    ON", 1);
}

void drainTankOff() {
  digitalWrite(Y2, HIGH); // open Y2
  
  Serial.println("DRAIN OFF");
  displayLine("DRAIN  OFF", 2);
}

void drainTankOn() {
  digitalWrite(Y2, LOW);  // close Y2

  Serial.println("DRAIN ON");
  displayLine("DRAIN   ON", 2);
}

void heaterOff() {
  digitalWrite(Y3, HIGH); // open Y3

  Serial.println("HEATER OFF");
  displayLine("HEATER OFF", 3);
}

void heaterOn() {
  digitalWrite(Y3, LOW);  // close Y3

  Serial.println("HEATER ON");
  displayLine("HEATER  ON", 3);
}

void initPins() {
  pinMode(X1, INPUT_PULLUP);
  pinMode(X2, INPUT_PULLUP);
  pinMode(X3, INPUT_PULLUP);
  pinMode(Y1, OUTPUT);
  pinMode(Y2, OUTPUT);
  pinMode(Y3, OUTPUT);
}

void initDisplay() {
  display.begin();
  display.clearDisplay();
}

void displayLine(String text, int lineNumber) {
  display.setTextSize(2);
  display.setTextColor(WHITE, BLACK);
  display.setCursor(0,16 * lineNumber);
  display.println(text);
  display.display();
}

void setStatus(String status) {
  Serial.println(status);
  display.setTextSize(1);
  display.setTextColor(WHITE, BLACK);
  display.setCursor(0,0);
  display.println("                    ");
  display.setCursor(0,0);
  display.println(status);
  display.display();
}
