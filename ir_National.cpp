// National AC class for A75C219
// https://diysmartmatter.com/wp-content/uploads/2022/12/National.jpg
// based on the IRremoteESP8266 library: https://github.com/crankyoldgit/IRremoteESP8266
// Supported functions are limitted to those of Apple HomeKit Heater/Cooler Accessory
// i.e. power(on,off)/mode(heater,cooler)/fanspeed/fanswing/temp are supported.

#include "ir_National.h"
#include "IRremoteESP8266.h"


/// Class constructor.
/// @param[in] pin GPIO to be used when sending.
/// @param[in] inverted Is the output signal to be inverted?
/// @param[in] use_modulation Is frequency modulation to be used?
IRNational::IRNational(const uint16_t pin, const bool inverted, const bool use_modulation)
    : _irsend(pin, inverted, use_modulation) { stateReset(); }

/// Set up hardware to be able to send a message.
void IRNational::begin(void) { _irsend.begin(); }

/// Send the current internal state as an IR message.
/// @param[in] repeat Nr. of times the message will be repeated.
void IRNational::send() {

  // Data for gap
  const uint16_t gap[2]= { kNationalBitMark, kNationalBitMark *16};
  // Data for section #1 
  uint8_t irdata1[4]= {_.raw[0], _.raw[0], _.raw[1], _.raw[1]};
  // Data for section #2
  uint8_t irdata2[4]= {_.raw[2], _.raw[2], _.raw[3], _.raw[3]};

  // Section #1 
  _irsend.sendGeneric(kNationalHeaderMark, kNationalHeaderSpace, kNationalBitMark,
              kNationalOneSpace, kNationalBitMark, kNationalZeroSpace,
              0, 0, &irdata1[0], 4,
              kNationalFreq, false, 0, 50);
  _irsend.sendGeneric(kNationalHeaderMark, kNationalHeaderSpace, kNationalBitMark,
              kNationalOneSpace, kNationalBitMark, kNationalZeroSpace,
              kNationalFooterMark, kNationalFooterSpace, &irdata1[0], 4,
              kNationalFreq, false, 0, 50);
    
  // Gap #1
  _irsend.sendRaw(gap, 2, kNationalFreq);

  // Section #2 
  _irsend.sendGeneric(kNationalHeaderMark, kNationalHeaderSpace, kNationalBitMark,
              kNationalOneSpace, kNationalBitMark, kNationalZeroSpace,
              0, 0, &irdata2[0], 4,
              kNationalFreq, false, 0, 50);
  _irsend.sendGeneric(kNationalHeaderMark, kNationalHeaderSpace, kNationalBitMark,
              kNationalOneSpace, kNationalBitMark, kNationalZeroSpace,
              kNationalFooterMark, kNationalFooterSpace, &irdata2[0], 4,
              kNationalFreq, false, 0, 50);

  // Gap #2
  _irsend.sendRaw(gap, 2, kNationalFreq);
  
  if( _.Stay == 0 ) {
    currentState = !currentState; //toggle the current state
    _.Stay = 1; //do not toggle anymore
  }

}

/// Disable/enable swing.
/// @param[in] on true, swing is on. false, swing is off.
void IRNational::setSwing(const bool enabled) {
  const uint8_t swingOn[4]  = { 0x80, 0x80, 0x30, 0x30};
  const uint8_t swingOff[4] = { 0x02, 0x02, 0x30, 0x30};
  const uint16_t gap[2]= { kNationalBitMark, kNationalBitMark *16};
  if(enabled) {
    for(int i=0;i<5;i++){
      _irsend.sendGeneric(kNationalHeaderMark, kNationalHeaderSpace, kNationalBitMark,
              kNationalOneSpace, kNationalBitMark, kNationalZeroSpace,
              kNationalFooterMark, kNationalFooterSpace, &swingOn[0], 4,
              kNationalFreq, false, 0, 50);
      // Gap
      _irsend.sendRaw(gap, 2, kNationalFreq);
    }
  } else {
    for(int i=0;i<4;i++){
      _irsend.sendGeneric(kNationalHeaderMark, kNationalHeaderSpace, kNationalBitMark,
              kNationalOneSpace, kNationalBitMark, kNationalZeroSpace,
              kNationalFooterMark, kNationalFooterSpace, &swingOff[0], 4,
              kNationalFreq, false, 0, 50);
      // Gap
      _irsend.sendRaw(gap, 2, kNationalFreq);
    }
  }//end of else
}

