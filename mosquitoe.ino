#include <Wire.h>
#include <Servo.h>
#include <RTClib.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <SD.h>

// LIDAR Variables
#define LIDAR_ADDRESS 0x62
#define MEASURE_REGISTER 0x00
#define MEASURE_VALUE 0x04
#define DISTANCE_REGISTER 0x8f

// Motor Control Pins
#define STEP_X 2
#define DIR_X 3
#define STEP_Y 4
#define DIR_Y 5

// Laser Control Pin
#define LASER_PIN 6

// SD Card Module
#define SD_CS_PIN 10

// OLED Display Settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Variables
int distance = 0;
const int targetThreshold = 20; // Detection threshold in cm
const int stepDelay = 500;      // Microseconds between motor steps

// RTC Module
RTC_DS3231 rtc;

// SD Card Logging
File logFile;

void setup() {
  Serial.begin(9600);

  // Initialize LIDAR
  Wire.begin();

  // Initialize OLED Display
  if (!display.begin(SSD1306_I2C_ADDRESS, 0x3C)) {
    Serial.println("OLED initialization failed.");
    for (;;);
  }
  display.clearDisplay();

  // Initialize RTC
  if (!rtc.begin()) {
    Serial.println("RTC initialization failed.");
    for (;;);
  }

  // Initialize SD Card
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD card initialization failed.");
    for (;;);
  }

  // Initialize Laser
  pinMode(LASER_PIN, OUTPUT);
  digitalWrite(LASER_PIN, LOW);

  // Display Startup Message
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("System Initialized");
  display.display();
  delay(2000);
}

void loop() {
  distance = getDistance();

  if (distance > 0 && distance < targetThreshold) {
    displayTarget();
    trackAndEliminate();
  } else {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("No target detected.");
    display.display();
  }

  delay(100);
}

// Function to Get Distance from LIDAR
int getDistance() {
  Wire.beginTransmission(LIDAR_ADDRESS);
  Wire.write(MEASURE_REGISTER);
  Wire.write(MEASURE_VALUE);
  Wire.endTransmission();

  delay(20);

  Wire.beginTransmission(LIDAR_ADDRESS);
  Wire.write(DISTANCE_REGISTER);
  Wire.endTransmission();

  Wire.requestFrom(LIDAR_ADDRESS, 2);
  if (Wire.available() >= 2) {
    int highByte = Wire.read();
    int lowByte = Wire.read();
    return (highByte << 8) | lowByte;
  }
  return -1;
}

// Function to Track and Eliminate Target
void trackAndEliminate() {
  moveStepper(STEP_X, DIR_X, 50, true);
  moveStepper(STEP_Y, DIR_Y, 25, false);

  fireLaser();
  logKill();
}

// Function to Move Stepper Motors
void moveStepper(int stepPin, int dirPin, int steps, bool direction) {
  digitalWrite(dirPin, direction);
  for (int i = 0; i < steps; i++) {
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(stepDelay);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(stepDelay);
  }
}

// Function to Fire Laser
void fireLaser() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Target acquired!");
  display.println("Firing laser...");
  display.display();

  digitalWrite(LASER_PIN, HIGH);
  delay(500);
  digitalWrite(LASER_PIN, LOW);

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Target eliminated.");
  display.display();
}

// Function to Log Kills
void logKill() {
  DateTime now = rtc.now();
  logFile = SD.open("kills.log", FILE_WRITE);
  if (logFile) {
    logFile.print("Kill at ");
    logFile.print(now.timestamp());
    logFile.println(" cm.");
    logFile.close();
  } else {
    Serial.println("Error writing to log.");
  }
}

// Function to Display Target Info on OLED
void displayTarget() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Target detected!");
  display.print("Distance: ");
  display.print(distance);
  display.println(" cm");
  display.display();
}
