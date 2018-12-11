# IoT-SONOFF-Touch
SONOFF Touch light switch with MQTT support

This is a personal project to control the room light with SONOFF Touch wall switch.
This device is equiped with an ESP8285 MCU, to which this software is been developed 
It is written in C++ under PlatformIO IDE (integrated on ATOM platform).
I'm coding my own variant of this popular project, with some inspiration and lessons (code Snippets) from some well know projects like:
    ESPURNA: https://github.com/SensorsIot/Espurna-Framework
    TASMOTA: https://github.com/arendst/Sonoff-Tasmota
    

  Goals of this project:
    1. Locally control the Room light with included touch Button
    2. Remote upgrade Over-the-Air (OTA) and/or HTTP Update.
    3. MQTT Publish/Subscribe support
        3.1.  Remote control of Room light
        3.2.  Double and Triple touch Button recognition.
    4. User commands feedback by flashing the "WiFi"LED. 
    5. Remote TELNET Debug
    
    Future features!!
    6. Power consumption optimization
    7. Long operational live (it will be running 365 days a year, so, it cannot loose Wifi or crash!)
    
    
    8. 1st time Install Web portal (currently disabled)
    

