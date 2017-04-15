#include <OneWire.h>


// ---------- Initialisation des variables ---------------------

uint32_t thermometer_start_time = millis();
uint32_t thermometer_detect_time = 0;

typedef struct s_thermometer {
  float                 temperature;
  byte                  addr[8];
  boolean               detected = false;
  struct s_thermometer  *next;
} t_thermometer;

typedef struct s_bus {
  OneWire               *ds;
  boolean               asked = false;
  float                 temperature;
  byte                  addr[8];
  struct s_thermometer  *thermometers = 0;    // chained list
} t_bus;

// Variables propres au DS18B20
//const int DS18B20_PIN=8;
const int DS18B20_ID=0x28;
// Déclaration de l'objet ds

// Variables générales
const int SERIAL_PORT=9600;

void             thermometer_setup(loaded_module* module, va_list args) {
  struct s_bus *thermometer = (struct s_bus *)malloc(sizeof(s_bus));
  module->custom = thermometer;
 
  int nbr = va_arg(args, int);
//  Serial.print("pin: ");
//  Serial.print(nbr);
  thermometer->ds = new OneWire(nbr); // on pin DS18B20_PIN (a 4.7K resistor is necessary)
  Serial.print(F(" --> OneWire init"));
  // Initialisation du port de communication avec le PC
//  Serial.begin(SERIAL_PORT);
//  Serial.println("Initialisation du programme");
  
  // TODO: check supported input pin (analog)
//  Serial.print(" --> setup thermometer on pin ");
//  Serial.print(thermometer->pin->pin);
//  Serial.print(" calibrated for ");
//  Serial.print(thermometer->mVperAmp);
//  Serial.print(power == MODULE_5A? " (5A)": (power == MODULE_20A? " (20A)": (power == MODULE_30A? " (30A)": "")));
  Serial.println();
}

void thermometer_reset_detected(struct s_bus *thermometer) {
  struct s_thermometer  **current = &(thermometer->thermometers);
  // reset detected flag
  for (current = &(thermometer->thermometers); *current; current = &((*current)->next)) {
    (*current)->detected = false;
  }
}

byte thermometer_count(struct s_bus *thermometer) {
  byte                  count = 0;
  struct s_thermometer  **current = &(thermometer->thermometers);
  while (*current) {
    count++;
    current = &((*current)->next);
  }
  return count;
}

byte print_mac(byte *addr) {
  if (addr[0] < 0x10) Serial.print('0');
  Serial.print(addr[0], HEX);
  Serial.print(":");
  if (addr[1] < 0x10) Serial.print('0');
  Serial.print(addr[1], HEX);
  Serial.print(":");
  if (addr[2] < 0x10) Serial.print('0');
  Serial.print(addr[2], HEX);
  Serial.print(":");
  if (addr[3] < 0x10) Serial.print('0');
  Serial.print(addr[3], HEX);
  Serial.print(":");
  if (addr[4] < 0x10) Serial.print('0');
  Serial.print(addr[4], HEX);
  Serial.print(":");
  if (addr[5] < 0x10) Serial.print('0');
  Serial.print(addr[5], HEX);
}

struct s_thermometer **thermometer_get_last(struct s_bus *thermometer) {
  struct s_thermometer  **last = &(thermometer->thermometers);
  
  while (*last)
    last = &((*last)->next);
  return last;
}

struct s_thermometer *thermometer_insert_at(struct s_thermometer **at) {
  struct s_thermometer *next = *at? (*at)->next: 0;
  *at = malloc(sizeof(struct s_thermometer));
  (*at)->next = next;
  return *at;
}

struct s_thermometer *thermometer_search(struct s_bus *bus, byte *addr, byte *pos) {
  struct s_thermometer **current = &(bus->thermometers);
  for (*pos = 0; *current; current = &((*current)->next), (*pos)++) {
    if (!memcmp(&((*current)->addr[1]), addr, 6)) {
      Serial.print(F(" (found "));
      Serial.print((int)*pos);
      Serial.print(F(" at)"));
      return *current;
    }
  }
  *pos = -1;
  return 0;
}

