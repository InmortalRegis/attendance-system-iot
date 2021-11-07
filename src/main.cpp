#include <Arduino.h>
#include <MFRC522.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <WiFiClient.h>
#include <Wire.h>
#define RST_PIN D3 // RST-PIN for RC522 - RFID - SPI - Module GPIO
#define SS_PIN D4  // SDA-PIN for RC522 - RFID - SPI - Module GPIO
#define BUZZER D8
#define Relay D0

MFRC522 mfrc522(SS_PIN, RST_PIN); // Create MFRC522 instance

// const char *host = "http://192.168.1.10:4400"; // IP address of your local server

const char *host = "https://attendance-system-back.herokuapp.com";
String url = "/logs";

const char *ssid = "";
const char *password = "";
#define ACCESS_DELAY 3000 // amount of time in miliseconds that the relay will remain open
#define DENIED_DELAY 1000

// OLED Width and Height
#define OLED_WIDTH 128 // Width  OLED
#define OLED_HEIGHT 64 // Height  OLED

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

static const unsigned char PROGMEM image_data_authorized[72] = {
    0xff, 0xff, 0xff,
    0xff, 0x41, 0xff,
    0xfc, 0xbe, 0x3f,
    0xfb, 0xff, 0xbf,
    0xf7, 0xff, 0xcf,
    0xef, 0xff, 0xf7,
    0xcf, 0xff, 0xf3,
    0xdf, 0xef, 0xfb,
    0xdf, 0xef, 0xfb,
    0xff, 0xf3, 0xfd,
    0xff, 0xf9, 0xfd,
    0xaa, 0x54, 0xfd,
    0x55, 0x55, 0xfd,
    0xff, 0xf9, 0xfd,
    0xff, 0xf7, 0xfb,
    0xdf, 0xe7, 0xfb,
    0xdf, 0xff, 0xfb,
    0xdf, 0xff, 0xf7,
    0xe7, 0xff, 0xe7,
    0xf7, 0xff, 0xdf,
    0xf9, 0xff, 0x9f,
    0xfe, 0x7c, 0x7f,
    0xff, 0x03, 0xff,
    0xff, 0xff, 0xff};

static const unsigned char PROGMEM image_data_unauthorized[72] = {
    0xff, 0x01, 0xff,
    0xfc, 0x00, 0x3f,
    0xf8, 0x00, 0x1f,
    0xe0, 0xfe, 0x07,
    0xe1, 0xab, 0x87,
    0xc7, 0x01, 0x83,
    0x84, 0x01, 0x21,
    0x8c, 0x07, 0x71,
    0x18, 0x04, 0x51,
    0x10, 0x19, 0xd8,
    0x18, 0x13, 0x08,
    0x10, 0x76, 0x08,
    0x10, 0x44, 0x18,
    0x19, 0xdc, 0x08,
    0x11, 0x10, 0x18,
    0x9f, 0x60, 0x11,
    0x8c, 0x40, 0x31,
    0x81, 0xc0, 0x61,
    0xc3, 0x00, 0xc3,
    0xe1, 0xd7, 0x87,
    0xe0, 0x7d, 0x0f,
    0xf8, 0x00, 0x1f,
    0xfc, 0x00, 0x3f,
    0xff, 0x81, 0xff};

void connectWiFi()
{
  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Timer set to 5 seconds (timerDelay variable), it will take 5 seconds before publishing the first reading.");
}

void initRFID()
{
  //Init Serial Communication
  SPI.begin();        //Init SPI Bus
  mfrc522.PCD_Init(); // Init MFRC522
  Serial.println("RFID reading UID");
}

void initOledDisplay()
{
  // Iniciar pantalla OLED en la dirección 0x3C
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println("OLED Screen not found!");
  }

  // Limpiar buffer
  display.clearDisplay();

  // Tamaño del texto
  display.setTextSize(1);
  // Color del texto
  display.setTextColor(SSD1306_WHITE);
  // Posición del texto
  display.setCursor(26, 32);
  //Activar página de código 437
  display.cp437(true);
  // Escribir el carácter ¡
  display.write(173);
  // Escribir texto
  display.println("Bienvenido!");

  // Enviar a pantalla
  display.display();

  delay(3000);
}

void oledDisplayCenter(String text)
{
  int16_t x1;
  int16_t y1;
  uint16_t width;
  uint16_t height;

  display.getTextBounds(text, 0, 0, &x1, &y1, &width, &height);

  // display on horizontal and vertical center
  display.clearDisplay(); // clear display
  display.setCursor((OLED_WIDTH - width) / 2, (OLED_HEIGHT - height) / 2);
  display.println(text); // text to display
  display.display();
}

void authorize()
{

  digitalWrite(Relay, HIGH);
  tone(BUZZER, 440);
  delay(ACCESS_DELAY); // wait for a second
  noTone(BUZZER);
  digitalWrite(Relay, LOW);
}

void initRelay()
{
  pinMode(Relay, OUTPUT);
  digitalWrite(Relay, LOW);
}

void initBuzzer()
{
  pinMode(BUZZER, OUTPUT);
  tone(BUZZER, 440);
  delay(ACCESS_DELAY);
  noTone(BUZZER);
}

void unauthorized()
{
  tone(BUZZER, 440);
  delay(DENIED_DELAY);
  noTone(BUZZER);
}

void setup()
{

  Serial.begin(9600);
  connectWiFi();
  initRFID();
  initOledDisplay();
  initBuzzer();
  initRelay();
}

void loop()
{

  /**
   * Read UID from RFID
   */
  // Revisamos si hay nuevas tarjetas  presentes
  if (!mfrc522.PICC_IsNewCardPresent())
  {
    delay(50);
    return;
  }
  //Seleccionamos una tarjeta
  if (!mfrc522.PICC_ReadCardSerial())
  {
    delay(50);
    return;
  }
  // We send your UID to serial monitor
  Serial.print("Tag UID:");
  String content = "";
  for (byte i = 0; i < mfrc522.uid.size; i++)
  {
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  Serial.println();
  content.toUpperCase();
  content.trim();
  Serial.println("Card read: " + content);
  //Finish tag reading
  mfrc522.PICC_HaltA();

  /** 
   * Server API
   */

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;

  // Your Domain name with URL path or IP address with path
  http.begin(client, host + url);

  // If you need an HTTP request with a content type: application/json, use the following:
  http.addHeader("Content-Type", "application/json");
  int httpResponseCode = http.POST("{\"uid\":\"" + content + "\"}");

  Serial.print("HTTP Response code: ");
  Serial.println(httpResponseCode);
  if (httpResponseCode > 0)
  {
    String payload = http.getString();
    Serial.println(payload);
    if (httpResponseCode == HTTP_CODE_OK)
    {

      Serial.println("Authorized Entry");
      oledDisplayCenter("Authorized!");
      display.drawBitmap(0, 20, image_data_authorized, 22, 22, SSD1306_WHITE);
      display.display();

      authorize();
    }
    else
    {
      Serial.println("Unauthorized entry");
      oledDisplayCenter("Unauthorized!");
      display.drawBitmap(0, 20, image_data_unauthorized, 22, 22, SSD1306_WHITE);
      display.display();

      for (size_t i = 0; i < 2; i++)
      {
        delay(250);
        unauthorized();
      }
    }
  }
  else
  {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
    oledDisplayCenter("No connection!");
  }

  // Free resources
  http.end();
}