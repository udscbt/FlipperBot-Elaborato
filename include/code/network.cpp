#include "schemo.h"
#include "fbcp.common.h"
#include "FBNet.h"
#include <ESP8266WiFi.h>
#include "ssidheap.h"

fbcp::string fbcp::serial;
fbcp::string ssid;

WiFiClient sockOut;

enum
{
  MODE_IDLE,
  MODE_CONNECTING,
  MODE_GAME,
  MODE_STANDALONE
} mode = MODE_IDLE;

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
  
  @IF (millis() - @VAR(t) >= @PARAM(timeout))
  {
    @RETURN(READ_TIMEOUT);
  }
  
  @IF (@VAR(msg)[@VAR(msg).length()-1] == '\0')
  {
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
    @VAR(i:byte)
    @VAR(n:int)
    @VAR(boardrobot:bool)
    @VAR(cmd:fbcp::COMMAND_LINE)
  }

  @WHILE
  {
    @IF (mode == MODE_IDLE)
    { 
      mode = MODE_CONNECTING;
      @VAR(n) = WiFi.scanNetworks();

      @VAR(i) = 0;
      clearSsidHeap();
      @WHILE (@VAR(i) < @VAR(n))
      {
        insertSsid(@VAR(i));
        ++@VAR(i);
      }
      @WHILE ( (@VAR(i)=popSsid()) < @VAR(n))
      {
        ssid = WiFi.SSID(@VAR(i)).c_str();
        if (ssid.startsWith(fbcp::BOARD_PREFIX))
        {
          @VAR(boardrobot) = true;
        }
        else if (ssid.startsWith(fbcp::ROBOT_PREFIX))
        {
          @VAR(boardrobot) = false;
        }
        else
        {
          @CONTINUE
        }
        
        WiFi.begin(ssid.c_str());
        sockOut.connect(IPAddress(192,168,1,1), FBNet::PORT);
        
        @VAR(cmd).command = &fbcp::Q_SINGLE_PRESENTATION;
        @VAR(cmd).params["serial"] = fbcp::serial;
        fbcp::string s = fbcp::writeCommand(@VAR(cmd));
        sockOut.print(s.c_str());
        
        @CALL(
          read_command;
          &sockOut;&@VAR(cmd);
          fbcp::HARD_TIMEOUT
        ) : understood;
        if (understood == READ_SUCCESS && @VAR(cmd).command->code == fbcp::A_GRANT_ACCESS.code)
        {
          if (@VAR(boardrobot))
          {
            mode = MODE_GAME;
          }
          else
          {
            mode = MODE_STANDALONE;
          }
          @BREAK
        }
        mode = MODE_IDLE;
      }

      @IF (mode != MODE_GAME && mode != MODE_STANDALONE)
      {
        mode = MODE_IDLE;
        @CALL(wait;10000):null;
      }
    }
  }
}

void setup()
{  
  // FBCP
  fbcp::serial = fbcp::CONTR_PREFIX;
  fbcp::serial += "00001";
  mode = MODE_IDLE;
  
  //ScheMo
  @INIT
  @SCHEDULE_ALL

  schemo::start_cycle();
}

void loop(){}
