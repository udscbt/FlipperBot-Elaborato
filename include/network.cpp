#include "schemo.h"
#include "fbcp.common.h"
#include "FBNet.h"

#include <ESP8266WiFi.h>

typedef unsigned char byte;

const unsigned char LED     = 15; //RED

WiFiClient sockOut;
WiFiServer server(FBNet::PORT);

bool hit = false;
fbcp::string hitCmd;
fbcp::string kalCmd;

typedef enum
{
  READ_TIMEOUT,
  READ_SUCCESS,
  READ_FAIL
} READ_RESULT;

@DECLARE

@FUNCTION(read_command)
@PARAM(sock:WiFiClient*)
@PARAM(cmd:fbcp::COMMAND_LINE*)
@PARAM(timeout:unsigned long)
@RETURN(READ_RESULT)
{
  @MEMORY
  {
    @VAR(msg:fbcp::string)
    @VAR(t:unsigned long)
  }  
  
  @VAR(msg) = "";
  @VAR(t) = millis();
  @WHILE (millis() - @VAR(t) < @PARAM(timeout))
  {
    int nc = @PARAM(sock)->available();
    for (int i = 0; i < nc; ++i)
    {
      char c = @PARAM(sock)->read();
      @VAR(msg) += c;
      
      if (c == '\n' or c == '\0')
      {
        @BREAK
      }
    }
  }
  
  Serial.print(F("Received: '"));
  Serial.print(@VAR(msg).c_str());
  Serial.println(F("'"));
  
  @IF (millis() - @VAR(t) >= @PARAM(timeout))
  {
    Serial.println(F("Timeout"));
    @RETURN(READ_TIMEOUT);
  }
  
  @IF (@VAR(msg)[@VAR(msg).length()-1] == '\0')
  {
    Serial.println(F("Remote host said something REALLY strange :S"));
    @RETURN(READ_FAIL);
  }
  
  @RETURN(fbcp::parseCommand(@VAR(msg), *@PARAM(cmd))?READ_SUCCESS:READ_FAIL);
}

@FUNCTION(wait)
@PARAM(dt:unsigned int)
@RETURN(char)
{
  @MEMORY
  {
    @VAR(t0:unsigned int)
  }
  @VAR(t0) = millis();
  @WHILE (millis() - @VAR(t0) < @PARAM(dt)) {}
  @RETURN(0);
}

