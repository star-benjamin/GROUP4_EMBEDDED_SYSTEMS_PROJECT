// Pin definitions for UART
#define MASTER_TX_PIN 10
#define MASTER_RX_PIN 11

#define MAX_LOGS 100
#define EEPROM_START_ADDR 0
#define CURRENT_INDEX_ADDR 0
#define LOGS_START_ADDR 2

void printLogResult(char result) {
  if (result == '1') {
    USART_Transmit_String("SUCCESS\n");
  } else if (result == '0') {
    USART_Transmit_String("FAILURE\n");
  }
}

void EEPROM_Write(uint16_t address, uint8_t data) {
  while (EECR & (1 << EEPE)); // Wait for completion of previous write
  EEAR = address;             // Set EEPROM address
  EEDR = data;                // Set data to write
  EECR = (1 << EEMPE);        // Enable master write
  EECR |= (1 << EEPE);        // Start EEPROM write
}

uint8_t EEPROM_Read(uint16_t address) {
  while (EECR & (1 << EEPE)); // Wait for completion of previous write
  EEAR = address;             // Set EEPROM address
  EECR |= (1 << EERE);        // Start EEPROM read
  return EEDR;                // Return data
}

void setup() {
  USART_Init(9600);
  
  if (EEPROM_Read(0) == 0xFF) {
    USART_Transmit_String("EEPROM not initialized, initializing now...\n");
    EEPROM_Write(CURRENT_INDEX_ADDR, 0);
    EEPROM_Write(CURRENT_INDEX_ADDR + 1, 0);
    for (uint16_t i = LOGS_START_ADDR; i < LOGS_START_ADDR + MAX_LOGS; i++) {
      EEPROM_Write(i, 0xFF);
    }
  } else {
    USART_Transmit_String("EEPROM already initialized.\n");
  }

  USART_Transmit_String("Logs board initialized.\n");
  printAllLogs();
}

void loop() {
  if (USART_DataAvailable()) {
    char result = USART_Receive();
    USART_Transmit_String("Received from master: ");
    USART_Transmit(result);
    USART_Transmit('\n');
    processLog(result);
  } else {
    USART_Transmit_String("No data received from master.\n");
  }
  _delay_ms(1000); // Optional: Delay for readability
}

void processLog(char result) {
  uint16_t currentIndex = getCurrentIndex();
  EEPROM_Write(LOGS_START_ADDR + currentIndex, result);
  currentIndex = (currentIndex + 1) % MAX_LOGS;
  updateCurrentIndex(currentIndex);

  USART_Transmit_String("Access attempt #");
  USART_Transmit_Number(currentIndex);
  USART_Transmit_String(": ");
  printLogResult(result);
}

uint16_t getCurrentIndex() {
  uint16_t index = EEPROM_Read(CURRENT_INDEX_ADDR);
  index |= (EEPROM_Read(CURRENT_INDEX_ADDR + 1) << 8);
  return index;
}

void updateCurrentIndex(uint16_t index) {
  EEPROM_Write(CURRENT_INDEX_ADDR, index & 0xFF);
  EEPROM_Write(CURRENT_INDEX_ADDR + 1, (index >> 8) & 0xFF);
}

void printAllLogs() {
  USART_Transmit_String("\n=== Access History ===\n");

  bool hasLogs = false;
  for (uint16_t i = 0; i < MAX_LOGS; i++) {
    char logValue = EEPROM_Read(LOGS_START_ADDR + i);
    if (logValue == '1' || logValue == '0') {
      hasLogs = true;
      USART_Transmit_String("Log #");
      USART_Transmit_Number(i);
      USART_Transmit_String(": ");
      printLogResult(logValue);
    }
  }

  if (!hasLogs) {
    USART_Transmit_String("No access attempts recorded yet.\n");
  }

  USART_Transmit_String("=== End of History ===\n\n");
}

void USART_Init(unsigned int baud) {
  unsigned int ubrr = F_CPU / 16 / baud - 1;
  UBRR0H = (unsigned char)(ubrr >> 8);
  UBRR0L = (unsigned char)ubrr;
  UCSR0B = (1 << RXEN0) | (1 << TXEN0);
  UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void USART_Transmit(unsigned char data) {
  while (!(UCSR0A & (1 << UDRE0)));
  UDR0 = data;
}

void USART_Transmit_String(const char *str) {
  while (*str) {
    USART_Transmit(*str++);
  }
}

unsigned char USART_Receive(void) {
  while (!(UCSR0A & (1 << RXC0)));
  return UDR0;
}

bool USART_DataAvailable() {
  return (UCSR0A & (1 << RXC0));
}

void USART_Transmit_Number(uint16_t number) {
  char buffer[10];
  itoa(number, buffer, 10);
  USART_Transmit_String(buffer);
}

