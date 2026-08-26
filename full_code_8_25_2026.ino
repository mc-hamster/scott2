#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>
// =====================================================
// RELAY SETTINGS
// =====================================================


const byte relayPins[] = {
  28,29,22,23,24,25,26,27,
  38,40,42,44,46,48,50,52,
  39,41,43,45,47,49,51,53
};
const byte NUM_RELAYS = (sizeof(relayPins));

const unsigned long relayDwellTimeMS = 500;
// =====================================================
// OLED SETTINGS
// =====================================================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  OLED_RESET
);

// =====================================================
// BUTTON PINS
// =====================================================

#define EXTERNAL_TOGGLE_BURST 7
#define EXTERNAL_TOGGLE_SHOT 5

#define EXTERNAL_BUTTON_FIRE_RANDOM 12
#define EXTERNAL_BUTTON_FIRE_NORMAL 13
#define EXTERNAL_BUTTON_FIRE_RED 11
#define EXTERNAL_BUTTON_FIRE_GREEN 10
#define EXTERNAL_BUTTON_FIRE_PINK 9

#define INTERNAL_BUTTON_NEXT 14
#define INTERNAL_BUTTON_INCREMENT 15
#define INTERNAL_BUTTON_DECREMENT 18
#define INTERNAL_BUTTON_SAVE 19

const int NUM_BUTTONS = 9;

// =====================================================
// VARIABLES
// =====================================================
// FireTimingValues are stored as hundredths of a second.

#define NUM_TIMING_VARIABLES 8
#define MIN_VALUE 0
#define MAX_VALUE 125

uint8_t FireTimingValues[NUM_TIMING_VARIABLES] = {
  0,  // RapidMix_Start 
  25,  // RapidMix_End
  25,  // RichMix_Start
  50,  // RichMix_End
  37,  // ColorMix_Start
  50,  // ColorMix_End
  50,  // I_Start
  60   // I_End
};

// Currently selected variable, used for display
uint8_t selected_timing_Variable = 0;

// True only after the OLED's 1 KB display buffer was allocated and the
// display answered at its I2C address.
bool oledAvailable = false;

// =====================================================
// EEPROM SETTINGS
// =====================================================

// EEPROM address used to indicate that valid data exists
#define EEPROM_MAGIC_ADDRESS 0

// EEPROM address where the five FireTimingValues are stored
#define EEPROM_FireTimingValues_ADDRESS 1

// Change this value if you ever want to invalidate
// previously stored EEPROM settings.
#define EEPROM_MAGIC 0x5A


// =====================================================
// BUTTON DEBOUNCE
// =====================================================
const unsigned long DEBOUNCE_TIME = 50;

// --------------------------------------------------
// Toggle state variables
// --------------------------------------------------

int Toggle_BurstState;
int Toggle_ShotQtyState;

int lastToggle_BurstReading;
int lastToggle_ShotQtyReading;

unsigned long lastToggle_BurstChange = 0;
unsigned long lastToggle_ShotQtyChange = 0;


bool lastButtonState[9] = {
  HIGH,
  HIGH,
  HIGH,
  HIGH,
  HIGH,
  HIGH,
  HIGH,
  HIGH,
  HIGH
};

// A held button must generate only one press event, without blocking the loop.
bool buttonPressHandled[NUM_BUTTONS] = { false };

unsigned long lastDebounceTime[9] = {
  0, 0, 0, 0, 0,0,0, 0,0
};

// Fire-control state. A sequence is either idle or running; a new fire
// request is intentionally ignored while a sequence is already running.
enum class FireState : uint8_t {
  IDLE,
  RUNNING
};

FireState fireState = FireState::IDLE;
uint8_t eventIndex = 0;
unsigned long fireStartTime = 0;
uint8_t activeFireTimingValues[NUM_TIMING_VARIABLES];
uint8_t activeColorButton = 0;

/*
int buttonState[NUM_BUTTONS];
int lastButtonReading[NUM_BUTTONS];
unsigned long lastButtonChange[NUM_BUTTONS];
*/
// =====================================================
// DISPLAY MESSAGE TIMING
// =====================================================

bool showingSavedMessage = false;
unsigned long savedMessageTime = 0;

const unsigned long SAVED_MESSAGE_DURATION = 1000;


// =====================================================
// FUNCTION PROTOTYPES
// =====================================================

