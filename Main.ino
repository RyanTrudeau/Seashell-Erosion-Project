// ---------------------------------------------------------------------------------------------------
// File: seashell_1.4.ino
// Last Date Updated: 11/24/2024
// Authors: Ryan Trudeau, Jon Kinney
// Course: EE437 Embedded Systems
// Description: The seashell angle reader reads raw data from accelerometer position, converts raw data to angles via calculations, then displays
// those angle calculations to OLED screen. A button is used to record the current angle data 
// on microSD card and external bluetooth device. The same button is also used to zero the angles if held down for 2 seconds. 
// An additional button is used to delete the previous angle record. A GUI for the buttons
// is also included on the OLED screen. The device sleeps during 2 minute inactivity and awakes when sensor detects movement
// --------------------------------------------------------------------------------------------------

// Libraries
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <SoftwareSerial.h>
#include "Arduino.h"
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h> 
#include <SD.h>
#include <ButtonHandler.h>
#include <avr/sleep.h>
#include <avr/power.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET   -1 // Reset pin # (or -1 if sharing Arduino reset pin)

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_MPU6050 mpu;

// Global variables for angle offsets
double offsetX = 0.0;
double offsetY = 0.0;
double offsetZ = 90.0;

// Pin definitions
const byte RxPin = 9;
const byte TxPin = 8;
SoftwareSerial BTSerial(RxPin, TxPin); // recieve and transmit pins
const int buttonPin1 = 2; // Button connected to pin 2
const int buttonPin2 = 3;
const int chipSelect = 10;  // Chip select pin for the SD card module

// Create ButtonHandler objects
ButtonHandler leftButton(buttonPin1, 5, 2000);
ButtonHandler rightButton(buttonPin2, 5, 2000);
bool deleteDecision = false;

// Motion and sleep detection variables
const unsigned long inactivityTime = 120000; // 2 minute threshold
const float angleThreshold = 5.0;
unsigned long lastMotionTime = 0;
bool isAsleep = false;

// MPU6050 initialization. Checks if device is transmittting and sets angles and frequency range.
void mpu_init()
{
    //delay(100);
    if (!mpu.begin())
    {
        Serial.println("Failed to find MPU6050 chip");
        while (1);  // Halt if MPU6050 is not found
    }
    mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
    mpu.setGyroRange(MPU6050_RANGE_250_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
}

// Function to initialize the OLED display. Checks device address and refreshes display 
void initDisplay() {

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
      Serial.println(F("SSD1306 allocation failed"));
      while (1); // 
  }
  display.clearDisplay();
  display.display();
  delay(500); // slight delay on start up
}

// Function to display angles on OLED. Sets header with given angles. Cursors are adjusted for each data point
void displayAngles(double angleX, double angleY, double angleZ) {

  display.clearDisplay(); // refreshes display

  // Angle header
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(30,0);
  display.println("ANGLES");

  // Data headers
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(10, 20);
  display.println("X: ");
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(30, 20); // Set cursor for the value of angleX right next to the label
  display.print(angleX, 1);

  display.setCursor(70, 20);
  display.println("Y: ");
  display.setCursor(90, 20); // Set cursor for the value of angleY right next to the label
  display.print(angleY, 1); 

  display.setCursor(40, 32);
  display.println("Z: ");
  display.setCursor(60, 32); // Set cursor for the value of angleZ right next to the label
  display.print(angleZ, 1);

  if (deleteDecision == false){
    displaybuttonUI(0); // displays default button ui at start of program 
  } else if (deleteDecision == true){
    displaybuttonUI(2); // displays the delete ui when user presses delete
  }
  
  display.display();

  
}

// Function to get MPU6050 sensor events
void getMPUValues(sensors_event_t &a, sensors_event_t &g, sensors_event_t &temp) {
    mpu.getEvent(&a, &g, &temp);  // Get sensor data and store in a, g, and temp
}

