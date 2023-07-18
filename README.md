# ir_NationalA75C219
ESP32 Smart IR remote for National A75C219 Air Conditioner  
This program uses IRremoteESP8266 library. https://github.com/crankyoldgit/IRremoteESP8266

The target of this library is National (old name of Panasonic) A/C (air conditioner) that uses National A75C219 remote controller. 

![A75C219](https://diysmartmatter.com/wp-content/uploads/2022/12/National.jpg)

You could buid the hardware, i.e. smart IR remote controller using ESP32:

![ESP32](https://diysmartmatter.com/wp-content/uploads/2023/02/pcb.jpg)

![Diagram](https://diysmartmatter.com/images/20221225150838.png)

that can be used by Apple HomeKit as a Heater/Cooler accessory. 

![HomeKit](https://diysmartmatter.com/images/20221123195125.png)

# Background

The IRremoteESP8266 library has a very excellent class for varaety of A/C on market, although the target National A/C is so old that it is out of support. This is a simple implementation of a class to support minimal functions required by Apple HomeKit. 

# Prerequisite

![Configuration](https://diysmartmatter.com/wp-content/uploads/2023/04/remo_E.jpg)


- Homebridge
- Plug-in for MQTT (MQTTThing)
- MQTT (Mosquitto)
- Arduino IDE
- ESP32 with IR LED(s)
- ESP32 Arduino (Board manager)
- Arduino libraries:
- IRremoteESP8266 
- ArduinoOTA
- EspMQTTClient
- DHT20


# How to use

- Create an Arduino sketch using IR_airconMQTT_n.ino, ir_National.h, and ir_National.cpp. (WiFi info and address should be changed to yours)
- Set up MQTTThing so that Homebridge can communicate with ESP32 through MQTT

This is an example of Homebridge config for MQTTThing.

<pre>
 {
    "type": "heaterCooler",
    "name": "NationalAC",
    "url": "mqtt://localhost:1883",
    "topics": {
        "setActive": "zigbee2mqtt/irNational/set/Active",
        "getActive": "zigbee2mqtt/irNational/get/Active",
        "setCoolingThresholdTemperature": "zigbee2mqtt/irNational/set/CoolingThresholdTemperature",
        "getCurrentTemperature": "zigbee2mqtt/your_temp_sensor$.temperature",
        "setHeatingThresholdTemperature": "zigbee2mqtt/irNational/set/HeatingThresholdTemperature",
        "setRotationSpeed": "zigbee2mqtt/irNational/set/RotationSpeed",
        "setTargetHeaterCoolerState": "zigbee2mqtt/irNational/set/TargetHeaterCoolerState",
        "setSwingMode": "zigbee2mqtt/irNational/set/SwingMode"
    },
    "accessory": "mqttthing"
},
</pre>
