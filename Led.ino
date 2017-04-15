
typedef struct s_module_led_custom {
  struct s_pin      *pin;
  byte              value;
  byte              logic;
} t_module_led_custom;

void             led_setup(struct t_loaded_module *module, va_list args) {
  struct s_module_led_custom *custom = (struct s_module_led_custom *)malloc(sizeof(s_module_led_custom));
  module->custom = custom;
  int nbr = va_arg(args, int);

  custom->pin = get_pin(nbr);
  Serial.print(F(" --> pin ")); Serial.print((int)nbr); Serial.print(F(" to mode OUTPUT")); Serial.println(F(""));
  pinMode(custom->pin->pin, OUTPUT);
}

void             led_loop(struct t_loaded_module *module) {
  // nothing to do
}

int              led_read(struct t_loaded_module *module) {
  t_module_led_custom *custom = (t_module_led_custom *)(module->custom);
  return custom->value;
}

void             led_write(struct t_loaded_module *module, int value) {
  t_module_led_custom *custom = (t_module_led_custom *)(module->custom);

//  if (custom->value != value) {
    custom->value = value;
    digitalWrite(custom->pin->pin, (custom->logic == INVERTED ? !custom->value : custom->value) ? HIGH : LOW);
    //  digitalWrite(pins[i].pin, pins[i].logic == INVERTED? !pins[i].value: pins[i].value);
//    Serial.print(custom->value ? "Y" : "N");
    broadcast_change(module);
//  }
}

byte             led_append_event(loaded_module* module, char* buffer) {
  t_module_led_custom *custom = (t_module_led_custom *)(module->custom);

//  Serial.print("serialize value ");
//  Serial.print((byte)custom->value);
//  Serial.println();
  buffer[0] = custom->value;
  return 1;
}
