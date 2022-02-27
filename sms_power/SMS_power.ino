// Set serial for debug console (to the Serial Monitor, default speed 115200)
#define SerialMon Serial

// Set serial for AT commands (to the module)
// Use Hardware Serial on Mega, Leonardo, Micro
#define SerialAT Serial1

#define TINY_GSM_MODEM_SIM7000
#define TINY_GSM_RX_BUFFER 1024 // Set RX buffer to 1Kb


// See all AT commands, if wanted
#define DUMP_AT_COMMANDS

// set GSM PIN, if any
#define GSM_PIN "2050"

// Your GPRS credentials, if any
const char apn[]  = "internet.telenor.se";     //SET TO YOUR APN
const char gprsUser[] = "";
const char gprsPass[] = "";

#include <TinyGsmClient.h>
#include <SPI.h>
#include <SD.h>
//#include <Ticker.h>
#include "at_parsing.h"
#include "dht_reader.h"
#include "secrets.h"

TinyGsm modem(SerialAT);

#define CONFIG_ARDUINO_LOOP_STACK_SIZE 8192

#define uS_TO_S_FACTOR      1000000ULL  // Conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP       60         // Time ESP32 will go to sleep (in seconds)

#define UART_BAUD           9600
#define PIN_DTR             25
#define PIN_TX              27
#define PIN_RX              26
#define PWR_PIN             4

#define SD_MISO             2
#define SD_MOSI             15
#define SD_SCLK             14
#define SD_CS               13
#define LED_PIN             12

#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  5        /* Time ESP32 will go to sleep (in seconds) */

//RTC_DATA_ATTR int bootCount = 0;

esp_sleep_wakeup_cause_t wakeup_reason;


static void List(String state = "")
{

    SerialMon.println(F("**********************************"));
    SerialMon.println(F("*********STACK*****************"));
    SerialMon.println(state);
    SerialMon.println(uxTaskGetStackHighWaterMark(NULL));
    SerialMon.println(F("**********************************"));
}

/*
void enableGPS(void)
{
  // Set SIM7000G GPIO4 LOW ,turn on GPS power
  // CMD:AT+SGPIO=0,4,1,1
  // Only in version 20200415 is there a function to control GPS power
  modem.sendAT("+SGPIO=0,4,1,1");
  if (modem.waitResponse(10000L) != 1) {
    DBG(" SGPIO=0,4,1,1 false ");
  }
}

void disableGPS(void)
{
  // Set SIM7000G GPIO4 LOW ,turn off GPS power
  // CMD:AT+SGPIO=0,4,1,0
  // Only in version 20200415 is there a function to control GPS power
  modem.sendAT("+SGPIO=0,4,1,0");
  if (modem.waitResponse(10000L) != 1) {
    DBG(" SGPIO=0,4,1,0 false ");
  }
}
*/

void modemPowerOn()
{
  pinMode(PWR_PIN, OUTPUT);
  digitalWrite(PWR_PIN, LOW);
  
  SerialAT.begin(UART_BAUD, SERIAL_8N1, PIN_RX, PIN_TX);
  
  delay(1000);    //Datasheet Ton mintues = 1S
  digitalWrite(PWR_PIN, HIGH);
}

void modemPowerOff()
{
  pinMode(PWR_PIN, OUTPUT);
  digitalWrite(PWR_PIN, LOW);
  SerialAT.end();
  delay(1500);    //Datasheet Ton mintues = 1.2S
  digitalWrite(PWR_PIN, HIGH);
}


void modemRestart()
{
  modemPowerOff();
  delay(1000);
  modemPowerOn();
}


void setup()
{

  // Set console baud rate
  SerialMon.begin(115200);
  delay(1000);



  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : SerialMon.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : SerialMon.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : SerialMon.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : SerialMon.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : SerialMon.println("Wakeup caused by ULP program"); break;
    default : SerialMon.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason); break;
  }

  delay(10);
  
  dht_setup();

}

