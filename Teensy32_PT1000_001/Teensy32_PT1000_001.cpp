// Do not remove the include below
#include "Teensy32_PT1000_001.h"


//The setup function is called once at startup of the sketch

#define CPU_RESTART_ADDR (uint32_t *)0xE000ED0C
#define CPU_RESTART_VAL 0x5FA0004
#define CPU_RESTART (*CPU_RESTART_ADDR = CPU_RESTART_VAL);

void setup()
{
	  pinMode(SENSOR, INPUT);
	  analogReference(DEFAULT);

	  Serial.begin(115200);
	  Serial.println(F("Serielle Schnittstellen gestartet"));

	  Serial1.begin(115200);
	  Serial1.setTimeout(5000);

	  Serial.println(F("Begin read eeprom"));
	  readEEPROM();

	  Serial.println(F("Begin wlan connect"));
	  esp_wlan_connect();

	  switch (mode) {
	    case UDP:
	      esp_UDP_config();
	      break;
	    case TCPSERVER:
	      esp_TCPServer_config();
	      break;
	  }

	  Serial1.setTimeout(1000);

	//  lox2Record = String("missing");
	//  lox2.begin(9600);
	//  lox2.setTimeout(3000);

	  Serial.println(F("Begin prompt"));
	  prompt();
}

// The loop function is called in an endless loop
void loop()
{
	  int connectionID;
	  String html;

	  if (Serial1.available()) {
	    if (Serial1.find("+IPD,")) {
	      switch (mode) {
	        case UDP:
	          if (Serial1.find("PT1000")) {
	            sendUDP("PT1000=" + String(PT1000()));
	          }
	          else {
	            debug(F("Wrong UDP Command"));
	            sendUDP(F("Wrong UDP Command"));
	            break;
	          case TCPSERVER:
	            processRequest();
	            break;
	          }
	      }
	    }
	  }

	//  if (lox2.available()) {
	//    lox2Record = lox2.readStringUntil('\r');
	//    }

	  command();
}



bool processRequest(void) {
  String path;
  String html;
  int connectionID;

  connectionID=Serial1.parseInt();
  Serial.println("connection-ID = " + String(connectionID));
  if (Serial1.find(",")) {
    unsigned int len = Serial1.parseInt();
    if (Serial1.find(":")) {
      if (Serial1.find("GET /")) {
        path = Serial1.readStringUntil('\r');
        debug("path=<"+path+">");
        if (path.endsWith(F(" HTTP/1.1"))) {
          path = path.substring(0,path.length()-9);
          debug("path=<"+path+">");
          if (path.equals("")) html = F("<h1>Hello World!</h1>");
          else if (path.equals(F("?PT1000"))) {
            html = F("<h1>Hello World!</h1>T = ");
            html = html + String(PT1000()) + " deg";
//            html = html + F("<br>LOX-2 : ") + lox2Record;
            }
          else if (path.equals(F("favicon.ico"))) html = F("HTTP/1.1 404 Not Found\r");
          else html = F("<h1>Hello World!</h1>Unzulaessige Anfrage");
          if (html!="") sendStringTCP(connectionID, html);
          }
        }
      }
    }
  }


//------------------------------------------------------------------------------
float PT1000(void) {
//------------------------------------------------------------------------------
  float r, t;
  int cnt;

  cnt = analogRead(SENSOR);
  r = (cnt * 5000.0) / (1024 - cnt);    // 5000: 5 kOhm zwischen ca. 5V und PT1000; 1024: 10 bit
  r = r / 1000.0 - 1;                   // 1000: Nominalwert des PT1000 - also 1000 Ohm
  t = (r * (255.7 + r * 10.62));        // quadratische Näherung
  debug("T = " + String(t) + " deg");
  return t;
}



//-----------------------------------------Config Serial1------------------------------------

//------------------------------------------------------------------------------
boolean esp_wlan_connect(void) {
//------------------------------------------------------------------------------
  boolean success;

  esp8266.cmdRST();
  esp8266.cmdATE(false);
  esp8266.qryGMR();
  debug(esp8266.qryCIFSR());

  for(byte i=0; i<MAXRECORDS; i++) {
    success = esp8266.cmdCWMODE_CUR('1'); // 1=station mode
//    debug(Serial1.qryCIPSTAMAC_CUR());
    success &= esp8266.cmdCWJAP_CUR(String(config.record[i].ssid),String(config.record[i].pw));
    if (success) {
      debug(F("WLAN Connected - My IP/MAC is:"));
      debug(esp8266.qryCIFSR());
      break;
      }
//    Serial1.cmdCWQAP();
    }

  success &= esp8266.cmdCIPMODE('0');//  sendCom(F("AT+CIPMODE=0"), "OK");
  success &= esp8266.cmdCIPMUX('0'); //sendCom(F("AT+CIPMUX=0"), "OK");
  return success;
}

//------------------------------------------------------------------------------
boolean esp_UDP_config() {
//------------------------------------------------------------------------------
  boolean success;
  success &= sendCom(F("AT+CIPSTART=\"UDP\",\"0\",5000,5001,2"), "OK"); //Importand Boradcast...Reconnect IP
  if (success) debug(F("UDP ready"));
  return success;
}

