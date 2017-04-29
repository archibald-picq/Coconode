
/********************************************
 * 
 * Communication
 * 
 * 
 ********************************************/
#define COMMAND_CONTINUE_BUFFER 1
#define COMMAND_SEND_MAGIC 2
#define COMMAND_VARIABLE_LENGTH 3
#define COMMAND_SEND_DEVICE_NAME 4
#define COMMAND_SEND_BOOT_TIME 5
#define COMMAND_SEND_BUILD_TIME 6
#define COMMAND_SEND_EVENT_QUEUE 7
#define COMMAND_CLEAR_EVENT_QUEUE 8
#define COMMAND_SET_NAME 9
#define COMMAND_SEND_MODULE_DEF 10
#define COMMAND_SET_MODULE_VALUE 11
#define COMMAND_CLEAR_BUFFER 12
#define COMMAND_ECHO 47

#define COMMAND_RESPONSE_OK 1
#define COMMAND_RESPONSE_MAGIC 42
#define COMMAND_RESPONSE_BAD_REQUEST -1
#define EVENT_QUEUE_SIZE 64

byte  send_buffer_length = 0;
byte  send_buffer[16];
byte  read_buffer_length = 0;
byte  read_buffer_position = 0;
byte  read_buffer_checksum;
byte  read_buffer[16];
void  (*buffer_command)(byte buffer[], byte length, void (*send)(const byte *buffer, byte length));
byte  i2c_pending_read = 0;

byte  event_queue[EVENT_QUEUE_SIZE];
byte  event_queue_position = 0;

void send_serial(const byte *buffer, byte length) {
  BTSerie.write(buffer, length);
}

void send_i2c(const byte *buffer, byte length) {
  // i2c_pending_read
//  Serial.print("send buffer {length: ");
//  Serial.print((int)length);
//  Serial.print(", i2c_pending_read: ");
//  Serial.print((int)i2c_pending_read);
//  Serial.print("}: ");
  if (i2c_pending_read < length) {
    memcpy(&(send_buffer[send_buffer_length]), &(buffer[i2c_pending_read]), length-i2c_pending_read);
    send_buffer_length += length-i2c_pending_read;
  }
//  for (byte c = 0; c < i2c_pending_read; c++) {
//    if (c)Serial.print(", ");
//    Serial.print(buffer[c], HEX);
//  }
  Wire.write(buffer, i2c_pending_read);
  i2c_pending_read = 0;
//  Serial.println();
}


void processEvent(byte c, void (*send)(const byte *buffer, byte length));



void receiveEventI2C(int howMany) {
  if (howMany) {
    return ;
  }
  status_leds_receive_event();
}

void requestEventI2C() {
  char c;
  while (Wire.available()) {
    c = Wire.read();
    processEvent(c, send_i2c);
  }
}


void send_return(byte value, void (*send)(const byte *buffer, byte length)) {
  uint8_t buffer[3];
  buffer[0] = value;
  buffer[1] = 0;
  buffer[2] = 0;
  send(buffer, 3);
}


byte count_events() {
  byte i = 0;
  byte count = 0;
  byte size;
  while (i < event_queue_position) {
    count++;
//    Serial.print("shift event of module ");
//    Serial.print(event_queue[i+0]);
//    Serial.print(" instance nbr ");
//    Serial.print(event_queue[i+1]);
//    Serial.print(" of ");
//    Serial.print(event_queue[i+2]);
//    Serial.println(" bytes");
    size = (event_queue[i] & 0xF0) >> 4;
    i += 1+size;
  }
  return count;
}

void shift_old_events(byte count) {
  byte i = 0;
  byte size;
  while (count && i < event_queue_position) {
    count--;
//    Serial.print("shift event of module ");
//    Serial.print(event_queue[i+0]);
//    Serial.print(" instance nbr ");
//    Serial.print(event_queue[i+1]);
//    Serial.print(" of ");
//    Serial.print(event_queue[i+2]);
//    Serial.println(" bytes");
    size = (event_queue[i] & 0xF0) >> 4;
    i += 1+size;
  }
//  Serial.print("shifting ");
//  Serial.print((int)i);
//  Serial.print(" bytes");
  for (byte c = i; c < EVENT_QUEUE_SIZE; c++)
    event_queue[c - i] = event_queue[c];
  event_queue_position -= i;
//  Serial.print(", now of size ");
//  Serial.print(event_queue_position);
//  Serial.print(" bytes for ");
//  Serial.print(count_events());
//  Serial.println(" events");
}

