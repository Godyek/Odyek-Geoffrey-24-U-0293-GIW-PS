#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ====== WiFi Credentials ======
const char* ssid = "eduroam.";
const char* password = "asku8674";

// ====== WhatsApp (CallMeBot) ======
// Phone number format: COUNTRYCODE + NUMBER (no +, no leading 0)
String phoneNumber = "256771239239";   // Example for Uganda
String apiKey = "4289205";             // Replace with your personal key

// ====== LCD Setup (I2C) ======
// ESP32 default I2C pins: SDA = 21, SCL = 22
LiquidCrystal_I2C lcd(0x27, 16, 2);  

// ====== Hardware Pins ======
#define SOUND_SENSOR_PIN 34   // Analog pin for sound sensor
#define BUZZER_PIN 25         // Digital pin for buzzer
#define LED_PIN 26            // Digital pin for LED

// ====== Settings ======
int noiseThreshold = 2000;              // Adjust based on testing
unsigned long messageInterval = 360000; // 6 minutes (in ms)
unsigned long lastMessageTime = 0;

// ====== Function to URL-encode text ======
String urlencode(String str) {
  String encodedString = "";
  char c;
  char code0;
  char code1;
  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (isalnum(c)) {
      encodedString += c;
    } else {
      code1 = (c & 0xf) + '0';
      if ((c & 0xf) > 9) code1 = (c & 0xf) - 10 + 'A';
      c = (c >> 4) & 0xf;
      code0 = c + '0';
      if (c > 9) code0 = c - 10 + 'A';
      encodedString += '%';
      encodedString += code0;
      encodedString += code1;
    }
  }
  return encodedString;
}

// ====== Function to send WhatsApp message ======
void sendWhatsAppMessage(String message) {
  if ((WiFi.status() == WL_CONNECTED)) {
    HTTPClient https;
    String url = "https://api.callmebot.com/whatsapp.php?phone=" + phoneNumber +
                 "&text=" + urlencode(message) + "&apikey=" + apiKey;

    https.begin(url);
    int httpCode = https.GET();

    if (httpCode == 200) {
      Serial.println("✅ WhatsApp message sent successfully!");
    } else if (httpCode == 503) {
      Serial.println("⚠️ Server busy, retrying after 5s...");
      delay(5000);
      https.GET(); // retry once
    } else {
      Serial.println("❌ Error sending message. HTTP code: " + String(httpCode));
    }

    https.end();
  }
}

// ====== Setup ======
void setup() {
  Serial.begin(115200);

  // LCD
  Wire.begin(21, 22);  // SDA = 21, SCL = 22
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("CalmZone Ready");

  // Buzzer & LED
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  // WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi Connected!");
}

// ====== Loop ======
void loop() {
  int soundValue = analogRead(SOUND_SENSOR_PIN);
  Serial.println("Sound Level: " + String(soundValue));

  if (soundValue > noiseThreshold) {
    // Activate alarm
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);

    // Display warning on LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Stop making noise");
    lcd.setCursor(0, 1);
    lcd.print("Exams in progress");

    // Send WhatsApp message every 6 min only
    if (millis() - lastMessageTime > messageInterval) {
      sendWhatsAppMessage("Some noise persisting, go ahead and warn them to stop making noise");
      lastMessageTime = millis();
    }  
  } else {
    // Silence alarm
    digitalWrite(LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);

    // LCD standby
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("CalmZone Active");
  }

  delay(1000); // 1 sec loop delay
}
