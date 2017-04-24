#include <avr/interrupt.h>    //library for interrupts
#include <Keypad.h>           //library for keypad
#include <Password.h>         //library for password
#include <LiquidCrystal.h>      //library for LCD
#include <TimerThree.h>   //library for Timer3
#include <TimerOne.h>     //library for Timer1
#include <SPI.h>         //library for thermostat
#include <WiFi.h>       //library for wifi
#include <Adafruit_Sensor.h>  //library for thermostat
#include <DHT.h>        //library for thermostat

#define Door 2             //setting pin 2 to Door
#define Motion 3           //setting pin 3 to Motion
#define Thermo 5     //sets thermostat to pin 5
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

char ssid[] = "eir45165092-2.4G";      // your network SSID (name)
char pass[] = "db4twyse";   // your network password
int keyIndex = 0;                 // your network key Index number (needed only for WEP)

int status = WL_IDLE_STATUS; //wifi status

WiFiServer server(80); //start server on port 80
DHT dht(Thermo, DHTTYPE); //defiens what type of thermostat it is

const int buzzer = 35;    //buzzer to arduino pin 35
int Alarm_Status = 0;       //setting Alarm_Status to the initial value of 0
int Alarm_Active = 0;       //setting Alarm_Active to the initial value of 0
int PinPos = 11;            //this is the position for the pin entry to start on the lcd
int Warning_Alarm = 0;      //setting the warning counter to the initial value of 0
int zone = 0;               //setting the zone to the initial value of 0
int LDRn = A0; // select the input pin for the potentiometer
int ledPin = 13; // select the pin for the LED
int ldrValue = 0; // variable to store the value coming from the ldr
int heatingCount = 0; //set initial value of heatingCount to zero
int heatingManual = 0;  //set initial value of heatingManual to zero
int lightManual = 0; //set initial value of lightManual to zero
int lightCount = 0; //set initial value of lightCount to zero
int ldrled = 8; //pin for ldr led
int thermoled = 9; //pin for thermsotat led

const byte ROWS = 4; //four rows
const byte COLS = 3; //three columns
char keys[ROWS][COLS] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},      // delcares the valeus for the keypad
  {'*', '0', '#'}
};

byte rowPins[ROWS] = {29, 24, 25, 27}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {28, 30, 26}; //connect to the column pinouts of the keypad

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS ); //defines the keyapd

Password password = Password( "1234" );  //set password to 1234

LiquidCrystal lcd(36, 38, 46, 44, 42, 40);  //initialising the LCD

void setup() {
  // put your setup code here, to run once:
  noInterrupts();// kill interrupts until everybody is set up
  interrupts();//restart interrupts
  Serial.begin(9600);
  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    while (true);       // don't continue
  }

  String fv = WiFi.firmwareVersion();
  if ( fv != "1.1.0" ) //checks the version of wifi sheild and if needs to be updated 
    Serial.println("Please upgrade the firmware");

  // attempt to connect to Wifi network:
  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to Network named: ");
    Serial.println(ssid);                   // print the network name (SSID);

    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
    // wait 5 seconds for connection:
    delay(5000);
  }
  server.begin();                           // start the web server on port 80
  printWifiStatus();                        // you're connected now, so print out the status
  lcd.begin(16, 2);         //LCD size
  pinMode(ledPin, OUTPUT);    //setting the ledPIN as an output
  digitalWrite(ledPin, LOW);  //setting the ledPIN as low
  pinMode(ldrled, OUTPUT);    //setting the ledPIN as an output
  digitalWrite(ldrled, LOW);  //setting the ledPIN as low
   pinMode(thermoled, OUTPUT);    //setting the ledPIN as an output
  digitalWrite(thermoled, LOW);  //setting the ledPIN as low
  pinMode(Door, INPUT);       //setting door sensor as input
  digitalWrite(Door, HIGH);   //setting door sensor as high
  pinMode(Motion, INPUT);       //setting motion sensor as input
  digitalWrite(Motion, LOW);   //setting PIR as low
  pinMode(buzzer, OUTPUT); // Set buzzer - pin 35 as an output
  pinMode(Thermo, INPUT); // Set Thermo as an imput
  Timer3.initialize(5000000);         // initialize timer1, and set a 5 second period
  Timer3.attachInterrupt(checkSensors);  // attaches warningTone() as a timer overflow interrupt
  keypad.addEventListener(keypadEvent); //this is to jump to keypad event if a key is pressed
  displayCodeEntryScreen();// goes to this function first
}

