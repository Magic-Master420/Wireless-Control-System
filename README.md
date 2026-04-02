# Wireless Control System

## Overview
This project was tasked by UNSW rocketry's propulsions sub-team to design a wireless module which is used to launch a two stage rocket over a 800m distance and retrieve sensor data.

The module is powered by an ESP32-S3-MINI-U1 which features a dual core processor, where each processor can be used for a dedicated function. For wireless communication, it is done over LoRa. An off-the-shelf module is utilized in this design. Other features includes an OLED display for viewing information, battery charging system to make the device modular, relays for high voltage/current control, and ethernet for high bandwidth wired communication.

## Known issues

## Notes
- Relays utilized can be connected to mains voltage for high power control. See BOM for part number and search the datasheet for more details on the part.
- The code is written such that the data is transmitted without checking whether or not it has successfully been received on the other end. The transport layer needs to be improved to ensure reliable communication. Here are some recommended transport layer protocols:
  - Stop-And-Wait:     low throughput, low memory
  - Go-Back-N          High throughput, low memory
  - Selective-Repeat   High throughput, high memory
