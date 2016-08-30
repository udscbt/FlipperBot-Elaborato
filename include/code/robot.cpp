§>
#include "schemo.h"      //ScheMo C++ Library
//#include "fbcp.h"      //FBCP C++ Library
#include "fbcp.common.h" // Additionally includes default
                         // handlers for common commands
#include "FBNet.h"       //IP addresses and ports used
#include <ESP8266WiFi.h> //From the ESP8266 SDK

§<
#include "motors.h"
typedef unsigned char byte;

const unsigned char L_MOTOR = 14;
const unsigned char R_MOTOR = 16;
const unsigned char HIT     = 05;
const unsigned char BUZ     = 04; //GREEN
//const unsigned char LED     = 13; //RED
const unsigned char LED     = 15; //RED

Motor leftMotor(
 Motor::RIGHT |
 Motor::NEW |
 Motor::pin(L_MOTOR)
);
Motor rightMotor(
 Motor::RIGHT |
 Motor::NEW |
 Motor::pin(R_MOTOR)
);

enum
{
 SEND_INIT,
 SEND_BUSY,
 SEND_FAIL,
 SEND_NEW,
 SEND_OK
} sendStatus = SEND_INIT;

const int WAIT_AFTER_REFUSED = 1000;

§>
/*
 * Every FBCP-compliant program should define its own serial code. A
 * suitable variable is already declared as extern in fbcp.h.
 */
fbcp::string fbcp::serial;
§<
const char serial[] = "00001";
fbcp::string ssid;
int status = WL_IDLE_STATUS;

const IPAddress host(
 FBNet::GATEWAY[0],
 FBNet::GATEWAY[1],
 FBNet::GATEWAY[2],
 FBNet::GATEWAY[3]
);
const IPAddress gateway(
 FBNet::GATEWAY[0],
 FBNet::GATEWAY[1],
 FBNet::GATEWAY[2],
 FBNet::GATEWAY[3]
);
const IPAddress submask(
 FBNet::NETMASK[0],
 FBNet::NETMASK[1],
 FBNet::NETMASK[2],
 FBNet::NETMASK[3]
);

WiFiClient sockOut;
WiFiServer server(FBNet::PORT);

enum
{
 MODE_IDLE,
 MODE_SCAN,
 MODE_NETCON,
 MODE_SERCON,
 MODE_GAME,
 MODE_STANDALONE
} mode = MODE_IDLE;

float ledFreq = 0;

enum
{
 FREQ_OFF = -1,
 FREQ_ON  = 0,
 FREQ_SLOW = 2,
 FREQ_FAST = 10
};

bool hit = false;
fbcp::string hitCmd;
fbcp::string kalCmd;

§>
typedef enum
{
 READ_TIMEOUT,
 READ_SUCCESS,
 READ_FAIL
} READ_RESULT;
/* Note that types definitions are put before the @DECLARE directive.
 * This is necessay because there are ScheMo-related entities that use
 * them (e.g. the function read_command returns a variable of type
 * READ_RESULT)
 */
 
/*
 * This declares all variables and functions needed by ScheMo to make
 * the program work properly. Since all declarations happen here, the
 * order in which the jobs and functions are defined is not important
 * even if they refer to each other
 */
@DECLARE
//All other directives should be AFTER it.

/*
 * Here is a function definition.
 * The first line must always defines the name of the function. This
 * name shouldn't be accessed directly and only with a @CALL directive.
 * All @PARAM lines are optional and define a parameter of the
 * function by setting the name with which they can be accessed inside
 * the function and the type of variable in which they are stored. The
 * order in which they are written is important since they are passed
 * to the function in that order.
 * The @RETURN line is mandatory and must be unique. It doesn't have
 * to be the last one but it makes the code more readable. It defines
 * the type of variable returned by the function. All comments are
 * removed by schemop before parsing the source file and this allows
 * to insert them anywhere without causing problems. This is
 * particularly useful for writing documentation comments AFTER the
 * function signature like in this example.
 */
