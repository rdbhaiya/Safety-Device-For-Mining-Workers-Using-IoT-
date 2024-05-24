#include <PulseSensorPlayground.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "DHT.h"
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

#define BLYNK_TEMPLATE_ID "TMPL3WggY70Jq"
#define BLYNK_TEMPLATE_NAME "Miners ko bachao"
#define BLYNK_AUTH_TOKEN "yW68EZhmim9PM8YeD3dIiTIxoKzMGyFh"
// Define pins and constants for gas sensor
#define CONTROL_PIN 16       // Define the pin on the NodeMCU board to control based on gas concentration
#define BUZZER_PIN D6        // Define the pin for the buzzer
const int gasSensorPin = A0; // Define the pin connected to the analog output of the MQ-4 sensor
const int s1 = D3;           // Pin for S1 of the multiplexer
const int s2 = D4;           // Pin for S2 of the multiplexer
const int s3 = D7;           // Pin for S3 of the multiplexer
const int gasChannel = 5;    // Channel 5 of the multiplexer for gas sensor

// Define constants for pulse sensor
const int pulseChannel = 4;  // Channel 4 of the multiplexer for pulse sensor

// Define pins and constants for DHT sensor
#define DPIN 14              // Pin to connect DHT sensor (GPIO number)
#define DTYPE DHT11          // Define DHT 11 or DHT22 sensor type
DHT dht(DPIN, DTYPE);

PulseSensorPlayground pulseSensor;
const int numReadings = 10;
int readings[numReadings]; // The readings from the pulse sensor
int readIndex = 0;         // The index of the current reading
int total = 0;             // The running total
int average = 0;           // The average

// Initialize LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "realme 11 Pro 5G";
char pass[] = "12442166";


void setup() {
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  Serial.begin(9600);
  pinMode(s1, OUTPUT);
  pinMode(s2, OUTPUT);
  pinMode(s3, OUTPUT);
  pinMode(CONTROL_PIN, OUTPUT); // Set the control pin as an output
  pinMode(BUZZER_PIN, OUTPUT);  // Set the buzzer pin as an output

  // Ensure all multiplexer pins are initially set to LOW
  digitalWrite(s1, LOW);
  digitalWrite(s2, LOW);
  digitalWrite(s3, LOW);

  // Initialize readings array to zero
  for (int i = 0; i < numReadings; i++) {
    readings[i] = 0;
  }
  
  // Initialize the pulse sensor
  pulseSensor.analogInput(gasSensorPin); // Note: using the same sensor pin
  pulseSensor.setThreshold(550);         // Adjust this threshold according to your sensor
  pulseSensor.begin();

  // Initialize the DHT sensor
  dht.begin();

  // Initialize the LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
}

void loop() {
 

  // Read and process gas sensor data
  readGasSensor();

  // Read and process pulse sensor data
  readPulseSensor();

  // Read and process DHT sensor data
  readDHTSensor();
  
  delay(3000); // Delay for stability
}

void readGasSensor() {
  Blynk.run();
  // Select channel 5 of the multiplexer
  digitalWrite(s1, bitRead(gasChannel, 0));
  digitalWrite(s2, bitRead(gasChannel, 1));
  digitalWrite(s3, bitRead(gasChannel, 2));

  // Allow some time for the multiplexer to stabilize
  delay(10);

  // Read analog value from the multiplexer
  int sensorValue = analogRead(gasSensorPin);
  
  // Convert analog value to voltage
  float voltage = sensorValue * (5.0 / 1023.0); // Assuming a 5V reference voltage for NodeMCU
  
  // Convert voltage to gas concentration (adjust this conversion according to your sensor datasheet)
  float gasConcentration = voltage * 10.0; // Assuming a linear relationship, adjust accordingly
  
  // Print gas concentration to serial monitor
  Serial.print("Gas Concentration: ");
  Serial.print(gasConcentration);
  Serial.println(" ppm");

  // Display gas concentration on LCD
  lcd.setCursor(0, 1);
  lcd.print("Gas: ");
  lcd.print(gasConcentration);
  lcd.print(" ppm");
  Blynk.virtualWrite(V2, GasConcentration);

  
  // Control the pin and buzzer based on gas concentration
  if (gasConcentration > 25) { // Adjust threshold if needed
    digitalWrite(CONTROL_PIN, HIGH); // Turn on the control pin
    // Activate the buzzer with a pattern to make it more noticeable
    for (int i = 0; i < 5; i++) {
      tone(BUZZER_PIN, 1000);  // Tone at 1000Hz
      delay(100);              // 100ms delay
      noTone(BUZZER_PIN);      // Turn off the tone
      delay(100);              // 100ms delay
    }
  } else {
    digitalWrite(CONTROL_PIN, LOW);  // Turn off the control pin
    noTone(BUZZER_PIN);              // Turn off the buzzer
  }
}

