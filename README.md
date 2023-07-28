# ir_NationalA75C219
ESP32 Smart IR remote for National A75C219 Air Conditioner  
This program uses IRremoteESP8266 library. https://github.com/crankyoldgit/IRremoteESP8266

The target of this library is National (old name of Panasonic) A/C (air conditioner) that uses National A75C219 remote controller. 

![A75C219](https://diysmartmatter.com/wp-content/uploads/2022/12/National.jpg)

# Background

The IRremoteESP8266 library has excellent classes for a varaety of A/C on market, although the target National A/C is so old that it is out of support. This is a simple implementation of a class to support with minimal functions required by Apple HomeKit. 


# IR Signal

This is an example of the IR signal pattern when the power button on the remo is pressed with settings of cool, 27 degree, auto fanspeed.

![IR pattern](https://diysmartmatter.com/images/20221216174111.png)

By assigning { 875 micro-sec high, 875 low} to 0, and {875 high, 2625 low} to 1, and assuming the bit sequence is LSB to MSB, this pattern can be decoded as followings.

<pre>0xfc 0xfc 0x02 0x02
0xfc 0xfc 0x02 0x02

0x00 0x00 0x36 0x36
0x00 0x00 0x36 0x36
</pre>

Apparently, data of 4 bytes, {0xfc, 0x02, 0x00, 0x36} are repeated 4 times for each. By changing mode, temp, fan settings, bits of the data of 4 bytes should be assigned as follows.

<table cellspacing="0" cellpadding="0">
<tbody>
<tr>
<th valign="top">Byte</th>
<th colspan="8">
<p align="center">MSB &lt;---&gt; LSB</p>
</th>
</tr>
<tr>
<th valign="top"></th>
<th valign="top">7</th>
<th valign="top">6</th>
<th valign="top">5</th>
<th valign="top">4</th>
<th valign="top">3</th>
<th valign="top">2</th>
<th valign="top">1</th>
<th valign="top">0</th>
</tr>
<tr>
<th valign="top">0</th>
<td colspan="4" valign="top">
<p align="center">Fan speed (0xF:auto 0x6:high 0x4:mid 0x2:low)</p>
</td>
<td colspan="4" valign="top">
<p align="center">Temerature - 15</p>
</td>
</tr>
<tr>
<th valign="top">1</th>
<td colspan="4" valign="top">
<p align="center">0x00</p>
</td>
<td valign="top">Stay power*</td>
<td colspan="3" valign="top">
<p align="center">2:cool, 3:dry, 4:heat, 6:auto</p>
</td>
</tr>
<tr>
<th valign="top">2</th>
<td colspan="8" valign="top">
<p align="center">0x04:swing, 0x00:no swing</p>
</td>
</tr>
<tr>
<th valign="top">3</th>
<td colspan="8" valign="top">
<p align="center">0x36 (something for timer settings)</p>
</td>
</tr>
</tbody>
<tfoot>
<tr>
<td colspan="9">Stay power*  0：toggle On/Off　1：no toggle</td>
</tr>
</tfoot>
</table>

When the "stay power" bit is 0, the power of the A/C toggles, while the "stay power" bit is 1, the power will not toggle but temperature or fan-speed are set.

# Hardware

You could buid the hardware, i.e. smart IR remote controller using ESP32:

![ESP32](https://diysmartmatter.com/wp-content/uploads/2023/02/pcb.jpg)

![Diagram](https://diysmartmatter.com/images/20221225150838.png)

that can be used by Apple HomeKit as a Heater/Cooler accessory. 

![HomeKit](https://diysmartmatter.com/images/20221123195125.png)

# Resolving toggle issue

The state of the A/C is unpredectable when the IR signal toggles the power of the A/C. To detect actual state of the A/C power, a magnet-contact sensor is used to attache to the flap of the A/C.


<img src="https://diysmartmatter.com/wp-content/uploads/2022/12/n05.jpg" alt="" width="401" height="297" class="alignnone wp-image-2815" />  <img src="https://diysmartmatter.com/wp-content/uploads/2022/12/n01.jpg" alt="" width="400" height="296" class="alignnone wp-image-2816" />

The sensor is a Zigbee contact sensor that works with Zigbee2MQTT server. Any contact-sensor that is suported by Zigbee2MQTT can be used. The sensor in the photo is:

https://ja.aliexpress.com/item/1005004779586263.html

I have set the Zigbee2MQTT to publish a message like this,

<pre>{"battery":100,"battery_low":false,"contact":true,"linkquality":138,"tamper":false,"voltage":3000}</pre>
 

to the following topic.

<pre>zigbee2mqtt/irNational/set/Flap</pre>

<img src="https://diysmartmatter.com/wp-content/uploads/2022/12/n26.jpg" />

# Prerequisite


<img src="https://diysmartmatter.com/wp-content/uploads/2022/12/n23.jpg"/>


- Homebridge (running on Rasbperry Pi)
- Plug-in for MQTT (MQTTThing) (running on Rasbperry Pi)
- MQTT (Mosquitto) (running on Rasbperry Pi)
- Zigbee contact sensor attached to the A/C flap
- Zigbee2MQTT server (running on Rasbperry Pi)
- Arduino IDE
- ESP32 with IR LED(s)
- ESP32 Arduino (Board manager)
- Arduino libraries:
- IRremoteESP8266 
- ArduinoOTA
- EspMQTTClient
- DHT20 (optional)

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
