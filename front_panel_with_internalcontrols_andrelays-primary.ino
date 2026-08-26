#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>
// =====================================================
// RELAY SETTINGS
// =====================================================


const byte relayPins[] = {
  20,21,22,23,24,25,26,27,
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
void FireControl(int buttonIndex);

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
  /*
  oledAvailable = display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  if (!oledAvailable) {
    // Do not disable the controls just because the OLED is absent or failed.
    Serial.println(F("OLED initialization failed; continuing without display."));
  } else {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
  }
  Serial.println(F("3"));
  */
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
    Serial.println("e");
    handleExternalButtonPress(0);
    FireControl(0);
  }
  if (ButtonPressed(5, EXTERNAL_BUTTON_FIRE_NORMAL)) {
    Serial.println("e");
    handleExternalButtonPress(1);
    FireControl(1);
  }
  if (ButtonPressed(6, EXTERNAL_BUTTON_FIRE_RED)) {
    Serial.println("e");
    handleExternalButtonPress(2);
    FireControl(2);
  }
  if (ButtonPressed(7, EXTERNAL_BUTTON_FIRE_GREEN)) {
    Serial.println("e");
    handleExternalButtonPress(3);
    FireControl(3);
  }
  if (ButtonPressed(8, EXTERNAL_BUTTON_FIRE_PINK)) {
    Serial.println("e");
    handleExternalButtonPress(4);
    FireControl(4);
  }

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

void FireControl(int buttonIndex) {
  for (int i=0; i<8; i++){
    switch (i) {
      case 0: Serial.print(F("RapidMix  Start Time: "));break;
      case 1: Serial.print(F("RapidMix  End Time: "));break;
      case 2: Serial.print(F("RichMix   Start Time: "));break;
      case 3: Serial.print(F("RichMix   End Time: "));break;
      case 4: Serial.print(F("ColorMix  Start Time: "));break;
      case 5: Serial.print(F("ColorMix  End Time: "));break;
      case 6: Serial.print(F("Igniter   Start Time: "));break;
      case 7: Serial.print(F("Igniter   End Time: "));break;
    }
    Serial.println(FireTimingValues[i]);
  }  
    Serial.print("Turning ON relays ");
   
    digitalWrite(relayPins[4], LOW);   // ON
    delay(relayDwellTimeMS);

    digitalWrite(relayPins[4], HIGH);  // OFF
    delay(relayDwellTimeMS);

    digitalWrite(relayPins[8], LOW);   // ON
    delay(relayDwellTimeMS);

    digitalWrite(relayPins[8], HIGH);  // OFF
    delay(relayDwellTimeMS);

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
