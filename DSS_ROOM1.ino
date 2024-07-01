/* 
  ---- Arduino code for connecting sensors to the ThingSpeak IoT platform ----
   
  This code reads sensor data from various sensors connected to an Arduino board
  and uploads the data to the ThingSpeak IoT platform using Wi-Fi.

  Data is collected from Arduino sensors every second, aggregated over a minute
  and then an average value for that period is sent to ThingSpeak

  Device: DSS_ROOM1
*/

// Include necessary libraries
#include <DHT.h>        // Library for DHT22 Temperature and Humidity Sensor
#include <WiFiNINA.h>   // Library for Wi-Fi communication
#include "ThingSpeak.h" // Library for ThingSpeak IoT platform

// Define WiFi and IoT connections
char ssid[] = "ssid";             // Your WiFi network SSID (name)
char pass[] = "pass";             // Your WiFi network password
WiFiClient client;                // Create a WiFi client instance for network communication

// ThingSpeak channel credentials
unsigned long myChannelNumber = 2015606;      // Your ThingSpeak channel number
const char *myWriteAPIKey = "myWriteAPIKey";  // Your ThingSpeak API Key

// Define sensor pins and create instances
#define IlluminanceSensorPin A0   // Analog pin for Illuminance Sensor
#define NoiseSensorPin A3         // Analog pin for Noise Sensor
#define DHTPIN 3                  // Digital pin for Temperature&Humidity Sensor
#define DHTTYPE DHT22             // DHT22 sensor type
#define PIRMotionSensorPin 2      // Digital pin for PIR Motion Sensor

DHT dht(DHTPIN, DHTTYPE);

// Initialize variables
float currentNoise = 0;
float noiseAccumulator = 0;
float averageNoise = 0;
float currentIlluminance = 0;
float illuminanceAccumulator = 0;
float averageIlluminance = 0;
float currentHumidity = 0;
float humidityAccumulator = 0;
float averageHumidity = 0;
float currentTemperature = 0;
float temperatureAccumulator = 0;
float averageTemperature = 0;
float currentPIRMotion = 0;
float PIRMotionAccumulator = 0;
int sampleCount = 0;

// Define interval constants (in seconds)
const unsigned long SENSORS_READING_INTERVAL = 1;   // Sensors readings interval
const unsigned long SENDING_DATA_INTERVAL = 60;     // Sending data to ThingSpeak interval

// Boolean variable for debugging
bool debug = true; // Set to true for debugging, false for normal operation

void setup() {
  Serial.begin(9600);  // Initialize serial communication

  // Configure sensors and wait for serial connection
  pinMode(PIRMotionSensorPin, INPUT);
  dht.begin();
  while (!Serial) {
    ; // Wait for the serial port to connect
  }

  // Check for WiFi module  
  if (WiFi.status() == WL_NO_MODULE) {
    if (debug){
      Serial.println("Communication with WiFi module failed!");
    }
    while (true); // Stop execution
  }

  // Connect to WiFi
  if (WiFi.status() != WL_CONNECTED) {
    connectToWiFi(); 
  }

  // Initialize ThingSpeak
  ThingSpeak.begin(client);

}

void loop() {
  // Connect or reconnect to WiFi
  if (WiFi.status() != WL_CONNECTED) {
    connectToWiFi(); 
  }

  // Read sensor data every second
  readSensorData();

  // Accumulate sensor values
  noiseAccumulator += currentNoise;
  illuminanceAccumulator += currentIlluminance;
  humidityAccumulator += currentHumidity;
  temperatureAccumulator += currentTemperature;
  PIRMotionAccumulator += currentPIRMotion;

  // Increment the sample count
  sampleCount++;

  // Calculate the number of elapsed seconds for Sending Data intervals
  static unsigned long lastSendingDataIntervalMillis = 0;
  unsigned long currentMillis = millis();
  unsigned long elapsedSeconds = (currentMillis - lastSendingDataIntervalMillis) / 1000;

  // Send data to ThingSpeak every minute
  if (elapsedSeconds >= SENDING_DATA_INTERVAL) {

    calculateAverages(); // Calculate the average of the accumulated values

    sendDatatoThingSpeak(); // Send data to ThingSpeak

    // Update the last Sending Data interval time
    lastSendingDataIntervalMillis = currentMillis;

    resetValues();
  }

  // Delay for 1 second
  delay(SENSORS_READING_INTERVAL * 1000); // Convert seconds to milliseconds

  if (debug){
    Serial.print(".");
  }
}

void connectToWiFi() {
  if(debug){
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
  }

  while (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(ssid, pass);
    delay(5000);
  }
  if(debug){
    Serial.println("\nConnected to WiFi.");
  }
}

void readSensorData() {
  currentNoise = analogRead(NoiseSensorPin);
  currentIlluminance = analogRead(IlluminanceSensorPin);
  currentHumidity = dht.readHumidity();
  currentTemperature = dht.readTemperature();
  currentPIRMotion = digitalRead(PIRMotionSensorPin); 
}

void calculateAverages() {
  averageNoise = noiseAccumulator / sampleCount;
  averageNoise = averageNoise / 5; // Calibration
  averageIlluminance = illuminanceAccumulator / sampleCount;
  averageHumidity = humidityAccumulator / sampleCount;
  averageTemperature = temperatureAccumulator / sampleCount;
}

void sendDatatoThingSpeak(){
  if (debug) {
    Serial.println(".");
    Serial.print("Average Noise: ");
    Serial.println(averageNoise);
    Serial.print("Average Illuminance: ");
    Serial.println(averageIlluminance);
    Serial.print("Average Humidity: ");
    Serial.println(averageHumidity);
    Serial.print("Average Temperature: ");
    Serial.println(averageTemperature);
    Serial.print("PIRMotion Accumulator: ");
    Serial.println(PIRMotionAccumulator);
  }
    
  // Set fields
  ThingSpeak.setField(1, averageNoise);
  ThingSpeak.setField(2, averageIlluminance);
  ThingSpeak.setField(3, averageHumidity);
  ThingSpeak.setField(4, averageTemperature);
  ThingSpeak.setField(5, PIRMotionAccumulator);

  // HTTP request to ThingSpeak
  int httpCode = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  if (httpCode != 200 && debug) {
    Serial.println("Problem updating channel. HTTP error code " + String(httpCode));
  }
}

void resetValues() {
  noiseAccumulator = 0;
  illuminanceAccumulator = 0;
  humidityAccumulator = 0;
  temperatureAccumulator = 0;
  sampleCount = 0;
  averageNoise = 0;
  averageIlluminance = 0;
  averageHumidity = 0;
  averageTemperature = 0;
  PIRMotionAccumulator = 0;
}
