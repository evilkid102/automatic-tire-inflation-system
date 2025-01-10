#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

LiquidCrystal_I2C lcd(0x27, 20, 4); // I2C LCD Address

// Pin Definitions
const int sensorPin = A0; // Pressure sensor analog pin
const int button1 = 1;    // Button to increase/decrease pressure
const int button2 = 2;    // Button to start/stop inflation
const int buzzer = 13;    // Buzzer pin
const int feed = 6;       // Air pump relay control pin

// Variables
int setvalue = 10;         // Target pressure (PSI)
int max_pressure = 40;     // Maximum allowable pressure
int min_pressure = 10;     // Minimum allowable pressure
int pressure_value = 0;    // Current pressure reading
int raw_sensor_value = 0;  // Raw sensor value from sensor
int cal_factor = 0;        // Calibration offset (determined dynamically)
bool isInflating = false;  // Inflation process status
bool isSettingPressure = true; // Flag to indicate PSI setting mode
unsigned long lastButtonPress = 0;
const unsigned long debounceDelay = 50; // Delay for debounce (in milliseconds)

void setup() {
    // Initialize pins
    pinMode(feed, OUTPUT);
    pinMode(button1, INPUT_PULLUP);
    pinMode(button2, INPUT_PULLUP);
    pinMode(buzzer, OUTPUT);
    digitalWrite(feed, HIGH);  // Turn off air pump initially
    digitalWrite(buzzer, LOW); // Ensure buzzer is off initially

    // Initialize LCD and Serial
    lcd.init();
    lcd.backlight();
    lcd.clear();
    Serial.begin(9600);

    // Perform sensor calibration
    calibrateSensor();

    // Load settings and show welcome screen
    load_settings();
    delay(500);
    lcd.setCursor(6, 0);
    lcd.print("Welcome");
    lcd.setCursor(4, 1);
    lcd.print("Automatic");
    lcd.setCursor(2, 2);
    lcd.print("Tyre Inflator");
    delay(3000);
    lcd.clear();
    updateSetPressureDisplay();
}

void loop() {
    // Button 1: Adjust target pressure in PSI setting mode
    if (isSettingPressure && digitalRead(button1) == LOW && (millis() - lastButtonPress) > debounceDelay) {
        lastButtonPress = millis();
        setvalue += 2; // Increase pressure by 2 PSI
        if (setvalue > max_pressure) setvalue = min_pressure; // Loop back to minimum
        Serial.print("Button 1 Pressed: Set value = ");
        Serial.println(setvalue);
        save_settings();
        updateSetPressureDisplay();
    }

    // Button 2: Start/Stop inflation
    if (digitalRead(button2) == LOW && (millis() - lastButtonPress) > debounceDelay) {
        lastButtonPress = millis();

        if (isSettingPressure) {
            isSettingPressure = false; // Exit PSI setting mode
            startInflation();
        } else {
            isInflating = !isInflating; // Toggle inflation state
            if (!isInflating) stopInflation();
        }
    }

    // Read raw sensor value and calculate pressure in PSI
    raw_sensor_value = analogRead(sensorPin);
    int adjusted_value = raw_sensor_value - cal_factor;
    pressure_value = map(adjusted_value, 0, 817, 0, 100); // Adjust based on your sensor's specs

    // Handle inflation process
    if (isInflating) {
        if (pressure_value < setvalue) {
            digitalWrite(feed, LOW); // Turn on pump
        } else {
            stopInflation();
            lcd.clear();
            delay(100); // Allow LCD time to refresh
            lcd.setCursor(0, 0);
            lcd.print("Inflation Done!    "); // Clear any leftover characters
            delay(2000);
            returnToDefaultState();
        }

        // Overpressure alert
        if (pressure_value > max_pressure + 5) {  // Adding a 5 PSI buffer for overpressure
            stopInflation();
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Overpressure!");
            tone(buzzer, 1000);         // Sound buzzer
            delay(2000);                // Alert duration
            noTone(buzzer);             // Turn off buzzer
            returnToDefaultState();
        }

        // Update pressure reading on LCD
        static int lastPressureValue = -1; // Prevent frequent updates
        if (pressure_value != lastPressureValue) {
            lcd.setCursor(0, 1);
            lcd.print("Pressure: ");
            lcd.print(pressure_value);
            lcd.print(" PSI   "); // Clear trailing characters
            lastPressureValue = pressure_value;
        }
    }

    delay(500); // Stability delay
}

void startInflation() {
    lcd.clear();
    delay(100); // Allow LCD time to refresh
    lcd.setCursor(0, 0);
    lcd.print("Inflating...");
    isInflating = true;
}

void stopInflation() {
    digitalWrite(feed, HIGH); // Turn off pump
    isInflating = false;

    // Clear and refresh the LCD
    lcd.clear();
    delay(100); // Allow LCD time to refresh
    lcd.setCursor(0, 0);
    lcd.print("Inflation Done!    "); // Add spaces to clear any residual characters
    delay(2000); // Pause for user to see message

    // Return to PSI setting mode
    returnToDefaultState();
}

void returnToDefaultState() {
    isSettingPressure = true;
    updateSetPressureDisplay();
}

void updateSetPressureDisplay() {
    lcd.clear();
    delay(100); // Allow LCD time to refresh
    lcd.setCursor(0, 0);
    lcd.print("Set Pressure:");
    lcd.setCursor(0, 1);
    lcd.print(setvalue);
    lcd.print(" PSI");
}

void save_settings() {
    EEPROM.update(0, setvalue);
}

void load_settings() {
    setvalue = EEPROM.read(0);
    if (setvalue < min_pressure || setvalue > max_pressure) {
        setvalue = 10; // Default pressure
    }
}

void calibrateSensor() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Calibrating...");
    delay(2000); // Let user see message
    cal_factor = analogRead(sensorPin); // Record raw value at 0 PSI (ambient)
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Calibration Done");
    delay(2000); // Let user see message
    lcd.clear();
}