void  broadcast_change(loaded_module* module) {
  if (!module->module->append_event)
    return;
  byte idx = find_module_index(module);

//    Serial.print(" push event at ");
//    Serial.print(event_queue_position);
//    Serial.print(" of size ");
//    event_queue[event_queue_position++] = 0;//module->module->code;
//    event_queue[event_queue_position] = idx;
  byte size = module->module->append_event(module, &(event_queue[event_queue_position+1]));
  event_queue[event_queue_position] = idx | (size << 4);
//    event_queue[event_queue_position+1] = size;
//    Serial.print(size);
  event_queue_position += size+1;
//    Serial.print(" ends at ");
//    Serial.print(event_queue_position);

  if (event_queue_position > EVENT_QUEUE_SIZE - 8)
    shift_old_events(5);
}

byte find_event_length_for_bytes(byte length) {
  byte i = 0;
  byte size;
  while (i < event_queue_position) {
//    Serial.print("send event of instance nbr ");
//    Serial.print((int)(event_queue[i] & 0x0F));
//    Serial.print(" of ");
//    Serial.print((int)((event_queue[i] & 0xF0) >> 4));
//    Serial.println(" bytes");
    size = (event_queue[i] & 0xF0) >> 4;
    if (i + 1 + size >= length)
      break;
    i += 1+size;
  }
  return i;
}

/**
 * Commands
 */

void command_send_echo(byte buffer[], byte length, void (*send)(const byte *buffer, byte length)) {
  Serial.print(F("echo value "));
  Serial.println((int)buffer[0]);
//  send_return(1);
}

void clear_event_queue(byte buffer[], byte length, void (*send)(const byte *buffer, byte length)) {
  byte checksum = resp_checksum(buffer, length);
  byte count = buffer[0];
//  Serial.print("clear queue of ");
//  Serial.print((int)count_events());
//  Serial.print(" events by ");
//  Serial.print((int)count);
//  Serial.print(" events validated by ");
//  Serial.print(read_buffer_checksum);
//  Serial.print(" vs received: ");
//  Serial.print(checksum);
//  Serial.println();
  if (read_buffer_checksum == checksum) {
//  send_return(1);
    shift_old_events(count);
//    Serial.print("now of size ");
//    Serial.print((int)count_events());
//    Serial.print(" events");
//    Serial.print(" ending at ");
//    Serial.print(event_queue_position);
//    Serial.println();
    send_return(COMMAND_RESPONSE_OK, send);
  }
  else {
    send_return(COMMAND_RESPONSE_BAD_REQUEST, send);
  }
}


void command_clear_buffer(byte buffer[], byte length, void (*send)(const byte *buffer, byte length)) {
  Serial.println(F("clear buffer"));
  send_buffer_length = 0;
  send_return(COMMAND_RESPONSE_OK, send);
}

void command_send_magic(byte buffer[], byte length, void (*send)(const byte *buffer, byte length)) {
  send_return(COMMAND_RESPONSE_MAGIC, send);
}

void command_send_device_name(byte buffer[], byte length, void (*send)(const byte *buffer, byte length)) {
  uint8_t resp[16];
  resp[0] = COMMAND_RESPONSE_OK;
  resp[1] = strlen(uniqName);
  memcpy(&(resp[3]), uniqName, resp[1]);
  Serial.print(F("name > send "));
  Serial.print((int)resp[1]);
  Serial.print(F(" bytes ("));
  Serial.print(uniqName);
  Serial.println(F(")"));
  resp[2] = resp_checksum(&(resp[3]), resp[1]);
  send(resp, resp[1]+3);
}

void command_send_boot_time(byte buffer[], byte length, void (*send)(const byte *buffer, byte length)) {
  unsigned long time = millis();
  uint8_t resp[6];
  resp[0] = COMMAND_RESPONSE_OK;
  resp[1] = sizeof(time);
  memcpy(&(buffer[3]), &time, resp[1]);
  Serial.print(F("boot > send "));
  Serial.print((int)resp[1]);
  Serial.println(F(" bytes"));
  resp[2] = resp_checksum(&(resp[3]), resp[1]);
  send(resp, resp[1]+3);
}

void command_send_build_time(byte buffer[], byte length, void (*send)(const byte *buffer, byte length)) {
  uint8_t resp[32];
  resp[0] = COMMAND_RESPONSE_OK;
  byte*  position = &(resp[3]);
  memcpy(position, __DATE__, strlen(__DATE__));
  position += strlen(__DATE__);
  *(position++) = ' ';
  memcpy(position, __TIME__, strlen(__TIME__));
  position += strlen(__TIME__);
  resp[1] = position - &(resp[3]);
  Serial.print(F("build > send "));
  Serial.print((int)resp[1]);
  Serial.println(F(" bytes"));
  resp[2] = resp_checksum(&(resp[3]), resp[1]);
  send(resp, resp[1]+3);
}

