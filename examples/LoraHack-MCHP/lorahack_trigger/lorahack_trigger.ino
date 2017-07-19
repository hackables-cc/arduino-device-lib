#include <TheThingsNetwork.h>
#include <TheThingsMessage.h>
#include <SoftwareSerial.h>
#include <LowPower.h>
#include <Vcc.h>

// Set your AppEUI and AppKey
const char *appEui = "0000000000000000";
const char *appKey = "00000000000000000000000000000000";

#define loraSerial SoftSerial
#define debugSerial Serial

#define TX_LED 9
#define RX_LED 8
#define RADIO_RST_PIN 5
#define SWITCH_PIN 2

// Replace REPLACE_ME with TTN_FP_EU868 or TTN_FP_US915
#define freqPlan TTN_FP_EU868

// Software serial - This will be used to comunicate with the RN2483 module.
// The Lora Hack - MCHP uses digital pin 3 for RX and pin 4 fot TX.
SoftwareSerial SoftSerial(3, 4); // RX, TX

TheThingsNetwork ttn(loraSerial, debugSerial, freqPlan);

// Variable used to flag the interrupt routine has been executted
bool interrupt_flag;

// For the VCC library
const float VccMin   = 0.0;            // Minimum expected Vcc level, in Volts.
const float VccMax   = 3.3;            // Maximum expected Vcc level, in Volts.
const float VccCorrection = 1.0/1.0;   // Measured Vcc by multimeter divided by reported Vcc

Vcc vcc(VccCorrection);

// Interrupt service routine triggered by the state change of SWITCH_PIN
//
void isr_routine()
{
  interrupt_flag = true;
}

// Function to flash the RX_LED i times
//
void flash_rx(int i)
{
  int x;
  for(x = 0; x < i; x++)
  {
    digitalWrite(RX_LED, HIGH);
    delay(150);
    digitalWrite(RX_LED, LOW);
    delay(150);
  }
}

// Function to flash the TX_LED i times
//
void flash_tx(int i)
{
  int x;
  for(x = 0; x < i; x++)
  {
    digitalWrite(TX_LED, HIGH);
    delay(150);
    digitalWrite(TX_LED, LOW);
    delay(150);
  }
}

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
}

void loop()
{
  debugSerial.println("-- LOOP");
  
  // Put the RN2483(A) to sleep
  ttn.sleep(604800000); // a week in miliseconds

  // Clear the interrupt flag
  interrupt_flag = false;
  
  // Enable interrupt for the switch pin, this will be used to wakeup the uc from sleep.
  attachInterrupt(digitalPinToInterrupt(SWITCH_PIN), isr_routine, CHANGE);

  //Flash 2 before sleep
  flash_rx(2);
  
  // Put the Microcontroller to sleep
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);

  // Sleeping now.
  // ...
  // zzzzzzz....
  
  // Restart everything including the RN2483
  setup();

  // Flash 1 after wake up
  flash_rx(1);

  // Prepare the data to be send
  float sys_voltage_f;
  int sys_voltage_i;
  byte data_to_send[3];

  // Encode trigger information
  if(interrupt_flag)
  {
    data_to_send[0] = 1;
  }
  else
  {
    data_to_send[0] = 0;
  }

  // Read the system voltage
  sys_voltage_f = vcc.Read_Volts();
  // Multiply it by 100
  sys_voltage_f *= 100;
  // Get only the integer part
  sys_voltage_i = int(sys_voltage_f);
  // Encode the voltage in the array to send
  data_to_send[1] = (byte)(sys_voltage_i >> 8);
  data_to_send[2] = (byte)(sys_voltage_i);


  debugSerial.print("Bat Voltage: ");
  debugSerial.println(sys_voltage_i);

  // Now take care of the transmission
  debugSerial.println("-- JOIN");
  ttn.join(appEui, appKey);

  if(ttn.sendBytes(data_to_send, sizeof(data_to_send)) == TTN_SUCCESSFUL_TRANSMISSION)
  {
    // Flash the TX LED 3 times
    flash_tx(3);
  }
  else
  {
    // Keep the LED ON for a long period
    digitalWrite(TX_LED, HIGH);
    delay(1000);
    digitalWrite(TX_LED, LOW);
  }

// Wait some seconds before going to sleep again
// this will avoid sending multiple times if the
// trigger happens repeatedly
  delay(30000); // 30 seconds

}
