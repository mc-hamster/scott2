// Arduino Uno
// 2 Toggle switches + 5 momentary buttons
// All inputs use INPUT_PULLUP
// All buttons and toggles are debounced

const int Toggle_Burst = 2;
const int Toggle_ShotQty = 3;

const int buttons[] = {4, 5, 6, 7, 8};
const int NUM_BUTTONS = 5;

const String InternalButtonNames[NUM_BUTTONS] = {
  "Random",
  "Normal",
  "Red",
  "Green",
  "Pink"
};

// Debounce time in milliseconds
const unsigned long debounceDelay = 50;

// --------------------------------------------------
// Button state variables
// --------------------------------------------------

int buttonState[NUM_BUTTONS];          // Debounced state
int lastButtonReading[NUM_BUTTONS];    // Previous raw reading
unsigned long lastButtonChange[NUM_BUTTONS];

// --------------------------------------------------
// Toggle state variables
// --------------------------------------------------

int Toggle_BurstState;
int Toggle_ShotQtyState;

int lastToggle_BurstReading;
int lastToggle_ShotQtyReading;

unsigned long lastToggle_BurstChange = 0;
unsigned long lastToggle_ShotQtyChange = 0;


// --------------------------------------------------
// Setup
// --------------------------------------------------

void setup() {

  Serial.begin(9600);

  // Toggle switches
  pinMode(Toggle_Burst, INPUT_PULLUP);
  pinMode(Toggle_ShotQty, INPUT_PULLUP);

  // Momentary buttons
  for (int i = 0; i < NUM_BUTTONS; i++) {
    pinMode(buttons[i], INPUT_PULLUP);

    // Initialize button states
    buttonState[i] = digitalRead(buttons[i]);
    lastButtonReading[i] = buttonState[i];
    lastButtonChange[i] = 0;
  }

  // Initialize toggle states
  Toggle_BurstState = digitalRead(Toggle_Burst);
  lastToggle_BurstReading = Toggle_BurstState;

  Toggle_ShotQtyState = digitalRead(Toggle_ShotQty);
  lastToggle_ShotQtyReading = Toggle_ShotQtyState;
}


// --------------------------------------------------
// Main loop
// --------------------------------------------------

void loop() {

  unsigned long currentTime = millis();


  // ==================================================
  // DEBOUNCE TOGGLE A
  // ==================================================

  int Toggle_BurstReading = digitalRead(Toggle_Burst);

  if (Toggle_BurstReading != lastToggle_BurstReading) {
    lastToggle_BurstChange = currentTime;
    lastToggle_BurstReading = Toggle_BurstReading;
  }

  if ((currentTime - lastToggle_BurstChange) >= debounceDelay) {
    Toggle_BurstState = Toggle_BurstReading;
  }


  // ==================================================
  // DEBOUNCE TOGGLE B
  // ==================================================

  int Toggle_ShotQtyReading = digitalRead(Toggle_ShotQty);

  if (Toggle_ShotQtyReading != lastToggle_ShotQtyReading) {
    lastToggle_ShotQtyChange = currentTime;
    lastToggle_ShotQtyReading = Toggle_ShotQtyReading;
  }

  if ((currentTime - lastToggle_ShotQtyChange) >= debounceDelay) {
    Toggle_ShotQtyState = Toggle_ShotQtyReading;
  }


  // ==================================================
  // DEBOUNCE ALL MOMENTARY BUTTONS
  // ==================================================

  for (int i = 0; i < NUM_BUTTONS; i++) {

    int currentReading = digitalRead(buttons[i]);

    // Has the raw button reading changed?
    if (currentReading != lastButtonReading[i]) {

      // Reset debounce timer
      lastButtonChange[i] = currentTime;

      lastButtonReading[i] = currentReading;
    }

    // Has the reading remained stable long enough?
    if ((currentTime - lastButtonChange[i]) >= debounceDelay) {

      // Has the debounced state actually changed?
      if (currentReading != buttonState[i]) {

        buttonState[i] = currentReading;

        // Button was just pressed
        // INPUT_PULLUP means LOW = pressed
        if (buttonState[i] == LOW) {

          Serial.print("Button ");
          Serial.print(InternalButtonNames[i]);
          
          //Serial.print(i + 1);
          Serial.print(" PRESSED | Toggle Burst: ");

          if (Toggle_BurstState == LOW) {
            Serial.print("Short");
          } else {
            Serial.print("Long");
          }

          Serial.print(" | Toggle Shot-Qty: ");

          if (Toggle_ShotQtyState == LOW) {
            Serial.println("Single");
          } else {
            Serial.println("Double");
          }
        }
      }
    }
  }
}
