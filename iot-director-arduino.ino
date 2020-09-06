#include <SPI.h>
#include <Ethernet.h>
#include <avr/wdt.h>

const byte mac_address[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
const char iot_host[] = "";
const int iot_port = 8000;
const int client_reconnect_max_attempts = 10;

const EthernetClient client;
IPAddress localIP;
bool force_reset_ethernet = false;
int client_reconnect_counter = 0;

void setup() {
  wdt_enable(WDTO_8S);

  setup_serial();
  setup_ethernet();
}

void setup_serial() {
  Serial.begin(115200);

  Serial.println("Serial initialized.");

  wdt_reset();
}

bool setup_ethernet() {
  wdt_reset();

  force_reset_ethernet = false;
  client_reconnect_counter = 0;
  
  Serial.println("Configuring Ethernet...");
  
  if (Ethernet.begin(mac_address)) {
    wdt_reset();

    localIP = Ethernet.localIP();

    Serial.print("Ethernet configured with IP address ");
    Serial.print(localIP);
    Serial.print(" and subnet mask ");
    Serial.print(Ethernet.subnetMask());
    Serial.println(".");

    wdt_reset();

    reset_client();

    return true;
  }
  
  wdt_reset();
  
  Serial.println("Failed to configure Ethernet using DHCP.");

  wdt_reset();

  return false;
}

bool maintain_ethernet() {
  wdt_reset();
  
  int mstatus = Ethernet.maintain();
  
  wdt_reset();

  if ((mstatus == 2) || (mstatus == 4)) {
    IPAddress newLocalIP = Ethernet.localIP();
    
    Serial.print("Ethernet renewed with IP address ");
    Serial.print(newLocalIP);
    Serial.print(" and subnet mask ");
    Serial.print(Ethernet.subnetMask());
    Serial.println(".");

    if (newLocalIP != localIP) {
      reset_client();
    }

    localIP = newLocalIP;
  } else if ((mstatus == 1) || (mstatus == 3)) {
    return setup_ethernet();
  }

  return true;
}

bool setup_client() {
  wdt_reset();

  reset_client();

  Serial.print("Connecting to ");
  Serial.print(iot_host);
  Serial.print(":");
  Serial.print(iot_port, DEC);
  Serial.println("...");

  if (client.connect(iot_host, iot_port)) {
    wdt_reset();

    Serial.print("Connected to ");
    Serial.print(iot_host);
    Serial.print(":");
    Serial.print(iot_port, DEC);
    Serial.println(".");

    wdt_reset();

    client_reconnect_counter = 0;
  
    return true;
  }
  
  wdt_reset();

  client_reconnect_counter++;

  if (client_reconnect_counter >= client_reconnect_max_attempts) {
    force_reset_ethernet = true;
  }

  return false;
}

void reset_client() {
  wdt_reset();
  
  client.stop();

  while (client.available()) {
    wdt_reset();
    
    client.read();
  }

  wdt_reset();
}

int parse_command_id() {
  wdt_reset();
  
  char command_char = client.read();

  wdt_reset();

  if (command_char != 'C') {
    Serial.println("Invalid code received");
    return 0;
  }

  wdt_reset();

  char digit1 = client.read();

  wdt_reset();

  char digit2 = client.read();

  wdt_reset();

  char digit3 = client.read();

  wdt_reset();

  char digit4 = client.read();

  if ((digit1 < 48) || (digit1 > 57) || (digit2 < 48) || (digit2 > 57) || (digit3 < 48) || (digit3 > 57) || (digit4 < 48) || (digit4 > 57)) {
    Serial.println("Invalid command id received");
    return 0;
  }

  return ((digit1 - 48) * 1000) + ((digit2 - 48) * 100) + ((digit3 - 48) * 10) + (digit4 - 48);
}

void write_command_id(int command_id) {
  wdt_reset();

  char command_prefix[6];
  sprintf(command_prefix, "C%04d", command_id);
  client.print(command_prefix);
  
  wdt_reset();
}

void parse_get_client_id_command(int command_id) {
  wdt_reset();
  
  char mac_address_str[17];
  sprintf(mac_address_str, "%02x:%02x:%02x:%02x:%02x:%02x", mac_address[0], mac_address[1], mac_address[2], mac_address[3], mac_address[4], mac_address[5]);

  wdt_reset();

  Serial.print("Sending client id ");
  Serial.println(mac_address_str);

  wdt_reset();

  write_command_id(command_id);
  client.println(mac_address_str);

  wdt_reset();
}

void parse_pin_mode_command() {
  wdt_reset();

  char digit1 = client.read();
  
  wdt_reset();
  
  char digit2 = client.read();
  
  wdt_reset();
  
  char mode = client.read();
  
  wdt_reset();

  if ((digit1 < 48) || (digit1 > 57) || (digit2 < 48) || (digit2 > 57)) {
    Serial.println("Invalid pin received");
    return;
  }
  
  int pin = ((digit1 - 48) * 10) + (digit2 - 48);

  switch (mode) {
    case 'P': {
      pinMode(pin, INPUT_PULLUP);

      Serial.print("Pin ");
      Serial.print(pin);
      Serial.println(" set to INPUT_PULLUP.");

      return;
    }
    
    case 'I': {
      pinMode(pin, INPUT);

      Serial.print("Pin ");
      Serial.print(pin);
      Serial.println(" set to INPUT.");

      return;
    }

    case 'O': {
      pinMode(pin, OUTPUT);

      Serial.print("Pin ");
      Serial.print(pin);
      Serial.println(" set to OUTPUT.");

      return;
    }

    default: {
      Serial.print("Invalid pin mode: ");
      Serial.println(mode);

      return;
    }
  }

  wdt_reset();
}

void parse_digital_read_command(int command_id) {
  wdt_reset();

  char digit1 = client.read();
  
  wdt_reset();
  
  char digit2 = client.read();
  
  wdt_reset();

  if ((digit1 < 48) || (digit1 > 57) || (digit2 < 48) || (digit2 > 57)) {
    Serial.println("Invalid pin received");
    return;
  }
  
  int pin = ((digit1 - 48) * 10) + (digit2 - 48);
  int val = digitalRead(pin);

  wdt_reset();

  Serial.print("Digital pin ");
  Serial.print(pin);
  Serial.print(" value: ");
  Serial.println(val);
  
  wdt_reset();

  write_command_id(command_id);
  client.println(val);

  wdt_reset();
}

void parse_digital_write_command() {
  wdt_reset();

  char digit1 = client.read();
  
  wdt_reset();
  
  char digit2 = client.read();
  
  wdt_reset();

  char value = client.read();

  wdt_reset();

  if ((digit1 < 48) || (digit1 > 57) || (digit2 < 48) || (digit2 > 57)) {
    Serial.println("Invalid pin received");
    return;
  }

  if ((value != '0') && (value != '1')) {
    Serial.println("Invalid digital write value received.");
    return;
  }
  
  int pin = ((digit1 - 48) * 10) + (digit2 - 48);

  if (value == '0') {
    digitalWrite(pin, 0);
  } else {
    digitalWrite(pin, 1);
  }

  wdt_reset();

  Serial.print("Digital pin ");
  Serial.print(pin);
  Serial.print(" set to: ");
  Serial.println(value);

  wdt_reset();
}

void parse_analog_read_command(int command_id) {
  wdt_reset();

  char digit1 = client.read();
  
  wdt_reset();
  
  char digit2 = client.read();
  
  wdt_reset();

  if ((digit1 < 48) || (digit1 > 57) || (digit2 < 48) || (digit2 > 57)) {
    Serial.println("Invalid pin received");
    return;
  }
  
  int pin = ((digit1 - 48) * 10) + (digit2 - 48);
  int val = analogRead(pin);

  wdt_reset();

  Serial.print("Analog pin ");
  Serial.print(pin);
  Serial.print(" value: ");
  Serial.println(val);
  
  wdt_reset();

  write_command_id(command_id);
  client.println(val);

  wdt_reset();
}

void parse_noop_command(int command_id) {
  wdt_reset();

  Serial.println("noop");
  
  wdt_reset();

  write_command_id(command_id);
  client.println("N");

  wdt_reset();
}

void parse_command() {
  wdt_reset();

  int command_id = parse_command_id();
  
  wdt_reset();

  if (command_id == 0) {
    read_to_newline();
    return;
  }
  
  wdt_reset();
  
  char command = client.read();
  
  wdt_reset();

  switch(command) {
    case 'I':
      parse_get_client_id_command(command_id);
      break;
      
    case 'M':
      parse_pin_mode_command();
      break;

    case 'R':
      parse_digital_read_command(command_id);
      break;

    case 'W':
      parse_digital_write_command();
      break;

    case 'A':
      parse_analog_read_command(command_id);
      break;

    case 'N':
      parse_noop_command(command_id);
      break;

    default:
      Serial.print("Invalid command: ");
      Serial.println(command);

      break;
  }

  read_to_newline();
  return;
}

void read_to_newline() {
  while(client.available()) {
    wdt_reset();
    
    char val = client.read();

    if (val == '\n') {
      break;
    }
  }

  wdt_reset();
}

bool check_connections() {
  if ((force_reset_ethernet) || (Ethernet.linkStatus() != LinkON)) {
    if (!setup_ethernet()) {
      return false;
    }
  }

  if (!maintain_ethernet()) {
    return false;
  }

  if (!client.connected()) {
    if (!setup_client()) {
      return false;
    }
  }

  return true;
}

void loop() {
  wdt_reset();
  
  delay(50);
  
  wdt_reset();
  
  if (!check_connections()) {
    wdt_reset();
    
    delay(1000);
    
    wdt_reset();
    
    return;
  }

  while (client.available()) {
    parse_command();
  }

  wdt_reset();
}
