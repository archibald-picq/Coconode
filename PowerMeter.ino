#define ADC_SCALE 1023.0
#define VREF 5.0
#define DEFAULT_FREQUENCY 50

uint32_t start_time = millis();
typedef struct s_power_meter {
  struct s_pin    *pin;
  short           mVperAmp;
  short           maxValue;          // store max value here
  unsigned short  minValue;          // store min value here
  unsigned short  result;
  unsigned short  prevResult;
  
  double          measurements_count;
  uint32_t        Isum;
} t_power_meter;

double toAmpRMS(double voltage, float mVperAmp) {
  double VRMS = (voltage / 2.0) * 0.707;
  return (VRMS * 1000) / mVperAmp;
}

void powermeter_setup(struct t_loaded_module *module, va_list args) {
  struct s_power_meter *power_meter = (struct s_power_meter *)malloc(sizeof(s_power_meter));
  module->custom = power_meter;
 
  int nbr = va_arg(args, int);
  int power = va_arg(args, int);
  
  power_meter->pin = get_pin(nbr);
  power_meter->mVperAmp = power;//MODULE_20A;
  power_meter->minValue = 1024;
  power_meter->maxValue = 0;

  power_meter->measurements_count = 0;
  power_meter->Isum = 0;
  
  // TODO: check supported input pin (analog)
  Serial.print(" --> setup power meter on pin ");
  Serial.print(power_meter->pin->pin);
  Serial.print(" calibrated for ");
  Serial.print(power_meter->mVperAmp);
  Serial.print(power == MODULE_5A? " (5A)": (power == MODULE_20A? " (20A)": (power == MODULE_30A? " (30A)": "")));
  Serial.println();
}

void powermeter_loop(struct t_loaded_module *module) {
  struct s_power_meter *power_meter = (struct s_power_meter*)(module->custom);
  int readValue;
  int32_t Inow;
  
  if ((millis()-start_time) < 1000) {
    readValue = analogRead(power_meter->pin->pin);

    // method 1
    if (readValue > power_meter->maxValue)
      power_meter->maxValue = readValue;
    if (readValue < power_meter->minValue)
      power_meter->minValue = readValue;

    // method 2
    Inow = readValue - 512;
    power_meter->Isum += Inow*Inow;
    power_meter->measurements_count++;
  }
  else {
    // DC
    // float I = (float)acc / 10.0 / ADC_SCALE * VREF / sensitivity;

    // AC
    float voltage = ((power_meter->maxValue - power_meter->minValue) * 5.0)/1024.0;
    double amps = toAmpRMS(voltage, (float)power_meter->mVperAmp);
    
//    Serial.print((int)power_meter->pin->pin);
//    Serial.print(" > min/max ");
//    Serial.print(power_meter->minValue);
//    Serial.print(" / ");
//    Serial.print(power_meter->maxValue);
//    Serial.print(", voltage: ");
//    Serial.print(voltage);
//    Serial.print(", amps: ");
//    Serial.print(amps);
//    Serial.println();
//    Serial.print("Isum: ");
//    Serial.print(power_meter->Isum);
//    Serial.print(", count: ");
//    Serial.print(power_meter->measurements_count);
//    Serial.print(", sqrt: ");
//    Serial.print(sqrt(power_meter->Isum / power_meter->measurements_count));
//    Serial.print(" / ADC_SCALE * VREF = ");
//    Serial.print((double)sqrt(power_meter->Isum / power_meter->measurements_count) / ADC_SCALE * VREF);
//    float sensitivity = (float)MODULE_20A / 1000;
//    Serial.print(", sensitivity: ");
//    Serial.print(sensitivity);
    double calc = (double)sqrt(power_meter->Isum / power_meter->measurements_count) / ADC_SCALE * VREF;
    calc = (calc / ((float)(power_meter->mVperAmp) / 1000)) * 1000;
//    Serial.print(", calc: ");
//    Serial.print(calc);
//    float Irms = sqrt(power_meter->Isum / power_meter->measurements_count) / ADC_SCALE * VREF / (float)(power_meter->mVperAmp);
    
//    Serial.print(", result: ");
//    Serial.print(Irms);
//    Serial.println();
    power_meter->result = calc;// amps * 1000;

    // reset method 1
    power_meter->maxValue = 0;
    power_meter->minValue = 1024;

    // reset method 2
    power_meter->measurements_count = 0;
    power_meter->Isum = 0;

//    unsigned long milliWatt = (unsigned long)(power_meter->result) * 220;
//    unsigned long watt = milliWatt/1000;
    if (power_meter->prevResult != power_meter->result) {
      power_meter->prevResult = power_meter->result;
      broadcast_change(module);
    }
//    Serial.print("value for ");
//    Serial.print(pin->pin);
//    Serial.print(" => ");
//    Serial.print(power_meter->result);
//    Serial.print(" mA, ");
//    Serial.print(watt);
//    Serial.print(" W");
//    Serial.println("");
    start_time = millis();
  }
}

//int              read_power_meter_func(struct t_loaded_module *module) {
//  struct s_power_meter *power_meter = (struct s_power_meter*)(module->custom);
  
//  return power_meter->result;
//  state->amp1 = power_meter[0].result;
//  state->amp2 = power_meter[1].result;
//  state->amp3 = power_meter[2].result;
//}

void             powermeter_string_value(loaded_module* module, char* buffer) {
  struct s_power_meter *power_meter = (struct s_power_meter*)(module->custom);
  
  unsigned long milliWatt = (unsigned long)(power_meter->result) * 220;
  unsigned long watt = milliWatt/1000;
  sprintf(buffer, "%i W", watt);
}

byte             powermeter_append_event(loaded_module* module, char* buffer) {
  struct s_power_meter *power_meter = (struct s_power_meter*)(module->custom);
  
  memcpy(buffer, &(power_meter->result), sizeof(unsigned short));
  return sizeof(unsigned short);
}