void loadEEPROM();
void saveCurrentVariable();
void displayCurrentVariable();
void displaySavedMessage();
bool ButtonPressed(uint8_t buttonIndex, uint8_t pin);
void printValue(uint8_t value);
void updateToggles(unsigned long currentTime);
void handleExternalButtonPress(int buttonIndex);
void printSelectedVariableName();
void printExternalButtonName(int buttonIndex);
void firecontrol(uint8_t buttonIndex);
void requestFireSequence(uint8_t buttonIndex);
void turnFireRelaysOff();
bool fireTimingIsValid();

// =====================================================
// SETUP
// =====================================================

void setup() {
  Serial.begin(9600);
  Serial.println(F("Starting setup"));
  //resetEEPROM();//comment this out unless you want to reset saved FireTimingValues during each arduino startup
  
  
  // ---------------------------------------------------
  // Configure relays
  // ---------------------------------------------------
  Serial.print("NUM_RELAYS: ");
  Serial.println(NUM_RELAYS);
  // Set all relay pins as outputs and turn all relays OFF
  for (byte i = 0; i < NUM_RELAYS; i++) {
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], HIGH);  // OFF for active-low relay board
    Serial.print(i);
    Serial.print(" ");
  }
  
  // ---------------------------------------------------
  // Configure buttons
  // ---------------------------------------------------

  pinMode(EXTERNAL_BUTTON_FIRE_RANDOM, INPUT_PULLUP);
  pinMode(EXTERNAL_BUTTON_FIRE_NORMAL, INPUT_PULLUP);
  pinMode(EXTERNAL_BUTTON_FIRE_RED, INPUT_PULLUP);
  pinMode(EXTERNAL_BUTTON_FIRE_GREEN, INPUT_PULLUP);
  pinMode(EXTERNAL_BUTTON_FIRE_PINK, INPUT_PULLUP);

  pinMode(INTERNAL_BUTTON_NEXT, INPUT_PULLUP);
  pinMode(INTERNAL_BUTTON_INCREMENT, INPUT_PULLUP);
  pinMode(INTERNAL_BUTTON_DECREMENT, INPUT_PULLUP);
  pinMode(INTERNAL_BUTTON_SAVE, INPUT_PULLUP);

  pinMode(EXTERNAL_TOGGLE_BURST, INPUT_PULLUP);
  pinMode(EXTERNAL_TOGGLE_SHOT, INPUT_PULLUP);
  Serial.println(F("1"));
  Toggle_BurstState = digitalRead(EXTERNAL_TOGGLE_BURST);
  lastToggle_BurstReading = Toggle_BurstState;

  Toggle_ShotQtyState = digitalRead(EXTERNAL_TOGGLE_SHOT);
  lastToggle_ShotQtyReading = Toggle_ShotQtyState;
  Serial.println(F("2"));
  // ---------------------------------------------------
  // Start OLED
  // ---------------------------------------------------
  
  oledAvailable = display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  if (!oledAvailable) {
    // Do not disable the controls just because the OLED is absent or failed.
    Serial.println(F("OLED initialization failed; continuing without display."));
  } else {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
  }
  Serial.println(F("3"));
  /**/
  // ---------------------------------------------------
  // Load saved FireTimingValues
  // ---------------------------------------------------

  loadEEPROM();


  // ---------------------------------------------------
  // Show initial variable
  // ---------------------------------------------------

  displayCurrentVariable();
  Serial.println(F("Setup complete"));
}


// =====================================================
// MAIN LOOP
// =====================================================