// Function converts mpu data to x, y, and z angles. Takes average of 30 readings to reduce sensitivity of small movements
void getAngles(double &angleX, double &angleY, double &angleZ){

  double angleX_values[30]; // vectors to capture 30 readings
  double angleY_values[30]; 
  double angleZ_values[30];
  double averageX = 0;
  double averageY = 0;

  double averageZ = 0;
  for (int i = 0; i < 30; i++){
    sensors_event_t a, g, temp;
      // Call the function to get MPU values
      getMPUValues(a, g, temp);

      // Calculate angles based on accelerometer data
      angleX_values[i] = atan2(a.acceleration.y, sqrt(a.acceleration.x * a.acceleration.x + a.acceleration.z * a.acceleration.z)) * 180 / PI;
      angleY_values[i] = atan2(-a.acceleration.x, sqrt(a.acceleration.y * a.acceleration.y + a.acceleration.z * a.acceleration.z)) * 180 / PI;
      angleZ_values[i] = atan2(a.acceleration.z, sqrt(a.acceleration.x * a.acceleration.x + a.acceleration.y * a.acceleration.y)) * 180 / PI;

      // Takes 30 angle readings and averages them 
      averageX += angleX_values[i];
      averageY += angleY_values[i];
      averageZ += angleZ_values[i];
  }

  // Finalize angle readings and substract offsets
  angleX = (averageX / 30) - offsetX; 
  angleY = (averageY / 30) - offsetY;
  angleZ = (averageZ / 30) - offsetZ;

}

// Function to zero the sensor by setting the offsets to the current angle
void zeroAngles() {
  double angleX, angleY, angleZ;

  offsetX = 0.0; offsetY = 0.0; offsetZ = 90.0;
  // Capture current angles as offsets
  getAngles(angleX, angleY, angleZ);

  // Store offsets
  offsetX = angleX;
  offsetY = angleY;
  offsetZ = angleZ;

  Serial.println("Sensor zeroed:");
  Serial.print("Offset X: "); Serial.println(offsetX);
  Serial.print("Offset Y: "); Serial.println(offsetY);
  Serial.print("Offset Z: "); Serial.println(offsetZ);
}


void setup() {
  
  // Define pin modes or Rx and Tx
  pinMode(RxPin, INPUT);
  pinMode(TxPin, OUTPUT);
  BTSerial.begin(9600);
  Serial.begin(9600);
  
  // Initialize the MPU6050
  mpu_init();
  // Initialize the display
  initDisplay();
  // Set up the button pin as an input with internal pull-up
  leftButton.begin();
  rightButton.begin();
  // Increments motion time
  lastMotionTime = millis();
  
}

void loop() {

  // Initialize angles
  double angleX, angleY, angleZ;
  
  if (!isAsleep){ // If arudino is awake, read and write angles
    // Gets mpu angle 
    getAngles(angleX, angleY, angleZ);
    
    // Call to function to display angles on OLED screen
    displayAngles(angleX, angleY, angleZ);

    // Check if the buttons were detected by the interrupt
    leftButton.update();
    rightButton.update();
  
    if (leftButton.isShortPress()){
      if (!deleteDecision){ // checks if the decision flag is false to perform the save button function
        Serial.println("Writing angles...");
        // writeangles
        displaybuttonUI(1); // displays a indicator for the button press and angles are written
        writeAngles("angles.csv", angleX, angleY, angleZ);
      } else { 
        // marks the last angle record to the file
        displaybuttonUI(4); // displays indicator for button press and angles are deleted
        markLastRowDeleted("angles.csv");
        deleteDecision = false; // reset decision flag
      }
  
    } 
    if (rightButton.isShortPress()){
      if (!deleteDecision){
        displaybuttonUI(3); // enables the delete angle ui
        deleteDecision = true; // sets the decision flag to true
      } else {
        // go back
        displaybuttonUI(5); // displays button press indication
        deleteDecision = false; // resets flag
      }

    }

  if (leftButton.isLongPress()){ // for a long press, the angles are zeroed
    Serial.println("Zeroing angle data...");
    // zero mpu values
    zeroAngles();
  }

    if (stable(angleX, angleY, angleZ)) { // Checks if angles are within the theshold
      //Serial.println(millis() - lastMotionTime);
      if ((millis() - lastMotionTime )>= inactivityTime) { // If the above expression is true, check inactivity time
        Serial.println("Entering sleep mode...");
        displayOff();
        enterSleepMode();
        lastMotionTime = millis();
      }
    } else {
      lastMotionTime = millis();
    }

  } else { // If arudino is asleep, check motion detected from sensor
    if (detectMotion()) { // Wake up on motion
      Serial.println("Motion detected, waking up...");
      wakeUp(); 
      isAsleep = false; // resets sleep flag
      lastMotionTime = millis();
    }
  }
  
}

