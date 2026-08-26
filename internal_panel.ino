#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>

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

#define INTERNAL_BUTTON_NEXT 10
#define INTERNAL_BUTTON_INCREMENT 11
#define INTERNAL_BUTTON_DECREMENT 12
#define INTERNAL_BUTTON_SAVE 13

// =====================================================
// VARIABLES
// =====================================================

// Five variables: A, B, C, D, E
//
// Values are stored as hundredths of a second.
//
// 0   = 0.00
// 1   = 0.01
// ...
// 25  = 0.25

#define NUM_VARIABLES 8
#define MIN_VALUE 0
#define MAX_VALUE 125

uint8_t values[NUM_VARIABLES] = {
  0,  // RapidMix_Start 
  25,  // RapidMix_End
  25,  // RichMix_Start
  50,  // RichMix_End
  37,  // ColorMix_Start
  50,  // ColorMix_End
  50,  // I_Start
  60   // I_End
};

// Currently selected variable
uint8_t selectedVariable = 0;

// Variable names
const String variableNames[NUM_VARIABLES] = {
  "RapidMix  Start Time",
  "RapidMix  End Time",
  "RichMix   Start Time",
  "RichMix   End Time",
  "ColorMix  Start Time",
  "ColorMix  End Time",
  "I   Start Time",
  "I   End Time"
};

// =====================================================
// EEPROM SETTINGS
// =====================================================

// EEPROM address used to indicate that valid data exists
#define EEPROM_MAGIC_ADDRESS 0

// EEPROM address where the five values are stored
#define EEPROM_VALUES_ADDRESS 1

// Change this value if you ever want to invalidate
// previously stored EEPROM settings.
#define EEPROM_MAGIC 0x5A


// =====================================================
// BUTTON DEBOUNCE
// =====================================================

const unsigned long DEBOUNCE_TIME = 40;

bool lastButtonState[4] = {
  HIGH,
  HIGH,
  HIGH,
  HIGH
};

unsigned long lastDebounceTime[4] = {
  0,
  0,
  0,
  0
};


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
bool buttonPressed(uint8_t buttonIndex, uint8_t pin);
void printValue(uint8_t value);


// =====================================================
// SETUP
// =====================================================

void setup() {

  //resetEEPROM();//comment this out unless you want to reset saved values during each arduino startup

  
  // ---------------------------------------------------
  // Configure buttons
  // ---------------------------------------------------

  pinMode(INTERNAL_BUTTON_NEXT, INPUT_PULLUP);
  pinMode(INTERNAL_BUTTON_INCREMENT, INPUT_PULLUP);
  pinMode(INTERNAL_BUTTON_DECREMENT, INPUT_PULLUP);
  pinMode(INTERNAL_BUTTON_SAVE, INPUT_PULLUP);


  // ---------------------------------------------------
  // Start OLED
  // ---------------------------------------------------
  //Serial.println("2a");

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
  //Serial.println("2a");

    // OLED initialization failed.
    // Stop here.

    while (true) {
    }
  }
  //Serial.println("2a");

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  //Serial.println("2a");


  // ---------------------------------------------------
  // Load saved values
  // ---------------------------------------------------

  loadEEPROM();


  // ---------------------------------------------------
  // Show initial variable
  // ---------------------------------------------------

  displayCurrentVariable();
}


// =====================================================
// MAIN LOOP
// =====================================================

