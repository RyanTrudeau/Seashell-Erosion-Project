#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <SoftwareSerial.h>
#include "Arduino.h"

Adafruit_MPU6050 mpu;


const byte RxPin = 9;
const byte TxPin = 8;
SoftwareSerial BTSerial(RxPin, TxPin); // recieve and transmit pins


// MPU6050 initialization
void mpu_init()
{

    if (!mpu.begin())
    {
        Serial.println("Failed to find MPU6050 chip");
        while (1);  // Halt if MPU6050 is not found
    }
    mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
    mpu.setGyroRange(MPU6050_RANGE_250_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

}

// Function to get MPU6050 sensor events
void getMPUValues(sensors_event_t &a, sensors_event_t &g, sensors_event_t &temp) {
    mpu.getEvent(&a, &g, &temp);  // Get sensor data and store in a, g, and temp
}

void setup() {
  
  // define pin modes or Rx and Tx
  pinMode(RxPin, INPUT);
  pinMode(TxPin, OUTPUT);
  BTSerial.begin(9600);
  Serial.begin(9600);
  // Initialize the MPU6050
  mpu_init();
  
}

void loop() {

  sensors_event_t a, g, temp;
  // Call the function to get MPU values
  getMPUValues(a, g, temp);

  // Calculate angles based on accelerometer data
  float angleX = atan2(a.acceleration.y, sqrt(a.acceleration.x * a.acceleration.x + a.acceleration.z * a.acceleration.z)) * 180 / PI;
  float angleY = atan2(-a.acceleration.x, sqrt(a.acceleration.y * a.acceleration.y + a.acceleration.z * a.acceleration.z)) * 180 / PI;
  float angleZ = atan2(a.acceleration.z, sqrt(a.acceleration.x * a.acceleration.x + a.acceleration.y * a.acceleration.y)) * 180 / PI;

  // Send data over Bluetooth
    BTSerial.print("Angle X: ");
    BTSerial.print(angleX);
    BTSerial.print(" Angle Y: ");
    BTSerial.print(angleY);
    BTSerial.print(" Angle Z: ");
    BTSerial.println(angleZ);
  delay(1000);
}





