#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_MPU6050.h>

Adafruit_MPU6050 mpu;

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    delay(10);  // Wait for the Serial monitor to open
  }



  // Initialize MPU6050
  if (!mpu.begin(0x68)) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 Found!");

  mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
  mpu.setGyroRange(MPU6050_RANGE_250_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  
  delay(100);
}

void loop() {
  /* Get new sensor events with the readings */
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // Calculate angles based on accelerometer readings
  float angleX = atan2(a.acceleration.y, sqrt(a.acceleration.x * a.acceleration.x + a.acceleration.z * a.acceleration.z)) * 180 / PI;
  float angleY = atan2(-a.acceleration.x, sqrt(a.acceleration.y * a.acceleration.y + a.acceleration.z * a.acceleration.z)) * 180 / PI;
  float angleZ = atan2(a.acceleration.z, sqrt(a.acceleration.x * a.acceleration.x + a.acceleration.y * a.acceleration.y)) * 180 / PI;

  // Print the angles
  Serial.print("Angle X: ");
  Serial.print(angleX);
  Serial.print(" | Angle Y: ");
  Serial.print(angleY);
  Serial.print(" | Angle Z: ");
  Serial.println(angleZ);

  delay(500);
}
