#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

#define OLED_RESET    -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C // I2C address for the SSD1306
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define SS_PIN 53  // Pin 53
#define RST_PIN 5  // Pin 5

MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class

MFRC522::MIFARE_Key key;

// Init array that will store new NUID
byte nuidPICC[4];

// Helper routine to dump a byte array as hex values to Serial
void printHex(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

// Helper routine to dump a byte array as dec values to Serial
void printDec(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(' ');
    Serial.print(buffer[i], DEC);
  }
}

void displayHex(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    display.print(buffer[i] < 0x10 ? " 0" : " ");
    display.print(buffer[i], HEX);
  }
  display.println();
}

void displayDec(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    display.print(buffer[i], DEC);
    display.print(" ");
  }
  display.println();
}

void setup() {
  Serial.begin(9600); // Serial monitor
  Serial2.begin(9600); // UART2 with Pin 16 is TX2 and Pin 17 is RX2

  SPI.begin(); // Init SPI bus
  rfid.PCD_Init(); // Init MFRC522

  Serial.println(F("Ready to read RFID tags"));

  // Initialize the display
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Clear the buffer
  display.clearDisplay();
  display.display();
}

void loop() {
  if (Serial2.available()) {
    String message = Serial2.readString();
    Serial.println("Received from ESP32: " + message);
  }

  // Nhan du lieu tu ban phim va gui den ESP32
  // Receive data from the keyboard or from the RFID and send it to the ESP32
  if (Serial.available()) {
    String input = Serial.readString();
    Serial2.println(input);
  }

  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if (!rfid.PICC_IsNewCardPresent())
    return;

  // Veryfy if the NUID has been readed
  if (!rfid.PICC_ReadCardSerial())
    return;

  Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  Serial.println(rfid.PICC_GetTypeName(piccType));

  // Check is the PICC of Classis MIFARE type
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&
      piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
      piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
        Serial.println(F("Your tag is not of type MIFARE Classic."));
        return;
      }

  if (rfid.uid.uidByte[0] != nuidPICC[0] ||
      rfid.uid.uidByte[1] != nuidPICC[1] ||
      rfid.uid.uidByte[2] != nuidPICC[2] ||
      rfid.uid.uidByte[3] != nuidPICC[3]) {

        Serial.println(F("A new card has been detected."));

        // Store NUID into nuidPICC array
        for (byte i = 0; i < 4; i++) {
          nuidPICC[i] = rfid.uid.uidByte[i];
        }

        Serial.println(F("The NUID tag is:"));
        Serial.print(F("In hex: "));
        printHex(rfid.uid.uidByte, rfid.uid.size);
        Serial.println();
        Serial.print(F("In dec: "));
        printDec(rfid.uid.uidByte, rfid.uid.size);
        Serial.println();

        // Create the message string
        String message = "PICC type: " + String(rfid.PICC_GetTypeName(piccType)) + "\n";
        message += "A new card has been detected.\n";
        message += "The NUID tag is:\n";
        message += "In hex: ";
        for (byte i = 0; i < rfid.uid.size; i++) {
          message += String(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
          message += String(rfid.uid.uidByte[i], HEX);
        }
        message += "\nIn dec: ";
        for (byte i = 0; i < rfid.uid.size; i++) {
          message += String(rfid.uid.uidByte[i], DEC) + " ";
        }
        message += "\n";

        // Send the message to ESP32
        Serial2.print(message);

        //Display on OLED
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.print(F("PICC type:"));
        display.println(rfid.PICC_GetTypeName(piccType));
        display.println(F("NUID tag:"));
        display.print(F("Hex: "));
        displayHex(rfid.uid.uidByte, rfid.uid.size);
        display.print(F("Dec: "));
        displayDec(rfid.uid.uidByte, rfid.uid.size);
        display.display();
      } 
      else {
        // Thẻ đã được đọc trước đó
        String message = "PICC type: " + String(rfid.PICC_GetTypeName(piccType)) + "\n";
        message += "Card read previously.\n";
        Serial2.print(message);
        
        Serial.println(F("Card read previously."));

        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.print(F("PICC type:"));
        display.println(rfid.PICC_GetTypeName(piccType));
        display.println(F("Card read previously."));
        display.display();
      }
    
    // Halt PICC
  rfid.PICC_HaltA();

  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();
  delay(100);
}