void loop() {
  keypad.getKey();              //check if keypad is pressed
  WiFiClient client = server.available();
  if (client) {                             // if you get a client,
    Serial.println("new client");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        if (c == '\n') {                    // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();
            client.print("<HEAD>");
            client.println("<TITLE>SmartHome</title>");//webpage title
            client.println("<h1 align= center>The smart home project</h1>"); //set up of webpage
            client.print("</head>");
            client.print("<BODY>");
            // the content of the HTTP response follows the header:

            client.print("<li>Click <a href=\"/H\">here</a> turn the Alarm on<br></li>");
            client.print("<li>Click <a href=\"/L\">here</a> turn the Alarm off<br></li>");// turns on or off the alarm
            int LDR = analogRead(LDRn);
            client.print("The LDR reading is: ");
            client.print(LDR);// printing the reading of the ldr
            client.print("<br>");
            float t = dht.readTemperature(); //gets value of thermostat in terms of t
            float hic = dht.computeHeatIndex(t, false); // Compute heat index in Celsius (isFahreheit = false)
            client.print("Heat level is:   ");
            client.print(t); //prints the values coming from the sensor on the screen
            client.print(" degrees celcius<br>");
            client.print("<form action='/heatingON' method='get'>");
            client.print("Click to activate the heating for one hour<button type='submit'>Heating on</button><br>");// this turns on the heating
            client.print("</form>");
             client.print("<form action='/heatingOFF' method='get'>");
            client.print("Click to deactivate the heating for one hour<button type='submit'>Heating off</button><br>");// turns off the heating
            client.print("</form>");
            client.print("<form action='/lightON' method='get'>");
             client.print("<br>");
            client.print("Click to turn on the lighting <button type='submit'>Turn On Light</button><br>");// turns on the lights
            client.print("</form>");
            client.print("<form action='/lightOFF' method='get'>");
             client.print("<br>");
            client.print("Click to turn off the lighting <button type='submit'>Turn Off Light</button><br>");// turns off the lights
            client.print("</form>");
            client.print("</body>");
         //   client.print("<img src= ait.logo.jpg  style= width:304px;height:228px;>");
        
            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          }
          else {      // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        }
        else if (c != '\r') {    // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }

        // Check to see if the client request was "GET /H" or "GET /L":
        if (currentLine.endsWith("GET /H")) {
          Activate();               // GET /H activates the alarm
        }
        if (currentLine.endsWith("GET /L")) {
          Deactivate();                // GET /L deactivates the alarm
        }
        if (currentLine.endsWith("GET /heatingON?")) {
          heatingManual = 1;                // GET /heatingON? turns the LED on
          digitalWrite(thermoled, HIGH);
        }
        if (currentLine.endsWith("GET /lightON?")) {
          lightManual = 1;                // GET /heatingON? turns the LED on
          digitalWrite(ldrled, HIGH);
        }
        if (currentLine.endsWith("GET /lightOFF?")) {
          lightManual = 2;                // GET /lightOFF? turns the LED off
          digitalWrite(ldrled, LOW);
        }
        if (currentLine.endsWith("GET /heatingOFF?")) {
           heatingManual = 2;                // GET /heatingOFF? turns the LED off
          digitalWrite(thermoled, LOW);
        }
      }
    }
    // close the connection:
    client.stop();
    Serial.println("client disonnected");
  }

}

void Door_Interrupt()
{
  if (Alarm_Status == 0 && Alarm_Active == 1) //if the alarm is turned on but not triggered
  {
Door_Opened();      //jump to Door_Opened code
  }
}