void loop() {
  updateToggles(millis());


  // ---------------------------------------------------
  // Internal Button 1 - NEXT VARIABLE
  // ---------------------------------------------------

  if (ButtonPressed(0, INTERNAL_BUTTON_NEXT)) {
    Serial.println(0);
    selected_timing_Variable++;

    if (selected_timing_Variable >= NUM_TIMING_VARIABLES) {
      selected_timing_Variable = 0;
    }

    showingSavedMessage = false;

    displayCurrentVariable();
  }


  // ---------------------------------------------------
  // Internal Button 2 - INCREMENT
  // ---------------------------------------------------

  if (ButtonPressed(1, INTERNAL_BUTTON_INCREMENT)) {
    Serial.println(1);
    if (FireTimingValues[selected_timing_Variable] < MAX_VALUE) {

      FireTimingValues[selected_timing_Variable]++;

      displayCurrentVariable();
    }
  }


  // ---------------------------------------------------
  // Internal Button 3 - DECREMENT
  // ---------------------------------------------------

  if (ButtonPressed(2, INTERNAL_BUTTON_DECREMENT)) {
    Serial.println(2);
    if (FireTimingValues[selected_timing_Variable] > MIN_VALUE) {

      FireTimingValues[selected_timing_Variable]--;

      displayCurrentVariable();
    }
  }


  // ---------------------------------------------------
  // Internal Button 4 - SAVE
  // ---------------------------------------------------

  if (ButtonPressed(3, INTERNAL_BUTTON_SAVE)) {
    Serial.println(3);
    saveCurrentVariable();

    displaySavedMessage();

    showingSavedMessage = true;
    savedMessageTime = millis();
  }


  // ---------------------------------------------------
  // Return to normal display after "Value Saved"
  // ---------------------------------------------------

  if (showingSavedMessage) {
    
    if (millis() - savedMessageTime >= SAVED_MESSAGE_DURATION) {

      showingSavedMessage = false;

      displayCurrentVariable();
    }
  }

  // ---------------------------------------------------
  // External Button 1 - Random
  // ---------------------------------------------------

  if (ButtonPressed(4, EXTERNAL_BUTTON_FIRE_RANDOM)) {
    requestFireSequence(0);
  }

  if (ButtonPressed(5, EXTERNAL_BUTTON_FIRE_NORMAL)) {
    requestFireSequence(1);
  }

  if (ButtonPressed(6, EXTERNAL_BUTTON_FIRE_RED)) {
    requestFireSequence(2);
  }

  if (ButtonPressed(7, EXTERNAL_BUTTON_FIRE_GREEN)) {
    requestFireSequence(3);
  }

  if (ButtonPressed(8, EXTERNAL_BUTTON_FIRE_PINK)) {
    requestFireSequence(4);
  }

  // Keep the active sequence running
  firecontrol(activeColorButton);

}

void handleExternalButtonPress(int buttonIndex) {

  Serial.print(F("Button "));
  printExternalButtonName(buttonIndex);

  Serial.print(F(" PRESSED | Toggle Burst: "));

  if (Toggle_BurstState == LOW) {
    Serial.print(F("Short"));
  }
  else {
    Serial.print(F("Long"));
  }

  Serial.print(F(" | Toggle Shot-Qty: "));

  if (Toggle_ShotQtyState == LOW) {
    Serial.println(F("Single"));
  }
  else {
    Serial.println(F("Double"));
  }
}

void requestFireSequence(uint8_t buttonIndex) {
  if (fireState == FireState::RUNNING) {
    Serial.println(F("Fire request ignored: sequence already running."));
    return;
  }

  if (!fireTimingIsValid()) {
    Serial.println(F("Fire request rejected: a start time exceeds its end time."));
    return;
  }

  // Snapshot the settings. Changes on the internal panel affect the next
  // firing only, never the sequence already in progress.
  for (uint8_t i = 0; i < NUM_TIMING_VARIABLES; i++) {
    activeFireTimingValues[i] = FireTimingValues[i];
  }

  turnFireRelaysOff();
  handleExternalButtonPress(buttonIndex);
  activeColorButton = buttonIndex;
  eventIndex = 0;
  fireStartTime = millis();
  fireState = FireState::RUNNING;
  Serial.println(F("Fire sequence started."));
}

bool fireTimingIsValid() {
  for (uint8_t i = 0; i < NUM_TIMING_VARIABLES; i += 2) {
    if (FireTimingValues[i] > FireTimingValues[i + 1]) {
      return false;
    }
  }
  return true;
}

void turnFireRelaysOff() {
  digitalWrite(relayPins[4], HIGH);   // RapidMix off
  digitalWrite(relayPins[8], HIGH);   // RichMix off
  // Color is selected by the fire button, so all five possible color
  // outputs must be forced off for a safe baseline.
  for (uint8_t i = 9; i <= 13; i++) {
    digitalWrite(relayPins[i], HIGH);
  }
  digitalWrite(relayPins[14], HIGH);  // Igniter off
}