// Function to write current angles to SD card and bluetooth. Checks if SD card is valid then writes angles.
// If the file fails to open, the data will still be written to bluetooth
void writeAngles(const char* filename, double &angleX, double &angleY, double &angleZ){

  // Initiallizes Micro SD card module
  if (!SD.begin(chipSelect)) {
     Serial.println(F("SD card initialization failed!"));
  } else {
     Serial.println(F("SD card initialized."));
  }
   //Open the file for writing
  File dataFile = SD.open(filename, FILE_WRITE);
  
  // Check if the file is available for writing
  if (dataFile) {

    // Writes data to the file
    dataFile.print(angleX, 2);
    dataFile.print(F(", "));
    dataFile.print(angleY, 2);
    dataFile.print(F(", "));
    dataFile.print(angleZ, 2);
    dataFile.println(F(",saved")); // fourth column for status

    // Prints angles to terminal on bluetooth device
    BTSerial.print(F("Angle X: "));
    BTSerial.print(angleX);
    BTSerial.print(F(" Angle Y: "));
    BTSerial.print(angleY);
    BTSerial.print(F(" Angle Z: "));
    BTSerial.println(angleZ);
    
    // Close the file
    dataFile.close();
    Serial.println(F("Data written to file."));
  } else {

    // If the file cannot be opened
    Serial.println("Error opening file.");

    // Still prints angles to terminal on bluetooth even if SD file fails
    BTSerial.print(F("Angle X: "));
    BTSerial.print(angleX);
    BTSerial.print(F(" Angle Y: "));
    BTSerial.print(angleY);
    BTSerial.print(F(" Angle Z: "));
    BTSerial.println(angleZ);
  }

}

// Function rewrites the status of the last angle to deleted. It's tricky to edit certain lines from an existing file so a temporary file
// is created to update those lines, then copy them back to the original file.
// Every line is copied to the temporary while the status of the last line is changed to "deleted"
void markLastRowDeleted(const char* filename) {
  File dataFile = SD.open(filename, FILE_READ);
  File tempFile = SD.open("temp.csv", FILE_WRITE);

  if (dataFile && tempFile) {
    String lastLine = "";
    String currentLine;

    // Read each line and find the last line
    while (dataFile.available()) {
      currentLine = dataFile.readStringUntil('\n');
      if (currentLine.length() > 0) {
        lastLine = currentLine;  // Update lastLine
      }
    }
    // Reset file to start processing from the beginning
    dataFile.seek(0);

    // Copy lines to the temporary file
    bool firstLine = true;  // Flag to control the first line

    while (dataFile.available()) {
      String line = dataFile.readStringUntil('\n');
      
      // If it's the last line, mark it as deleted
      if (line == lastLine) {
        int lastComma = line.lastIndexOf(',');
        String updatedLine = line.substring(0, lastComma) + ",deleted";
        
        if (!firstLine) {
          tempFile.print("\n");  // Ensure no extra newline at the beginning
        }
        tempFile.print(updatedLine);  // Write the updated last line
        firstLine = false;  // After the first line, set firstLine to false
      } else {
        if (!firstLine) {
          tempFile.print("\n");  // Ensure no extra newline at the beginning
        }
        tempFile.print(line);  // Write the other lines as they are
        firstLine = false;  // After the first line, set firstLine to false
      }
    }

    dataFile.close();
    tempFile.close();

    // Remove the original file
    if (SD.remove(filename)) {
      Serial.println(F("Original file removed."));
    } else {
      Serial.println(F("Failed to remove original file."));
      return;
    }

    // Rename the temporary file
    tempFile = SD.open("temp.csv", FILE_READ);
    File newFile = SD.open(filename, FILE_WRITE);

    if (tempFile && newFile) {
      while (tempFile.available()) {
        newFile.write(tempFile.read());
      }
      newFile.println();
      tempFile.close();
      newFile.close();

      // Remove temporary file after renaming
      SD.remove("temp.csv");
      Serial.println(F("Temporary file renamed to original filename."));
    } else {
      Serial.println(F("Error renaming temporary file."));
    }
  } else {
    Serial.println(F("Error opening files."));
  }
}



// Function checks angles and returns true if they are within the angle threshold
bool stable(double x, double y, double z){
  
  static double lastAngleX = x;
  static double lastAngleY = y;
  static double lastAngleZ = z;

  double angleXDiff = abs(x - lastAngleX);
  double angleYDiff = abs(y - lastAngleY);
  double angleZDiff = abs(z - lastAngleZ);

  lastAngleX = x;
  lastAngleY = y;
  lastAngleZ = z;

  return (angleXDiff < angleThreshold && angleYDiff < angleThreshold && angleZDiff < angleThreshold);

}

