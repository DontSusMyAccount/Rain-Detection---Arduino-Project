#include <ESP8266WiFi.h>
#include <TridentTD_LineNotify.h>

const char* WIFI_SSID = "Your SSID"; // ชื่อ wifi
const char* WIFI_PASSWORD = "Your Pass"; // รหัส wifi
const char* LINE_TOKEN = "Your Line Token"; // ใส่ line token
const char* OPENWEATHERMAP_API_KEY = "Your API KEY "; // ใส่ API Key ของ OpenWeatherMap
const char* OPENWEATHERMAP_CITY = "Your City Name"; // เมืองที่คุณต้องการ

String OPENWEATHERMAP_API_URL = "http://api.openweathermap.org/data/2.5/weather?q=";

int rainSensorPin = A0; // หมายเลขขาที่เชื่อมต่อ Rain Sensor
int rainThreshold = 200; // ค่าที่ถือว่ามีฝนตก
bool raining = false; // ตัวแปรสถานะการฝน

unsigned long previousMillis = 0;
const long interval = 20000; // ระยะเวลาในการส่งข้อมูล (หน่วย: มิลลิวินาที) 1000 = 1 วินาที

void setup() {
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  LINE.setToken(LINE_TOKEN);
}

float getTemperatureFromOpenWeatherMap() {
  WiFiClient client;
  if (!client.connect("api.openweathermap.org", 80)) {
    Serial.println("Connection to OpenWeatherMap failed");
    return -1; // ค่าอุณหภูมิไม่ถูกต้อง
  }

  String apiUrl = OPENWEATHERMAP_API_URL + OPENWEATHERMAP_CITY + "&appid=" + OPENWEATHERMAP_API_KEY;
  
  client.print(String("GET ") + apiUrl + " HTTP/1.1\r\n" +
               "Host: api.openweathermap.org\r\n" +
               "Connection: close\r\n\r\n");

  while (!client.available());

  String response = "";
  while (client.available()) {
    char c = client.read();
    response += c;
  }

  // ตรวจสอบค่า response จาก OpenWeatherMap API
  if (response.indexOf("temp") == -1) {
    Serial.println("Temperature data not found");
    return -1; // ค่าอุณหภูมิไม่ถูกต้อง
  }

  // ดึงข้อมูลอุณหภูมิจาก JSON response
  int tempIndex = response.indexOf("\"temp\":");
  int tempEndIndex = response.indexOf(",", tempIndex);
  String tempString = response.substring(tempIndex + 7, tempEndIndex);
  float temperature = tempString.toFloat();

  // คำนวณอุณหภูมิเป็นองศาเซลเซียส
  temperature -= 273.15;

  return temperature;
}

void sendWeatherData(float temperature, bool isRaining) {
  String weatherStatus = isRaining ? "กำลังฝนตก" : "ไม่มีฝนตก";
  
  String message = "สภาพอากาศ Bangkok:\n";
  message += "อุณหภูมิ: " + String(temperature) + " °C\n";
  message += "สถานะการฝน: " + weatherStatus;

  const char* messageChar = message.c_str();
  
  LINE.notify(messageChar);
}

void loop() {
  unsigned long currentMillis = millis();

  // อ่านค่า Rain Sensor
  int rainValue = analogRead(rainSensorPin);
  Serial.print("Rain Value: ");
  Serial.println(rainValue);

  delay(1000);

  // ถ้าเวลาผ่านไปตามระยะเวลาที่กำหนด (interval)
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    float temperature = getTemperatureFromOpenWeatherMap();
    if (temperature != -1) {
      Serial.print("Temperature: ");
      Serial.println(temperature);
      sendWeatherData(temperature, raining); // ส่งข้อมูลสภาพอากาศ
    } else {
      Serial.println("Failed to fetch temperature data");
    }
  }

  // ตรวจสอบสถานะการฝน
  if (rainValue <= rainThreshold && !raining) {
    LINE.notify("มีฝนตกลงมา!");
    raining = true;
  }

  if (rainValue > rainThreshold && raining) {
    LINE.notify("ฝนหยุดตกแล้ว");
    raining = false;
  }
}