//------------------------------------------------------------------------------
boolean esp_TCPServer_config(void) {
//------------------------------------------------------------------------------
  boolean success;
  success = (sendCom(F("AT+CIPMUX=1"), "OK"));
  success &= (sendCom(F("AT+CIPSERVER=1,80"), "OK"));
  if (success) debug(F("TCP-Server ready"));
  return success;
}

//------------------------------------------------------------------------------
void sendStringTCP(int connectionID, String html) {
//------------------------------------------------------------------------------
  if (sendCom("AT+CIPSEND=" + String(connectionID) + "," + String(html.length()), ">")) {
    sendCom(html, "SEND OK");
    sendCom("AT+CIPCLOSE=" + String(connectionID), "OK");
    debug(F("Send and Close"));
  }
}



//-----------------------------------------------Controll ESP-----------------------------------------------------

//------------------------------------------------------------------------------
boolean sendUDP(String Msg) {
//------------------------------------------------------------------------------
  boolean success;
  success = sendCom("AT+CIPSEND=" + String(Msg.length() + 2), ">");    //+",\"192.168.4.2\",90", ">");
  if (success) {
    success &= sendCom(Msg, "OK");
  }
  return success;
}

//------------------------------------------------------------------------------
boolean sendCom(String command, char respond[]) {
//------------------------------------------------------------------------------
  debug(F("\r\nto Serial1:"));
  debug(command);
  Serial1.println(command);
  if (Serial1.findUntil(respond, "ERROR")) {
    debug(F("\r\nfm esp826> OK"));
    return true;
    }
  else {
    debug("\r\nfm Serial1> ERROR: " + command);
    return false;
  }
}

//------------------------------------------------------------------------------
String sendCom(String command) {
//------------------------------------------------------------------------------
  debug(F("\r\nto Serial1:"));
  debug(command);
  Serial1.println(command);
  return Serial1.readString();
}



//-------------------------------------------------Debug Functions------------------------------------------------------

//------------------------------------------------------------------------------
void debug(String Msg) {
//------------------------------------------------------------------------------
  if (DEBUG) {
    Serial.print(Msg);
  }
}

//------------------------------------------------------------------------------
void debugln(String Msg) {
//------------------------------------------------------------------------------
  if (DEBUG) {
    Serial.println(Msg);
  }
}

//------------------------------------------------------------------------------
void debugln(void) {
//------------------------------------------------------------------------------
  if (DEBUG) {
    Serial.println();
  }
}

//------------------------------------------------------------------------------
void debug(char c) {
//------------------------------------------------------------------------------
  if (DEBUG) {
    Serial.write(c);
  }
}



//====== CMD-Interpreter =======================================================

//------------------------------------------------------------------------------
void command(void) {
//------------------------------------------------------------------------------
  char c;
  int l;
  String message;

  if (Serial.available() == 0) return;

  c = Serial.read();
  Serial.write(c);

  switch (c) {
    case 0x0d:

      l = splitCmdLine();

      if (l > 0) {
        if (token[0].equals("h") || token[0].equals("help")) cmdHelp(false);
        else if (token[0].equals("list")) cmdList();
        else if (token[0].equals("clear")) cmdClear();
        else if (token[0].equals("set")) {
          if (!cmdSet(message)) Serial.println(message);
          }
        else if (token[0].equals("write")) cmdWrite();
        else if (token[0].equals("read")) cmdRead();
        else if (token[0].equals("reboot")) cmdReboot();
        }
      else cmdHelp(true);

      prompt();
      break;
    case 0x0a:
      break;
    default:
      cmdLine = cmdLine + c;
    }
  }

//------------------------------------------------------------------------------
void cmdHelp(boolean unknownCommand) {
//------------------------------------------------------------------------------
  if (unknownCommand) Serial.println(F("\r\nunbekanntes Kommando"));
  Serial.println(F("\r\nhelp oder h"));
  Serial.println(F("list"));
  Serial.println(F("set \"SSID[32]\" \"Password[63]\""));
  Serial.println(F("clear \"SSID[32]\""));
  Serial.println(F("write"));
  Serial.println(F("reboot"));
  Serial.println("l");
  }


//------------------------------------------------------------------------------
boolean cmdSet(String &message) {
//------------------------------------------------------------------------------

  if (token[1].length() > 34) {
    message = F("SSID zu lang - max 32");
    return false;
    }

  if (token[2].length() > 65) {
    message = F("Passwort zu lang - max 63");
    return false;
    }

  if (!stripToken(token[1],1)) {
    message = F("Fehler in SSID");
    return false;
    }

  if (!stripToken(token[2],0)) {
    message = F("Fehler in Passwort");
    return false;
    }

  int index=-1;
  for (int i = 0; i < MAXRECORDS; i++) {
    if (token[1].equals(config.record[i].ssid)) {
      index = i;
      break;
      }
    }

  if (index < 0) {
    if (config.n < MAXRECORDS) {
      index = config.n;
      config.n++;
      }
    else {
      message = F("keine weitere Verbindung mehr moeglich");
      return false;
      }
    }

  token[1].toCharArray(config.record[index].ssid,33);
  token[2].toCharArray(config.record[index].pw,64);
  }



