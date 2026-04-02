/*
  LoRa Duplex communication wth callback

  Sends a message every half second, and uses callback
  for new incoming messages. Implements a one-byte addressing scheme,
  with 0xFF as the broadcast address.

  Note: while sending, LoRa radio is not listening for incoming messages.
  Note2: when using the callback method, you can't use any of the Stream
  functions that rely on the timeout, such as readString, parseInt(), etc.

  created 28 April 2017
  by Tom Igoe
*/
#include <SPI.h>              // include libraries
#include <LoRa.h>
#include <Arduino.h>
#include <U8x8lib.h>
#include <Wire.h>

#ifdef U8X8_HAVE_HW_SPI
#endif

#ifdef ARDUINO_SAMD_MKRWAN1300
#error "This example is not compatible with the Arduino MKR WAN 1300 board!"
#endif

const int csPin = 10;          // LoRa radio chip select
const int resetPin = 14;       // LoRa radio reset
const int irqPin = 15;         // change for your board; must be a hardware interrupt pin
int pushButtonMiddlePin = 17;
volatile bool isPushButtonPressed = false;
int relay1Pin = 2;
int relay1State = 0;
int lastInterruptTime = 0;
char timeString[16];

String message = "HeLoRa World!";   // send a message
String recievedMessage = "";
String toggleRelayMiddle = "Toggle relay";
String sendMyMessage = "";

String outgoing;              // outgoing message
byte msgCount = 0;            // count of outgoing messages
byte localAddress = 0xBB;     // address of this device
byte destination = 0xFF;      // destination to send to
int lastSendTime = 0;        // last send time
int lastRecievedTime = 0;    // last recieved time
int interval = 2000;          // interval between sends

U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(U8X8_PIN_NONE);

void setup() {
  Serial.begin(115200);                   // initialize serial

  while (!Serial);

  // ********** LoRa setup **********
  Serial.println("LoRa Duplex with callback");
  //SPI.begin(SCK, MISO, MOSI, SS);
  SPI.begin(12, 13, 11, 10);  
  // override the default CS, reset, and IRQ pins (optional)
  LoRa.setPins(csPin, resetPin, irqPin);// set CS, reset, IRQ pin
  if (!LoRa.begin(915E6)) {             // initialize ratio at 915 MHz
    Serial.println("LoRa init failed. Check your connections.");
    while (true);                       // if failed, do nothing
  }
  LoRa.onReceive(onReceive);
  LoRa.receive();
  Serial.println("LoRa init succeeded.");
  // ********** LoRa setup **********
  
  // ********** Button and relay setup **********
  attachInterrupt(digitalPinToInterrupt(pushButtonMiddlePin), pushButtonISR, RISING);
  pinMode(relay1Pin, OUTPUT);
  digitalWrite(relay1Pin, LOW);
  // ********** Button and relay setup **********

  // ********** Display setup **********
  Wire.begin(9, 8);    // Set custom SDA and SCL pins
  u8x8.begin();
  u8x8.setPowerSave(0);
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.setInverseFont(1);
  u8x8.drawString(0,0," UNSW ROCKETRY ");
  u8x8.setInverseFont(0);
  // ********** Display setup **********
}

void loop() {
  if (millis() - lastSendTime > interval) {
    if(isPushButtonPressed){
      sendMessage(toggleRelayMiddle);
      Serial.println("->:" + toggleRelayMiddle);
      sendMyMessage = "->:" + toggleRelayMiddle;
      u8x8.drawString(0,1,"->:             ");
      u8x8.drawString(0,1,sendMyMessage.c_str());
      isPushButtonPressed = false;
    } else{
      sendMessage(message);
      Serial.println("->:" + message);
      sendMyMessage = "->:" + message;
      u8x8.drawString(0,1,"->:             ");
      u8x8.drawString(0,1,sendMyMessage.c_str());
    }
    
    sprintf(timeString, "%d", (millis() - lastSendTime));
    sendMyMessage = "Last sent:" + String(timeString);
    u8x8.drawString(0,2,"Last sent:      ");
    u8x8.drawString(0,2,sendMyMessage.c_str());
    lastSendTime = millis();            // timestamp the message
    interval = 250;//random(2000) + 1000;     // 2-3 seconds
    LoRa.receive();                     // go back into receive mode
    if(recievedMessage == toggleRelayMiddle){
      (relay1State == 1) ? relay1State = 0 : relay1State = 1;
      digitalWrite(relay1Pin, relay1State);
      message = "HeLoRa World!"; // reset back to HeLoRa World to prevent constant change
    }
  }

  sendMyMessage = "<-:" + recievedMessage;
  u8x8.drawString(0,3,"<-:             ");
  u8x8.drawString(0,3,sendMyMessage.c_str());
  sprintf(timeString, "%d", (millis() - lastRecievedTime));
  sendMyMessage = "Last rcvd:" + String(timeString);
  u8x8.drawString(0,4,"Last rcvd:      ");
  u8x8.drawString(0,4,sendMyMessage.c_str());

  sendMyMessage = "Relay 1: " + String(relay1State);
  u8x8.drawString(0,5,"Relay 1:        ");
  u8x8.drawString(0,5,sendMyMessage.c_str());
}

void pushButtonISR(){
  unsigned long currentTime = millis();
  if (currentTime - lastInterruptTime > 200) { // 200 ms debounce
    isPushButtonPressed = true;
    lastInterruptTime = currentTime;
  }
}

void sendMessage(String outgoing) {
  LoRa.beginPacket();                   // start packet
  LoRa.write(destination);              // add destination address
  LoRa.write(localAddress);             // add sender address
  LoRa.write(msgCount);                 // add message ID
  LoRa.write(outgoing.length());        // add payload length   
  LoRa.print(outgoing);                 // add payload
  LoRa.endPacket();                     // finish packet and send it
  msgCount++;                           // increment message ID
}

void onReceive(int packetSize) {
  if (packetSize == 0){
    u8x8.drawString(0,1,"Scanning...");
    return;          // if there's no packet, return
  }


  // read packet header bytes:
  int recipient = LoRa.read();          // recipient address
  byte sender = LoRa.read();            // sender address
  byte incomingMsgId = LoRa.read();     // incoming msg ID
  byte incomingLength = LoRa.read();    // incoming msg length

  String incoming = "";                 // payload of packet

  while (LoRa.available()) {            // can't use readString() in callback, so
    incoming += (char)LoRa.read();      // add bytes one by one
  }

  if (incomingLength != incoming.length()) {   // check length for error
    Serial.println("error: message length does not match length");
    return;                             // skip rest of function
  }

  // if the recipient isn't this device or broadcast,
  if (recipient != localAddress && recipient != 0xFF) {
    Serial.println("This message is not for me.");
    return;                             // skip rest of function
  }

  recievedMessage = incoming;
  // if message is for this device, or broadcast, print details:
  Serial.println("Received from: 0x" + String(sender, HEX));
  Serial.println("Sent to: 0x" + String(recipient, HEX));
  Serial.println("Message ID: " + String(incomingMsgId));
  Serial.println("Message length: " + String(incomingLength));
  Serial.println("Message: " + incoming);
  Serial.println("RSSI: " + String(LoRa.packetRssi()));
  Serial.println("Snr: " + String(LoRa.packetSnr()));
  Serial.println();
  lastRecievedTime = millis();
}

