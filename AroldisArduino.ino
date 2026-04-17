#include <Servo.h>
#include <stdint.h>

#define DEBUG 1

#if DEBUG
  #define DBG_BEGIN(x)      Serial.begin(x)
  #define DBG_PRINT(x)      Serial.print(x)
  #define DBG_PRINTLN(x)    Serial.println(x)
#else
  #define DBG_BEGIN(x)
  #define DBG_PRINT(x)
  #define DBG_PRINTLN(x)
#endif

// Pins
constexpr uint8_t RED_BUTTON_PIN    = 2;
constexpr uint8_t BLUE_BUTTON_PIN   = 6;
constexpr uint8_t GREEN_BUTTON_PIN  = 7;
constexpr uint8_t YELLOW_BUTTON_PIN = 8;
constexpr uint8_t WHITE_BUTTON_PIN  = 10;
constexpr uint8_t BLACK_BUTTON_PIN  = 11;

constexpr uint8_t GREEN_LED_PIN = 3;
constexpr uint8_t BLUE_LED_PIN  = 4;
constexpr uint8_t RED_LED_PIN   = 5;

constexpr uint8_t SERVO_PIN = 9;

// Pattern steps
constexpr uint8_t STEP_RED   = 0;
constexpr uint8_t STEP_BLUE  = 1;
constexpr uint8_t STEP_GREEN = 2;
constexpr uint8_t STEP_LEFT  = 3;
constexpr uint8_t STEP_RIGHT = 4;

// Servo rules
constexpr int8_t SERVO_MIN_OFFSET = -3;
constexpr int8_t SERVO_MAX_OFFSET = 3;

constexpr uint8_t SERVO_CENTER_DEGREES = 90;
constexpr uint8_t SERVO_STEP_DEGREES   = 30;

// Timing
constexpr uint8_t  DEBOUNCE_MS                 = 50;
constexpr uint16_t FLASH_MS                    = 350;
constexpr uint16_t GAP_MS                      = 250;
constexpr uint16_t DIRECTIONAL_CHAIN_PAUSE_MS  = 200;
constexpr uint16_t ROUND_PAUSE_MS              = 1000;
constexpr uint16_t WAG_PAUSE_MS                = 600;

// Game state
constexpr uint8_t MAX_PATTERN_LENGTH = 100;
uint8_t pattern[MAX_PATTERN_LENGTH];
uint8_t currentPatternLength = 0;

// Tracks where the servo ends up after the full generated pattern,
// assuming the sequence starts from neutral offset 0.
int8_t patternServoOffset = 0;

// Tracks servo position during user input and playback
int8_t userServoOffset = 0;
int8_t playbackServoOffset = 0;

bool blueLedIsOn = false;

Servo gameServo;

