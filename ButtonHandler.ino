typedef struct s_module_button_custom {
  s_pin*            pin;
  byte              lastButtonState;
  long              lastDebounceTime;
  byte              logic;
  byte              value;
} t_module_button_custom;

void             button_setup(loaded_module *module, va_list args) {
  int nbr = va_arg(args, int);
  
  t_pin *pin = get_pin(nbr);
  Serial.print(F(" --> pin "));Serial.print((int)(nbr));Serial.print(F(" to mode INPUT"));Serial.println("");
  if (pin->used) {
    Serial.print(" pin already in use");
    return;
  }
  
  t_module_button_custom *custom = (t_module_button_custom *)malloc(sizeof(t_module_button_custom));
  memset(custom, 0, sizeof(t_module_button_custom));
  module->custom = custom;
  custom->pin = pin;
  custom->pin->used = 1;
  pinMode(custom->pin->pin, INPUT);
}

void             button_loop(loaded_module *module) {
  t_module_button_custom *custom = (t_module_button_custom *)(module->custom);
  byte reading = read_value(module);
//  Serial.print("Button pin ");Serial.print((int)(custom->pin->pin));Serial.print(" reads ");Serial.print(reading);Serial.println("");
  
  if (reading != custom->lastButtonState) {
    // reset the debouncing timer
    custom->lastDebounceTime = millis();
  }
  if ((millis() - custom->lastDebounceTime) > DEBOUNCE_DELAY) {
    // if the button state has changed:
    if (reading != custom->value) {
      custom->value = reading;
      Serial.print(F("Button pin "));Serial.print((int)(custom->pin->pin));Serial.print(F(" changed to "));Serial.print(custom->value == 1? "HIGH": "LOW");Serial.println();
      broadcast_change(module);
    }
  }
  custom->lastButtonState = reading;
}

byte             read_value(loaded_module* module) {
  t_module_button_custom *custom = (t_module_button_custom *)(module->custom);
  byte reading;
  if (custom->pin->pin >= A0 && custom->pin->pin <= A7) {
    int value = analogRead(custom->pin->pin);
    reading = value > 512? HIGH: LOW;
  }
  else
    reading = digitalRead(custom->pin->pin);
  reading = custom->logic == INVERTED? !reading: reading;
  return reading;
}

int              button_read(struct t_loaded_module* module) {
  t_module_button_custom *custom = (t_module_button_custom *)(module->custom);
  custom->value = read_value(module);
  return custom->value;
}

void             button_string_value(loaded_module* module, char* buffer) {
  t_module_button_custom *custom = (t_module_button_custom *)(module->custom);

  if (custom->value)
    memcpy(buffer, F("HIGH"), 5);
  else
    memcpy(buffer, F("LOW"), 4);
}

byte             button_append_event(loaded_module* module, char* buffer) {
  t_module_button_custom *custom = (t_module_button_custom *)(module->custom);

  buffer[0] = custom->value == HIGH? 1: 0;
  return 1;
}

