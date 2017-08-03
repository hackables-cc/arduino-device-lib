#include <Arduino.h>        // required before wiring_private.h
#include "wiring_private.h" // pinPeripheral() function
#include <TheThingsNetwork.h>
#include <TheThingsMessage.h>

// Set your AppEUI and AppKey
const char *appEui = "0000000000000000";
const char *appKey = "00000000000000000000000000000000";

// Replace REPLACE_ME with TTN_FP_EU868 or TTN_FP_US915
#define freqPlan TTN_FP_EU868

// Pins Mapping for the IOT Immagination Board
// Radio
#define RADIO_RST_PIN 8
// Mikro Bus 1
#define MB1_AN  A0
#define MB1_RST A2
#define MB1_CS  A4
#define MB1_PWM 3
#define MB1_INT 6
// Mikro Bus 2
#define MB2_AN  A1
#define MB2_RST A3
#define MB2_CS  A5
#define MB2_PWM 4
#define MB2_INT 7

// Serial ports
// The serail ports in the IOT Imag. board are 
// distributed in the following manner:
// - Serial  . Used for debug (redirected to USB serial).
// - Serial1 . Used for MikroBus 1 Serial
// - Serial2 . Used for MikroBus 2 Serial
// - Serial3 . Used to communicate with Lora radio RN2483A

// This will redirect the serial prints to the USB.
#if defined(ARDUINO_SAMD_ZERO) && defined(SERIAL_PORT_USBVIRTUAL)
  // Required for Serial on Zero based boards
  #define Serial SERIAL_PORT_USBVIRTUAL
#endif

#define loraSerial Serial3
#define debugSerial Serial

// Serial ports
// Serial3 pin and pad definitions (in Arduino files Variant.h & Variant.cpp)
#define PIN_SERIAL3_RX       (5ul)                // Pin description number for PIO_SERCOM on D5
#define PIN_SERIAL3_TX       (2ul)                // Pin description number for PIO_SERCOM on D4
#define PAD_SERIAL3_TX       (UART_TX_PAD_2)      // SERCOM pad 2 TX
#define PAD_SERIAL3_RX       (SERCOM_RX_PAD_3)    // SERCOM pad 3 RX

#define PIN_SERIAL2_RX       (13ul)               // Pin description number for PIO_SERCOM on D5
#define PIN_SERIAL2_TX       (11ul)               // Pin description number for PIO_SERCOM on D4
#define PAD_SERIAL2_TX       (UART_TX_PAD_0)      // SERCOM pad 2 TX
#define PAD_SERIAL2_RX       (SERCOM_RX_PAD_1)    // SERCOM pad 3 RX

// Serial1 already setup by default on Arduino Zero
Uart Serial2( &sercom1, PIN_SERIAL2_RX, PIN_SERIAL2_TX, PAD_SERIAL2_RX, PAD_SERIAL2_TX );
Uart Serial3( &sercom2, PIN_SERIAL3_RX, PIN_SERIAL3_TX, PAD_SERIAL3_RX, PAD_SERIAL3_TX );

void SERCOM1_Handler()
{
  Serial2.IrqHandler();
}

void SERCOM2_Handler()
{
  Serial3.IrqHandler();
}

TheThingsNetwork ttn(loraSerial, debugSerial, freqPlan);

devicedata_t data = api_DeviceData_init_default;

void setup()
{
  loraSerial.begin(57600); 
  debugSerial.begin(9600);

  // Assign pins 11 & 13 SERCOM functionality on Serial2
  pinPeripheral(11, PIO_SERCOM);
  pinPeripheral(13, PIO_SERCOM);
  // Assign pins 2 & 5 SERCOM functionality on Serial3
  pinPeripheral(2, PIO_SERCOM);
  pinPeripheral(5, PIO_SERCOM);

  // Wait a maximum of 10s for Serial Monitor
  while (!debugSerial && millis() < 10000)
    ;

  debugSerial.println("Staring Hard Reset to RN2483");
  
  // RN2483(A) Hard Reset
  pinMode(RADIO_RST_PIN, OUTPUT);
  digitalWrite(RADIO_RST_PIN, LOW);
  delay(1000);
  digitalWrite(RADIO_RST_PIN, HIGH);
  delay(1000);

  debugSerial.println("Done Hard Reset");
  
  debugSerial.println("-- STATUS");
  ttn.showStatus();

  debugSerial.println("-- JOIN");
  ttn.join(appEui, appKey);

  // Select what fields to include in the encoded message
  data.has_motion = true;
  data.has_water = false;
  data.has_temperature_celcius = true;
  data.has_temperature_fahrenheit = true;
  data.has_humidity = true;
}

void loop()
{
  debugSerial.println("-- LOOP");

  // Read the sensors
  data.motion = true;
  data.water = 682;
  data.temperature_celcius = 30;
  data.temperature_fahrenheit = 86;
  data.humidity = 97;

  // Encode the selected fields of the struct as bytes
  byte *buffer;
  size_t size;
  TheThingsMessage::encodeDeviceData(&data, &buffer, &size);

  if(ttn.sendBytes(buffer, size) == TTN_SUCCESSFUL_TRANSMISSION)
  {
    // Do something if succedded

  }
  else
  {
    // Do something if Not succedded

  }

  delay(60000);
}