void setup() {
  pinMode(RED_BUTTON_PIN, INPUT_PULLUP);
  pinMode(BLUE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(GREEN_BUTTON_PIN, INPUT_PULLUP);
  pinMode(YELLOW_BUTTON_PIN, INPUT_PULLUP);
  pinMode(WHITE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(BLACK_BUTTON_PIN, INPUT_PULLUP);

  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(BLUE_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);

  ledOff(RED_LED_PIN);
  ledOff(BLUE_LED_PIN);
  ledOff(GREEN_LED_PIN);

  gameServo.attach(SERVO_PIN);
  resetServoNeutral();

  DBG_BEGIN(115200);
  delay(200);

  DBG_PRINTLN();
  DBG_PRINTLN(F("=== Optimized Random Simon Test Game ==="));
  DBG_PRINTLN(F("Choices = RED, BLUE, GREEN, LEFT (WHITE), RIGHT (YELLOW)"));
  DBG_PRINTLN(F("Press BLACK to start"));
  DBG_PRINTLN();
}

void loop() {
  waitForBlackStart();

  const unsigned long seed = micros();
  randomSeed(seed);

  currentPatternLength = 0;
  patternServoOffset = 0;
  userServoOffset = 0;
  playbackServoOffset = 0;

  resetServoNeutral();
  addRandomStep();

  DBG_PRINTLN(F("[GAME] Starting new game"));
  DBG_PRINT(F("[GAME] Random seed = "));
  DBG_PRINTLN(seed);

  uint8_t round = 1;

  while (true) {
    DBG_PRINT(F("[ROUND] "));
    DBG_PRINTLN(round);

    printCurrentPattern();
    playRound(currentPatternLength);

    // After playback, servo returns to neutral logical position 0.
    resetServoNeutral();
    userServoOffset = 0;

    for (uint8_t i = 0; i < currentPatternLength; i++) {
      DBG_PRINT(F("   Checking Step "));
      DBG_PRINT(i);
      DBG_PRINT(F(" of round "));
      DBG_PRINTLN(round);

      const uint8_t expected = pattern[i];
      const uint8_t got = readUserStep();

      DBG_PRINT(F("[CHECK] expected = "));
      printStepName(expected);
      DBG_PRINT(F(", got = "));
      printStepName(got);
      DBG_PRINTLN();

      if (got != expected) {
        DBG_PRINTLN(F("[GAME] WRONG INPUT"));
        failSignal();
        servoWagGameOver();
        resetServoNeutral();

        DBG_PRINT(F("[GAME] Final score = "));
        DBG_PRINTLN(currentPatternLength - 1);
        DBG_PRINTLN(F("[GAME] Press BLACK to try again"));
        DBG_PRINTLN();
        return;
      }
    }

    DBG_PRINTLN(F("[ROUND] Correct"));
    delay(ROUND_PAUSE_MS);

    if (currentPatternLength >= MAX_PATTERN_LENGTH) {
      DBG_PRINTLN(F("[GAME] MAX PATTERN LENGTH REACHED"));
      successSignal();
      resetServoNeutral();
      DBG_PRINTLN(F("[GAME] Press BLACK to start over"));
      DBG_PRINTLN();
      return;
    }

    addRandomStep();
    round++;
  }
}

void addRandomStep() {
  if (currentPatternLength >= MAX_PATTERN_LENGTH) return;

  uint8_t step;

  while (true) {
    step = random(0, 5);  // 0=RED, 1=BLUE, 2=GREEN, 3=LEFT, 4=RIGHT

    // Colors are always valid
    if (step == STEP_RED || step == STEP_BLUE || step == STEP_GREEN) {
      break;
    }

    // LEFT only valid if not already at max left
    if (step == STEP_LEFT && patternServoOffset > SERVO_MIN_OFFSET) {
      break;
    }

    // RIGHT only valid if not already at max right
    if (step == STEP_RIGHT && patternServoOffset < SERVO_MAX_OFFSET) {
      break;
    }
  }

  pattern[currentPatternLength] = step;

  if (step == STEP_LEFT) {
    patternServoOffset--;
  } else if (step == STEP_RIGHT) {
    patternServoOffset++;
  }

  DBG_PRINT(F("[GAME] Added step "));
  DBG_PRINT(currentPatternLength);
  DBG_PRINT(F(" = "));
  printStepName(step);
  DBG_PRINTLN();

  DBG_PRINT(F("[GAME] Pattern servo offset now = "));
  DBG_PRINTLN(patternServoOffset);

  currentPatternLength++;
}

void printCurrentPattern() {
  DBG_PRINT(F("[GAME] Current pattern = "));
  for (uint8_t i = 0; i < currentPatternLength; i++) {
    printStepName(pattern[i]);
    if (i < currentPatternLength - 1) DBG_PRINT(F(", "));
  }
  DBG_PRINTLN();
}

void waitForBlackStart() {
  DBG_PRINTLN(F("[WAIT] Waiting for BLACK press"));

  while (!isPressed(BLACK_BUTTON_PIN)) {
  }
  delay(DEBOUNCE_MS);

  DBG_PRINTLN(F("[BTN] BLACK DOWN"));

  while (isPressed(BLACK_BUTTON_PIN)) {
  }
  delay(DEBOUNCE_MS);

  DBG_PRINTLN(F("[BTN] BLACK UP"));
}

void playRound(uint8_t roundLength) {
  ledOff(RED_LED_PIN);
  ledOff(BLUE_LED_PIN);
  ledOff(GREEN_LED_PIN);

  playbackServoOffset = 0;
  resetServoNeutral();
  delay(200);

  DBG_PRINT(F("[PLAYBACK] Sequence: "));
  for (uint8_t i = 0; i < roundLength; i++) {
    printStepName(pattern[i]);
    if (i < roundLength - 1) DBG_PRINT(F(", "));
  }
  DBG_PRINTLN();

  for (uint8_t i = 0; i < roundLength; i++) {
    playStep(pattern[i]);

    if (i < roundLength - 1) {
      if (isDirectionalStep(pattern[i]) && isDirectionalStep(pattern[i + 1])) {
        delay(DIRECTIONAL_CHAIN_PAUSE_MS);
      } else {
        delay(GAP_MS);
      }
    }
  }

  ledOff(RED_LED_PIN);
  ledOff(BLUE_LED_PIN);
  ledOff(GREEN_LED_PIN);

  resetServoNeutral();

  DBG_PRINTLN(F("[PLAYBACK] Waiting for user input"));
}

void playStep(uint8_t step) {
  DBG_PRINT(F("[PLAYBACK] Step = "));
  printStepName(step);
  DBG_PRINTLN();

  if (step == STEP_BLUE) {
    ledOn(BLUE_LED_PIN);
    delay(FLASH_MS);
    ledOff(BLUE_LED_PIN);
  } else if (step == STEP_RED) {
    ledOn(RED_LED_PIN);
    delay(FLASH_MS);
    ledOff(RED_LED_PIN);
  } else if (step == STEP_GREEN) {
    ledOn(GREEN_LED_PIN);
    delay(FLASH_MS);
    ledOff(GREEN_LED_PIN);
  } else if (step == STEP_LEFT) {
    moveServoLeftPlayback();
    delay(FLASH_MS);
  } else if (step == STEP_RIGHT) {
    moveServoRightPlayback();
    delay(FLASH_MS);
  }
}

uint8_t readUserStep() {
  while (true) {
    if (isPressed(BLUE_BUTTON_PIN)) {
      delay(DEBOUNCE_MS);
      if (isPressed(BLUE_BUTTON_PIN)) {
        DBG_PRINTLN(F("[BTN] BLUE DOWN"));
        ledOn(BLUE_LED_PIN);

        while (isPressed(BLUE_BUTTON_PIN)) {
        }

        ledOff(BLUE_LED_PIN);
        delay(DEBOUNCE_MS);
        DBG_PRINTLN(F("[BTN] BLUE UP"));
        return STEP_BLUE;
      }
    }

    if (isPressed(RED_BUTTON_PIN)) {
      delay(DEBOUNCE_MS);
      if (isPressed(RED_BUTTON_PIN)) {
        DBG_PRINTLN(F("[BTN] RED DOWN"));
        ledOn(RED_LED_PIN);

        while (isPressed(RED_BUTTON_PIN)) {
        }

        ledOff(RED_LED_PIN);
        delay(DEBOUNCE_MS);
        DBG_PRINTLN(F("[BTN] RED UP"));
        return STEP_RED;
      }
    }

    if (isPressed(GREEN_BUTTON_PIN)) {
      delay(DEBOUNCE_MS);
      if (isPressed(GREEN_BUTTON_PIN)) {
        DBG_PRINTLN(F("[BTN] GREEN DOWN"));
        ledOn(GREEN_LED_PIN);

        while (isPressed(GREEN_BUTTON_PIN)) {
        }

        ledOff(GREEN_LED_PIN);
        delay(DEBOUNCE_MS);
        DBG_PRINTLN(F("[BTN] GREEN UP"));
        return STEP_GREEN;
      }
    }

    if (isPressed(WHITE_BUTTON_PIN)) {
      delay(DEBOUNCE_MS);
      if (isPressed(WHITE_BUTTON_PIN)) {
        DBG_PRINTLN(F("[BTN] WHITE DOWN"));
        moveServoLeftUser();

        while (isPressed(WHITE_BUTTON_PIN)) {
        }

        delay(DEBOUNCE_MS);
        DBG_PRINTLN(F("[BTN] WHITE UP"));
        return STEP_LEFT;
      }
    }

    if (isPressed(YELLOW_BUTTON_PIN)) {
      delay(DEBOUNCE_MS);
      if (isPressed(YELLOW_BUTTON_PIN)) {
        DBG_PRINTLN(F("[BTN] YELLOW DOWN"));
        moveServoRightUser();

        while (isPressed(YELLOW_BUTTON_PIN)) {
        }

        delay(DEBOUNCE_MS);
        DBG_PRINTLN(F("[BTN] YELLOW UP"));
        return STEP_RIGHT;
      }
    }
  }
}

void failSignal() {
  for (uint8_t i = 0; i < 3; i++) {
    ledOn(RED_LED_PIN);
    delay(120);
    ledOff(RED_LED_PIN);
    delay(120);
  }
}

void successSignal() {
  for (uint8_t i = 0; i < 2; i++) {
    ledOn(BLUE_LED_PIN);
    delay(120);
    ledOff(BLUE_LED_PIN);
    delay(120);
  }
}

void servoWagGameOver() {
  DBG_PRINTLN(F("[SERVO] GAME OVER WAG"));

  for (uint8_t i = 0; i < 3; i++) {
    writeServoOffset(SERVO_MIN_OFFSET);
    delay(WAG_PAUSE_MS);
    writeServoOffset(SERVO_MAX_OFFSET);
    delay(WAG_PAUSE_MS);
  }

  writeServoOffset(0);
  delay(WAG_PAUSE_MS);

  userServoOffset = 0;
  playbackServoOffset = 0;
}

void resetServoNeutral() {
  userServoOffset = 0;
  playbackServoOffset = 0;
  writeServoOffset(0);
  DBG_PRINTLN(F("[SERVO] NEUTRAL"));
}

void writeServoOffset(int8_t offset) {
  int16_t angle = SERVO_CENTER_DEGREES + (offset * SERVO_STEP_DEGREES);

  if (angle < 0) angle = 0;
  if (angle > 180) angle = 180;

  gameServo.write(angle);

  DBG_PRINT(F("[SERVO] angle = "));
  DBG_PRINT(angle);
  DBG_PRINT(F(" (offset "));
  DBG_PRINT(offset);
  DBG_PRINTLN(F(")"));
}

void moveServoLeftPlayback() {
  if (playbackServoOffset > SERVO_MIN_OFFSET) {
    playbackServoOffset--;
  }

  writeServoOffset(playbackServoOffset);
  DBG_PRINTLN(F("[SERVO] PLAYBACK LEFT (WHITE)"));
}

void moveServoRightPlayback() {
  if (playbackServoOffset < SERVO_MAX_OFFSET) {
    playbackServoOffset++;
  }

  writeServoOffset(playbackServoOffset);
  DBG_PRINTLN(F("[SERVO] PLAYBACK RIGHT (YELLOW)"));
}

void moveServoLeftUser() {
  if (userServoOffset > SERVO_MIN_OFFSET) {
    userServoOffset--;
    writeServoOffset(userServoOffset);
    DBG_PRINTLN(F("[SERVO] USER LEFT (WHITE)"));
  } else {
    DBG_PRINTLN(F("[SERVO] USER LEFT (WHITE) blocked at limit"));
  }
}

void moveServoRightUser() {
  if (userServoOffset < SERVO_MAX_OFFSET) {
    userServoOffset++;
    writeServoOffset(userServoOffset);
    DBG_PRINTLN(F("[SERVO] USER RIGHT (YELLOW)"));
  } else {
    DBG_PRINTLN(F("[SERVO] USER RIGHT (YELLOW) blocked at limit"));
  }
}

bool isDirectionalStep(uint8_t step) {
  return step == STEP_LEFT || step == STEP_RIGHT;
}

bool isPressed(uint8_t pin) {
  return digitalRead(pin) == HIGH; //DO NOT CHANGE THIS
}

void printStepName(uint8_t step) {
  switch (step) {
    case STEP_RED:   DBG_PRINT(F("RED")); break;
    case STEP_BLUE:  DBG_PRINT(F("BLUE")); break;
    case STEP_GREEN: DBG_PRINT(F("GREEN")); break;
    case STEP_LEFT:  DBG_PRINT(F("LEFT (WHITE)")); break;
    case STEP_RIGHT: DBG_PRINT(F("RIGHT (YELLOW)")); break;
    default:         DBG_PRINT(F("UNKNOWN")); break;
  }
}

void ledOn(uint8_t pin) {
  digitalWrite(pin, HIGH);   // DO NOT CHANGE THIS!

  if (pin == BLUE_LED_PIN) {
    if (!blueLedIsOn) {
      blueLedIsOn = true;
      DBG_PRINTLN(F("[LED] BLUE ON"));
    }
  }
}

void ledOff(uint8_t pin) {
  digitalWrite(pin, LOW);  // DO NOT CHANGE THIS!

  if (pin == BLUE_LED_PIN) {
    if (blueLedIsOn) {
      blueLedIsOn = false;
      DBG_PRINTLN(F("[LED] BLUE OFF"));
    }
  }
}