void readPulseSensor() {
  Blynk.run();
  // Select channel 4 of the multiplexer
  digitalWrite(s1, bitRead(pulseChannel, 0));
  digitalWrite(s2, bitRead(pulseChannel, 1));
  digitalWrite(s3, bitRead(pulseChannel, 2));
  
  // Allow some time for the multiplexer to stabilize
  delay(10);

  // Check if a beat is detected
  if (pulseSensor.sawStartOfBeat()) {
    Serial.println("Beat detected!");
  }

  // Read pulse sensor data
  int pulse = pulseSensor.getBeatsPerMinute();
  
  // Remove the oldest reading
  total = total - readings[readIndex];
  
  // Store the new reading
  readings[readIndex] = pulse;
  
  // Add the new reading to the total
  total = total + readings[readIndex];
  
  // Advance to the next position in the array
  readIndex = (readIndex + 1) % numReadings;

  // Calculate the average
  average = total / numReadings;

  // Check if pulse value is valid and print it
  if (pulse > 0) {
    Serial.print("Current Pulse: ");
    Serial.print(pulse);
    Serial.print(" BPM, Smoothed Pulse: ");
    Serial.print(average);
    Serial.println(" BPM");

    // Display pulse on LCD
    lcd.setCursor(0, 0);
    lcd.print("Pulse: ");
    lcd.print(average);
    lcd.print(" BPM");
    Blynk.virtualWrite(V4, average);
  } else {
    Serial.println("No pulse detected");

    // Display no pulse on LCD
    lcd.setCursor(0, 0);
    lcd.print("Pulse: No data   ");
  }

}

void readDHTSensor() {
  Blynk.run();
  // Wait a few seconds between measurements
  delay(2000);

  // Read DHT sensor data
  float temperature = dht.readTemperature(); // Read temperature in Celsius
  float humidity = dht.readHumidity();       // Read humidity

  // Check if any reads failed and exit early (to try again).
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // Print DHT sensor data to serial monitor
  Serial.print("Temp: ");
  Serial.print(temperature);
  Serial.print(" C, Hum: ");
  Serial.print(humidity);
  Serial.println("%");

  // Display temperature and humidity on LCD
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(temperature);
  lcd.print(" C");
  lcd.setCursor(9, 0);
  lcd.print("Hum: ");
  lcd.print(humidity);
  lcd.print("%");
  Blynk.virtualWrite(V0, humidity);
  Blynk.virtualWrite(V1, temperature);


  if (temperature >= 40) {
    digitalWrite(CONTROL_PIN, HIGH); // Turn on the control pin
    // Activate the buzzer with a pattern to make it more noticeable
    for (int i = 0; i < 5; i++) {
      tone(BUZZER_PIN, 1000);  // Tone at 1000Hz
      delay(100);              // 100ms delay
      noTone(BUZZER_PIN);      // Turn off the tone
      delay(100);              // 100ms delay
    }
  } else {
    digitalWrite(CONTROL_PIN, LOW);  // Turn off the control pin
    noTone(BUZZER_PIN);              // Turn off the buzzer
  }
}

void tone(int pin, int frequency) {
  analogWriteFreq(frequency);  // Set PWM frequency
  analogWrite(pin, 512);       // Set duty cycle (50%)
}

void noTone(int pin) {
  analogWrite(pin, 0);         // Set duty cycle to 0 (stop the sound)
}