void Door_Opened() {
  detachInterrupt(0);
  detachInterrupt(1);// deatches the interrupts so the alarm cant be triggered again
  zone = 1; // changes the value off zone so user will know what section was actiivated
  lcd.clear();
  lcd.setCursor(0, 0); // on top line of lcd
  lcd.print("Door Open");// lcd prints door open 
  lcd.setCursor(0, 1);// on bottom line
  lcd.print("Enter PIN: ");// lcd prints enter pin
  PinPos = 11;// sets the position to 11 so it dosent overwrite previous words
  Timer1.initialize(1000000);         // initialize timer1, and set a 1 second period
  Timer1.attachInterrupt(warningTone);  // attaches warningTone() as a timer overflow interrupt
}

void warningTone() {
  tone(buzzer, 250);// buzzer set at tone 250
  Warning_Alarm++;// warning alarm is set to 0 if it reaches 10 it goes to alarm triggered and detaches interrupt
  if (Warning_Alarm == 10) {
    Warning_Alarm = 0;
    Timer1.detachInterrupt();
    Alarm_Triggered();
  }
}

void Motion_Interrupt()
{
  if (Alarm_Status == 0 && Alarm_Active == 1) //if the alarm is turned on but not triggered
  {
Motion_detected();  //jump to Motion_detected code
  }
}

void Motion_detected() {
  detachInterrupt(1);
  zone = 2; // interrupt detached zone set to 2 buzzer is set at tone 2000 and goes to alarm triggered
  tone(buzzer, 2000);
  Alarm_Triggered();
}

void keypadEvent(KeypadEvent Key) {
  switch (keypad.getState()) {
    case PRESSED:                   //once a key is pressed jump to here
      lcd.setCursor((PinPos++), 1);   //sets the position on LCD for pin entry and moves each time a key is pressed
      switch (Key) {
        case '*':                 //* is to reset password attempt
          password.reset();         //clears stored password
          lcd.clear();              //clears LCD
          Invalid();                //peeforms function
          delay(1500);
          displayCodeEntryScreen();
          break;
        case '#':                 //# is to validate password
          Check_Pin();              //jump to function
          break;
        default:
          password.append(Key);   //stores the character that has been pressed
          lcd.print("*");
      }        //print to LCD
  }
}

void displayCodeEntryScreen()    // Dispalying start screen for users to enter PIN
{
  lcd.clear();
  lcd.setCursor(0, 0);// the start of the alarm code is here waits for a user to input pin and set the alarm
  lcd.print("Welcome");
  lcd.setCursor(0, 1);
  lcd.print("Enter PIN: ");
  PinPos = 11;
}

void Check_Pin() {
  if (password.evaluate()) {           //check is pin entered matched delcared password and if it does then execute if statements
    password.reset();
    if ((Alarm_Active == 0) && (Alarm_Status == 0)) {  //if alarm is off
      Activate();                   //activate alarm
    } else if ((Alarm_Active == 1) && (Alarm_Status == 1)) { //if alarm is on and activated
      Deactivate();                 //deactivate Alarm
    } else if ((Alarm_Active == 1) && (Alarm_Status == 0)) { //if alarm is on but not activated
      Deactivate();
    }
  } else
    Invalid();                  //if pin enetered is incorrect display invalid code
}

void Activate() {     // Activate the system if correct PIN entered and display message on the screen
  attachInterrupt(0, Door_Interrupt, RISING);   //attach INT0 to Door_Interrupt on rising edge
  attachInterrupt(1, Motion_Interrupt, RISING); //attach INT1 to Motion_Interrupt on rising edge
  Alarm_Active = 1;
  password.reset();
  lcd.setCursor(0, 0);
  lcd.print("Activating");
  int j = 10;
  for (int i = 0; i < 4; i++) { //for loop to have the dot moving across the LCD
    lcd.setCursor(j, 0);
    lcd.print(".");
    j++; // once a number has been entered the lcd prints out "." and then moves on one space using j++
    delay(1000);
    if (j > 15) {
      lcd.setCursor(10, 0);
      lcd.print("       "); //clears the dots and starts again
      delay(1000);
      j = j - 6; 
    }
  }
  lcd.setCursor(0, 0);
  lcd.print("SYSTEM ACTIVE!");
  delay(2000); // prints system active and waits 2 seconds before clearing the lcd
  lcd.clear();
  PinPos = 11;
  displayCodeEntryScreen(); // jumps to this function
}