void command_send_module_def(byte buffer[], byte length, void (*send)(const byte *buffer, byte length)) {
  
  uint8_t resp[32];
  resp[0] = COMMAND_RESPONSE_OK;
  resp[1] = dump_modules(&(resp[3]));
  
  resp[2] = resp_checksum(&(resp[3]), resp[1]);
  Serial.print(F("--- send module def of "));
  Serial.print(resp[1]);
  Serial.print(F(" bytes with checksum "));
  Serial.print(resp[2]);
  Serial.println();
  send(resp, resp[1]+3);
//  trigger_all_modules_change();
}

void command_set_module_value(byte buffer[], byte length, void (*send)(const byte *buffer, byte length)) {
  if (length < 3) {
    Serial.println("Insufficient buffer for command");
    return;
  }

  byte checksum = resp_checksum(&(buffer[1]), length-1);
  if (checksum != buffer[0]) {
    Serial.print(F("Bad checksum for command body, found "));
    Serial.print((int)checksum);
    Serial.print(F(" should be "));
    Serial.print((int)buffer[0]);
    Serial.println();
    return;
  }
  Serial.print(F("checksum "));
  Serial.print((int)buffer[0]);
  Serial.print(F(" OK, set module value "));
  Serial.print((int)(buffer[1]));
  Serial.print(F(" to value "));
  Serial.print((int)(buffer[2]));
  Serial.print(F(" ("));
  Serial.print(length - 2);
  Serial.print(F(" bytes)"));
  Serial.println();

  loaded_module* module = find_module_by_index(buffer[1]);

  if (!module->module->write_func) {
    Serial.println(F("Module is not writable: read only"));
    return;
  }
  module->module->write_func(module, (int)(buffer[2]));
  send_return(COMMAND_RESPONSE_OK, send);
}

void command_send_event_queue(byte buffer[], byte length, void (*send)(const byte *buffer, byte length)) {
  byte i = find_event_length_for_bytes(13);

  uint8_t resp[16];
  resp[0] = COMMAND_RESPONSE_OK;
  resp[1] = i;
  memcpy(&(resp[3]), event_queue, resp[1]);
//  Serial.print("queue > send ");
//  Serial.print((int)resp[1]);
//  Serial.print(" bytes (");
//  Serial.print(i);
//  Serial.println(")");
  resp[2] = resp_checksum(&(resp[3]), resp[1]);
  send(resp, resp[1]+3);
}

void command_wait_for_variable_command(byte buffer[], byte length, void (*send)(const byte *buffer, byte length)) {
  Serial.print(F("wait for variable command "));
  Serial.print((int)buffer[0]);
  Serial.print(F(" of size "));
  Serial.print((int)buffer[1]);
  Serial.print(F(" with checksum "));
  Serial.print((int)buffer[2]);
  Serial.println();
  
  read_buffer_length = buffer[1];
  read_buffer_position = 0;
  read_buffer_checksum = buffer[2];
  
  byte c = (int)read_buffer[0];
  if (c == COMMAND_ECHO)
    buffer_command = command_send_echo;
  else if (c == COMMAND_CLEAR_EVENT_QUEUE) {
    buffer_command = clear_event_queue;
  }
//  else if (c == COMMAND_SET_BITMASK_VALUE)
//    buffer_command = command_set_bitmask_value;
  else {
    // unsupported variable command
    read_buffer_length = 0;
    send_return(0, send);
  }
}

/**
 * Dispatch byte event and send back response through call back `send`
 */
