#Sunbie

Sunbie is a Tamagotchi inspired product that utilises a Waveshare ESP 32- C3 microprocessor.
It uses sensors such as the ADXL345 and the DHT11 so as to gather data from the environment, it also has a sprite (in this case a fox) 
that responds to any changes in the temperature. The accelerometer enables the user to interact with the sprite so as to feed her, play with her,
pet her or (if she's sick) give her meds. You look after it by tilting the device to pick actions from a menu.

<img width="407" height="353" alt="image" src="https://github.com/user-attachments/assets/a2cab186-5890-4097-b109-2d365a4b7186" />

<img width="472" height="405" alt="image" src="https://github.com/user-attachments/assets/873c6659-78f3-4cf9-9953-49e0ebe6bcdf" />
<img width="453" height="425" alt="image" src="https://github.com/user-attachments/assets/4ae8e494-6ba1-4fd6-9915-67e0d28b10f1" />

| Action | Joy | Energy | Fullness | Health |
|---|---|---|---|---|
| Nap | +1 | +18 | −4 | +3 |
| Play | +12 | −9 | −5 | −4 |
| Feed | +3 | −2 | +15 | +1 |
| Pet | +7 | +2 | 0 | 0 |
| Meds | −7 | −3 | 0 | +7 |


A 3d visual of the device on Kicad

Why was this made?

This was made as a part of Hack Club's Half Life programme.

Who is the sprite?

The sprite (pet) can be set to anything the user desires by changing the Bit Image in the source code. However, I use a fox.
inspired by the image below.
<img width="91" height="96" alt="Annotation 2026-10-10 102953 (2)" src="https://github.com/user-attachments/assets/6004e3a8-292f-489c-bfd6-f32ab7e495ad" />

The code is written in C++ Arduino framework and uses an SSDI306 OLED to display the fox and menu, an ADXL345 in order to detect tilts and also detect shaking (shake/dizzy pet reactions), a DHT11 for temperature and humidity readings and temperature reactions, and a buzzer for sounds. The pet is saved, it can be reset by setting RESET_SAVED_PET to true,uploading it and then setting it to false again!!

|Item|Part_Name                                                      |Description                                                                    |Quantity|Unit_Cost|Total_Cost|Supplier   |
|----|---------------------------------------------------------------|-------------------------------------------------------------------------------|--------|---------|----------|-----------|
|1   |DHT-11 temperature and humidity sensor                         |It senses differences in the surroundings which react.                         |1       |3.91     |3.91      |electro:kit|
|2   |Resistor                                                       |Resistor for DHT11 data line                                                   |1       |0.10     |0.10      |electro:kit|
|3   |ADXL345 - I2C SPI                                              |Accelerometer(motion sensor) it detects motion allowing for the Sunbie to react|1       |6.90     |6.90      |electro:kit|
|4   |ESP32-C3 developmentboard mini 4MB                             |The main controls,microprocessor in sunbie                                     |1       |7.90     |7.90      |electro:kit|
|5   |OLED Display 0.96 inch I2C 128x64 pixels for RPi/Arduino/Atmega|Display for sunbie, allows it to show responses                                |1       |6.50     |6.50      |jmux.se    |
|6   |pcb                                                            |Main part/base of whole pcb                                                    |1       |4.00     |4.00      |jlc pcb    |
|7   |Passive piezo buzzer 70dB PS12 PCB ø12x6.5mm                   |play sound alongside reactions                                                 |1       |0.75     |0.75      |electro:kit|
|8   |WS Standard Linear Switch                                      |Interaction with the device                                                    |2       |0.15     |0.30      |Max Gaming |
|9   |Tax & shipping                                                 |for the whole order                                                            |-       |-        |2.00      |-          |


Credits
Made for Hack Club-Half Life