void firecontrol(uint8_t buttonIndex) {
  if (fireState != FireState::RUNNING) {
    return;
  }

  unsigned long elapsed = millis() - fireStartTime;

  // Process all events that are now due
  while (eventIndex < NUM_TIMING_VARIABLES &&
         elapsed >= (unsigned long)activeFireTimingValues[eventIndex] * 10UL) {

    switch (eventIndex) {
      case 0:
        Serial.println("RapidMix_Start");
        digitalWrite(relayPins[4], LOW); 
        break;

      case 1:
        Serial.println("RapidMix_End");
        digitalWrite(relayPins[4], HIGH);
        break;

      case 2:
        Serial.println("RichMix_Start");
        digitalWrite(relayPins[8], LOW);
        break;

      case 3:
        Serial.println("RichMix_End");
        digitalWrite(relayPins[8], HIGH);
        break;

      case 4:
        Serial.println("ColorMix_Start");
        printExternalButtonName(buttonIndex);
        switch (buttonIndex) {
          case 0: digitalWrite(relayPins[9], LOW); break;
          case 1: digitalWrite(relayPins[10], LOW); break;
          case 2: digitalWrite(relayPins[11], LOW); break;
          case 3: digitalWrite(relayPins[12], LOW); break;
          case 4: digitalWrite(relayPins[13], LOW); break;
        }
        break;

      case 5:
        Serial.println("ColorMix_End");
        switch (buttonIndex) {
          case 0: digitalWrite(relayPins[9], HIGH); break;
          case 1: digitalWrite(relayPins[10], HIGH); break;
          case 2: digitalWrite(relayPins[11], HIGH); break;
          case 3: digitalWrite(relayPins[12], HIGH); break;
          case 4: digitalWrite(relayPins[13], HIGH); break;
        }
        break;

      case 6:
        Serial.println("I_Start");
        digitalWrite(relayPins[14], LOW);
        break;

      case 7:
        Serial.println("I_End");
        digitalWrite(relayPins[14], HIGH);
        break;
    }

    eventIndex++;
  }
 
 // Sequence finished
  if (eventIndex >= NUM_TIMING_VARIABLES) {
    turnFireRelaysOff();
    fireState = FireState::IDLE;
    Serial.println(F("Fire sequence complete."));
  }
  
}

//Internal Panel Functions
// =====================================================
// EEPROM LOAD
// =====================================================


void loadEEPROM() {

  // Check whether EEPROM has been initialized

  if (EEPROM.read(EEPROM_MAGIC_ADDRESS) == EEPROM_MAGIC) {

    // Valid EEPROM data exists

    for (uint8_t i = 0; i < NUM_TIMING_VARIABLES; i++) {

      FireTimingValues[i] = EEPROM.read(
        EEPROM_FireTimingValues_ADDRESS + i
      );

      // Safety check in case EEPROM contains
      // an invalid value.

      if (FireTimingValues[i] > MAX_VALUE) {
        FireTimingValues[i] = 0;
      }
    }

  } else {

    // EEPROM has never been initialized.
    // Start all FireTimingValues at zero.

    for (uint8_t i = 0; i < NUM_TIMING_VARIABLES; i++) {
      FireTimingValues[i] = 0;
    }
  }
}


// =====================================================
// EEPROM SAVE
// =====================================================

void saveCurrentVariable() {

  // Write the selected variable to EEPROM.

  EEPROM.update(
    EEPROM_FireTimingValues_ADDRESS + selected_timing_Variable,
    FireTimingValues[selected_timing_Variable]
  );

  // Mark EEPROM as initialized.

  EEPROM.update(
    EEPROM_MAGIC_ADDRESS,
    EEPROM_MAGIC
  );
}


// =====================================================
// DISPLAY CURRENT VARIABLE
// =====================================================

void displayCurrentVariable() {
  if (!oledAvailable) {
    return;
  }

  display.clearDisplay();


  // ---------------------------------------------------
  // Variable name
  // ---------------------------------------------------

  display.setTextSize(2);

  display.setCursor(0, 0);

  printSelectedVariableName();


  // ---------------------------------------------------
  // Value
  // ---------------------------------------------------

  display.setTextSize(2);

  //display.setCursor(0, 28);

  display.setCursor(0, 48);

  printValue(FireTimingValues[selected_timing_Variable]);

  display.print(F(" sec"));


  // ---------------------------------------------------
  // Send buffer to OLED
  // ---------------------------------------------------

  display.display();
}


