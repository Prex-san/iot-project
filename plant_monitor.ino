#include <WiFi.h>
#include "DHT.h"
#include "ThingSpeak.h"
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <ESP32Servo.h>

#define WIFI_SSID "######"
#define WIFI_PASS "######"

#define DHTPIN 4
#define DHTTYPE DHT11
#define SERVO_PIN 13
#define MOISTURE_PIN 34
#define LDR_PIN 35
#define WATER_LEVEL_PIN 32

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C

#define CHANNEL_ID 3100507
#define WRITE_API "######"

#define DRY_THRESHOLD 2500
#define WET_THRESHOLD 1200
#define LIGHT_BRIGHT 1000
#define LIGHT_DARK 3000
#define WATER_FULL 2800
#define WATER_LOW 1200

#define DATA_UPDATE_INTERVAL 15000
#define DISPLAY_SWITCH_INTERVAL 3000
#define SERVO_SWEEP_INTERVAL 20

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
WiFiClient client;
DHT dht(DHTPIN, DHTTYPE);
Servo myServo;

unsigned long lastUpdateTime = 0;
unsigned long lastDisplaySwitch = 0;
unsigned long lastServoMove = 0;

float temperature = 0;
float humidity = 0;
int moistureValue = 0;
int lightValue = 0;
int waterLevelValue = 0;

int servoPos = 0;
int servoDirection = 1;
int displayMode = 0;

void connectWiFi()
{
    Serial.print("Connecting to WiFi");
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(300);
    }
    Serial.println("\nWiFi connected!");
}

void setupDisplay()
{
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR))
    {
        Serial.println("OLED init failed!");
        while (1)
            ;
    }
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(15, 25);
    display.println("Smart Plant System");
    display.display();
    delay(1000);
}

void drawBar(int x, int y, int width, int height, int value, int maxValue)
{
    int filled = map(value, 0, maxValue, 0, width);
    display.drawRect(x, y, width, height, SSD1306_WHITE);
    display.fillRect(x, y, filled, height, SSD1306_WHITE);
}

void updateSensorData()
{
    temperature = dht.readTemperature();
    humidity = dht.readHumidity();
    moistureValue = analogRead(MOISTURE_PIN);
    lightValue = analogRead(LDR_PIN);
    waterLevelValue = analogRead(WATER_LEVEL_PIN);

    Serial.printf("Temp: %.1f°C | Hum: %.1f%% | Moist: %d | Light: %d | Water: %d\n",
                  temperature, humidity, moistureValue, lightValue, waterLevelValue);
}

void sendToThingSpeak()
{
    ThingSpeak.setField(1, temperature);
    ThingSpeak.setField(2, humidity);
    ThingSpeak.setField(3, moistureValue);
    ThingSpeak.setField(4, lightValue);
    ThingSpeak.setField(5, waterLevelValue);
    ThingSpeak.writeFields(CHANNEL_ID, WRITE_API);
}

void showTemperature()
{
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(10, 20);
    display.print("Temp: ");
    display.print(temperature, 1);
    display.println("C");
    display.display();
}

void showHumidity()
{
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(10, 20);
    display.print("Hum: ");
    display.print(humidity, 1);
    display.println("%");
    display.display();
}

void showMoisture()
{
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("Soil Moisture:");

    if (moistureValue > DRY_THRESHOLD)
    {
        display.setCursor(20, 20);
        display.println("  (x_x)");
        display.setCursor(5, 35);
        display.println("Dry! Needs water");
    }
    else if (moistureValue < WET_THRESHOLD)
    {
        display.setCursor(20, 20);
        display.println("  (~_~)");
        display.setCursor(15, 35);
        display.println("Too wet!");
    }
    else
    {
        display.setCursor(20, 20);
        display.println("  (^_^)");
        display.setCursor(10, 35);
        display.println("Perfect soil");
    }

    drawBar(10, 55, 100, 6, 4095 - moistureValue, 4095);
    display.display();
}

void showLight()
{
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("Light Sensor:");

    if (lightValue < LIGHT_BRIGHT)
    {
        display.setCursor(20, 25);
        display.println("[ Sunlight! ]");
    }
    else if (lightValue > LIGHT_DARK)
    {
        display.setCursor(20, 25);
        display.println("[ It's Dark ]");
    }
    else
    {
        display.setCursor(20, 25);
        display.println("[ Moderate ]");
    }

    drawBar(10, 55, 100, 6, 4095 - lightValue, 4095);
    display.display();
}

void showWaterLevel()
{
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("Water Tank:");

    if (waterLevelValue > WATER_FULL)
    {
        display.setCursor(15, 25);
        display.println("[ FULL ]");
    }
    else if (waterLevelValue < WATER_LOW)
    {
        display.setCursor(15, 25);
        display.println("[ LOW  ]");
    }
    else
    {
        display.setCursor(15, 25);
        display.println("[ OK   ]");
    }

    drawBar(10, 55, 100, 6, waterLevelValue, 4095);
    display.display();
}

void showDashboard()
{
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("== Plant Dashboard ==");

    display.setCursor(0, 12);
    display.printf("Temp: %.1fC\n", temperature);
    display.setCursor(0, 22);
    display.printf("Hum : %.1f%%\n", humidity);
    display.setCursor(0, 32);
    display.printf("Soil: %d\n", moistureValue);
    display.setCursor(0, 42);
    display.printf("Light: %d\n", lightValue);
    display.setCursor(0, 52);
    display.printf("Water: %d\n", waterLevelValue);

    display.display();
}

void moveServoAuto()
{
    if (moistureValue > DRY_THRESHOLD && waterLevelValue > WATER_LOW)
    {
        Serial.println("Dry soil -> Watering plant...");
        myServo.write(45);
        delay(2000);
        myServo.write(0);
    }
    else if (waterLevelValue <= WATER_LOW)
    {
        Serial.println("Tank empty -> Skipping watering.");
    }
}

void setup()
{
    Serial.begin(115200);
    dht.begin();
    myServo.attach(SERVO_PIN);
    connectWiFi();
    ThingSpeak.begin(client);
    setupDisplay();
    pinMode(MOISTURE_PIN, INPUT);
}

void loop()
{
    unsigned long currentMillis = millis();

    if (currentMillis - lastUpdateTime >= DATA_UPDATE_INTERVAL)
    {
        updateSensorData();
        sendToThingSpeak();
        moveServoAuto();
        lastUpdateTime = currentMillis;
    }

    if (currentMillis - lastDisplaySwitch >= DISPLAY_SWITCH_INTERVAL)
    {
        displayMode = (displayMode + 1) % 6;
        switch (displayMode)
        {
        case 0:
            showTemperature();
            break;
        case 1:
            showHumidity();
            break;
        case 2:
            showMoisture();
            break;
        case 3:
            showLight();
            break;
        case 4:
            showWaterLevel();
            break;
        case 5:
            showDashboard();
            break;
        }
        lastDisplaySwitch = currentMillis;
    }
}
