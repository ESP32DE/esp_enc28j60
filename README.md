# esp_enc28j60
##Ethernet connectivity to an ESP8266 via an ENC28J60 

This project adds the ENC28J60 to the ESP8266, and allows Sprites HTTPD functionality via a wired connection.
- Allows for concurrent connections to the ESP, both via wired connection and wireless to serve the HTTPD project.

Forum discussion - http://www.esp8266.com/viewtopic.php?f=11&t=9413

![alt tag](https://github.com/Cicero-MF/esp_enc28j60/blob/master/ethernet%20to%20esp%20wiring.png)

**SDK Version:** v1.5.0 27-11-2015

## Running and testing esp_enc28j60
- Connect up your ENC28J60 to your ESP8266 as shown
- Project has been copied in full, so compile and run as you would the httpd project normally
- Lastly, dont hesitate to contact me with any questions you may have.  Good luck!

##Thanks
Special thanks should go to the following people who have provided the building blocks of each aspect, I've just managed to piece them together in some sort of fashion and suprisingly it works.  

1. For the stack, Ulrich Radig http://www.ulrichradig.de/
2. For the HTTPD project - Sprite_tm http://www.esp8266.com/viewforum.php?f=34&sid=cd5a2e06f4567c5293795af3709402e7
3. MetalPhreak for the ESP8266 SPI functionality - https://github.com/MetalPhreak/ESP8266_SPI_Driver
