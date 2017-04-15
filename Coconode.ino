#include <stdarg.h>
#include <Wire.h>
#include <SoftwareSerial.h>  //Software Serial Port  
#define RxD 11    //Pin 10 pour RX (pin0=serial) vert
#define TxD 12    //Pin 11 pour TX, on peut changer noir
SoftwareSerial BTSerie(RxD,TxD);

#define INVERTED             1
#define NORMAL               0
#define DEBOUNCE_DELAY       50

#define BUTTON_MODULE        0x01
#define LED_MODULE           0x02
#define POWER_METER_MODULE   0x03
#define STATUS_LEDS_MODULE   0x04
#define SERVO_MODULE         0x05
#define LINKY_MODULE         0x06
#define THERMOMETER_MODULE   0x07

#define MODULE_5A            185
#define MODULE_20A           100
#define MODULE_30A           66

typedef struct t_loaded_module {
  struct s_module*  module;
  void*             custom;
} loaded_module;

typedef struct s_pin {
  byte              pin;
  byte              used;
} t_pin;

typedef struct s_module {
  byte              code;
  char*             name;
  void              (*setup_func)(struct t_loaded_module* module, va_list args);
  void              (*loop_func)(struct t_loaded_module* module);
  int               (*read_func)(struct t_loaded_module* module);
  void              (*write_func)(struct t_loaded_module* module, int value);
  void              (*get_string_value)(struct t_loaded_module* module, byte* buffer);
  byte              (*append_event)(struct t_loaded_module* module, byte* buffer);
} t_module;

char* uniqName;
char* get_uniq_board_name();
byte get_uniq_board_channel();
void processEvent(byte c, void (*send)(const byte *buffer, byte length));

s_pin pins[] = {
  {0,  1},    // 0    // used by Serial TxD
  {1,  1},    // 1    // used by Serial RxD
  {2,  0},    // 2
  {3,  0},    // 3
  {4,  0},    // 4
  {5,  0},    // 5
  {6,  0},    // 6
  {7,  0},    // 7
  {8,  0},    // 8
  {9,  0},    // 9
  {10, 0},    // 10
  {11, 1},    // 11    // used by SoftwareSerial RxD
  {12, 1},    // 12    // used by SoftwareSerial TxD
  {13, 0},     // 13
  {A0, 0},    // A0
  {A1, 0},    // A1
  {A2, 0},    // A2
  {A3, 0},    // A3
  {A4, 1},    // A4    // used by i2c SDA
  {A5, 1},    // A5    // used by i2c SCL
  {A6, 0},    // A6
  {A7, 0},    // A7
  {0,  0}
};

void             button_setup(loaded_module* module, va_list args);
void             button_loop(loaded_module* module);
int              button_read(loaded_module* module);
void             button_string_value(loaded_module* module, char* buffer);
byte             button_append_event(loaded_module* module, char* buffer);

void             led_setup(loaded_module* module, va_list args);
void             led_loop(loaded_module* module);
int              led_read(loaded_module* module);
void             led_write(loaded_module* module, int value);
byte             led_append_event(loaded_module* module, char* buffer);

void             powermeter_setup(loaded_module* module, va_list args);
void             powermeter_loop(loaded_module* module);
//int              read_power_meter_func(loaded_module* module);
void             powermeter_string_value(loaded_module* module, char* buffer);
byte             powermeter_append_event(loaded_module* module, char* buffer);

void             setup_status_leds(loaded_module* module, va_list args);
void             loop_status_leds(loaded_module* module);

void             linky_setup(loaded_module* module, va_list args);
void             linky_loop(loaded_module* module);
byte             linky_event(loaded_module* module, char* buffer);

void             thermometer_setup(loaded_module* module, va_list args);
void             thermometer_loop(loaded_module* module);
byte             thermometer_event(loaded_module* module, char* buffer);


s_module compiled_modules[] = {
  {BUTTON_MODULE,      "Button",     button_setup,      button_loop,       button_read, 0,          button_string_value,     button_append_event},
  {LED_MODULE,         "Led",        led_setup,         led_loop,          led_read,    led_write,  0,                       led_append_event},
  {POWER_METER_MODULE, "PowerMeter", powermeter_setup,  powermeter_loop,   0,           0,          powermeter_string_value, powermeter_append_event},
  {STATUS_LEDS_MODULE, "StatusLeds", setup_status_leds, loop_status_leds},
  {LINKY_MODULE,       "Linky Relay",linky_setup,       linky_loop,        0,           0,          0,                       linky_event},
  {THERMOMETER_MODULE, "Thermometer",thermometer_setup, thermometer_loop,  0,           0,          0,                       thermometer_event},
  0
};