@FUNCTION (read_command)
@PARAM (sock    : WiFiClient*)
@PARAM (cmd     : fbcp::COMMAND_LINE*)
/* The fbcp::COMMAND_LINE structure contains parsed data about a full
 * FBCP command string.
 */
@PARAM (timeout : unsigned long)
@RETURN (READ_RESULT)
/**
 * Tries to read a command from the socket @PARAM(sock) and
 * stores it in *@PARAM(cmd) if successful.
 * The possible return values are:
 * READ_SUCCESS : A valid FBCP command was received and
 *                correctly parsed
 * READ_TIMEOUT : @PARAM(timeout) time without receiving a
 *                complete FBCP command
 * READ_FAIL    : Protocol error, no valid FBCP command was
 *                received
 */
{
 @MEMORY
 {
  /*
   * This is a memory block. All variables with job/function-wide
   * scope must be put here.
   *
   * The following directive declares a variable called msg of type
   * fbcp::string. The fbcp::string class is a reduced version of
   * std::string, only implementing the necessary features and
   * allowing the addition of non-standard ones if future developments
   * of the FBCP implementation require them.
   */
  @VAR (msg : fbcp::string)
 }
 @MEMORY
 {
  /*
   * Multiple memory blocks can be put in the same job/function block
   * and they will be joined together by schemop.
   */
  @VAR (t : unsigned long)
 }§< 
 
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
  Serial.println(
   F("Remote host said something REALLY strange :S")
  );
  @RETURN(READ_FAIL);
 }
 §>
 /*
  * This @RETURN directive is not the same as the one used in the
  * function definition (notice the semicolon at the end). It
  * specifies what value should be returned to the caller and stops
  * the function. More than one of this directive can be (and usually
  * is) present in the same function.
  */ 
 @RETURN(            // This is the standard way to pars ean FBCP
  fbcp::parseCommand(// command contained in a fbcp::string. The
   @VAR(msg),        // result is stored in the second parameter which
   *@PARAM(cmd)      // is of type fbcp::COMMAND_LINE&. I also returns
  )                  // whether or not the parsing was successful.
  ?READ_SUCCESS:READ_FAIL
 );
}§<

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

§>
/*
 * This is a job definition. Its name can be used to either schedule
 * or deschedule it.
 */
