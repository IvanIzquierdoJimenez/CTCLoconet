#include <Arduino.h>
#include <EEPROM.h>
#include <LocoNet.h>
#include <Adafruit_NeoPixel.h>
#include <ClickButton.h>
#include <Config.h>

lnMsg *LnPacket;
LocoNetCVClass lnCV;
Adafruit_NeoPixel leds(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ400);

#ifndef ocupaccy
  ClickButton btn(BUTTON_PIN, LOW, CLICKBTN_PULLUP);
#endif

volatile uint16_t myAddress = 1;      // Direccion desvio inicial

int numArt = 6020;
bool programmingMode;
int nLNCV = 10;
bool direcctionSwitch = false;

void dumpPacket(UhlenbrockMsg & ub) {
  Serial.print(" PKT: ");
  Serial.print(ub.command, HEX);
  Serial.print(" ");
  Serial.print(ub.mesg_size, HEX);
  Serial.print(" ");
  Serial.print(ub.SRC, HEX);
  Serial.print(" ");
  Serial.print(ub.DSTL, HEX);
  Serial.print(" ");
  Serial.print(ub.DSTH, HEX);
  Serial.print(" ");
  Serial.print(ub.ReqId, HEX);
  Serial.print(" ");
  Serial.print(ub.PXCT1, HEX);
  Serial.print(" ");
  for (int i(0); i < 8; ++i) {
    Serial.print(ub.payload.D[i], HEX);
    Serial.print(" ");
  }
  Serial.write("\n");
}

//write LNCV into EEPROM:
void lncv_W (uint16_t Adr, uint16_t val) {
  unsigned int Address = Adr * 2;
  EEPROM.update(Address, val & 0xFF);   //LSB
  EEPROM.update(Address + 1, val >> 8); //MSB
}
//read LNCV from EEPROM
uint16_t lncv_R (uint16_t Adr) {
  unsigned int Address = Adr * 2;
  unsigned int val = EEPROM.read(Address) | (EEPROM.read(Address + 1) << 8);  //LSB & MSB
  return val;
}

void setup()
{
  Serial.begin(9600);
  LocoNet.init(7);
  myAddress = lncv_R(0);
  programmingMode = false;
  #ifndef ocupaccy
  #endif
  leds.begin();
}

void loop()
{
  LnPacket = LocoNet.receive();
  if (LnPacket) {
    uint8_t packetConsumed(LocoNet.processSwitchSensorMessage(LnPacket));
    if (packetConsumed == 0) {
      Serial.print("Loop ");
      Serial.print((int)LnPacket);
      dumpPacket(LnPacket->ub);
      packetConsumed = lnCV.processLNCVMessage(LnPacket);
      Serial.print("End Loop\n");
    }
  }

  #ifndef ocupaccy
    if(btn.changed){
      direcctionSwitch != direcctionSwitch;
      LocoNet.requestSwitch(lncv_R(1), 1, direcctionSwitch);
      LocoNet.requestSwitch(lncv_R(1), 0, direcctionSwitch);
    }
  #endif
}

void notifySwitchOutputsReport( uint16_t Address, uint8_t ClosedOutput, uint8_t ThrownOutput) 
{

}

void notifySensor(uint16_t Address, uint8_t State)
{

}

int8_t notifyLNCVprogrammingStart(uint16_t & ArtNr, uint16_t & ModuleAddress)
{
  if (ArtNr == numArt)
  {
    if (ModuleAddress == lncv_R(0)) {
      programmingMode = true;
      return LNCV_LACK_OK;
    } else if (ModuleAddress == 0xFFFF) {
      ModuleAddress = lncv_R(0);
      return LNCV_LACK_OK;
    }
    return -1;
  }
}

int8_t notifyLNCVread(uint16_t ArtNr, uint16_t lncvAddress, uint16_t, uint16_t & lncvValue)
{
  if (programmingMode) {
    if (ArtNr == numArt) {
      if (lncvAddress < nLNCV) {
        lncvValue = lncv_R(lncvAddress);
        Serial.print(" LNCV Value: ");
        Serial.print(lncvValue);
        Serial.print("\n");
        return LNCV_LACK_OK;
      } else {
        // Invalid LNCV address, request a NAXK
        return LNCV_LACK_ERROR_UNSUPPORTED;
      }
    } else {
      return -1;
    }
  } else {
    return -1;
  }
}

int8_t notifyLNCVwrite(uint16_t ArtNr, uint16_t lncvAddress, uint16_t lncvValue)
{
  if (!programmingMode) {
    return -1;
  }

  if (ArtNr == numArt) {
    if (lncvAddress < nLNCV) {
      lncv_W(lncvAddress, lncvValue);
      Serial.print(" LNCV Value: ");
      Serial.print(lncvValue);
      Serial.print("\n");
      return LNCV_LACK_OK;
    }
    else {
      return LNCV_LACK_ERROR_UNSUPPORTED;
    }
  }
  else {
    return -1;
  }
}

void notifyLNCVprogrammingStop(uint16_t ArtNr, uint16_t ModuleAddress)
{
  if (programmingMode)
  {
    if (ArtNr == numArt && ModuleAddress == lncv_R(0)) {
      programmingMode = false;
    }
    else {
      if (ArtNr != numArt) {
        return;
      }
      if (ModuleAddress != lncv_R(0)) {
        return;
      }
    }
  }
}