void init_SD() {


  SerialMon.println("========SDCard Detect.======");
  SPI.begin(SD_SCLK, SD_MISO, SD_MOSI);
  if (!SD.begin(SD_CS)) {
    SerialMon.println("SDCard MOUNT FAIL");
  } else {
    uint32_t cardSize = SD.cardSize() / (1024 * 1024);
    String str = "SDCard Size: " + String(cardSize) + "MB";
    SerialMon.println(str);
  }
  SerialMon.println("===========================");

}


void start_modem() {
  
  String res; 
  SerialMon.println("========SIMCOMATI======");
  modem.sendAT("+SIMCOMATI");
  modem.waitResponse(1000L, res);
  res.replace(GSM_NL "OK" GSM_NL, "");
  SerialMon.println(res);
  res = "";
  SerialMon.println("=======================");

  SerialMon.println("=====Preferred mode selection=====");
  modem.sendAT("+CNMP?");
  if (modem.waitResponse(1000L, res) == 1) {
    res.replace(GSM_NL "OK" GSM_NL, "");
    SerialMon.println(res);
  }
  res = "";
  SerialMon.println("=======================");
  SerialMon.println("=====Preferred selection between CAT-M and NB-IoT=====");
  modem.sendAT("+CMNB?");
  if (modem.waitResponse(1000L, res) == 1) {
    res.replace(GSM_NL "OK" GSM_NL, "");
    SerialMon.println(res);
  }
  res = "";
  SerialMon.println("=======================");


  String name = modem.getModemName();
  SerialMon.println("Modem Name: " + name);

  String modemInfo = modem.getModemInfo();
  SerialMon.println("Modem Info: " + modemInfo);

  // Unlock your SIM card with a PIN if needed
  if ( GSM_PIN && modem.getSimStatus() != 3 ) {
    modem.simUnlock(GSM_PIN);
  }


  for (int i = 0; i <= 4; i++) {
    uint8_t network[] = {
      2,  //Automatic
      13, //GSM only
      38, //LTE only
      51  //GSM and LTE only
    };
    SerialMon.printf("Try %d method\n", network[i]);
    modem.setNetworkMode(network[i]);
    delay(3000);
    bool isConnected = false;
    int tryCount = 60;
    while (tryCount--) {
      int16_t signal =  modem.getSignalQuality();
      SerialMon.print("Signal: ");
      SerialMon.print(signal);
      SerialMon.print(" ");
      SerialMon.print("isNetworkConnected: ");
      isConnected = modem.isNetworkConnected();
      SerialMon.println( isConnected ? "CONNECT" : "NO CONNECT");
      if (isConnected) {
        break;
      }
      delay(1000);
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    }
    if (isConnected) {
      break;
    }
  }
  digitalWrite(LED_PIN, HIGH);

  SerialMon.println();
  SerialMon.println("Device is connected .");
  SerialMon.println();

  SerialMon.println("=====Inquiring UE system information=====");
  modem.sendAT("+CPSI?");
  if (modem.waitResponse(1000L, res) == 1) {
    res.replace(GSM_NL "OK" GSM_NL, "");
    SerialMon.println(res);
  }

  SerialMon.println("**********************************************************");
  SerialMon.println("Setting textmode  ");
  SerialMon.println("**********************************************************\n\n");

  int serincoming;
  res = "";
  modem.sendAT("+CMGF=1");
  if (modem.waitResponse(10000L, res) == 1) {
    SerialMon.println("Textmode set");
    SerialMon.println(res);
  } else {
    SerialMon.println("Textmode not set, no valid response.");
    SerialMon.println(res);
  }

}


void tests() {
  SerialMon.println();
  
  SerialMon.println("Test secrets:");
  SerialMon.println(wifi_name);
  SerialMon.println("Test DHT:");
  SerialMon.println(dht_message());  
}

/*
 * React to any incoming data. Return true if we had a stay awake-instruction.
 */

