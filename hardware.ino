#include <avr/io.h>
#include <util/delay.h>

#define MASTER_TX PD2  // Using PD2 (Pin 2) for Master TX
#define MASTER_RX PD3  // Using PD3 (Pin 3) for Master RX
#define LOCK_PIN PB1   // Using PB1 (Pin 9) for Lock
#define BUZZER_PIN PD5 // Using PD5 (Pin 5) for Buzzer
#define GREEN_LED_PIN PD6 // Using PD6 (Pin 6) for Green LED
#define RED_LED_PIN PD7   // Using PD7 (Pin 7) for Red LED

void setup();
void loop();
void processCommand(char command);
void unlockSolenoid(volatile uint8_t *port, uint8_t pin);
void lockSolenoid(volatile uint8_t *port, uint8_t pin);
void successFeedback(volatile uint8_t *port, uint8_t greenPin, uint8_t redPin);
void failureFeedback(volatile uint8_t *port, uint8_t greenPin, uint8_t redPin);
void Tone(uint8_t pin, uint16_t frequency, uint16_t duration);
void sendChar(char c);
void sendString(const char *str);

void setup() {
  // Set pin modes using DDR registers
  DDRB |= (1 << DDB1); // Set LOCK_PIN as output
  DDRD |= (1 << DDD5) | (1 << DDD6) | (1 << DDD7); // Set BUZZER_PIN, GREEN_LED_PIN, RED_LED_PIN as outputs

  // Configure UART for serial communication
  UBRR0H = 0;
  UBRR0L = 103; // Set baud rate to 9600 for 16 MHz clock
  UCSR0B = (1 << RXEN0) | (1 << TXEN0); // Enable RX and TX
  UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // 8 data bits, no parity, 1 stop bit

  lockSolenoid(&PORTB, LOCK_PIN);
  sendString("Hardware board initialized.\n");
}

void loop() {
  if (UCSR0A & (1 << RXC0)) { // Check if data is available
    char command = UDR0;      // Read data
    processCommand(command);
  }
}

void processCommand(char command) {
  sendString("Received command: ");
  sendChar(command);
  sendString("\n");

  switch (command) {
    case '1':
      unlockSolenoid(&PORTB, LOCK_PIN);
      successFeedback(&PORTD, GREEN_LED_PIN, RED_LED_PIN);
      break;
    case '0':
      lockSolenoid(&PORTB, LOCK_PIN);
      failureFeedback(&PORTD, GREEN_LED_PIN, RED_LED_PIN);
      break;
    default:
      sendString("Invalid command\n");
      break;
  }
}

void unlockSolenoid(volatile uint8_t *port, uint8_t pin) {
  *port |= (1 << pin); // Set pin HIGH
  sendString("Unlocking solenoid (HIGH)\n");
}

void lockSolenoid(volatile uint8_t *port, uint8_t pin) {
  *port &= ~(1 << pin); // Set pin LOW
  sendString("Locking solenoid (LOW)\n");
}

void successFeedback(volatile uint8_t *port, uint8_t greenPin, uint8_t redPin) {
  *port |= (1 << greenPin);   // GREEN_LED_PIN HIGH
  *port &= ~(1 << redPin);    // RED_LED_PIN LOW
  Tone(BUZZER_PIN, 1000, 500);
}

void failureFeedback(volatile uint8_t *port, uint8_t greenPin, uint8_t redPin) {
  *port &= ~(1 << greenPin);  // GREEN_LED_PIN LOW
  *port |= (1 << redPin);     // RED_LED_PIN HIGH
  Tone(BUZZER_PIN, 200, 1000);
}


void Tone(uint8_t pin, uint16_t frequency, uint16_t duration) {
  uint16_t delay = 1000000 / (frequency * 2); // Microseconds per half-wave
  uint16_t cycles = (frequency * duration) / 1000; // Total cycles

  for (uint16_t i = 0; i < cycles; i++) {
    PORTD ^= (1 << PORTD5); // Toggle BUZZER_PIN
    _delay_us(delay);
  }
}

void sendChar(char c) {
  while (!(UCSR0A & (1 << UDRE0))); // Wait until the transmit buffer is empty
  UDR0 = c; // Send the character
}

void sendString(const char *str) {
  while (*str) {
    sendChar(*str++);
  }
}