void processEvent(byte c, void (*send)(const byte *buffer, byte length)) {
//  Serial.print((int)c);
//  Serial.print(".");
  status_leds_request_event();
  
  if (read_buffer_length) {
    // reading arbitrary bytes for variable length body command
    read_buffer_length--;
    read_buffer[read_buffer_position++] = c;
//    Serial.print("append '");
//    Serial.print((int)c);
//    Serial.print("' to variable command until ");
//    Serial.print((int)read_buffer_length);
//    Serial.println();
    if (!read_buffer_length)
      buffer_command(read_buffer, read_buffer_position, send);
    return ;
  }
  
  byte command = (c & 0x0F);
  byte params = (c & 0xF0) >> 4;
  c = c & 0x0F;
//  Serial.print("packet: ");
//  Serial.print((int)c);
//  Serial.print(", command: ");
//  Serial.print((int)command);
//  Serial.print(", params: ");
//  Serial.print(params);
//  Serial.println(" bytes");  

  
  if (command == COMMAND_CONTINUE_BUFFER) {
    
    byte length = send_buffer_length > 16? 16: send_buffer_length;
//    Serial.print("got continue packet for ");
//    Serial.print((int)length);
//    Serial.println(" bytes");
    
    Wire.write(send_buffer, length);
    i2c_pending_read = length;
    send(send_buffer, length);
    for (byte i = length; i < send_buffer_length; i++)
      send_buffer[i-length] = send_buffer[i];
    send_buffer_length -= length;
    status_leds_set_synchronized();
    return;
  }
  
  if (command == COMMAND_SEND_MAGIC) {  // magic marker
    i2c_pending_read = 3;
    buffer_command = command_send_magic;
  }
  else if (command == COMMAND_SEND_DEVICE_NAME) {  // name
    i2c_pending_read = 3;
    buffer_command = command_send_device_name;
    status_leds_set_identified();
  }
  else if (command == COMMAND_SEND_BOOT_TIME) {  // time from boot
    i2c_pending_read = 3;
    buffer_command = command_send_boot_time;
  }
  else if (command == COMMAND_SEND_BUILD_TIME) {  // build datetime
    i2c_pending_read = 3;
    buffer_command = command_send_build_time;
  }
  else if (command == COMMAND_SEND_EVENT_QUEUE) {
    i2c_pending_read = 3;
    buffer_command = command_send_event_queue;
  }
  else if (command == COMMAND_CLEAR_EVENT_QUEUE) {
    i2c_pending_read = 3;
    read_buffer[0] = params;
    read_buffer_position = 1;
    read_buffer_checksum = resp_checksum(read_buffer, read_buffer_position);
    buffer_command = clear_event_queue;
  }
  else if (command == COMMAND_SET_MODULE_VALUE) {
    i2c_pending_read = 3;
//    read_buffer[0] = params;
    read_buffer_position = 0;
    read_buffer_length = params;
    //read_buffer_checksum = resp_checksum(read_buffer, read_buffer_position);
    buffer_command = command_set_module_value;
  }
  else if (command == COMMAND_SET_NAME) {
    Serial.println("receive variable command SET_NAME, wait for data ...");
    read_buffer_length = params;
    read_buffer_position = 0;
//    buffer_command = command_wait_for_variable_command;
  }
  else if (command == COMMAND_SEND_MODULE_DEF) {
    i2c_pending_read = 3;
    buffer_command = command_send_module_def;
  }
  else if (command == COMMAND_CLEAR_BUFFER) {
    i2c_pending_read = 3;
    buffer_command = command_clear_buffer;
  }
//  else if (command == COMMAND_SET_BITMASK_VALUE) {
//    Serial.println("receive command SET_BITMASK, wait for data (2 bytes)");
//    read_buffer_length = 2;
//    read_buffer_position = 0;
//    buffer_command = command_set_bitmask_value;
//  }
//  else if (command == COMMAND_VARIABLE_LENGTH) {
//    Serial.println("receive variable command, wait for data ...");
//    read_buffer_length = 3;
//    read_buffer_position = 0;
//    buffer_command = command_wait_for_variable_command;
////    send_return(COMMAND_RESPONSE_OK, send);
//  }
  else {
    i2c_pending_read = 3;
    
    Serial.print(F(" > onRequest(), unsupported command "));
    Serial.print((int)command);
    Serial.print(F(" with remaining params "));
    Serial.print((int)params);
    Serial.println();
    
    send_return(COMMAND_RESPONSE_BAD_REQUEST, send);
    return;
  }

  if (!read_buffer_length)
      buffer_command(read_buffer, read_buffer_position, send);
}

uint8_t resp_checksum(byte* buffer, int length) {
  uint8_t chk = 0;
  
  for (int i=0; i<length; i++)
    chk += buffer[i] % 256;

  chk = chk % 42;
  return chk;
}








//void send_wire(char channel, const char *device, const char *eventName) {
//  // send event
//  Wire.beginTransmission(channel);
//  Wire.write("dev:");
//  Wire.write(uniqName);
//  Wire.write(":update:");
//  Wire.write(device);
//  Wire.write(":");
//  Wire.write(eventName);
//  Wire.write("\n");
//  byte ret = Wire.endTransmission();
//  print_i2c_error(ret);
//}
//
//void print_i2c_error(byte ret) {
//  if (ret == 0) {
//    Serial.println("packet sent");
//  }
//  else if (ret == 1) {
//    Serial.println("packet too large for i2c buffer");
//  }
//  else if (ret == 2) {
//    Serial.println("NACK received for addr, should send STOP");
//  }
//  else if (ret == 3) {
//    Serial.println("NACK received for data, can send STOP, or repeated START");
//  }
//  else if (ret > 3) {
//    Serial.println("i2c error");
//  }
//}


