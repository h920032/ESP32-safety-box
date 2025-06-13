#include <Keypad.h>
#include <Servo.h>
#include <BluetoothSerial.h>
#include <LiquidCrystal_I2C.h>
#include <vector>

// ----- Keypad configuration -----
const byte ROWS = 4; // four rows
const byte COLS = 3; // three columns
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
byte rowPins[ROWS] = {2, 3, 4, 5};    // adjust to your wiring
byte colPins[COLS] = {6, 7, 8};       // adjust to your wiring

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// ----- LCD configuration -----
LiquidCrystal_I2C lcd(0x27, 16, 2);   // I2C address 0x27, 16 column, 2 rows

// ----- Buzzer and Servo -----
const int buzzerPin = 9;              // PWM capable pin for buzzer
const int servoPin  = 10;             // PWM pin for servo

Servo lockServo;
const int lockedPos   = 0;            // servo angle when locked
const int unlockedPos = 90;           // servo angle when unlocked

// ----- Bluetooth -----
BluetoothSerial SerialBT;

// ----- Password storage -----
std::vector<String> combinations;     // store typed combinations
String currentInput;                  // current digits entered

bool remoteUnlock = false;            // set by Bluetooth command

// helper to play tone per key
void playTone(char key) {
  int freq = 1000;                    // base frequency
  if (key >= '0' && key <= '9') {
    freq += (key - '0') * 100;        // simple mapping 0 -> 1000Hz, 1 -> 1100Hz ...
  } else if (key == '*') {
    freq = 1500;
  } else if (key == '#') {
    freq = 1600;
  }
  tone(buzzerPin, freq, 150);         // play for 150ms
}

// servo operation
void unlockSafe() {
  lcd.clear();
  lcd.print("Unlocking...");
  lockServo.write(unlockedPos);
  delay(1000);                        // hold open briefly
  lockServo.write(lockedPos);
  lcd.clear();
  lcd.print("Locked");
}

// handle key press events
void handleKey(char key) {
  playTone(key);
  if (key == '#') {                   // end of combination
    if (currentInput.length() > 0) {
      combinations.push_back(currentInput);
      lcd.clear();
      lcd.print("Stored ");
      lcd.print(currentInput.length());
      lcd.print(" digits");
      if (remoteUnlock) {
        unlockSafe();
        remoteUnlock = false;
      }
      currentInput = "";
      delay(1000);
      lcd.clear();
      lcd.print("Digits: 0");
    }
  } else if (key == '*') {            // clear current input
    currentInput = "";
    lcd.clear();
    lcd.print("Digits: 0");
  } else {                            // add digit
    currentInput += key;
    lcd.clear();
    lcd.print("Digits: ");
    lcd.print(currentInput.length());
  }
}

void setup() {
  Serial.begin(115200);
  SerialBT.begin("SafeBox");         // start Bluetooth with device name

  lockServo.attach(servoPin);
  lockServo.write(lockedPos);

  pinMode(buzzerPin, OUTPUT);

  lcd.init();
  lcd.backlight();
  lcd.print("Digits: 0");
}

void loop() {
  char key = keypad.getKey();
  if (key) {
    handleKey(key);
  }

  // check Bluetooth for commands
  if (SerialBT.available()) {
    String cmd = SerialBT.readStringUntil('\n');
    cmd.trim();
    cmd.toUpperCase();
    if (cmd == "GET") {
      for (const String &c : combinations) {
        SerialBT.println(c);
      }
      SerialBT.println("END");
    } else if (cmd == "UNLOCK") {
      remoteUnlock = true;
      SerialBT.println("Unlock armed");
    } else {
      SerialBT.println("Unknown command");
    }
  }
}