bool react(ATParser parser) {
  
  String currfrom = "";
  String currmsg = "";
  String res = "";
  
  bool result = false;
  for (int i = 0; i < parser.messages.size(); i++) {
    currfrom = parser.messages[i].phone_number;
    currmsg = parser.messages[i].message;
    
    SerialMon.println("--- Message parsed ---");
    SerialMon.print("From: ");
    SerialMon.println(currfrom);
    SerialMon.print("When received: ");
    SerialMon.println(parser.messages[i].message_datetime);
    SerialMon.print("Message: ");
    SerialMon.println(currmsg);
    if (currmsg.substring(0, 6) == "getenv")
    {
      SerialMon.print("Sending voltage..");
      res = "";
      String mess = dht_message();
      SerialMon.println("dht_message data:");
      SerialMon.println(mess);
      
      res = modem.sendSMS(currfrom, mess);
      SerialMon.print("SMS response");
      SerialMon.println(res);
    
    }
    if (currmsg.substring(0, 9) == "stayawake")
    {
      SerialMon.print("Received orders to stay awake..");
      res = "";
  
      res = modem.sendSMS(currfrom, "Will stay awake");
      // No use awaiting result.
      result = true;
    
    }  
    if (currmsg.substring(0, 5) == "sleep")
    {
      SerialMon.print("Received orders to go to sleep..");
      res = "";
  
      res = modem.sendSMS(currfrom, "Going to sleep..zzzz..");
      // No use awaiting result.
      
      esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
      SerialMon.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) + " Seconds");
      delay(2000);
      SerialMon.println("Going to sleep now");
      SerialMon.flush(); 
      digitalWrite(LED_PIN, HIGH);
      esp_deep_sleep_start();
    
    }      
         
  }
  parser.messages.clear();
  return result;

}


void monitorIncoming() {

  List("In monitor incoming");  
  String res = "";
  List("In monitor incoming");   
  SerialMon.println("--- Setdfgldasdfasdfasdfasdfasd ---");
  delay(10000);
  SerialMon.println("DAFUQ");
//  modem.sendAT("+CNMI=2,2,0,0,0");
//  if (modem.waitResponse(10000L, res) == 1) {
//    SerialMon.println("--- CNMI - Set direct routing to the TE using CMTs---");
//    SerialMon.println(res);
//  } else {
//    SerialMon.println("CNMI not set, no valid response.");
//    SerialMon.println(res);
//  }
//
//
//  res = "";
  
  
  // Init Parser
  ATParser parser(modem);

  SerialMon.print("Monitoring for SMS and other events..");
  
  while (1) {
//    
//      while (SerialAT.available()) {
//        SerialMon.write(SerialAT.read());
//      }
//      while (SerialMon.available()) {
//        serincoming = SerialMon.read();
//
//        SerialAT.write(serincoming);;
//        SerialMon.write(serincoming);
//      }
//    
//    

    //modem.sendAT("+CMGD=0,3");
    delay(100);
    if (SerialAT.available()) {
      delay(100);
      while (SerialAT.available()) {
        res += static_cast<char>(SerialAT.read());
      }
      SerialMon.println("--- Data received ---");
      SerialMon.println(res);
      parser.parse_at(res);
      react(parser);

      res = "";
    }
  }  
}


void check_send_sleep() {
  String res = "";  

  // Init Parser
  ATParser parser(modem);
  
  // Check for unread messages
  modem.sendAT("+CMGL=\"REC UNREAD\""); // AT+CMGL="REC UNREAD"

  if (modem.waitResponse(10000L, res) == 1) {
    SerialMon.println("We have data");
    SerialMon.println(res);
    parser.parse_at(res);
    if (react(parser) == true) {
      monitorIncoming();
    }
  
  } 
  
  SerialMon.println("No stayawake orders; going back to sleep.");
  SerialMon.println(res);
  

  while (1) {
  
  }
}



void loop()
{



  /*
    SerialMon.println("========INIT========");

    if (!modem.init()) {
      modemRestart();
      delay(2000);
      SerialMon.println("Failed to restart modem, attempting to continue without restarting");
      return;
    }
  */

  List();
  // Set LED OFF
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  modemPowerOn();

  //init_SD();
  
  start_modem();

  tests();
  List("After tests");

  check_send_sleep();
  monitorIncoming();
  
  while (1) {
    SerialMon.println(F("Moving"));
    List();
    delay(1000);
  }
  

}
