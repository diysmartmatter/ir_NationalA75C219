// National AC class for A75C219
// https://diysmartmatter.com/wp-content/uploads/2022/12/National.jpg
// based on the IRremoteESP8266 library: https://github.com/crankyoldgit/IRremoteESP8266
// Supported functions are limitted to those of Apple HomeKit Heater/Cooler Accessory
// i.e. power(on,off)/mode(heater,cooler)/fanspeed/fanswing/temp are supported.

//#include "IRrecv.h"
#include "IRremoteESP8266.h"
#include "IRsend.h"
#include <stdint.h>
#include <string> //for max()

// National Constants
const uint8_t kNationalCool = 0b010; //cool mode 
const uint8_t kNationalHeat = 0b100; //heat mode 
const uint8_t kNationalAuto = 0b110; //auto mode 
const uint8_t kNationalMinTemp = 16; 
const uint8_t kNationalMaxTemp = 30; 
const uint16_t kNationalStateLength = 4; 
const uint16_t kNationalFreq = 38000; 
const uint16_t kNationalHeaderMark = 3500; 
const uint16_t kNationalHeaderSpace = 3500; 
const uint16_t kNationalFooterMark = 3500; 
const uint16_t kNationalFooterSpace = 3500; 
const uint16_t kNationalBitMark = 875; 
const uint16_t kNationalOneSpace = 2625; 
const uint16_t kNationalZeroSpace = 875; 
const uint8_t kNationalFan1 = 0b0010; 
const uint8_t kNationalFan2 = 0b0100; 
const uint8_t kNationalFan3 = 0b0110; 
const uint8_t kNationalFanAuto = 0b1111;

/// Representation of a National A75C219 A/C message.
union NationalProtocol{
  struct{
    uint8_t raw[kNationalStateLength];  ///< The state of the IR remote. (4 bytes)
  };
  struct {
    uint8_t Temp :4; //actual setting - 15
    uint8_t Fan  :4; //{1,2,3,Auto}={2,4,6,0xF}
    uint8_t Mode :3; //cool=2, heat=4
    uint8_t Stay :1; //1=stay the AC power, 0 = toggle the power
    uint8_t      :4; //unknown 
    uint8_t Swing; //swing=4, no swing=0
    uint8_t Timer; //default = 0x36
  };
};

/// Class for handling National A75C219 A/C remote messages
/// @note Code by diySmartMatter, Reverse engineering analysis by diySmartMatter
class IRNational {
 public:
  explicit IRNational(const uint16_t pin, const bool inverted = false, const bool use_modulation = true);

  void send(void);
  void begin(void);
  void on(void);
  void off(void);
  void setPower(const bool state);
  void updateState(const bool newState);
  void setTemp(const uint8_t temp);
  uint8_t getTemp(void) const;
  void setFan(const uint8_t fan);
  void setMode(const uint8_t mode);
  void setSwing(const bool enabled);
  char* toChars(void);


 private:
  IRsend _irsend;  ///< instance of the IR send class
  NationalProtocol _;
  bool currentState; //actual power state of the AC
  void stateReset(void);
};