void loop() {

  // ---------------------------------------------------
  // Internal Button 1 - NEXT VARIABLE
  // ---------------------------------------------------

  if (InternalButtonPressed(0, INTERNAL_BUTTON_NEXT)) {

    selectedVariable++;

    if (selectedVariable >= NUM_VARIABLES) {
      selectedVariable = 0;
    }

    showingSavedMessage = false;

    displayCurrentVariable();
  }


  // ---------------------------------------------------
  // Internal Button 2 - INCREMENT
  // ---------------------------------------------------

  if (InternalButtonPressed(1, INTERNAL_BUTTON_INCREMENT)) {

    if (values[selectedVariable] < MAX_VALUE) {

      values[selectedVariable]++;

      displayCurrentVariable();
    }
  }


  // ---------------------------------------------------
  // Internal Button 3 - DECREMENT
  // ---------------------------------------------------

  if (InternalButtonPressed(2, INTERNAL_BUTTON_DECREMENT)) {

    if (values[selectedVariable] > MIN_VALUE) {

      values[selectedVariable]--;

      displayCurrentVariable();
    }
  }


  // ---------------------------------------------------
  // Internal Button 4 - SAVE
  // ---------------------------------------------------

  if (InternalButtonPressed(3, INTERNAL_BUTTON_SAVE)) {

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
}

//Internal Panel Functions
// =====================================================
// EEPROM LOAD
// =====================================================

void loadEEPROM() {

  // Check whether EEPROM has been initialized

  if (EEPROM.read(EEPROM_MAGIC_ADDRESS) == EEPROM_MAGIC) {

    // Valid EEPROM data exists

    for (uint8_t i = 0; i < NUM_VARIABLES; i++) {

      values[i] = EEPROM.read(
        EEPROM_VALUES_ADDRESS + i
      );

      // Safety check in case EEPROM contains
      // an invalid value.

      if (values[i] > MAX_VALUE) {
        values[i] = 0;
      }
    }

  } else {

    // EEPROM has never been initialized.
    // Start all values at zero.

    for (uint8_t i = 0; i < NUM_VARIABLES; i++) {
      values[i] = 0;
    }
  }
}


// =====================================================
// EEPROM SAVE
// =====================================================

void saveCurrentVariable() {

  // Write the selected variable to EEPROM.

  EEPROM.update(
    EEPROM_VALUES_ADDRESS + selectedVariable,
    values[selectedVariable]
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

  display.clearDisplay();


  // ---------------------------------------------------
  // Variable name
  // ---------------------------------------------------

  display.setTextSize(2);

  display.setCursor(0, 0);

  //display.print("Variable ");

  display.print(variableNames[selectedVariable]);


  // ---------------------------------------------------
  // Value
  // ---------------------------------------------------

  display.setTextSize(2);

  //display.setCursor(0, 28);

  //display.print("Value:");


  display.setCursor(0, 48);

  printValue(values[selectedVariable]);

  display.print(" sec");


  // ---------------------------------------------------
  // Send buffer to OLED
  // ---------------------------------------------------

  display.display();
}


// =====================================================
// DISPLAY SAVED MESSAGE
// =====================================================

void displaySavedMessage() {

  display.clearDisplay();

  display.setTextSize(2);

  display.setCursor(10, 10);

  display.print("Value");

  display.setCursor(10, 35);

  display.print("Saved");

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

  display.print("0.");

  if (value < 10) {
    display.print("0");
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

bool InternalButtonPressed(uint8_t buttonIndex, uint8_t pin) {

  bool reading = digitalRead(pin);


  // If the physical state changed,
  // restart debounce timer.

  if (reading != lastButtonState[buttonIndex]) {

    lastDebounceTime[buttonIndex] = millis();

    lastButtonState[buttonIndex] = reading;
  }


  // Has the state remained stable long enough?

  if ((millis() - lastDebounceTime[buttonIndex])
      > DEBOUNCE_TIME) {

    // Button is currently pressed

    if (reading == LOW) {

      // Wait until button is released before
      // allowing another press.

      while (digitalRead(pin) == LOW) {
        delay(1);
      }

      return true;
    }
  }

  return false;
}

void resetEEPROM() {
  uint8_t defaultValues[NUM_VARIABLES] = {
    0,   // RapidMix_Start
    25,  // RapidMix_End
    25,  // RichMix_Start
    50,  // RichMix_End
    37,  // ColorMix_Start
    50,  // ColorMix_End
    50,  // I_Start
    60   // I_End
  };

  for (uint8_t i = 0; i < NUM_VARIABLES; i++) {
    EEPROM.update(EEPROM_VALUES_ADDRESS + i, defaultValues[i]);
  }

  EEPROM.update(EEPROM_MAGIC_ADDRESS, EEPROM_MAGIC);
}