loaded_module modules[16];

struct s_pin* get_pin(byte nbr) {
  for (byte i = 0; i == 0 || pins[i].pin; i++)
    if (pins[i].pin == nbr)
      return &(pins[i]);
  return 0;
}

byte first_empty_module() {
  for (byte i=0; i<16; i++)
    if (!modules[i].module)
      return i;
  return -1;
}

struct s_module* find_module(byte code) {
  for (char c = 0; compiled_modules[c].code; c++)
    if (compiled_modules[c].code == code)
      return &(compiled_modules[c]);
  return 0;
}

int find_module_index(loaded_module* module) {
  for (char i = 0; i < 16; i++)
    if (&(modules[i]) == module)
      return i;
  return -1;
}

loaded_module* find_module_by_index(byte index) {
  return &(modules[index]);
}

void   add_module(byte code, ...) {
  va_list args;
//  Serial.print("searching for module ");
//  Serial.print((int)code);

  byte empty = first_empty_module();
  struct s_module *module = find_module(code);
  va_start ( args, code );
  if (module) {
//    Serial.print(" (");Serial.print(module->name);Serial.print(") insert at ");Serial.print((int)empty);Serial.print(" : ");Serial.println();
    modules[empty].module = module;
    Serial.print(F("call "));
    Serial.print(modules[empty].module->name);
    Serial.print(F(" setup: "));
    modules[empty].module->setup_func(&(modules[empty]), args);
//    Serial.println("");
    return;
  }
  va_end ( args );
  Serial.println(F("NOT FOUND"));
}

void  trigger_all_modules_change() {
  for (char i = 0; i < 16; i++)
    if (modules[i].module) {
      broadcast_change(&(modules[i]));
    }
}

byte dump_modules(byte* buffer) {
  byte* position = buffer;
  for (char i = 0; i < 16; i++)
    if (modules[i].module) {
      *position = modules[i].module->code;
      position++;
    }
  return position - buffer;
}

void setup() {
  Serial.begin(9600);
  // Configuration du bluetooth
  pinMode(RxD, INPUT);
  pinMode(TxD, OUTPUT);
  BTSerie.begin(115200);
  uniqName = get_uniq_board_name();
  byte channel = get_uniq_board_channel();
  
//  Serial.println(F("+-----------------+"));
  Serial.print(F("+ Coco node: "));Serial.print(uniqName);Serial.print(", i2c id: ");Serial.println((int)channel);
  Serial.println("Compiled: " __DATE__ ", " __TIME__ ", " __VERSION__);
  Serial.println(F("+-----------------+"));

  Serial.println(F("+ init modules"));
  // breadboard
//  add_module(BUTTON_MODULE, 7);
//  add_module(BUTTON_MODULE, A6);
//  add_module(BUTTON_MODULE, A7);
//  add_module(STATUS_LEDS_MODULE, 4, 5, 6);
//  add_module(LED_MODULE, 4);
//  add_module(LED_MODULE, 5);
//  add_module(LED_MODULE, 6);
//  add_module(POWER_METER_MODULE, A1, MODULE_20A);
  add_module(THERMOMETER_MODULE, 8);

  // triplite
//  add_module(LED_MODULE, 4);
//  add_module(LED_MODULE, 5);
//  add_module(LED_MODULE, 6);
//  add_module(POWER_METER_MODULE, A1, MODULE_20A);

  // disjoncteur
//  add_module(POWER_METER_MODULE, A0, MODULE_20A);
//  add_module(POWER_METER_MODULE, A1, MODULE_20A);
//  add_module(POWER_METER_MODULE, A2, MODULE_20A);
//  add_module(POWER_METER_MODULE, A3, MODULE_30A);

  // Linky rekay
//  add_module(LINKY_MODULE, 9, 10);
  Serial.println(F("+-----------------+"));
  
//  Serial.print("+ Register i2c bus at ");Serial.print((int)channel);Serial.println();

//  Serial.println(F("+-----------------+"));

  // init
  Wire.begin(channel);
  Wire.onReceive(receiveEventI2C); // register event
  Wire.onRequest(requestEventI2C);
}

void loop() {
  byte recvChar;
    
  if (BTSerie.available()) {
    recvChar = BTSerie.read();
    processEvent(recvChar, send_serial);
  }
  // Serial.write(blueToothSerial.read());
  if (Serial.available()) {
    recvChar = Serial.read();
    BTSerie.write(recvChar);
  }
  
  // loop pins
  for (byte i = 0; i < 16; i++)
    if (modules[i].module)
      modules[i].module->loop_func(&(modules[i]));
}

