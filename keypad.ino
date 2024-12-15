// Define registers for port manipulation
#define PORTB_REG *((volatile uint8_t*)0x25)
#define DDRB_REG *((volatile uint8_t*)0x24)
#define PINB_REG *((volatile uint8_t*)0x23)
#define PORTD_REG *((volatile uint8_t*)0x2B)
#define DDRD_REG *((volatile uint8_t*)0x2A)
#define PIND_REG *((volatile uint8_t*)0x29)

const char rows = 4;
const char cols = 4;

const char keys[rows][cols] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

// Row pins: 9(PB1), 8(PB0), 7(PD7), 6(PD6)
const uint8_t rowPinMasks[rows] = {0x02, 0x01, 0x80, 0x40};
const bool rowPinIsPortB[rows] = {true, true, false, false};

// Column pins: 5(PD5), 4(PD4), 3(PD3), 2(PD2)
const uint8_t colPinMasks[cols] = {0x20, 0x10, 0x08, 0x04};

bool keyWasPressed = false;
char currentKey = '\0';
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

void setup() {
    // Initialize UART registers for 9600 baud
    UBRR0H = 0;
    UBRR0L = 103; // 9600 baud rate for 16 MHz clock
    UCSR0B = (1 << TXEN0) | (1 << RXEN0); // Enable transmitter and receiver
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // 8 data bits, no parity, 1 stop bit

    // Set row pins as inputs with pull-ups
    for (char r = 0; r < rows; r++) {
        if (rowPinIsPortB[r]) {
            DDRB_REG &= ~rowPinMasks[r];  // Input
            PORTB_REG |= rowPinMasks[r];   // Pull-up
        } else {
            DDRD_REG &= ~rowPinMasks[r];  // Input
            PORTD_REG |= rowPinMasks[r];   // Pull-up
        }
    }

    // Set column pins as outputs (high)
    for (char c = 0; c < cols; c++) {
        DDRD_REG |= colPinMasks[c];    // Output
        PORTD_REG |= colPinMasks[c];   // High
    }
}

void loop() {
    char key = getKeyFromKeypad();
    if (key != '\0') {
        // Send key through UART
        uartSend(key);
        delay(10);
    }
}

char getKeyFromKeypad() {
    bool keyIsPressed = false;
    char pressedKey = '\0';

    // Scan the keypad
    for (char c = 0; c < cols; c++) {
        PORTD_REG &= ~colPinMasks[c];  // Set column LOW

        for (char r = 0; r < rows; r++) {
            bool pinState;
            if (rowPinIsPortB[r]) {
                pinState = !(PINB_REG & rowPinMasks[r]);
            } else {
                pinState = !(PIND_REG & rowPinMasks[r]);
            }

            if (pinState) {
                keyIsPressed = true;
                pressedKey = keys[r][c];
            }
        }
        PORTD_REG |= colPinMasks[c];  // Set column HIGH
    }

    // Key state machine
    if (keyIsPressed && !keyWasPressed && (millis() - lastDebounceTime) > debounceDelay) {
        keyWasPressed = true;
        currentKey = pressedKey;
        lastDebounceTime = millis();
    }
    else if (!keyIsPressed && keyWasPressed && (millis() - lastDebounceTime) > debounceDelay) {
        keyWasPressed = false;
        char keyToReturn = currentKey;
        currentKey = '\0';
        lastDebounceTime = millis();
        return keyToReturn;
    }

    return '\0';
}

void uartSend(char data) {
    // Wait for the UART Data Register to be empty
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = data; // Send the data
}