/// Prepare hex (0-9 and A-F) chars of the current internal state.
/// @return PTR to the char.
char* IRNational::toChars(void) {
  static char result[kNationalStateLength * 2 + 1];
  for(int i=0;i<kNationalStateLength;i++){
    uint8_t byte,hex1,hex2;
    byte=_.raw[i];
    hex1=byte >> 4;
    hex2=byte & 0xF;
    if(hex1 < 10) hex1 += '0'; else hex1 += 'A' - 10;
    if(hex2 < 10) hex2 += '0'; else hex2 += 'A' - 10;
    result[i * 2] = (char)hex1;
    result[i * 2 + 1] = (char)hex2;
  }
  result[kNationalStateLength * 2]='\0';
  return result;
}

/// Reset the internal state to a fixed known good state.
// the state is: temp=28deg. power=off, mode=cool, fan=max, swing=no
void IRNational::stateReset(void) {
  for (uint8_t i = 0; i < kNationalStateLength; i++) _.raw[i] = 0x0;
  _.raw[0] = 0xFA; //Fan=auto, Temp=10+15=25
  _.raw[1] = 0x0E; //Stay=1 (no toggle the power), Mode=6 (Auto)
  _.raw[2] = 0x00; // no swing
  _.raw[3] = 0x36; // default for timer
  currentState=false; //set current power state to off
}

/// Change the power setting to On.
void IRNational::on(void) { setPower(true); }

/// Change the power setting to Off.
void IRNational::off(void) { setPower(false); }

/// Change the power setting.
/// @param[in] on true, the setting is on. false, the setting is off.
void IRNational::setPower(const bool on) {
  if(on == currentState) {
    _.Stay = 1; //do not toggle the power in the next irsend
  } else {
    _.Stay = 0; //toggle the power in the next irsend
  }
}

/// The AC has been operated then update the actual power state
/// @param[in] accuired power state by a Zigbee contact sensor 
void IRNational::updateState(const bool newState) { currentState = newState; }

/// Set the operating mode of the A/C.
/// @param[in] desired_mode The desired operating mode.
void IRNational::setMode(const uint8_t desired_mode) {
  uint8_t mode = desired_mode;
  switch (mode) {
    case kNationalCool:
    case kNationalHeat: 
    case kNationalAuto: break;
    default: return; //do nothing
  }
  _.Mode = mode;
}

/// Set the temperature.
/// @param[in] desired The temperature in degrees celsius.
void IRNational::setTemp(const uint8_t desired) {
  // The A/C has a different min temp if in cool mode.
  uint8_t temp = std::min(kNationalMaxTemp, std::max(kNationalMinTemp,desired));
  _.Temp = temp - 15;
}

/// Get the current temperature setting.
/// @return The current setting for temp. in degrees celsius.
uint8_t IRNational::getTemp(void) const { return _.Temp + 15; }

/// Set the speed of the fan.
/// @param[in] fan The desired setting.
/// @note 1-3 or Auto
void IRNational::setFan(const uint8_t desired_fan) {
  uint8_t fan = desired_fan;
  switch (fan) {
    case kNationalFan1:
    case kNationalFan2:
    case kNationalFan3:
    case kNationalFanAuto: break;
    default: fan = kNationalFanAuto;
  }
  _.Fan = fan;
}
