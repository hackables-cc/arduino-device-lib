#include <TheThingsNetwork.h>
#include <TheThingsMessage.h>
#include <SoftwareSerial.h>

// Set your AppEUI and AppKey
const char *appEui = "0000000000000000";
const char *appKey = "00000000000000000000000000000000";

#define loraSerial SoftSerial
#define debugSerial Serial

#define TX_LED 9
#define RX_LED 8
#define RADIO_RST_PIN 5

// Replace REPLACE_ME with TTN_FP_EU868 or TTN_FP_US915
#define freqPlan TTN_FP_EU868

// Software serial - This will be used to comunicate with the RN2483 module.
// The Lora Hack - MCHP uses digital pin 3 for RX and pin 4 fot TX.
SoftwareSerial SoftSerial(3, 4); // RX, TX

TheThingsNetwork ttn(loraSerial, debugSerial, freqPlan);

devicedata_t data = api_DeviceData_init_default;

void setup()
{
  // LED TX(Yellow)
  pinMode(TX_LED, OUTPUT);
  digitalWrite(TX_LED, LOW);
  
  // LED RX(Red)
  pinMode(RX_LED, OUTPUT);
  digitalWrite(RX_LED, LOW);
  
  // The RN2483(A) has a default baudrate of 57600 but the SofSerial @ 8MHz
  // do not perform as needed so we will use 9600. We will take advantage 
  // of the auto baud functionality of the RN2483(A).
  loraSerial.begin(9600); 
  debugSerial.begin(9600);

  // Wait a maximum of 10s for Serial Monitor
  while (!debugSerial && millis() < 10000)
    ;

  // RN2483(A) Hard Reset
  pinMode(RADIO_RST_PIN, OUTPUT);
  digitalWrite(RADIO_RST_PIN, HIGH);
  delay(1000);
  digitalWrite(RADIO_RST_PIN, LOW);
  delay(1000);

  // The reset will issue an autobaud, so it should
  // always be called after a hard reset and before any operation.
  ttn.reset();

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
    // Flash the TX LED 3 times
    digitalWrite(TX_LED, HIGH);
    delay(250);
    digitalWrite(TX_LED, LOW);
    delay(250);
    digitalWrite(TX_LED, HIGH);
    delay(250);
    digitalWrite(TX_LED, LOW);
    delay(250);
    digitalWrite(TX_LED, HIGH);
    delay(250);
    digitalWrite(TX_LED, LOW);
  }
  else
  {
    // Keep the LED ON for a long period
    digitalWrite(TX_LED, HIGH);
    delay(1000);
    digitalWrite(TX_LED, LOW);
  }

  delay(60000);
}
