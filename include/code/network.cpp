/**
 * Example code 
 */
#include "schemo.h"
#include "fbcp.common.h"
#include "FBNet.h"
#include <ESP8266WiFi.h>

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
 
 @RETURN(
  fbcp::parseCommand(
   @VAR(msg),
   *@PARAM(cmd)
  )?READ_SUCCESS:READ_FAIL
 );
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
   mode = MODE_SCAN;
   @VAR(connected) = false;

   WiFi.scanNetworks(true);
   @WHILE ((@VAR(n) = WiFi.scanComplete()) == WIFI_SCAN_RUNNING)
   {}
   @IF (@VAR(n) > 0)
   {
    Serial.print(@VAR(n));
    @VAR(i) = -1;
    @VAR(connected) = false;
    @WHILE (++@VAR(i) < @VAR(n))
    {     
     if (!ssid.startsWith(fbcp::BOARD_PREFIX))
     {
      @CONTINUE
     }
     node = MODE_NETCON;
     
     @VAR(t1) = millis();

     @WHILE (
      status != WL_CONNECTED &&
      millis() - @VAR(t1) < fbcp::HARD_TIMEOUT
     )
     {
      status = WiFi.begin(ssid.c_str());
      @CALL(wait;500):null;
     }
     if (status != WL_CONNECTED)
     {
      @CONTINUE
     }
     status = WL_IDLE_STATUS;

     mode = MODE_SERCON;

     @VAR(connected) = sockOut.connect(gateway, FBNet::PORT);
      
     if (!@VAR(connected))
     {
      @CONTINUE
     }

     @VAR(cmd).command = &fbcp::Q_SINGLE_PRESENTATION;
     @VAR(cmd).params["serial"] = fbcp::serial;
     fbcp::string s = fbcp::writeCommand(@VAR(cmd));
     sockOut.print(s.c_str());
     
     @CALL(
      read_command;
      &sockOut;
      &@VAR(cmd);
      fbcp::HARD_TIMEOUT
     ):understood;
     @VAR(connected) = false;
     if (
      understood == READ_SUCCESS &&
      @VAR(cmd).command->code == fbcp::A_GRANT_ACCESS.code
     )
     {
      @VAR(connected) = true;
      mode = MODE_GAME;
      @BREAK
     }
    }
   }

   if (!@VAR(connected))
   {
    ssid = fbcp::serial;
    WiFi.softAP(ssid.c_str());
    WiFi.softAPConfig(host, gateway, submask);
    mode = MODE_STANDALONE;
   }
  }
 }
}

void setup()
{
 // FBCP
 fbcp::serial = fbcp::ROBOT_PREFIX;
 fbcp::serial += serial;
 mode = MODE_IDLE;
 fbcp::COMMAND_LINE cmd;
 cmd.command = &fbcp::Q_HIT;
 hitCmd = fbcp::writeCommand(cmd);
 cmd.command = &fbcp::Q_HEARTBEAT;
 kalCmd = fbcp::writeCommand(cmd);
 
 //ScheMo
 @INIT

 @SCHEDULE_ALL

 schemo::start_cycle();
}

void loop() {}
