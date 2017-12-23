#include <Wire.h>
#include "Arduino.h"
#include "MCP23017.h"


/* コンストラクタ */
MCP23017::MCP23017(int addr, int sda, int scl) {
  expanderAddr = addr;
  sdaPin = sda;
  sclPin = scl;
}


/* MCP23017の初期化 */
void MCP23017::init() {
  Wire.begin(sdaPin, sclPin);
  writeByte(expanderAddr, IODIRA, 0x00); //ポートAを出力に設定
  writeByte(expanderAddr, IODIRB, 0x00); //ポートBを出力に設定
}


/* 指定ポートにByte単位でデータを送信 */
void MCP23017::writeByte(uint8_t slaveAddr, uint8_t targetReg, uint8_t data) {
  Wire.beginTransmission(slaveAddr);
  Wire.write(targetReg);
  Wire.write(data);
  Wire.endTransmission();
}


/* 指定した列の7Segにデータを一括送信 */
void MCP23017::writeToRaw(seg7LedId_t data, uint8_t row) {
  // 一番左の桁表示
  writeByte(expanderAddr, PORTB, 0x00);
  writeByte(expanderAddr, PORTA, data.leftTen);
  writeByte(expanderAddr, PORTB, 0x01 << row);

  // 左から2番目の桁表示
  writeByte(expanderAddr, PORTB, 0x00);
  writeByte(expanderAddr, PORTA, data.leftOne);
  writeByte(expanderAddr, PORTB, 0x02 << row);

  // 右から2番目の桁表示
  writeByte(expanderAddr, PORTB, 0x00);
  writeByte(expanderAddr, PORTA, data.rightTen);
  writeByte(expanderAddr, PORTB, 0x04 << row);

  // 一番右の桁表示
  writeByte(expanderAddr, PORTB, 0x00);
  writeByte(expanderAddr, PORTA, data.rightOne);
  writeByte(expanderAddr, PORTB, 0x08 << row);

  writeByte(expanderAddr, PORTB, 0x00);
  writeByte(expanderAddr, PORTA, 0x00);
  writeByte(expanderAddr, PORTB, 0x00);
}


/* 全ての列の7Segに数値データを一括送信 */
void MCP23017::writeToAll(seg7LedId_t data1, seg7LedId_t data2) {
  writeToRaw(data1, RAW1);
  writeToRaw(data2, RAW2);
}
