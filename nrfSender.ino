#include <SPI.h>
#include "printf.h"
#include "RF24.h"

#define batPin 5
#define ECHOPIN 2// Pin to receive echo pulse
#define TRIGPIN 3// Pin to send trigger pulse
#define CE_PIN 7
#define CSN_PIN 8
// instantiate an object for the nRF24L01 transceiver
RF24 radio(CE_PIN, CSN_PIN);

// Let these addresses be used for the pair
uint8_t address[][6] = { "1Node", "2Node" };
// It is very helpful to think of an address as a path instead of as an identifying device destination


int tot = 0;
int bat = 0;
int calibrate = 0;
int preVal = 0;
int distance = 0;
int payload[2];
//bool reverse=false;


void setup() {
  
  pinMode(ECHOPIN, INPUT);
  pinMode(TRIGPIN, OUTPUT);

  Serial.begin(115200);
  while (!Serial) {
    // some boards need to wait to ensure access to serial over USB
  }

  // initialize the transceiver on the SPI bus
  if (!radio.begin()) {
    Serial.println(F("radio hardware is not responding!!"));
    while (1) {                 // hold in infinite loop
      Serial.print(".");
      if (radio.begin()) {
        break;
        }  
      }
  }

  // print example's introductory prompt
  Serial.println(F("Let's Get Started"));
  
  // Set the PA Level low
  radio.setPALevel(RF24_PA_LOW);  // RF24_PA_MAX is default.

  // save on transmission time by setting the radio to only transmit the
  // number of bytes we need to transmit a float
  radio.setPayloadSize(sizeof(payload));  // float datatype occupies 4 bytes

  // set the TX address of the RX node into the TX pipe
  radio.openWritingPipe(address[0]);  // always uses pipe 0

  // set the RX address of the TX node into a RX pipe
  radio.openReadingPipe(1, address[1]);  // using pipe 1

  // additional setup specific to the node's role
  radio.stopListening();  // put radio in TX mode
  

  // For debugging info
  // printf_begin();             // needed only once for printing details
  // radio.printDetails();       // (smaller) function that prints raw register values
  // radio.printPrettyDetails(); // (larger) function that prints human readable data

}  // setup

void loop() {
    distance = sonar();
//    Serial.println(distance);
    bat = batfun();
    int payload[] = {distance,bat};

    unsigned long start_timer = micros();                // start the timer
    bool report = radio.write(payload, radio.getPayloadSize());  // transmit & save the report
    unsigned long end_timer = micros();                  // end the timer
    

    if (report) {
      tot = end_timer - start_timer;
      Serial.print(F("Transmission successful! "));  // payload was delivered
      Serial.print(F("Time to transmit = "));
      Serial.println(tot);  // print the timer result
      Serial.print(F(" us. Sent: "));
      Serial.println(distance);  // print payload sent
    } else {
      Serial.println(F("Transmission failed or timed out"));  // payload was not delivered
    }
delay(1000);

}  // loop

int sonar(){
  digitalWrite(TRIGPIN, LOW); // Set the trigger pin to low for 2uS
  delayMicroseconds(2);
  digitalWrite(TRIGPIN, HIGH); // Send a 10uS high to trigger ranging
  delayMicroseconds(10);
  digitalWrite(TRIGPIN, LOW); // Send pin low again
  int distance = pulseIn(ECHOPIN, HIGH,26000); // Read in times pulse
  distance= distance/58;
  Serial.print(distance);
  Serial.print(" cm    ");
  delay(50);// Wait 50mS before next ranging
  return stabalize(distance);
 }

 int stabalize(int nowVal){
  if(calibrate){
    int jump = (preVal - nowVal);
    jump < 0 ? jump*=-1: jump;
    if(jump>1 && jump<5){
      preVal = nowVal;
    }
    if(preVal > 30){
      calibrate = false;
    }
  }
  else{
    preVal = nowVal;
    if(preVal < 26){
      calibrate = true;
    }
  }
  return preVal;  
 }


int batfun(){
    int analogValue = analogRead(batPin);
    float voltage = analogValue*(5.0/1023.0);
    if(voltage >= 3.95){
      return 100;
      }
    else if(voltage > 3.8){
      return 80;
    }
    else if(voltage > 3.73){
      return 60;
    }
    else if(voltage > 3.68){
      return 40;
    }
    else if(voltage <= 3.68){
      return 20;
    }
}
