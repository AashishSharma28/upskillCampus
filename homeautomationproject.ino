#include <WiFi.h>            // For ESP32 WiFi
#include <PubSubClient.h>    // MQTT Library
#include <DHT.h>             // DHT Sensor Library

// WiFi Credentials
const char* ssid = "Your_SSID";         // Replace with your WiFi SSID
const char* password = "Your_PASSWORD"; // Replace with your WiFi Password

// MQTT Broker Information (Use HiveMQ, Mosquitto, or AWS IoT)
const char* mqtt_server = "broker.hivemq.com";  
const int mqtt_port = 1883;

// Define Sensor & Relay Pins
#define DHTPIN  4   // DHT11 Sensor Pin
#define PIR_PIN 5   // PIR Motion Sensor
#define LDR_PIN 36  // Light Sensor (LDR)

#define RELAY_1 16  // Light Control
#define RELAY_2 17  // Fan Control
#define RELAY_3 18  // Smart Plug
#define RELAY_4 19  // Security System

// Initialize DHT Sensor
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// WiFi and MQTT Setup
WiFiClient espClient;
PubSubClient client(espClient);

// Function to Connect to WiFi
void setup_wifi() {
    Serial.print("Connecting to WiFi...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("\nWiFi Connected!");
}

// MQTT Callback Function
void callback(char* topic, byte* payload, unsigned int length) {
    String message = "";
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    Serial.println("Received MQTT Message: " + message);

    // Control Relays Based on MQTT Commands
    if (strcmp(topic, "home/lights") == 0) digitalWrite(RELAY_1, message == "ON" ? HIGH : LOW);
    if (strcmp(topic, "home/fan") == 0) digitalWrite(RELAY_2, message == "ON" ? HIGH : LOW);
    if (strcmp(topic, "home/plug") == 0) digitalWrite(RELAY_3, message == "ON" ? HIGH : LOW);
    if (strcmp(topic, "home/security") == 0) digitalWrite(RELAY_4, message == "ON" ? HIGH : LOW);
}

// Function to Reconnect MQTT if Disconnected
void reconnect() {
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        if (client.connect("ESP32_HomeAuto")) {
            Serial.println("Connected to MQTT!");
            client.subscribe("home/lights");
            client.subscribe("home/fan");
            client.subscribe("home/plug");
            client.subscribe("home/security");
        } else {
            Serial.println("Failed. Retrying in 5s...");
            delay(5000);
        }
    }
}

void setup() {
    Serial.begin(115200);
    setup_wifi();
    
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);
    
    dht.begin();

    pinMode(RELAY_1, OUTPUT);
    pinMode(RELAY_2, OUTPUT);
    pinMode(RELAY_3, OUTPUT);
    pinMode(RELAY_4, OUTPUT);
    pinMode(PIR_PIN, INPUT);
    pinMode(LDR_PIN, INPUT);
}

void loop() {
    if (!client.connected()) reconnect();
    client.loop();

    // Read Temperature & Humidity from DHT11
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();

    // Read Light Sensor (LDR)
    int lightValue = analogRead(LDR_PIN);
    if (lightValue < 500) digitalWrite(RELAY_1, HIGH);  // Auto Turn ON Light in Dark
    else digitalWrite(RELAY_1, LOW);

    // Read PIR Motion Sensor
    if (digitalRead(PIR_PIN) == HIGH) {
        digitalWrite(RELAY_4, HIGH);
        client.publish("home/security_alert", "Motion Detected!");
    }

    // Publish Sensor Data to MQTT
    String tempStr = String(temperature);
    String humStr = String(humidity);
    client.publish("home/temperature", tempStr.c_str());
    client.publish("home/humidity", humStr.c_str());

    delay(3000);
}