void Deactivate() {   // Deactivate the system if correct PIN entered and display message on the screen
  lcd.clear();
  noTone(buzzer);// stops the buzzer
  Timer1.detachInterrupt(); // detaches the timer interrupt
  Alarm_Status = 0; // resets the status and active settings to 0
  Alarm_Active = 0;
  password.reset(); // resets the password 
  lcd.setCursor(0, 0);
  lcd.print("DEACTIVATED!");// prints out deactivated
  delay(3000);
  lcd.clear();
  PinPos = 11;
  displayCodeEntryScreen();// goes to this function
}

void Invalid() {
  password.reset();// resets password
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Invalid Pin");// prints invaid pin
  lcd.setCursor(0, 1);
  lcd.print("Enter Pin: "); // asksuser for the pin and sets the pin position to 11
  PinPos = 11;
}

void Alarm_Triggered() {
  tone(buzzer, 1000);
  password.reset();       //clears any charaters that have been stored for the password
  Alarm_Status = 1;
  if (zone == 1) { // if zone is 1
    lcd.setCursor(0, 0);     //set cursor to first row first charater
    lcd.print("Alarm Door");    //print to LCD
    lcd.setCursor(0, 1);         //set cursor to second row first charater
    lcd.print("Enter PIN: ");
  }    //print to LCD

  if (zone == 2) { // if zone is 2
    lcd.setCursor(0, 0);
    lcd.print("Alarm Motion"); // print to lcd
    lcd.setCursor(0, 1);
    lcd.print("Enter PIN: "); // asks user for pin again
  }
}

void checkSensors() {// functions that the timers will run
  checkLDR();
  checkThermo();
}

void checkLDR() {
  ldrValue = analogRead(LDRn); //gets the value of the ldr
  if (lightManual == 0) {// if lightmaunal is 0
    if (ldrValue < 90) { // if ldr value is less than 90 turn on the led 
      digitalWrite(ldrled, HIGH);
    } else digitalWrite(ldrled, LOW);
  } else if (lightManual == 1) { // if light manual is 1
    digitalWrite(ldrled, HIGH);// turn on led
    lightCount++; 
    if (lightCount == 3) {// if light count is 3 change both manual and count to 0 .. this code is for the webpage to overide the timers
      lightCount = 0;
      lightManual = 0;
    }
  }else if (lightManual == 2) { // if lightmanual is 2 turn off led
    digitalWrite(ldrled, LOW);
    lightCount++;
    if (lightCount == 3) { //if count is 3 reset count and manual to 0
      lightCount = 0;
      lightManual = 0;
    }
  }
}
void checkThermo() {
  float temp = dht.readTemperature(); // gets the temeperature and stores it as a float
  float hic = dht.computeHeatIndex(temp, false); // Compute heat index in Celsius (isFahreheit = false)
  Serial.println(temp); // serial monitor prints the temperature
  if (heatingManual == 0) {
    if (temp < 18) { // if manual is zero and temp is under 18 tunr on the led
      digitalWrite(thermoled, HIGH);
    } else digitalWrite(thermoled, LOW);
  } else if (heatingManual == 1) {// if manaul is 1 turn on led and change to 2
    digitalWrite(thermoled, HIGH);
    heatingCount++;
    if (heatingCount == 3) { // if count is 3 reset both Count and maunal to 0
      heatingCount = 0;
      heatingManual = 0;
    }
  } else if (heatingManual == 2) { // if manual is 2  turn off led change Count to 3
    digitalWrite(thermoled, LOW);
    heatingCount++;
    if (heatingCount == 3) { //if count is 3 change both count and manual to 0
      heatingCount = 0;
      heatingManual = 0;
    }
  }
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID()); // prints out the ssid on serila monitor

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");// prints out the ip address of the webpage
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi); // prints out the signal strength
  Serial.println(" dBm");
  // print where to go in a browser:
  Serial.print("To see this page in action, open a browser to http://"); // tells you the webpage to go to to view it
  Serial.println(ip);
}



