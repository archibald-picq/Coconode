
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
//#include "printf.h"


// Radio pipe addresses for the 2 nodes to communicate.
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

typedef struct s_linky {
  byte            iinst;  // in Ampere, can be from 0 to max subrscribed power (generaly 27 = 6kw, 40 = 9kw, 12kw = 54 ...), no more than 255
  unsigned short  papp;
  RF24            *radio;
} t_linky;

typedef struct s_radio_packet {
  byte type;
  byte checksum;
  byte value[4];
} radio_packet;

void display_ping(loaded_module* module, struct s_radio_packet *packet) {
  struct s_linky *custom = (struct s_linky*)(module->custom);
  unsigned long value;
  memcpy(&value, &(packet->value), sizeof(unsigned long));
  // Delay just a little bit to let the other unit
  // make the transition to receiver
  delay(20);

  // First, stop listening so we can talk
  custom->radio->stopListening();
  
  Serial.print("Got ping ");
  Serial.print(value);
  Serial.print(" ms");
  // Send the final one back.
  custom->radio->write((void*)&(packet->checksum), sizeof(byte));
  Serial.println(", pong back.");

  // Now, resume listening so we catch the next packets.
  custom->radio->startListening();
}

void display_iinst(loaded_module* module, struct s_radio_packet *packet) {
  struct s_linky *custom = (struct s_linky*)(module->custom);
  
  unsigned long value;
  memcpy(&value, &(packet->value), sizeof(unsigned long));
  custom->iinst = value;
  Serial.print("Got iinst ");
  Serial.print(custom->iinst);
  Serial.print(" A");
  Serial.println();
  broadcast_change(module);
}

void display_papp(loaded_module* module, struct s_radio_packet *packet) {
  struct s_linky *custom = (struct s_linky*)(module->custom);
  
  unsigned long value;
  memcpy(&value, &(packet->value), sizeof(unsigned long));
  custom->papp = value;
  Serial.print("Got papp ");
  Serial.print(custom->papp);
  Serial.print(" VA");
  Serial.println();
  broadcast_change(module);
}

radio_packet packet;

// IINST from Linky Relay
// PAPP from Linky Relay
void             linky_setup(loaded_module* module, va_list args) {
  struct s_linky *custom = (struct s_linky *)malloc(sizeof(s_linky));
  module->custom = custom;


  int ce = va_arg(args, int);
  int cs = va_arg(args, int);
  custom->radio = new RF24(ce, cs);
  
  Serial.print(F(" --> "));
  custom->radio->begin();

  // optionally, increase the delay between retries & # of retries
  custom->radio->setRetries(15, 15);

  // optionally, reduce the payload size.  seems to improve reliability
  custom->radio->setPayloadSize(8);

  //
  // Open pipes to other nodes for communication
  // This simple sketch opens two pipes for these two nodes to communicate back and forth.

  custom->radio->openWritingPipe(pipes[1]);
  custom->radio->openReadingPipe(1, pipes[0]);

  custom->radio->startListening();
  custom->iinst = 0;
  custom->papp = 0;

  Serial.println();
//  radio.printDetails();
}

void             linky_loop(loaded_module* module) {
  struct s_linky *custom = (struct s_linky*)(module->custom);
  // if there is data ready
  if (!custom->radio->available())
    return;

  // Dump the payloads until we've gotten everything
  bool done = false;
  while (!done) {
    // Fetch the payload, and see if this was the last one.
    done = custom->radio->read( &packet, sizeof(packet) );

    if (packet.type == 1)
      display_ping(module, &packet);
    else if (packet.type == 2)
      display_iinst(module, &packet);
    else if (packet.type == 3)
      display_papp(module, &packet);
    else {
      Serial.print("Got unknown packet ");
      Serial.print((int)packet.type);
      Serial.println();
    }
  }
}

byte             linky_event(loaded_module* module, char* buffer) {
  struct s_linky *custom = (struct s_linky*)(module->custom);
  byte length = 0;
  
  memcpy(&(buffer[length]), &(custom->iinst), sizeof(byte));
  length += sizeof(byte);
  memcpy(&(buffer[length]), &(custom->papp), sizeof(unsigned short));
  length += sizeof(unsigned short);
  return length;
}


