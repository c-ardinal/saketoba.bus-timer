#ifndef MCP23017_H
#define MCP23017_H

#define IODIRA   0x00
#define IODIRB   0x01
#define PORTA    0x12
#define PORTB    0x13

#define RAW1     0
#define RAW2     4

#define _0       0x3f
#define _1       0x06
#define _2       0x5b
#define _3       0x4f
#define _4       0x66
#define _5       0x6d
#define _6       0x7d
#define _7       0x27
#define _8       0x7f
#define _9       0x6f
#define _A       0x77
#define _B       0x7c
#define _C       0x58
#define _D       0x5e
#define _E       0x79
#define _F       0x71
#define _G       0x3d
#define _H       0x74
#define _I       0x11
#define _J       0x1e
#define _K       0x75
#define _L       0x38
#define _M       0x37
#define _N       0x54
#define _O       0x5c
#define _P       0x73
#define _Q       0x67
#define _R       0x50
#define _S       0x6c
#define _T       0x78
#define _U       0x1c
#define _V       0x3e
#define _W       0x7e
#define _X       0x76
#define _Y       0x6e
#define _Z       0x1b
#define _HYPHEN  0x40
#define _NULL    0x00


typedef struct seg7LedId_t {
  uint8_t leftTen;
  uint8_t leftOne;
  uint8_t rightTen;
  uint8_t rightOne;
};


class MCP23017 {
  public:
    MCP23017(int, int, int);
    void init();
    void writeToRaw(seg7LedId_t, uint8_t);
    void writeToAll(seg7LedId_t, seg7LedId_t);
  private:
    int expanderAddr, sdaPin, sclPin;
    void writeByte(uint8_t, uint8_t, uint8_t);
};

#endif