// =====================================================
// DISPLAY SAVED MESSAGE
// =====================================================

void displaySavedMessage() {
  if (!oledAvailable) {
    return;
  }

  display.clearDisplay();

  display.setTextSize(2);

  display.setCursor(10, 10);

  display.print(F("Value"));

  display.setCursor(10, 35);

  display.print(F("Saved"));

  display.display();
}


// =====================================================
// PRINT VALUE
// =====================================================
//
// Converts integer hundredths into:
//
// 0  -> 0.00
// 1  -> 0.01
// 2  -> 0.02
// ...
// 25 -> 0.25
//

void printValue(uint8_t value) {

  display.print(F("0."));

  if (value < 10) {
    display.print(F("0"));
  }

  display.print(value);
}


// =====================================================
// BUTTON DEBOUNCE
// =====================================================
//
// Returns TRUE once when a button is pressed.
//
// Buttons use INPUT_PULLUP, therefore:
//
// HIGH = not pressed
// LOW  = pressed
//

bool ButtonPressed(uint8_t buttonIndex, uint8_t pin) {

  bool reading = digitalRead(pin);


  // If the physical state changed,
  // restart debounce timer.

  if (reading != lastButtonState[buttonIndex]) {

    lastDebounceTime[buttonIndex] = millis();

    lastButtonState[buttonIndex] = reading;
  }


  // Generate one event for a stable press.  Unlike the prior implementation,
  // this never waits for a button to be released, so one stuck button cannot
  // freeze the other controls.
  if ((millis() - lastDebounceTime[buttonIndex]) > DEBOUNCE_TIME) {
    if (reading == LOW && !buttonPressHandled[buttonIndex]) {
      buttonPressHandled[buttonIndex] = true;
      return true;
    }

    if (reading == HIGH) {
      buttonPressHandled[buttonIndex] = false;
    }
  }

  return false;
}

void updateToggles(unsigned long currentTime) {
  int burstReading = digitalRead(EXTERNAL_TOGGLE_BURST);
  if (burstReading != lastToggle_BurstReading) {
    lastToggle_BurstChange = currentTime;
    lastToggle_BurstReading = burstReading;
  }
  if (currentTime - lastToggle_BurstChange > DEBOUNCE_TIME) {
    Toggle_BurstState = burstReading;
  }

  int shotReading = digitalRead(EXTERNAL_TOGGLE_SHOT);
  if (shotReading != lastToggle_ShotQtyReading) {
    lastToggle_ShotQtyChange = currentTime;
    lastToggle_ShotQtyReading = shotReading;
  }
  if (currentTime - lastToggle_ShotQtyChange > DEBOUNCE_TIME) {
    Toggle_ShotQtyState = shotReading;
  }
}

void printSelectedVariableName() {
  switch (selected_timing_Variable) {
    case 0: display.print(F("RapidMix  Start Time")); break;
    case 1: display.print(F("RapidMix  End Time")); break;
    case 2: display.print(F("RichMix   Start Time")); break;
    case 3: display.print(F("RichMix   End Time")); break;
    case 4: display.print(F("ColorMix  Start Time")); break;
    case 5: display.print(F("ColorMix  End Time")); break;
    case 6: display.print(F("Igniter   Start Time")); break;
    case 7: display.print(F("Igniter   End Time")); break;
  }
}

void printExternalButtonName(int buttonIndex) {
  switch (buttonIndex) {
    case 0: Serial.print(F("Random")); break;
    case 1: Serial.print(F("Normal")); break;
    case 2: Serial.print(F("Red")); break;
    case 3: Serial.print(F("Green")); break;
    case 4: Serial.print(F("Pink")); break;
  }
}

void resetEEPROM() {
  uint8_t defaultFireTimingValues[NUM_TIMING_VARIABLES] = {
    0,   // RapidMix_Start
    25,  // RapidMix_End
    25,  // RichMix_Start
    50,  // RichMix_End
    37,  // ColorMix_Start
    50,  // ColorMix_End
    50,  // I_Start
    60   // I_End
  };

  for (uint8_t i = 0; i < NUM_TIMING_VARIABLES; i++) {
    EEPROM.update(EEPROM_FireTimingValues_ADDRESS + i, defaultFireTimingValues[i]);
  }

  EEPROM.update(EEPROM_MAGIC_ADDRESS, EEPROM_MAGIC);
}
