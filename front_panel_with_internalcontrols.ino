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

#define EXTERNAL_TOGGLE_BURST 2
#define EXTERNAL_TOGGLE_SHOT 3

#define EXTERNAL_BUTTON_FIRE_RANDOM 4
#define EXTERNAL_BUTTON_FIRE_NORMAL 5
#define EXTERNAL_BUTTON_FIRE_RED 6
#define EXTERNAL_BUTTON_FIRE_GREEN 7
#define EXTERNAL_BUTTON_FIRE_PINK 8

#define INTERNAL_BUTTON_NEXT 10
#define INTERNAL_BUTTON_INCREMENT 11
#define INTERNAL_BUTTON_DECREMENT 12
#define INTERNAL_BUTTON_SAVE 13

const int buttons[] = {4, 5, 6, 7, 8, 10,11,12,13};
const int NUM_BUTTONS = 9;
const String ExternalButtonNames[5] = {
  "Random",
  "Normal",
  "Red",
  "Green",
  "Pink"
};

// =====================================================
// VARIABLES
// =====================================================
// Values are stored as hundredths of a second.

#define NUM_TIMING_VARIABLES 8
#define MIN_VALUE 0
#define MAX_VALUE 125

uint8_t values[NUM_TIMING_VARIABLES] = {
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

// Variable names
const String variableNames[NUM_TIMING_VARIABLES] = {
  "RapidMix  Start Time",
  "RapidMix  End Time",
  "RichMix   Start Time",
  "RichMix   End Time",
  "ColorMix  Start Time",
  "ColorMix  End Time",
  "Igniter   Start Time",
  "Igniter   End Time"
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
void updateButtons(unsigned long currentTime);
void handleExternalButtonPress(int buttonIndex);

// =====================================================
// SETUP
// =====================================================

void setup() {
  Serial.begin(9600);
  Serial.println("presetup complete");
//Serial.print("Long");
  //resetEEPROM();//comment this out unless you want to reset saved values during each arduino startup

  
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

  Toggle_BurstState = digitalRead(EXTERNAL_TOGGLE_BURST);
  lastToggle_BurstReading = Toggle_BurstState;

  Toggle_ShotQtyState = digitalRead(EXTERNAL_TOGGLE_SHOT);
  lastToggle_ShotQtyReading = Toggle_ShotQtyState;
  // ---------------------------------------------------
  // Start OLED
  // ---------------------------------------------------

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {

    // OLED initialization failed.
    // Stop here.

    while (true) {
    }
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);


  // ---------------------------------------------------
  // Load saved values
  // ---------------------------------------------------

  loadEEPROM();


  // ---------------------------------------------------
  // Show initial variable
  // ---------------------------------------------------

  displayCurrentVariable();
  Serial.println("setup complete");
}


// =====================================================
// MAIN LOOP
// =====================================================

void loop() {
  //unsigned long currentTime = millis();
  //updateToggles(currentTime);


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
    if (values[selected_timing_Variable] < MAX_VALUE) {

      values[selected_timing_Variable]++;

      displayCurrentVariable();
    }
  }


  // ---------------------------------------------------
  // Internal Button 3 - DECREMENT
  // ---------------------------------------------------

  if (ButtonPressed(2, INTERNAL_BUTTON_DECREMENT)) {
    Serial.println(2);
    if (values[selected_timing_Variable] > MIN_VALUE) {

      values[selected_timing_Variable]--;

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
  }
  if (ButtonPressed(5, EXTERNAL_BUTTON_FIRE_NORMAL)) {
    Serial.println("e");
    handleExternalButtonPress(1);
  }
  if (ButtonPressed(6, EXTERNAL_BUTTON_FIRE_RED)) {
    Serial.println("e");
    handleExternalButtonPress(2);
  }
  if (ButtonPressed(7, EXTERNAL_BUTTON_FIRE_GREEN)) {
    Serial.println("e");
    handleExternalButtonPress(3);
  }
  if (ButtonPressed(8, EXTERNAL_BUTTON_FIRE_PINK)) {
    Serial.println("e");
    handleExternalButtonPress(4);
  }

}

void handleExternalButtonPress(int buttonIndex) {

  Serial.print("Button ");
  Serial.print(ExternalButtonNames[buttonIndex]);

  Serial.print(" PRESSED | Toggle Burst: ");

  if (Toggle_BurstState == LOW) {
    Serial.print("Short");
  }
  else {
    Serial.print("Long");
  }

  Serial.print(" | Toggle Shot-Qty: ");

  if (Toggle_ShotQtyState == LOW) {
    Serial.println("Single");
  }
  else {
    Serial.println("Double");
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

    for (uint8_t i = 0; i < NUM_TIMING_VARIABLES; i++) {
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
    EEPROM_VALUES_ADDRESS + selected_timing_Variable,
    values[selected_timing_Variable]
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

  display.print(variableNames[selected_timing_Variable]);


  // ---------------------------------------------------
  // Value
  // ---------------------------------------------------

  display.setTextSize(2);

  //display.setCursor(0, 28);

  //display.print("Value:");


  display.setCursor(0, 48);

  printValue(values[selected_timing_Variable]);

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

bool ButtonPressed(uint8_t buttonIndex, uint8_t pin) {

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
  uint8_t defaultValues[NUM_TIMING_VARIABLES] = {
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
    EEPROM.update(EEPROM_VALUES_ADDRESS + i, defaultValues[i]);
  }

  EEPROM.update(EEPROM_MAGIC_ADDRESS, EEPROM_MAGIC);
}
