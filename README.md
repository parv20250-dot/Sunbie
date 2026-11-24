# Sunbie

Sunbie is a Tamagotchi inspired product that utilises a Waveshare ESP 32- C3 microprocessor.
It uses sensors such as the ADXL345 and the DHT11 so as to gather data from the environment, it also has a sprite (in this case a fox) 
that responds to any changes in the temperature. The accelerometer enables the user to interact with the sprite so as to feed her, play with her,
pet her or (if she's sick) give her meds. You look after it by tilting the device to pick actions from a menu.

## PCB Diagram and 3d view
<img width="382" height="317" alt="pcb new 2" src="https://github.com/user-attachments/assets/aacaa1a9-850a-4469-9fc8-4bae0f72e7b5" />


<img width="483" height="397" alt="Annotation 2026-10-22 135644" src="https://github.com/user-attachments/assets/69ed9cfa-b0d9-4de6-9bbe-8f7fc1d4341a" />


<img width="418" height="368" alt="Annotation 2026-10-22 135615" src="https://github.com/user-attachments/assets/b47526b4-8fea-4532-8c8b-2923065ea7a3" />

## Schematic

<img width="575" height="323" alt="image" src="https://github.com/user-attachments/assets/64201c1b-918a-4c77-af43-c01141430028" />

The above image contains the schematic that the PCB diagram is made with reference to,  labels have been used for different nets so as to make the schematic look cleaner.

## Actions


| Action | Joy | Energy | Fullness | Health |
|---|---|---|---|---|
| Nap | +1 | +18 | −4 | +3 |
| Play | +12 | −9 | −5 | −4 |
| Feed | +3 | −2 | +15 | +1 |
| Pet | +7 | +2 | 0 | 0 |
| Meds | −7 | −3 | 0 | +7 |


A 3d visual of the device on Kicad

## Why was this made?

This was made as a part of Hack Club's Half Life programme.

## Who is the sprite?

The sprite (pet) can be set to anything the user desires by changing the Bit Image in the source code. However, I use a fox.
inspired by the image below.

<img width="91" height="96" alt="Annotation 2026-10-10 102953 (2)" src="https://github.com/user-attachments/assets/6004e3a8-292f-489c-bfd6-f32ab7e495ad" />

The code is written in C++ Arduino framework and uses an SSDI306 OLED to display the fox and menu, an ADXL345 in order to detect tilts and also detect shaking (shake/dizzy pet reactions), a DHT11 for temperature and humidity readings and temperature reactions, and a buzzer for sounds. The pet is saved, it can be reset by setting RESET_SAVED_PET to true,uploading it and then setting it to false again!!

## BOM
|Item                                 |Qty|Supplier   |Cost/item|Total Cost|Funding Source|Link                                                                    |
|-------------------------------------|---|-----------|---------|----------|--------------|------------------------------------------------------------------------|
|ADXL345 - I2C SPI                    |1  |electro:kit|$5.52    |$5.52     |HC            |https://www.electrokit.com/adxl345-accelerometer-3-axel-monterad-pa-kort|
|ESP32-C3 developmentboard mini 4MB   |1  |electro:kit|$7.12    |$7.12     |HC            |https://www.electrokit.com/esp32-c3-utvecklingskort-with-headers                   |
|OLED Display 0.96 inch I2C 128x64    |1  |electro:kit|$6.32    |$6.32     |HC            |https://www.electrokit.com/lcd-oled-0.96128x64px-i2c-ssd1306            |
|Passive piezo buzzer 70dB PS12 PCB   |1  |electro:kit|$1.20    |$1.20     |HC            |https://www.electrokit.com/passiv-piezosummer-70db-pcb-12x6.5mm         |
|WS Standard Linear Switch            |2  |Max Gaming |$0.15    |$0.30     |HC            |https://www.maxgaming.se/sv/switchar/ws-standard-linear-switch          |
|PCB                                  |1  |JLCPCB     |$4.00    |$4.00     |Self          |https://jlcpcb.com/                                                     |
|10kΩ Resistor                        |1  |electro:kit|$0.10    |$0.10     |Self(have)    |https://www.electrokit.com/motstand-kolfilm-0.25w-10kohm-10k            |
|DHT11 Temperature and Humidity Sensor|1  |electro:kit|$3.9     |$3.9      |TO BE ORDERED |https://www.electrokit.com/temp/fuktsensor-dht11                        |
|Estimated total shipping             |1  |           |$11.74   |$11.74    |HC            |                                                                        |
|TOTAL                                |   |           |         | $40.2    |              |                                                                        |


## Credits
Made for Hack Club-Half Life
