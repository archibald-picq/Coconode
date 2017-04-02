#include <EEPROM.h>

int eeprom_addr = 0;
unsigned long uniq = 0;
char uniqNameConst[4] = {0, 0, 0, 0};

byte cocosum(unsigned long u) {
  return (u+42%256);
}

byte get_uniq_board_channel() {
  return uniq2channel(uniq);
}

char* get_uniq_board_name() {
  uniq = read_or_create_uniq_id();
  uniqNameConst[0] = (char)(uniq % 256);
  uniqNameConst[1] = (char)((uniq / 256) % 256);
  uniqNameConst[2] = (char)((uniq / 65536L) % 256);
  return uniqNameConst;
}

unsigned long read_or_create_uniq_id() {
  byte c1 = EEPROM.read(eeprom_addr + 0);
  byte o1 = EEPROM.read(eeprom_addr + 1);
  byte c2 = EEPROM.read(eeprom_addr + 2);
  byte o2 = EEPROM.read(eeprom_addr + 3);
  byte a = EEPROM.read(eeprom_addr + 4);
  byte b = EEPROM.read(eeprom_addr + 5);
  byte c = EEPROM.read(eeprom_addr + 6);
  byte d = EEPROM.read(eeprom_addr + 7);
  unsigned long u = c;
//  Serial.print("u = c ");
//  Serial.print(u);
  u = u * 256;
  u = u * 256;
//  Serial.print(" >> 16 : ");
//  Serial.print(u);
//  Serial.println("");
  u += a + (b<<8);
  byte chk = cocosum(u);

  if (c1 == 'c' && o1 == 'o' && c2 == 'c' && o2 == 'o' && chk == d &&
        a != 255 && b != 255 && c != 255 && d != 255) {
//    Serial.print("valid ");
//    Serial.print(" (");
//    Serial.print((char)a);
//    Serial.print(", ");
//    Serial.print((char)b);
//    Serial.print(", ");
//    Serial.print((char)c);
//  //  Serial.print(", ");
//  //  Serial.print((int)d);
//    Serial.print(")");
////    Serial.print(u);
//    Serial.println("");
    return u;
  }
//  return u;
  randomSeed(analogRead(0));
  a = random(97, 122);
  b = random(97, 122);
  c = random(97, 122);
  u = c;
//  Serial.print("u = c ");
//  Serial.print(u);
  u = u * 256;
  u = u * 256;
//  Serial.print(" >> 16 : ");
//  Serial.print(u);
//  Serial.println("");
  u += a + (b<<8);
  d = cocosum(u);
  
  if (c1 != 'c')
    EEPROM.write(eeprom_addr + 0, 'c');
  if (o1 != 'o')
    EEPROM.write(eeprom_addr + 1, 'o');
  if (c2 != 'c')
    EEPROM.write(eeprom_addr + 2, 'c');
  if (o2 != 'o')
    EEPROM.write(eeprom_addr + 3, 'o');
  EEPROM.write(eeprom_addr + 4, a);
  EEPROM.write(eeprom_addr + 5, b);
  EEPROM.write(eeprom_addr + 6, c);
  EEPROM.write(eeprom_addr + 7, d);
  return u;
}

byte uniq2channel(unsigned long u) {
  byte c = (byte)(u % 256);
  return c - 97 + 3;
}