void detect_bus_devices(struct s_bus *bus) {
  byte                  addr[8];
  struct s_thermometer  **current = &(bus->thermometers);
  byte                  pos = 0;

  thermometer_reset_detected(bus);
  
  Serial.println("detecting devices ...");
  Serial.flush();
  while (bus->ds->search(addr)) {
    
    // Cette fonction sert à surveiller si la transmission s'est bien passée
    if (OneWire::crc8(addr, 7) != addr[7]) {
      Serial.println(F("getTemperatureDS18b20 : <!> CRC is not valid! <!>"));
      continue;
    }
    
    // On vérifie que l'élément trouvé est bien un DS18B20
    if (addr[0] != DS18B20_ID) {
      Serial.println(F("Not a DS18B20"));
      continue;
    }

    
    Serial.print("found device ");
    print_mac(&(addr[1]));

    // search existing
    struct s_thermometer *found = thermometer_search(bus, &(addr[1]), &pos);

    if (!found) {
      // find last
      struct s_thermometer  **last = thermometer_get_last(bus);
      Serial.print(F(" (allocate)"));
      Serial.flush();

      found = thermometer_insert_at(last);
      Serial.print(F(", copy addr"));
      Serial.flush();
      memcpy(found->addr, addr, 8);
    }
    
    found->detected = true;
    
    Serial.println();
  }

  // remove undetected
  current = &(bus->thermometers);
  struct s_thermometer  **to_free;
  for (pos = 0; *current; pos++) {
    if (!(*current)->detected) {
      Serial.print("pop device at ");
      Serial.print((int)pos);
      Serial.print(" ");
      print_mac(&((*current)->addr[1]));
      
      to_free = current;
      *current = (*current)->next;
      Serial.print("");
      free(*to_free);
      Serial.println();
    }
    else {
      current = &((*current)->next);
    }
  }

  byte count = thermometer_count(bus);
  Serial.print(F("thermometer count "));
  Serial.print((int)count);
  Serial.print(" => ");
  
  current = &(bus->thermometers);
  while (*current) {
    print_mac(&((*current)->addr[1]));
    current = &((*current)->next);
    if (*current)
      Serial.print(" || ");
  }

  Serial.println();
  
  if (bus->thermometers) {
//    Serial.println("using default device");
    memcpy(bus->addr, bus->thermometers->addr, 8);
  }
}

void             thermometer_loop(loaded_module* module) {
  struct s_bus *thermometer = (struct s_bus*)(module->custom);

  if ((millis() - thermometer_detect_time) > 5000) {
      detect_bus_devices(thermometer);
      thermometer_detect_time = millis();
  }

//  if ((millis()-thermometer_start_time) < 850) {
//    if (!thermometer->asked) {
////      float temp = 0.0;
//      
//      thermometer->asked = true;
//
//
//      
//      // Demander au capteur de mémoriser la température et lui laisser 850ms pour le faire (voir datasheet)
//      thermometer->ds->reset();
//      thermometer->ds->select(thermometer->addr);
//      thermometer->ds->write(0x44);
//
//    }
//  }
//  else {
//    if (!thermometer->asked) {
//      start_time = millis();
//      
//    }
//    else {
//      
//      byte data[12];
//      byte i;
//      // Demander au capteur de nous envoyer la température mémorisé
//      thermometer->ds->reset();
//      thermometer->ds->select(thermometer->addr);
//      thermometer->ds->write(0xBE);
//      
//      // Le MOT reçu du capteur fait 9 octets, on les charge donc un par un dans le tableau data[]
//      for ( i = 0; i < 9; i++) {
//        data[i] = thermometer->ds->read();
//      }
//      // Puis on converti la température (*0.0625 car la température est stockée sur 12 bits)
//      thermometer->temperature = ( (data[1] << 8) + data[0] )*0.0625;
//      
//      //thermometer->temperature = getTemperatureDS18b20(); // On lance la fonction d'acquisition de T°
//      
//      thermometer_start_time = millis();
//      thermometer->asked = false;
//      // on affiche la T°
////      Serial.print(F("temperature: ")); 
////      Serial.println(thermometer->temperature);
//    }
//  }
}

byte             thermometer_event(loaded_module* module, char* buffer) {
  return 0;
}


