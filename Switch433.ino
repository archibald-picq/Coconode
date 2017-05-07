#include <RCSwitch.h>
RCSwitch *mySwitch;

typedef struct s_module_switch433_custom {
  byte              value;
  char              group[6];
  char              device[6];
} t_module_switch433_custom;

void             switch433_setup(loaded_module* module, va_list args) {
  struct s_module_switch433_custom *custom = (struct s_module_switch433_custom *)malloc(sizeof(s_module_switch433_custom));
  module->custom = custom;
  
  int emitterPin = va_arg(args, int);
  char *group = va_arg(args, char *);
  char *device = va_arg(args, char *);

  memcpy(custom->group, group, 6);
  memcpy(custom->device, device, 6);

  Serial.print(" with ");
  Serial.print(custom->group);
  Serial.print(" : ");
  Serial.print(custom->device);
  Serial.println();
  if (!mySwitch) {
    mySwitch = new RCSwitch();
    mySwitch->enableTransmit(emitterPin);
    
    // Optional set protocol (default is 1, will work for most outlets)
    // mySwitch.setProtocol(2);
  
    // Optional set pulse length.
    // mySwitch.setPulseLength(320);
    
    // Optional set number of transmission repetitions.
    // mySwitch.setRepeatTransmit(15);
  }
}

void             switch433_loop(loaded_module* module) {
  
}

int              switch433_read(struct t_loaded_module *module) {
  t_module_switch433_custom *custom = (t_module_switch433_custom *)(module->custom);
  return custom->value;
}

void             switch433_write(struct t_loaded_module *module, int value) {
  t_module_switch433_custom *custom = (t_module_switch433_custom *)(module->custom);

  custom->value = value;
  if (value) {
    Serial.print("switch on ");
    Serial.print(custom->group);
    Serial.print(" : ");
    Serial.print(custom->device);
    Serial.println();
    mySwitch->switchOn(custom->group, custom->device);
  }
  else {
    Serial.print("switch off ");
    Serial.print(custom->group);
    Serial.print(" : ");
    Serial.print(custom->device);
    Serial.println();
    mySwitch->switchOff(custom->group, custom->device);
  }
  broadcast_change(module);
}


byte             switch433_event(loaded_module* module, char* buffer) {
  t_module_switch433_custom *custom = (t_module_switch433_custom *)(module->custom);

//  Serial.print("serialize value ");
//  Serial.print((byte)custom->value);
//  Serial.println();
  buffer[0] = custom->value;
  return 1;
}
