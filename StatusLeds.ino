
#define STATE_UNINITILIZED          0
#define STATE_INITIALIZED           1
#define STATE_I2C_READY             10
#define STATE_I2C_MASTER_DETECTED   11      // when receive advertise query
#define STATE_I2C_IDENTIFYING       12      // when sending name & id
#define STATE_I2C_CONFIGURING       13      // when receive gpio to read/write/notify
#define STATE_I2C_SYNCHRO           20      // when buffer is empty or less than 1s
#define STATE_I2C_POLLING           22      // when receiving query


typedef struct s_status_leds {
  struct s_pin    *ledRed;
  struct s_pin    *ledOrange;
  struct s_pin    *ledGreen;
} t_status_leds;

byte state = STATE_UNINITILIZED;
byte prevState = STATE_UNINITILIZED;

const int heartBeat = 5000;
unsigned long lastPolled = 0;
unsigned long lastAdvertise = 0;
unsigned long lastHeartBeat = -heartBeat;
// hearthbeat interval
#define TIMEOUT_SYNCHRO 500
#define TIMEOUT_ADVERTISE 30000

void setup_status_leds(struct t_loaded_module *module, va_list args) {
  struct s_status_leds *status_leds = (struct s_status_leds *)malloc(sizeof(s_status_leds));
  module->custom = status_leds;
  
  int ledRedNbr = va_arg(args, int);
  int ledOrangeNbr = va_arg(args, int);
  int ledGreenNbr = va_arg(args, int);
  
  status_leds->ledRed = get_pin(ledRedNbr);
  status_leds->ledOrange = get_pin(ledOrangeNbr);
  status_leds->ledGreen = get_pin(ledGreenNbr);
  
  pinMode(status_leds->ledRed->pin, OUTPUT);
  pinMode(status_leds->ledOrange->pin, OUTPUT);
  pinMode(status_leds->ledGreen->pin, OUTPUT);
  state = STATE_INITIALIZED;

  Serial.print(" --> StatusLeds setup {red: ");Serial.print((int)(status_leds->ledRed->pin));Serial.print(", orange: ");Serial.print((int)status_leds->ledOrange->pin);Serial.print(", green: ");Serial.print((int)status_leds->ledGreen->pin);Serial.println("}");
}

void status_leds_receive_event() {
  if (state < STATE_I2C_MASTER_DETECTED) {
    lastAdvertise = millis();
    state = STATE_I2C_MASTER_DETECTED;
  }
}

void status_leds_request_event() {
  lastPolled = millis();
}

void status_leds_set_synchronized() {
  state = STATE_I2C_SYNCHRO;
}

void status_leds_set_identified() {
  state = STATE_I2C_IDENTIFYING;
}

void status_leds_set_synchro() {
  state = STATE_I2C_SYNCHRO;
}

void write_status_leds(struct t_loaded_module *module) {
  struct s_status_leds *status_leds = (struct s_status_leds*)(module->custom);
  if (state == prevState)
    return ;
//  Serial.print("  to ");
//  Serial.print(state);
//  Serial.println("");
  prevState = state;
  if (state < STATE_I2C_READY) {
    digitalWrite(status_leds->ledRed->pin, HIGH);
    digitalWrite(status_leds->ledOrange->pin, LOW);
    digitalWrite(status_leds->ledGreen->pin, LOW);
  }
  else if (state < STATE_I2C_SYNCHRO) {
    digitalWrite(status_leds->ledRed->pin, LOW);
    digitalWrite(status_leds->ledOrange->pin, HIGH);
    digitalWrite(status_leds->ledGreen->pin, LOW);
  }
  else {
    digitalWrite(status_leds->ledRed->pin, LOW);
    digitalWrite(status_leds->ledOrange->pin, LOW);
    digitalWrite(status_leds->ledGreen->pin, HIGH);
  }
}

void loop_status_leds(struct t_loaded_module *module) {
  if ((millis() - lastHeartBeat) > heartBeat) {
//    Serial.print("heartbeat ");
//    Serial.print(millis() - lastHeartBeat);
//    Serial.println(" ms");
    lastHeartBeat = millis();
//    send_hearthbeat();
  }
  if ((millis() - lastAdvertise) > TIMEOUT_ADVERTISE) {
    if (state > STATE_INITIALIZED) {
//      Serial.print("timeout advertise after ");
//      Serial.print(millis() - lastHeartBeat);
//      Serial.println(" ms");
      // retrograde if no polling for long period
      state = STATE_INITIALIZED;
    }
    lastAdvertise = millis();
  }
  if ((millis() - lastPolled) > TIMEOUT_SYNCHRO) {
    if (state > STATE_I2C_CONFIGURING) {
      Serial.print("timeout synchro master after ");
      Serial.print(millis() - lastHeartBeat);
      Serial.println(" ms");
      // retrograde if no polling for long period
      state = STATE_I2C_CONFIGURING;
    }
    lastPolled = millis();
  }
  write_status_leds(module);
}
