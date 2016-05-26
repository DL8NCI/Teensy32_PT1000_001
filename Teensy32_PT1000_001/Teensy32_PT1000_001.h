// Only modify this file to include
// - function definitions (prototypes)
// - include files
// - extern variable definitions
// In the appropriate section

#ifndef _TEENSY32_PT1000_001_H_
#define _TEENSY32_PT1000_001_H_
#include "Arduino.h"
//add your includes for the project PT1000_5 here
#include "Stream.h"
#include <EEPROM.h>
#include <avr/pgmspace.h>
#include <Tesp8266.h>
#include <TCmdInterpreter.h>
//end of add your includes here


//add your function definitions for the project PT1000_5 here
#define SENSOR A7
#define LED 9

#define UDP 1
#define TCPSERVER 2

#define DEBUG true



//Serial1 lox2(19,18); // RX, TX
Tesp8266 esp8266(&Serial1,&Serial);

int mode = TCPSERVER;

#define MAXTOKENS 4
String cmdLine = "";
String token[MAXTOKENS];
//String lox2Record;

struct tRecord {
  char ssid[33]; // nullterminiert
  char pw[64];   // nullterminiert
  };

#define MAXRECORDS 4

struct tRecords {
  byte n;
  tRecord record[MAXRECORDS];
  unsigned long crc;
  } config;


bool processRequest(void);
float PT1000(void);
boolean esp_wlan_connect(void);
boolean esp_UDP_config();
boolean esp_TCPServer_config(void);
void sendStringTCP(int connectionID, String html);
boolean sendUDP(String Msg);
boolean sendCom(String command, char respond[]);
String sendCom(String command);
void debug(String Msg);
void debugln(String Msg);
void debugln(void);
void debug(char c);
void command(void);
void cmdHelp(boolean unknownCommand);
boolean cmdSet(String &message);
boolean stripToken(String &token, int lMin);
void cmdClear(void);
void cmdWrite(void);
void cmdRead(void);
void cmdReboot(void);
void readEEPROM(void);
void cmdList(void);
void prompt(void);
int splitCmdLine(void);
boolean getNextToken(String &s, String &token);
unsigned long recordCrc(struct tRecords *pr);
//Do not add code below this line
#endif /* _TEENSY32_PT1000_001_H_ */