//------------------------------------------------------------------------------
boolean stripToken(String &token, int lMin) {
//------------------------------------------------------------------------------
  int l = token.length();
  if (l < (lMin+3)) return false;
  if (!token.startsWith("\"")) return false;
  if (!token.endsWith("\"")) return false;
  token = token.substring(1,l-1);
  token.trim();
  if (token.length() < lMin) return false;
  return true;
  }

//------------------------------------------------------------------------------
void cmdClear(void) {
//------------------------------------------------------------------------------
  config.n = 0;
  }



//------------------------------------------------------------------------------
void cmdWrite(void) {
//------------------------------------------------------------------------------

  config.crc = recordCrc(&config);
  EEPROM.put(0,config);
  }


//------------------------------------------------------------------------------
void cmdRead(void) {
//------------------------------------------------------------------------------
  EEPROM.get(0, config);
  if (recordCrc(&config) != config.crc) config.n = 0;
  }

//------------------------------------------------------------------------------
void cmdReboot(void) {
//------------------------------------------------------------------------------
  CPU_RESTART
  }



//------------------------------------------------------------------------------
void readEEPROM(void) {
//------------------------------------------------------------------------------

  EEPROM.get(0, config);
  if (recordCrc(&config) != config.crc) {
    Serial.println(F("EEPROM wird zurueckgesetzt"));
    config.n = 0;
    }
  }

//------------------------------------------------------------------------------
void cmdList(void) {
//------------------------------------------------------------------------------

  Serial.println("\r\n" + String(config.n) + F(" Eintraege"));
  if (config.n == 0) return;
  Serial.println(F("ID SSID Pw"));

  for (int i = 0; i < config.n; i++) {
    Serial.println(String(i + 1) + "  \"" +config.record[i].ssid + "\"  \"" + config.record[i].pw + "\"");
    }
  }






//------------------------------------------------------------------------------
void prompt(void) {
//------------------------------------------------------------------------------
  cmdLine = "";
  Serial.print("\r\n>");
}

//------------------------------------------------------------------------------
int splitCmdLine(void) {
//------------------------------------------------------------------------------

  for (int i = 0; i < MAXTOKENS; i++) token[i] = "";

  cmdLine.trim();

  if (cmdLine.equals("")) return -1; // no components

  if (getNextToken(cmdLine, token[0])) return 1;
  if (getNextToken(cmdLine, token[1])) return 2;
  if (getNextToken(cmdLine, token[2])) return 3;
  if (getNextToken(cmdLine, token[3])) return 4;

  return -2;  // too many components
}


//------------------------------------------------------------------------------
boolean getNextToken(String &s, String &token) {
//------------------------------------------------------------------------------

  s.trim();
  token = "";

  int p = s.indexOf(' ', 0);

  if (p == -1) {
    token = s;
    s = "";
    return true;
    }
  else {
    token = s.substring(0, p);
    s.remove(0, p + 1);
    return false;
  }
}



//------------------------------------------------------------------------------
unsigned long recordCrc(struct tRecords *pr) {
//------------------------------------------------------------------------------

    const unsigned long crc_table[16] = {
      0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
      0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
      0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
      0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
      };

    unsigned long crc = ~0L;
    byte *b = (byte*)pr;

    for (int i = 0; i < (sizeof(tRecords)-4); i++) {
      crc = crc_table[(crc ^ b[i]) & 0x0f] ^ (crc >> 4);
      crc = crc_table[(crc ^ ((b[i]) >> 4)) & 0x0f] ^ (crc >> 4);
      crc = ~crc;
      }
    return crc;
}

/*
class console {
  protected:
    String cmdLine;
    String token[MAXTOKENS];

  public:
    console(void) {
      cmdLine = "";
      }

    void poll(void) {
      String message;
      char c;
      int l;

      if (Serial.available() == 0) return;
      c = Serial.read();
      Serial.write(c);

      switch (c) {
        case 0x0d:

        l = splitCmdLine();

        if (l > 0) {
          if (token[0].equals("h") || token[0].equals("help")) cmdHelp(false);
          else if (token[0].equals("list")) cmdList();
          else if (token[0].equals("clear")) cmdClear();
          else if (token[0].equals("set")) {
            if (!cmdSet(message)) Serial.println(message);
            }
          else if (token[0].equals("write")) cmdWrite();
          else if (token[0].equals("read")) cmdRead();
          }
        else cmdHelp(true);

      prompt();
      break;
    case 0x0a:
      break;
    default:
      cmdLine = cmdLine + c;
      }

    }
  };

  class TCommand {
    protected:
      String command;

      void execute(void) {
        }
    public:
      TCommand(String command) {
        this->command = command;
        }

    boolean check(String token) {
      if (token.equals(this->command) {
        execute();
        return true;
        }
      else return false;
      }
    };

class TCmdList:public TCommand {

  };


*/