@JOB (job_network)
{
  @MEMORY
  {
    @VAR(i:int)
    @VAR(n:int)
    @VAR(connected:bool)

    //tryConnect
    @VAR(t1:unsigned long)
    
    @VAR(cmd:fbcp::COMMAND_LINE)
  }

  @WHILE
  {
    @IF (mode == MODE_IDLE)
    {
      /////////////////
      // WiFi discovery:
      //WiFi.mode(WIFI_STA);
      //WiFi.disconnect();  
      Serial.println(F("Scan start"));
      mode = MODE_SCAN;
      @VAR(connected) = false;

      WiFi.scanNetworks(true);
      @WHILE ((@VAR(n) = WiFi.scanComplete()) == WIFI_SCAN_RUNNING) {}
      if (@VAR(n) == WIFI_SCAN_FAILED)
      {
        Serial.println(F("Scan failed"));
      }
      else
      {
        Serial.println("Scan done");
      }
      if (@VAR(n) == 0)
      {
        Serial.println("No networks found");
      }
      @IF (@VAR(n) > 0)
      {
        Serial.print(@VAR(n));
        Serial.println(F(" networks found"));
        @VAR(i) = -1;
        @VAR(connected) = false;
        @WHILE (++@VAR(i) < @VAR(n) && !@VAR(connected))
        {
          // Print SSID and RSSI for each network found
          Serial.print(@VAR(i) + 1);
          Serial.print(F(": "));
          ssid = WiFi.SSID(@VAR(i)).c_str();
          Serial.print(ssid.c_str());
          Serial.print(F(" ("));
          Serial.print(WiFi.RSSI(@VAR(i)));
          Serial.print(F(")"));
          bool noenc = (WiFi.encryptionType(@VAR(i)) == ENC_TYPE_NONE);
          Serial.println(noenc?F(" "):F("*"));
          // Search for suitable network
          if (!noenc)
          {
            @CONTINUE
          }
          
          if (ssid.startsWith(fbcp::BOARD_PREFIX))
          {
            Serial.println(F("Found suitable board network"));
          }
          else
          {
            @CONTINUE
          }

          /*******************
           * tryConnect START *
           ********************/
          mode = MODE_NETCON;
          Serial.print(F("Connecting to "));
          Serial.println(ssid.c_str());
          
          @VAR(t1) = millis();

          @WHILE (status != WL_CONNECTED && millis() - @VAR(t1) < fbcp::HARD_TIMEOUT)
          {
            status = WiFi.begin(ssid.c_str());
            @CALL(wait;500):null;
            Serial.print(F("."));
          }
          Serial.println(F(""));

          @VAR(connected) = false;
          switch (status)
          {
            case WL_CONNECTED:
              Serial.println(F("WiFi connected"));
              @VAR(connected) = true;
              break;
            case WL_NO_SHIELD:
              Serial.println(F("No WiFi shield is present"));
              @CONTINUE
            case WL_IDLE_STATUS:
              Serial.println(F("Timeout"));
              @CONTINUE
            case WL_NO_SSID_AVAIL:
              Serial.println(F("No SSID are available"));
              @CONTINUE
            case WL_SCAN_COMPLETED:
              Serial.println(F("Scan networks is completed"));
              @CONTINUE
            case WL_CONNECT_FAILED:
              Serial.println(F("Connection failed for all the attempts"));
              @CONTINUE
            case WL_CONNECTION_LOST:
              Serial.println(F("Connection lost"));
              @CONTINUE
            case WL_DISCONNECTED:
              Serial.println(F("Disconnected from a network"));
              @CONTINUE
          }
          status = WL_IDLE_STATUS;
          
          Serial.print(F("IP address: "));
          Serial.println(WiFi.localIP());

          mode = MODE_SERCON;
          Serial.print(F("Trying to connect to "));
          Serial.print(FBNet::GATEWAY[0]);
          Serial.print(F("."));
          Serial.print(FBNet::GATEWAY[1]);
          Serial.print(F("."));
          Serial.print(FBNet::GATEWAY[2]);
          Serial.print(F("."));
          Serial.print(FBNet::GATEWAY[3]);
          Serial.print(F(":"));
          Serial.println(FBNet::PORT);

          @VAR(connected) = sockOut.connect(gateway, FBNet::PORT);
            
          if (!@VAR(connected))
          {
            Serial.println(F("Can't connect to server"));
            @CONTINUE
          }

          Serial.println(F("Connected"));
          
          @VAR(cmd).command = &fbcp::Q_SINGLE_PRESENTATION;
          @VAR(cmd).params["serial"] = fbcp::serial;
          fbcp::string s = fbcp::writeCommand(@VAR(cmd));
          Serial.print(F("Sent: "));
          Serial.println(s.c_str());
          sockOut.print(s.c_str());
          
          @CALL(read_command;&sockOut;&@VAR(cmd);fbcp::HARD_TIMEOUT):understood;
          @VAR(connected) = false;
          if (understood == READ_FAIL)
          {
            Serial.println(F("Couldn't understand server response"));
          }
          else if (understood = READ_TIMEOUT)
          {
            Serial.println(F("Server timed out"));
          }
          else if (@VAR(cmd).command->code == fbcp::A_GRANT_ACCESS.code)
          {
            Serial.println(F("Server allowed connection"));
            @VAR(connected) = true;
          }
          else if (@VAR(cmd).command->code == fbcp::A_DENY_ACCESS.code)
          {
            Serial.println(F("Server refused connection"));
          }
          else
          {
            Serial.println(F("Server answered something strange :S"));
          }
          /*******************
           * tryConnect STOP  *
           ********************/
  
          if (@VAR(connected))
          {
            Serial.println(F("Connection estabilished"));
            mode = MODE_GAME;
          }
          else
          {
            Serial.println(F("Connection failed"));
          }
        }
      }
      Serial.println(F(""));
      /////////////////

      if (!@VAR(connected))
      {
        Serial.print("Starting AccessPoint: ");
        ssid = fbcp::serial;
        Serial.println(ssid.c_str());
        WiFi.softAP(ssid.c_str());
        WiFi.softAPConfig(host, gateway, submask);
        Serial.print("IP address: ");
        Serial.println(WiFi.softAPIP());
        mode = MODE_STANDALONE;
      }
    }
  }
}

void setup()
{
  // Initialise serial communication
  Serial.begin(115200);
  delay(10);

  Serial.println(F("Starting"));

  // FBCP
  Serial.println(F("FBCP"));
  fbcp::serial = fbcp::ROBOT_PREFIX;
  fbcp::serial += serial;
  mode = MODE_IDLE;
  fbcp::COMMAND_LINE cmd;
  cmd.command = &fbcp::Q_HIT;
  hitCmd = fbcp::writeCommand(cmd);
  cmd.command = &fbcp::Q_HEARTBEAT;
  kalCmd = fbcp::writeCommand(cmd);
  
  //ScheMo
  Serial.println(F("ScheMo"));
  @INIT

  //WiFi
  WiFi.persistent(false);

  @SCHEDULE_ALL
  Serial.println(F("Jobs scheduled"));

  schemo::start_cycle();
}

void loop() {}
