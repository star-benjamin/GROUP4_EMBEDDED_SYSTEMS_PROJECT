#define HARDWARE_CONTROL_RX 10
#define HARDWARE_CONTROL_TX 11
#define KEYPAD_CONTROL_RX 12
#define KEYPAD_CONTROL_TX 13

const char *CORRECT_PASSCODE = "1234ABCD";
char enteredPasscode[17];
int passcodeIndex = 0;
bool lastAttemptFailed = false;
int accessCount = 0;

void setup() {
    // UART0 Initialization (for Serial communication)
    UBRR0H = 0;
    UBRR0L = 103; // Baud rate 9600 for 16 MHz
    UCSR0B = (1 << RXEN0) | (1 << TXEN0); // Enable Receiver and Transmitter
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // 8-bit data

    // Initialize I2C (TWI) for LCD communication
    TWBR = 72;  // Set SCL frequency to 100 kHz with 16 MHz CPU

    displayWelcomeScreen();
    uart0Print("Master board initialized.\n");
}

void loop() {
    if (UCSR0A & (1 << RXC0)) { // Check if data is available in UART
        char key = UDR0;
        handleKeyPress(&key);
    }
    _delay_ms(10);
}

void handleKeyPress(char *key) {
    uart0Print("Received key: ");
    uart0Transmit(*key);
    uart0Print("\n");

    if (*key == '*') {  // Clear
        clearPasscode();
    } else if (*key == '#' || *key == '\n') {  // Enter
        processPasscode();
    } else if (passcodeIndex < sizeof(enteredPasscode) - 1) {
        enteredPasscode[passcodeIndex] = *key;
        passcodeIndex++;
        enteredPasscode[passcodeIndex] = '\0';
        lcdSetCursor(passcodeIndex - 1, 1);
        lcdPrintChar('*');
    }
}

void clearPasscode() {
    memset(enteredPasscode, 0, sizeof(enteredPasscode));
    passcodeIndex = 0;
    lcdSetCursor(0, 1);
    lcdPrint("                "); // Clear line
    lcdSetCursor(0, 1);
}

void processPasscode() {
    bool isSuccessful = checkPasscode(enteredPasscode);
    sendCommandToHardware(isSuccessful ? '0' : '1');
    displayAccessResult(&isSuccessful);
    accessCount++;

    _delay_ms(2000);
    displayEnterCodePrompt();
    clearPasscode();
}

void displayWelcomeScreen() {
    lcdClear();
    lcdPrint("Access Control");
    lcdSetCursor(0, 1);
    lcdPrint("System Ready");
    _delay_ms(2000);
    displayEnterCodePrompt();
}

void displayEnterCodePrompt() {
    lcdClear();
    lcdPrint("Enter Passcode:");
    lcdSetCursor(0, 1);
}

void lcdPrintInt(int value) {
    char buffer[10];
    itoa(value, buffer, 10); // Convert integer to string
    lcdPrint(buffer);
}

void displayAccessResult(bool *isSuccessful) {
    lcdClear();
    lcdPrint(*isSuccessful ? "Access Granted" : "Access Denied");
    lcdSetCursor(0, 1);
    lcdPrint("Count: ");
    lcdPrintInt(accessCount);
}

bool checkPasscode(const char *passcode) {
    return strcmp(passcode, CORRECT_PASSCODE) == 0;
}

void sendCommandToHardware(char command) {
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = command;
    uart0Print("Sent command to hardware: ");
    uart0Transmit(command);
    uart0Print("\n");
}

// UART0 Transmit Function
void uart0Transmit(char data) {
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = data;
}

// UART0 Print String
void uart0Print(const char *str) {
    while (*str) {
        uart0Transmit(*str++);
    }
}

// LCD Functions
void lcdSendCommand(uint8_t command) {
    TWCR = (1 << TWSTA) | (1 << TWEN); // Send START condition
    while (!(TWCR & (1 << TWINT)));

    TWDR = 0x4E; // LCD I2C Address
    TWCR = (1 << TWINT) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)));

    TWDR = 0x80; // Control byte (Command mode)
    TWCR = (1 << TWINT) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)));

    TWDR = command;
    TWCR = (1 << TWINT) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)));

    TWCR = (1 << TWSTO) | (1 << TWEN); // Send STOP condition
}

void lcdPrint(const char *str) {
    while (*str) {
        lcdPrintChar(*str++);
    }
}

void lcdPrintChar(char data) {
    TWCR = (1 << TWSTA) | (1 << TWEN); // Send START condition
    while (!(TWCR & (1 << TWINT)));

    TWDR = 0x4E; // LCD I2C Address
    TWCR = (1 << TWINT) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)));

    TWDR = 0x40; // Control byte (Data mode)
    TWCR = (1 << TWINT) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)));

    TWDR = data;
    TWCR = (1 << TWINT) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)));

    TWCR = (1 << TWSTO) | (1 << TWEN); // Send STOP condition
}

void lcdClear() {
    lcdSendCommand(0x01); // Clear display command
    _delay_ms(2);
}

void lcdSetCursor(uint8_t col, uint8_t row) {
    uint8_t addr = (row == 0) ? col | 0x80 : col | 0xC0;
    lcdSendCommand(addr);
}