@JOB (job_network)
/**
 * Job that manages the connection to a network.
 * When the robot is idle (neither in game mode nor in standalone
 * mode), it first tries to connect to a board and pass to game mode,
 * if it fails it gets into standalone mode.
 */
{
 // Memory blocks in jobs are identical to the ones in functions
 @MEMORY
 {
  @VAR(i:int)
  @VAR(n:int)
  
  @VAR(t1:unsigned long)
  
  @VAR(cmd:fbcp::COMMAND_LINE)
 }

 @WHILE // This is an infinite loop
 {
  @IF (mode == MODE_IDLE) // Using a ScheMo @IF here allows to use
  {                       // other directives inside
§<
   Serial.println(F("Scan start"));
   mode = MODE_SCAN;

§>
   /*
    * The (true) argument makes this function asynchronous, making it
    * possible to write the following @WHILE statement to make the job
    * wait for scan completion without blocking other jobs. This shows
    * the real importance of using @WHILE directives instead of normal
    * while statements.
    */
   WiFi.scanNetworks(true);
   @WHILE (
    (@VAR(n) = WiFi.scanComplete())
    == WIFI_SCAN_RUNNING
   ) {}
   /*
    * Notice the use of a job-wide @VAR variable inside the loop,
    * making it possible to still use it in the following code.
    */
§<
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
§>
   @IF (@VAR(n) > 0)
   {
§<
    Serial.print(@VAR(n));
    Serial.println(F(" networks found"));
§>
    @VAR(i) = -1;
    @WHILE (++@VAR(i) < @VAR(n)) 
    {
§<
     // Print SSID and RSSI for each network found
     Serial.print(@VAR(i) + 1);
     Serial.print(F(": "));
     ssid = WiFi.SSID(@VAR(i)).c_str();
     Serial.print(ssid.c_str());
     Serial.print(F(" ("));
     Serial.print(WiFi.RSSI(@VAR(i)));
     Serial.print(F(")"));
     bool noenc = (
      WiFi.encryptionType(@VAR(i))
      == ENC_TYPE_NONE
     );
     Serial.println(noenc?F(" "):F("*"));
     // Search for suitable network
     if (!noenc)
     {
      @CONTINUE
     }
     
§>
     /*
      * BOARD_PREFIX is included in the FBCP library instead of the
      * FBNet one because it refers to the prefix of the serial code
      * of the board and thus it is part of the protocol.
      * The fact that it is also used for the SSID of the
      * board-related network is just for convenience.
      */
     if (ssid.startsWith(fbcp::BOARD_PREFIX))
     {
      Serial.println(F("Found suitable board network"));
     }
     else
     {
      /*
       * @CONTINUE and @BREAK directives must be placed inside @WHILE
       * blocks, but they can be contained inside normal C++ blocks
       * inside of them such as this if-else statement. It's important
       * to note however that they refer to the innermost @WHILE loop,
       * not any loop.
       * @WHILE (true)
       * {
       *  while (true)
       *  {
       *   if (condition)
       *   {
       *    break; //stops the while loop
       *   }
       *   else
       *   {
       *    @BREAK //stops the @WHILE loop
       *   }
       *  }
       * }
       */
      @CONTINUE
     }
§<

     mode = MODE_NETCON;
     Serial.print(F("Connecting to "));
     Serial.println(ssid.c_str());
     
     @VAR(t1) = millis();

     @WHILE (
      status != WL_CONNECTED &&
      millis() - @VAR(t1) < fbcp::HARD_TIMEOUT
     )
     {
      status = WiFi.begin(ssid.c_str());
      @CALL(wait;500):null;
      Serial.print(F("."));
     }
     Serial.println(F(""));

     switch (status)
     {
      case WL_CONNECTED:
       Serial.println(F("WiFi connected"));
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
       Serial.println(
        F("Connection failed for all the attempts")
       );
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
      
§>
     if (!sockOut.connect(gateway, FBNet::PORT))
     {
      Serial.println(F("Can't connect to server"));
      @CONTINUE
     }
§<
     
     Serial.println(F("Connected"));
     
§>
     /*
      * Here is shown how to obtain a valid FBCP command string. As
      * defined above, @VAR(cmd) is a fbcp::COMMAND_LINE, a structured
      * representation of an FBCP command string.
      * The .command member is of type fbcp::COMMAND* and specifies
      * the type of command used; all FBCP commands are available in
      * the library and accessible with their name.
      * The .params member contains all mandatory parameters, mapped
      * by their name to the actual parameter string.
      * Additional parameters added to .params are ignored, but they
      * can be put in a third member (unused here) called .other which
      * content is simply appended to the end of the command line.
      */
     @VAR(cmd).command = &fbcp::Q_SINGLE_PRESENTATION;
     @VAR(cmd).params["serial"] = fbcp::serial;
     /*
      * The actual string to be sent out is generated using the
      * fbcp::writeCommand function. In case invalid data are supplied
      * (e.g. missing mandatory parameters) an empty string is
      * returned instead.
      */
     fbcp::string s = fbcp::writeCommand(@VAR(cmd));
§<
     Serial.print(F("Sent: "));
     Serial.println(s.c_str());
§>
     sockOut.flush();
     sockOut.print(s.c_str());
     
     /*
      * This is a ScheMo function call. The function will temporarily
      * take the place of the job from the scheduler point of view,
      * being treated as the latter. The function, however, isn't
      * really part of the job and can't access job-wide variables.
      */
     @CALL(
      read_command; // Function to be called
      &sockOut;          //
      &@VAR(cmd);        // Parameters
      fbcp::HARD_TIMEOUT //
     ) : understood; // Return value, its type is defined by the
                     // function definition (in this case READ_RESULT)
     /*
      * The return value has scope in the task right after the
      * function call, making it accessible here. If a ScheMo control
      * flow directive is used, however, it will go out of scope.
      */
     if (understood == READ_FAIL)
     {
      Serial.println(
       F("Couldn't understand server response")
      );
     }
     else if (understood = READ_TIMEOUT)
     {
      Serial.println(F("Server timed out"));
     }
     else if (
      @VAR(cmd).command->code      // This is the correct way to check
      == fbcp::A_GRANT_ACCESS.code // for command equality
     )
     {
      Serial.println(F("Server allowed connection"));
      Serial.println(F("Connection estabilished"));
      mode = MODE_GAME;
      @BREAK
     }
     else if (
      @VAR(cmd).command->code
      == fbcp::A_DENY_ACCESS.code
     )
     {
      Serial.println(F("Server refused connection"));
     }
     else
     {
      Serial.println(
       F("Server answered something strange")
      );
     }
     
     sockOut.stop();
     Serial.println(F("Connection failed"));
    }
   }
§<
   Serial.println(F(""));
   
§>
   if (mode != MODE_GAME)
   {
    /*
     * If the robot failed to connect to a board and get in game mode,
     * it passes to standalone mode and makes itself visible to
     * controllers by becoming a WiFi access point.
     */
    Serial.print("Starting AccessPoint: ");
    ssid = fbcp::serial;
    Serial.println(ssid.c_str());
    WiFi.softAP(ssid.c_str());
    WiFi.softAPConfig(host, gateway, submask);
§<
    Serial.print("IP address: ");
    Serial.println(WiFi.softAPIP());
§>
    mode = MODE_STANDALONE;
   }
  }
 }
}
§<

@JOB(job_client)
{
 @MEMORY
 {
  @VAR(cmd:fbcp::COMMAND_LINE)
  @VAR(t:unsigned long)
 }
 
 @WHILE
 {
  @VAR(t) = millis();
  @WHILE (mode == MODE_GAME && sockOut.connected())
  {
   @VAR(cmd).command = &fbcp::Q_ROBOT_COMMAND;
   
   if (digitalRead(HIT))
   {
    if (!hit)
    {
     hit = true;
     sendStatus = SEND_NEW;
    }
   }
   else
   {
    hit = false;
   }
   
   @IF (
    sendStatus == SEND_BUSY ||
    sendStatus == SEND_NEW ||
    sendStatus == SEND_FAIL
   )
   {
    Serial.print("Sent: ");
    Serial.println(hitCmd.c_str());
    sockOut.flush();
    sockOut.print(hitCmd.c_str());
    
    @CALL(
     read_command;
     &sockOut;
     &@VAR(cmd);
     fbcp::SOFT_TIMEOUT
    ) : understood;
    if (understood == READ_SUCCESS)
    {
     @VAR(t) = millis();
     if (
      @VAR(cmd).command->code
      == fbcp::A_ACCEPT.code
     )
     {
      Serial.println("Command was accepted");
      sendStatus = SEND_OK;
     }
     else if (
      @VAR(cmd).command->code
      == fbcp::A_REFUSE.code
     )
     {
      Serial.println("Command was refused");
      sendStatus = SEND_BUSY;
     }
     else if (
      @VAR(cmd).command->code
      == fbcp::A_ERROR.code
     )
     {
      Serial.println("Server didn't understand command");
      sendStatus = SEND_FAIL;
     }
     else
     {
      Serial.println("Server answered something strange");
      sendStatus = SEND_FAIL;
     }
    }
    else if (understood == READ_FAIL)
    {
     Serial.println("Server answered something strange");
     sendStatus = SEND_FAIL;
    }
    else if (understood == READ_TIMEOUT)
    {
     sockOut.stop();
     sendStatus = SEND_INIT;
     @BREAK
    }
   }
   
   @IF (millis() - @VAR(t) > fbcp::HARD_TIMEOUT/2)
   {
    Serial.print("Sent: ");
    Serial.println(kalCmd.c_str());
    sockOut.flush();
    sockOut.print(kalCmd.c_str());
    @CALL(
     read_command;
     &sockOut;
     &@VAR(cmd);
     fbcp::SOFT_TIMEOUT
    ) : understood;
    if (understood == READ_SUCCESS)
    {
     @VAR(t) = millis();
     Serial.println("Server answered to heartbeat");
    }
    else if (understood == READ_TIMEOUT)
    {
     sockOut.stop();
     sendStatus = SEND_INIT;
     @BREAK
    }
   }
  }
  
  if (mode == MODE_GAME)
  {
   Serial.println("Disconnected");
   mode = MODE_IDLE;
  }
 }
}

bool manageDirection(fbcp::string direction)
{
 bool hit = digitalRead(HIT);
 if (direction == fbcp::DIRECTION_FORWARD_LEFT.str)
 {
  if (hit)
   return manageDirection(fbcp::DIRECTION_LEFT.str);
  leftMotor.stop();
  rightMotor.forward();
 }
 else if (direction == fbcp::DIRECTION_FORWARD.str)
 {
  if (hit)
   return manageDirection(fbcp::DIRECTION_STOP.str);
  leftMotor.forward();
  rightMotor.forward();
 }
 else if (direction == fbcp::DIRECTION_FORWARD_RIGHT.str)
 {
  if (hit)
   return manageDirection(fbcp::DIRECTION_RIGHT.str);
  leftMotor.forward();
  rightMotor.stop();
 }
 else if (direction == fbcp::DIRECTION_LEFT.str)
 {
  leftMotor.backward();
  rightMotor.forward();
 }
 else if (direction == fbcp::DIRECTION_STOP.str)
 {
  leftMotor.stop();
  rightMotor.stop();
 }
 else if (direction == fbcp::DIRECTION_RIGHT.str)
 {
  leftMotor.forward();
  rightMotor.backward();
 }
 else if (direction == fbcp::DIRECTION_BACKWARD_LEFT.str)
 {
  leftMotor.stop();
  rightMotor.backward();
 }
 else if (direction == fbcp::DIRECTION_BACKWARD.str)
 {
  leftMotor.backward();
  rightMotor.backward();
 }
 else if (direction == fbcp::DIRECTION_BACKWARD_RIGHT.str)
 {
  leftMotor.backward();
  rightMotor.stop();
 }
 else
 {
  return false;
 }
 Serial.print(F("Set direction to: "));
 Serial.println(direction);
 return true;
}

@JOB (job_server)
{
 @MEMORY
 {
  @VAR(client:WiFiClient)
  @VAR(cmd:fbcp::COMMAND_LINE)
  @VAR(controller:bool)
 }
 server.begin();
 @VAR(controller) = false;
 
 @WHILE
 {
  @WHILE (mode == MODE_STANDALONE && !@VAR(controller))
  {
   @VAR(client) = server.available();
   @IF (@VAR(client))
   {
    @WHILE(@VAR(client).connected() && !@VAR(controller))
    {
     @CALL(
      read_command;
      &@VAR(client);
      &@VAR(cmd);
      fbcp::HARD_TIMEOUT
     ) : understood;
     fbcp::COMMAND_LINE cmd;
     cmd.command = &fbcp::A_GRANT_ACCESS;
     if (understood == READ_FAIL)
     {
      cmd.command = &fbcp::A_ERROR;
      Serial.println(F("Couldn't understand message"));
     }
     else if (understood == READ_TIMEOUT)
     {
      cmd.command = NULL;
      Serial.println(F("Client timed out"));
     }
     else if (
      @VAR(cmd).command->code
      == fbcp::Q_SINGLE_PRESENTATION.code
     )
     {
      Serial.print(F("Controller connected: "));
      Serial.println(@VAR(cmd).params["serial"]);
      @VAR(controller) = true;
     }
     
     if (cmd.command == NULL)
     {
      Serial.println(F("Disconnecting"));
      @VAR(client).stop();
     }
     else
     {
      fbcp::string s = fbcp::writeCommand(cmd);
      Serial.print(F("Sent: "));
      Serial.println(s.c_str());
      @VAR(client).print(s.c_str());
     }
    }
   }
  }
  
  @IF (
   mode == MODE_GAME ||
   (
    mode == MODE_STANDALONE && @VAR(controller)
   )
  )
  {
   @IF (mode == MODE_GAME)
   {
    @VAR(client) = server.available();
   }
   @IF (@VAR(client))
   {
    @WHILE (@VAR(client).connected())
    {
     @CALL(
      read_command;
      &@VAR(client);
      &@VAR(cmd);
      fbcp::HARD_TIMEOUT
     ) : understood;
     fbcp::COMMAND_LINE cmd;
     cmd.command = &fbcp::A_ACCEPT;
     
     if (understood == READ_FAIL)
     {
      cmd.command = &fbcp::A_ERROR;
      Serial.println(F("Couldn't understand message"));
     }
     else if (understood == READ_TIMEOUT)
     {
      cmd.command = NULL;
      Serial.println(F("Client timed out"));
     }
     else if (fbcp::common::handleRequest(@VAR(cmd), cmd))
     {
      Serial.println("Library managed");
     }
     else if (
      @VAR(cmd).command->code
      == fbcp::Q_ROBOT_COMMAND.code
     )
     {
      Serial.println(F("Change direction"));
      if (manageDirection(@VAR(cmd).params["direction"]))
      {
       cmd.command = &fbcp::A_ERROR;
       Serial.println(F("Direction not recognized"));
      }
     }
     else if (
      @VAR(cmd).command->code
      == fbcp::Q_MOTOR_COMMAND.code
     )
     {
      Serial.println(F("Change motor speed"));
      bool left = false;
      bool right = false;
      if (
       @VAR(cmd).params["motor"] == fbcp::MOTOR_LEFT.str
      ) left = true;
      else if (
       @VAR(cmd).params["motor"] == fbcp::MOTOR_RIGHT.str
      ) right = true;
      else if (
       @VAR(cmd).params["motor"] == fbcp::MOTOR_BOTH.str
      ) {left = true; right=true;}
      else
      {
       cmd.command = &fbcp::A_ERROR;
       Serial.println(F("Motor not recognized"));
      }
      if (
       @VAR(cmd).params["direction"]
       == fbcp::DIRECTION_FORWARD.str
      )
      {
       if (left) leftMotor.value(1);
       if (right) rightMotor.value(1);
      }
      else if (
       @VAR(cmd).params["direction"]
       == fbcp::DIRECTION_STOP.str
      )
      {
       if (left) leftMotor.value(0);
       if (right) rightMotor.value(0);
      }
      else if (
       @VAR(cmd).params["direction"]
       == fbcp::DIRECTION_BACKWARD.str
      )
      {
       if (left) leftMotor.value(-1);
       if (right) rightMotor.value(-1);
      }
      else
      {
       cmd.command = &fbcp::A_ERROR;
       Serial.println(F("Direction not recognized"));
      }
     }
     else if (
      @VAR(cmd).command->code
      == fbcp::Q_CLEAN.code
     )
     {
      cmd.command = &fbcp::A_CLEAN;
      Serial.println(F("Client requested disconnection"));
     }
     else
     {
      cmd.command = &fbcp::A_ERROR;
      Serial.println(
       F("Client said something strange :S")
      );
     }
     
     if (cmd.command == NULL)
     {
      Serial.println(F("Disconnecting"));
      @VAR(controller) = false;
      @VAR(client).stop();
      @BREAK
     }
     else
     {
      fbcp::string s = fbcp::writeCommand(cmd);
      Serial.print(F("Sent: "));
      Serial.println(s.c_str());
      @VAR(client).print(s.c_str());
      if (cmd.command->code == fbcp::A_CLEAN.code)
      {
       @VAR(controller) = false;
       @VAR(client).stop();
       @BREAK
      }
     }
    }
    Serial.println(F("Disconnected"));
    
    mode = MODE_IDLE;
   }
  }
 }
}

@JOB (job_link_and_bumper)
{
 @MEMORY
 {
  @VAR(led:bool)
  @VAR(buz:bool)
 }
 
 @VAR(led) = false;
 @VAR(buz) = false;
 
 @WHILE
 {
  if (digitalRead(HIT))
  {
   bool left = false;
   bool right = false;
   if (
    leftMotor.getDirection()
    == Motor::Direction::DIR_FORWARD
   ) left = true;
   if (
    rightMotor.getDirection()
    == Motor::Direction::DIR_FORWARD
   ) right = true;
   if (left)
   {
    if (right)
    {
     leftMotor.stop();
     rightMotor.stop();
    }
    else
    {
     rightMotor.backward();
    }
   }
   else
   {
    if (right)
    {
     leftMotor.backward();
    }
   }
  }
  
  ledFreq = FREQ_FAST;
  
  switch (mode)
  {
   case MODE_IDLE:
   case MODE_SCAN:
   case MODE_NETCON:
   case MODE_SERCON:
   case MODE_STANDALONE:
    @VAR(led) = true;
    break;
   case MODE_GAME:
    @VAR(led) = !@VAR(led);
    break;
   default:
    @VAR(led) = false;
  }
  
  switch (mode)
  {
   case MODE_IDLE:
    @VAR(buz) = false;
    break;
   case MODE_SCAN:
    @VAR(buz) = !@VAR(buz);
    ledFreq = FREQ_SLOW;
    break;
   case MODE_NETCON:
    @VAR(buz) = !@VAR(buz);
    ledFreq = FREQ_FAST;
    break;
   case MODE_SERCON:
    @VAR(buz) = !@VAR(buz);
    ledFreq = FREQ_FAST;
    break;
   case MODE_STANDALONE:
    @VAR(buz) = true;
    break;
   case MODE_GAME:
    @VAR(buz) = !@VAR(led);
    ledFreq = FREQ_SLOW;
    break;
   default:
    @VAR(buz) = false;
  }
  
  digitalWrite(LED, @VAR(led));
  digitalWrite(BUZ, @VAR(buz));
  
  @CALL(wait;1000/ledFreq):null;
 }
}

@JOB (avoid_breaking)
{
 @WHILE {yield();}
}

§>
void setup()
{
§<
 // Initialise serial communication
 Serial.begin(115200);
 delay(10);
 
 Serial.println(F("Starting"));
 
 // Initialise pins
 pinMode(LED, OUTPUT);
 pinMode(BUZ, OUTPUT);
 pinMode(HIT, INPUT);
 
 analogWriteRange(100);
 pinMode(R_MOTOR, OUTPUT);
 analogWrite(R_MOTOR, 0);
 pinMode(L_MOTOR, OUTPUT);
 analogWrite(L_MOTOR, 0);
 pinMode(HIT, INPUT);

 // FBCP
 Serial.println(F("FBCP"));
§>
 /*
  * As already stated above, it's important to define a serial code
  * before starting any communications.
  */
 fbcp::serial = fbcp::ROBOT_PREFIX;
 fbcp::serial += serial;
 mode = MODE_IDLE;
§<
 fbcp::COMMAND_LINE cmd;
 cmd.command = &fbcp::Q_HIT;
 hitCmd = fbcp::writeCommand(cmd);
 cmd.command = &fbcp::Q_HEARTBEAT;
 kalCmd = fbcp::writeCommand(cmd);
 
 //ScheMo
 Serial.println(F("ScheMo"));
§>
 /*
  * This initializes both the scheduler and all variables containing
  * job/function/task data.
  * While @DECLARE should be the first directive to be written in the
  * source file and has only effect at compile time, @INIT should be
  * placed in such a way that its generated code is executed before
  * any other schemop-generated code.
  */
 @INIT
§<
 
 //WiFi
 WiFi.persistent(false);
 
§>
 /*
  * Here two ways to schedule a job are shown.
  * The first one uses a C++ function and adds a single job to the
  * scheduler queue, the second one uses a schemop directive and
  * schedules all tracked jobs (i.e. the ones defined with @JOB).
  */
 //schemo::schedule_job(job_network)
 @SCHEDULE_ALL
§<
 Serial.println(F("Jobs scheduled"));

§>
 /*
  * The following function actually starts the scheduler.
  * All code written inside functions, jobs or tasks is actually
  * executed inside this function.
  */
 schemo::start_cycle();
 /*
  * Code put here will be executed after ALL jobs have terminated.
  */
}

void loop() {}
§<