// Function puts the arduino in idle
void enterSleepMode() {
  isAsleep = true;
  set_sleep_mode(SLEEP_MODE_IDLE);  // Use idle mode to still use i2c
  sleep_enable(); // Enable sleep

  sleep_cpu();  // Enter sleep mode

  sleep_disable(); // Disable sleep mode when cpu wakes
}

// Function reinitiallizes I2C devices when arduino wakes up
void wakeUp(){
  isAsleep = false;

  // Initialize the MPU6050
  mpu_init();

  // Initialize the display
  initDisplay();
  displayOn();

}

// Function clears display then turns off display power
void displayOff(){
  display.clearDisplay();
  display.display();
  display.ssd1306_command(SSD1306_DISPLAYOFF);

}

// Function turns on and refreshes display
void displayOn(){
  display.ssd1306_command(SSD1306_DISPLAYON);
  display.clearDisplay();
  display.display();
}

// Function checks the sensor movement. It runs in main loop while the CPU is in idel
bool detectMotion(){
  
  double angleX, angleY, angleZ;
  getAngles(angleX, angleY, angleZ);
  return !stable(angleX, angleY, angleZ);

}

// Function stores the display functions for the buttons. The input is an index tied to a set of display functions
void displaybuttonUI(int function){
  // display functions index:
  // 0 - main ui
  // 1 - save button press
  // 2 - delete ui
  // 3 - delete button press right
  // 4 - delete button press left
  // 5 - go back button press
  switch (function){
    case 0:
      // main ui
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.setCursor(20, 57);
      display.print("SAVE");
      display.drawRect(1, 55, 60, 10, WHITE);
      display.setCursor(70, 57);
      display.print("DELETE");
      display.drawRect(64, 55, 60, 10, WHITE);
      display.display();
      break;
    case 1:
      // save button press
      display.fillRect(1, 55, 60, 9, WHITE);
      display.setCursor(20, 57);
      display.setTextColor(SSD1306_BLACK);
      display.print("SAVE");
      display.display();
      delay(650);

      display.fillRect(1, 55, 60, 9, BLACK);
      display.display();

      display.setTextColor(WHITE);
      display.setCursor(1, 45);
      display.print("Angle Saved");
      display.setCursor(20, 57);
      display.print("SAVE");
      display.drawRect(1, 55, 60, 10, WHITE);
      display.setCursor(70, 57);
      display.display();
      delay(1000);
      break;

    case 2:
      // delete ui
      display.setTextColor(WHITE);
      display.setCursor(1, 45);
      display.print("Delete last angle?");
      display.setCursor(15, 57);
      display.print("DELETE");
      display.drawRect(1, 55, 60, 10, WHITE);
      display.setCursor(70, 57);
      display.print("GO BACK");
      display.drawRect(64, 55, 60, 10, WHITE);
      display.display();
      break;
    case 3:
      // delete buttton press right
      display.fillRect(64, 55, 60, 10, WHITE);
      display.setCursor(70, 57);
      display.setTextColor(SSD1306_BLACK);
      display.print("DELETE");
      display.display();
      delay(650);
      break;
    case 4:
      // delete button left press
      display.fillRect(1, 55, 60, 9, WHITE);
      display.setCursor(15, 57);
      display.setTextColor(SSD1306_BLACK);
      display.print("DELETE");
      display.display();
      delay(650);

      display.fillRect(1, 55, 60, 9, SSD1306_BLACK);
      display.fillRect(1, 43, 128, 10, SSD1306_BLACK);
      display.display();

      display.setTextColor(WHITE);
      display.setCursor(1, 45);
      display.print("Angle deleted");
      display.setCursor(15, 57);
      display.print("DELETE");
      display.drawRect(1, 55, 60, 10, WHITE);
      display.display();
      delay(1000);
      break;
    case 5:
      // go back button press
      display.fillRect(64, 55, 60, 9, WHITE); 
      display.setCursor(70, 57);
      display.setTextColor(SSD1306_BLACK);
      display.print("GO BACK");
      display.display();
      delay(650);
      break;
    default:
      // do nothing
      break;
  }


}