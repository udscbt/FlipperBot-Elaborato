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

void setup()
{
  @INIT
  @SCHEDULE_ALL
  schemo::start_cycle();
}

void loop() {}
