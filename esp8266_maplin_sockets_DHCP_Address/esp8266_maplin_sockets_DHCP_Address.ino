/*
Arduino Maplin Socket switch code from:
http://www.fanjita.org/serendipity/archives/53-Interfacing-with-radio-controlled-mains-sockets-part-2.html
Timings updated and codes in hex so you can see the pattern.
*/

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

const char* ssid = "Maplin";
const char* passphrase = "maplinsockets";
String st;
String content;
int statusCode;

const int payloadSize = 48;
const int vccPin = 1;   // TX Data
const int dataPin = 3;  // RX Data
const int pulseWidth = 400;

const long buttons[] = {
  0x33353335L,
  0x33533335L,
  0x35333335L,
  0x53333335L,
  0x33353353L,
  0x33533353L,
  0x35333353L,
  0x53333353L,
  0x33353533L,
  0x33533533L,
  0x35333533L,
  0x53333533L,
  0x33355333L,
  0x33535333L,
  0x35335333L,
  0x53335333L  
};

ESP8266WebServer server(80);

void setup () {

  pinMode(vccPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  radioOff();

  EEPROM.begin(512);
  delay(10);

  String esid;
  for (int i = 0; i < 32; ++i)
    {
      esid += char(EEPROM.read(i));
    }
    
  String epass = "";
  for (int i = 32; i < 96; ++i)
    {
      epass += char(EEPROM.read(i));
    }
    
  if ( esid.length() > 1 ) {
      WiFi.begin(esid.c_str(), epass.c_str());
      if (testWifi()) {
        launchWeb(0);
        return;
      } 
  }
  
  setupAP();

}

bool testWifi(void) {
  int c = 0;
  while ( c < 20 ) {
    if (WiFi.status() == WL_CONNECTED) { return true; } 
    delay(500);  
    c++;
  }
  return false;
} 

void launchWeb(int webtype) {
  createWebServer(webtype);
  server.begin();
}

void setupAP(void) {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  st = "<ol>";
  for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      st += "<li>";
      st += WiFi.SSID(i);
      st += " (";
      st += WiFi.RSSI(i);
      st += ")";
      st += (WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*";
      st += "</li>";
    }
  st += "</ol>";
  delay(100);
  WiFi.softAP(ssid, passphrase, 6);
  launchWeb(1);
}

void createWebServer(int webtype) {
  
  if ( webtype == 1 ) {
    server.on("/", []() {
        int group = 0;
        int socket = 0;
        int state = 0;
        boolean error = false;
        
        IPAddress ip = WiFi.softAPIP();
        String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
        content = "<!DOCTYPE HTML>\r\n<html>Hello from ESP8266 at ";
        content += ipStr;
        content += "<p> 192.168.4.1/?group=[a-d]&socket=[1-4]&state=[on/off]";
        content += "<p> 192.168.4.1/cleareeprom to delete WiFi password<p>";
        content += st;
        content += "</p><form method='get' action='setting'><label>SSID: </label><input name='ssid' length=32><label>Pass: </label><input name='pass' length=64><input type='submit'></form>";
        content += "</html>";
        server.send(200, "text/html", content);  
       
        String Group = server.arg("group");
        if(Group.equalsIgnoreCase("A")) {
          group = 1;     
        }
        else if(Group.equalsIgnoreCase("B")) {
          group = 2;
        }
        else if(Group.equalsIgnoreCase("C")) {
          group = 3;
        }
        else if(Group.equalsIgnoreCase("D")) {
          group = 4;
        }
        else {
          error = true;
        }
        
        String Socket = server.arg("socket");
        if(Socket.equalsIgnoreCase("1")) {
          socket = 1;
        }
        else if(Socket.equals("2")) {
          socket = 2;
        }       
        else if(Socket.equals("3")) {
          socket = 3;
        }
        else if(Socket.equals("4")) {
          socket = 4;
        }   
        else {
          error = true;
        }
                    
        String State = server.arg("state");
        if(State.equalsIgnoreCase("ON")) {
          state = 1;
        }
        else if(State.equalsIgnoreCase("OFF")) {
          state = 0;
        }
        else {
          error = true;
        }
        
        if(!error) {
          radioOn();
          switchSocket(group, socket, state);
          radioOff();
        }
       
    });

    server.on("/cleareeprom", []() {
      content = "<!DOCTYPE HTML>\r\n<html>";
      content += "<p>Clearing the EEPROM</p></html>";
      server.send(200, "text/html", content);
      for (int i = 0; i < 96; ++i) { EEPROM.write(i, 0); }
      EEPROM.commit();
      WiFi.disconnect();
    });
    server.on("/setting", []() {
        String qsid = server.arg("ssid");
        String qpass = server.arg("pass");
        if (qsid.length() > 0 && qpass.length() > 0)
          {
          for (int i = 0; i < 96; ++i) {
            EEPROM.write(i, 0);
          }
          for (int i = 0; i < qsid.length(); ++i)
            {
              EEPROM.write(i, qsid[i]);
            }
          for (int i = 0; i < qpass.length(); ++i)
            {
              EEPROM.write(32+i, qpass[i]);
            }    
          EEPROM.commit();
          content = "Success: saved to eeprom... reset to boot into new wifi";
          statusCode = 200;
        } else {
          content = "Error: 404 not found";
          statusCode = 404;
        }
        server.send(statusCode, "text/html", content);
    });
  } else if (webtype == 0) {
    server.on("/", []() {
      
        int group = 0;
        int socket = 0;
        int state = 0;
        boolean error = false;
      
      IPAddress ip = WiFi.localIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      server.send(200, "text/html", "IPAddress:" + ipStr + "<p>" +ipStr + "/?group=[a-d]&socket=[1-4]&state=[on/off]" + "<p>" + ipStr + "/cleareeprom to delete WiFi password");

      String Group = server.arg("group");
      if(Group.equalsIgnoreCase("A") or Group.equals("1")) {
        group = 1;     
      }
      else if(Group.equalsIgnoreCase("B") or Group.equals("2")) {
        group = 2;
      }
      else if(Group.equalsIgnoreCase("C") or Group.equals("3")) {
        group = 3;
      }
      else if(Group.equalsIgnoreCase("D") or Group.equals("4")) {
        group = 4;
      }
      else {
        error = true;
      }
      
      String Socket = server.arg("socket");
      if(Socket.equalsIgnoreCase("1")) {
        socket = 1;
      }
      else if(Socket.equals("2")) {
        socket = 2;
      }       
      else if(Socket.equals("3")) {
        socket = 3;
      }
      else if(Socket.equals("4")) {
        socket = 4;
      }   
      else {
        error = true;
      }
                  
      String State = server.arg("state");
      if(State.equalsIgnoreCase("ON")) {
        state = 1;
      }
      else if(State.equalsIgnoreCase("OFF")) {
        state = 0;
      }
      else {
        error = true;
      }
      
      if(!error) {
        radioOn();
        switchSocket(group, socket, state);
        radioOff();
      }
      
    });
    server.on("/cleareeprom", []() {
      content = "<!DOCTYPE HTML>\r\n<html>";
      content += "<p>Clearing the EEPROM</p></html>";
      server.send(200, "text/html", content);
      for (int i = 0; i < 96; ++i) 
        {
        EEPROM.write(i, 0);
        }
      EEPROM.commit();
      WiFi.disconnect();
    });
  }
}

void radioOff() {
  digitalWrite(dataPin, LOW);
  digitalWrite(vccPin, LOW);
}

void radioOn() {
  digitalWrite(dataPin, LOW);
  digitalWrite(vccPin, HIGH);
}

void txData(long payload1, long payload2) {

  // Send a preamble of 12.4ms low pulse
  digitalWrite(dataPin, LOW);
  for (int i = 0; i < 31; i++)
  {
    delayMicroseconds(pulseWidth);
  }
  
  // send sync pulse : high for 0.4 ms
  digitalWrite(dataPin, HIGH);
  delayMicroseconds(pulseWidth);
  digitalWrite(dataPin, LOW);
  
  // Now send the digits.  
  // We send a 1 as a state change for 1.2ms, and a 0 as a state change for 0.4ms
  long mask = 1;
  char txState = HIGH;  //TODO boolean?
  long payload = payload1;
  
  for (int j = 0; j < payloadSize; j++)
  {
    if (j == 32)
    {
      payload = payload2;
      mask = 1;
    }
    
    char bit = (payload & mask) ? 1 : 0;
    mask <<= 1;
      
    txState = !txState;
    digitalWrite(dataPin, txState);
  
    delayMicroseconds(pulseWidth);  
    if (bit)
    {
      delayMicroseconds(pulseWidth);  
      delayMicroseconds(pulseWidth);  
    }
  }
}

void switchSocket(int group, int socket, int state) {
  
  long payload1 = buttons[(group - 1) * 4 + (socket - 1)];
  long payload2 = state? 0x3333L : 0x5333L;  //  on : off

  // Send the data 10 times
  for (int i= 0; i < 10; i++)
  {
    txData(payload1, payload2);
  }
}

void loop() {
  
  server.handleClient();
 
}
