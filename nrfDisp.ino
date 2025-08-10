#include <Adafruit_GFX.h>    
#include <Adafruit_ST7735.h> 
#include <SPI.h>
#include "printf.h"
#include "RF24.h"

#define CE_PIN   7
#define CSN_PIN  8
#define TFT_CS   10
#define TFT_RST  9 
#define TFT_DC   6

RF24 radio(CE_PIN, CSN_PIN);
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

uint8_t address[][6] = { "1Node", "2Node" };
bool radioNumber = 1;  // 0 uses address[0] to transmit, 1 uses address[1] to transmit
int tankHeight = 150;
int payload[2];
int bufer = 0;


void setup() {
  tft.initR(INITR_BLACKTAB);      // Init ST7735S chip, black tab
  tft.setRotation(1);
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0, 60);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  Serial.begin(115200);
  while (!Serial) {
    // some boards need to wait to ensure access to serial over USB
  }

  if (!radio.begin()) {
    Serial.println(F("radio hardware is not responding!!"));
    tft.println("nRF not responding!!");
    while (1) {
      if (radio.begin()){
        tft.fillScreen(ST77XX_BLACK);
        tft.setCursor(0, 60);
        tft.println("connecting to nRF using SPI");
        delay(1000);
        tft.fillScreen(ST77XX_BLACK);
        break;
      }
    }
  }

  Serial.println(F("nRF: Connected through SPI"));
  tft.println("nRF: Connected through SPI");

  radio.setPALevel(RF24_PA_LOW);  // RF24_PA_MAX is default.
  radio.setPayloadSize(sizeof(payload));  // float datatype occupies 4 bytes

  // set the TX address of the RX node into the TX pipe
  radio.openWritingPipe(address[radioNumber]);  // always uses pipe 0

  // set the RX address of the TX node into a RX pipe
  radio.openReadingPipe(1, address[!radioNumber]);  // using pipe 1
  radio.startListening();  
  
  tft.println("listening...");
  delay(1500);
  tft.fillScreen(ST77XX_BLACK);

}

void loop() {
  uint8_t pipe;
  if (radio.available(&pipe)) {              // is there a payload? get the pipe number that received it
    uint8_t bytes = radio.getPayloadSize();  // get the size of the payload
    radio.read(payload, bytes);              // fetch payload from FIFO
    drawtext(payload, tankHeight);
    Serial.print(F("Received "));
    Serial.print(bytes);  // print the size of the payload
    Serial.print(F(" bytes on pipe "));
    Serial.print(pipe);  // print the pipe number
    Serial.print(F(": "));
    Serial.print(payload[0]);  // print the payload's value
    Serial.print("    ");
    Serial.print(payload[1]);
    Serial.print(" volts\n");
    
    delay(1000);
  }
  else {
    tft.fillScreen(ST77XX_BLACK);
    while(!radio.available(&pipe)){
      tft.setCursor(52+bufer,50);
      tft.print(".");
      tft.setCursor(52,72);
      tft.println("No Signal");
      if(!(bufer = (bufer+5)%30)){
        tft.fillRoundRect(40, 38, 72, 30, 0, ST77XX_BLACK);
      }
      delay(1000);
    }
    tft.fillScreen(ST77XX_BLACK);
  }

  if (Serial.available()) {
    Serial.println("Default tank height is 100cm.\nEnter tank height(cm): ");  
    Serial.setTimeout(5000);
    int val = Serial.parseInt();
    if(val != 0){
      tankHeight = val;
    }
    Serial.println(tankHeight);
    
  }
}
  
void drawtext(int payload[2], int tankHeight) {
  tft.fillRoundRect(78, 38, 72, 125, 0, ST77XX_BLACK);
  tft.setTextSize(4);
  tft.setCursor(80, 40);
  tft.setTextColor(ST77XX_WHITE);
  int percent = percentify(payload[0], tankHeight);
  drawIndicator(percent);
  tft.println(percent);
  tft.setTextSize(1);
  tft.setCursor(80,72);
  tft.println("percentage");
  tft.setCursor(80,85);
  tft.print(tankHeight - payload[0]);
  tft.print(" cm");
  tft.setCursor(80,98);
  if(payload[1] > 20){
    tft.print("bat: ");
    tft.print(payload[1]);
    tft.print(" %");
  }else {
    tft.setTextColor(ST77XX_RED);
    tft.println("Low Battery!");
  }
  tft.setTextColor(ST77XX_YELLOW);
  tft.setCursor(80,111);
  tft.print(payload[0]);
}


void drawIndicator(int percent){
  percent = 10 - (percent/10);
  for (int i=0; i<percent; i++){
    tft.fillRoundRect(10, (i*12)+5, 45, 10, 0, ST77XX_BLACK);
  }
  for (int i=percent; i<10; i++){
    tft.fillRoundRect(10, (i*12)+5, 45, 10, 0, ST77XX_CYAN);
  }
}


int percentify(int distance, int tankHeight){
  if(distance <= 20 ){
    Serial.println("the tank is full");
    return 100;
  }
  else if (distance > 20 && distance < tankHeight){
    int level = (tankHeight - distance);
    int percent = (level * 100 ) / tankHeight;
    return percent;
  } 
  else if(distance> tankHeight){
    Serial.println("the tank is empty");
    return 0;
  } 
  else {
    Serial.println("something went wrong");
    return 0;
  }
}
