// based on NTP clock with ESP8266 on commom chatode 7 segment led display with webpage control -  https://github.com/tehniq3/NTPclock_7seg_cc
// used base schematic from http://www.valvewizard.co.uk/iv18clock.html 
// NTP clock, used info from // https://lastminuteengineers.com/esp8266-ntp-server-date-time-tutorial/
// NTP info: https://randomnerdtutorials.com/esp8266-nodemcu-date-time-ntp-client-server-arduino/
// changed for common cathode multiplexed led display by Nicu FLORICA (niq_ro)
// added real temperature and humidity measurements with DHT22 
// added 12-hour format - https://en.wikipedia.org/wiki/12-hour_clock
// added web control for Daylight Saving Time (or summer time) - https://github.com/tehniq3/NTP_DST_ESP8266
// added web control for TimeZome: https://nicuflorica.blogspot.com/2021/02/ceas-gps-cu-reglaj-ora-locala-4.html
// added WiFi Manager, see http://nicuflorica.blogspot.com/2019/10/configurare-usoara-conectare-la-retea.html
// display IP on led display
// changed also by Nicu FLORICA (niq_ro) to TM1637 display in 15.10.2022, Craiova 
// display state AP or IP in TM1637 display for easy know the IP of local control webpage (eg: 192.168.3.1)
// v.1a2a4 - small updates for issue at date chamge after change TimeZone or DST
// v.1a3 - solved issue with special TimeZone (-9., 2.75,etc)
// v.1a4 - added brightness control from webpage
// v.1b - added automatic brightness due to sunrise/sunset using https://github.com/jpb10/SolarCalculator library
// v.2.0 - added thermometer & hygromether with DHT sensor using DHTesp library
// v.2.a - added button on webpage to set the info for night (just clock, info just night, permanent) + read DHT more rarely
// v.2.a.1 - corrected changes from 23:59 to 0:00 (sometimes remains 20:00)and reading data every hour
// v.3.0 - changed display with 8-digit 7-segment with MAX7219 driver
// v.3.a - flashing seconds

#include <SolarCalculator.h> //  https://github.com/jpb10/SolarCalculator
#include <EEPROM.h>
#include <DS3231.h>       // For the DateTime object, originally was planning to use this RTC module: https://www.arduino.cc/reference/en/libraries/ds3231/
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>  //https://github.com/tzapu/WiFiManager                                                                                                              
#include "DHTesp.h" // Click here to get the library: https://www.arduinolibraries.info/libraries/dht-sensor-library-for-es-px
#include "LedControl.h"  https://github.com/wayoda/LedControl/releases -> https://github.com/wayoda/LedControl/releases/download/1.0.6/LedControl-1.0.6.zip


#define DHTPIN 14 // GPIO14 = D5    // what pin we're connected the DHT output

#define DIN  12 // GPIO12 = D6   // Definitions using https://randomnerdtutorials.com/esp8266-pinout-reference-gpios/ 
#define LOAD 13 // GPIO13 = D7 
#define CLK  15 // GPIO15 = D8      

LedControl lc=LedControl(D6,D8,D7,1); // D6-->DIN, D8-->Clk, D7-->LOAD, no.of devices is 1
DHTesp dht;

float TIMEZONE = 0; // Define your timezone to have an accurate clock (hours with respect to GMT +2)
// "PST": -7 
// "MST": -6 
// "CST": -5 
// "EST": -4 
// "GMT": 0 

int timezone0 = 16; 
int timezone1 = 0;
float diferenta[38] = {-12., -11.,-10.,-9.5,-9.,-8.,-7.,-6.,-5.,-4.,-3.5,-3.,-2.,-1.,0,
                      1.,2.,3.,3.5,4.,4.5,5.,5.5,5.75,6.,6.5,7.,8.,8.75,9.,9.5,10.,10.5,
                      11.,12.,12.75,13.,14};   // added manualy by niq_ro
#define adresa  100  // adress for store the
byte zero = 0;  // variable for control the initial read/write the eeprom memory

const long utcOffsetInSeconds =  2*3600;  // +3 hour

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);


// Definitions using https://randomnerdtutorials.com/esp8266-pinout-reference-gpios/ 

// DST State
byte DST = 0;
byte DST0 = 0;
String oravara = ""; //

int ora, minut, secunda, rest;
int ora0, minut0;
byte ziua, luna, an;
unsigned long ceas = 0;
unsigned long ceas0 = 0;
unsigned long tpcitire = 0;

byte citire = 0;
byte citire2 = 0;
byte refres = 0;

unsigned long epochTime;
byte tensHour, unitsHour, tensMin, unitsMin, tensSec, unitsSec;

String ip = "";
byte lungip = 0;
int aipi[20];

int citire3 = 0; // variable to read data

int A,B,H;
byte pm =0;
byte h1224;  // 12/24-hour format
byte h12240 = 0;
String formatora = "";
byte stergere = 0;
byte intensitate11 = 2; // brightness for day
byte intensitate10 = 2;
byte intensitate21 = 0; // brightness for night
byte intensitate20 = 0;

// Web Host
String header; // Variable to store the HTTP request
unsigned long currentTime = millis(); // Current time
unsigned long previousTime = 0; // Previous time
const long timeoutTime = 2000; // Define timeout time in milliseconds (example: 2000ms = 2s)

// Hour State
// * DateTime objects cannot be modified after creation, so I get the current DateTime and use it to create the default 7AM alarm *
DateTime today = DateTime(timeClient.getEpochTime());

WiFiServer server(80); // Set web server port number to 80

int aipi2[4][4];
int aipi0[4][4]= {1,9,2,0,
                  1,6,8,0,
                  0,0,4,0,
                  0,0,1,0};
int j, k;


  // Location - Craiova: 44.317452,23.811336
  double latitude = 44.31;
  double longitude = 23.81;
  int utc_offset = 3;

double transit, sunrise, sunset;
char str[6];
int ora1, ora2, oraactuala;
int m1, hr1, mn1, m2, hr2, mn2; 
unsigned long tpcitire4;
int digitoneT, digittwoT;
int digitoneH, digittwoH;
int temperature, humidity;

byte infoclock = 0;  // 0 = just clock, 1 - auto (just clock in night / full info just in day), 2 - full (day/night)
byte infoclock0 = 0;
unsigned long tpcitire5; // temporary time for read DHT sensor
String infoclock1 = "";
byte noapte = 1;  // 1 - night, 0 - day
byte extrainfo = 0;
 
void setup(){
  EEPROM.begin(512);  //Initialize EEPROM  - http://www.esp8266learning.com/read-and-write-to-the-eeprom-on-the-esp8266.php
  Serial.begin(115200);   // Setup Arduino serial connection
  Serial.println(" ");
  Serial.println("-------------------");

  lc.shutdown(0,false);    // Enable display
  lc.setIntensity(0,1);   // Set brightness level (0 is min, 15 is max)
  lc.clearDisplay(0);      // Clear display register
  
  Serial.println("-----clock------");

zero = EEPROM.read(adresa - 1); // variable for write initial values in EEPROM
if (zero != 16)
{
EEPROM.write(adresa - 1, 16);  // zero
EEPROM.write(adresa, timezone0); // time zone (0...37 -> -12...+12) // https://en.wikipedia.org/wiki/Coordinated_Universal_Time
EEPROM.write(adresa + 1, 1);  // 0 => winter time, 1 => summer time
EEPROM.write(adresa + 2, 0);  // 0 => auto 12/24-hour format, 1 => 12-hour format, 2 - 24-hour format
EEPROM.write(adresa + 3, 7);  // day: 7, BRIGHT_DARKEST = 0,BRIGHTEST = 15;
EEPROM.write(adresa + 4, 0);  // day: 0, BRIGHT_DARKEST = 0,BRIGHTEST = 15;
EEPROM.write(adresa + 5, 1); // 0 = just clock, 1 - auto (just clock in night / full info just in day), 2 - full (day/night)
EEPROM.commit();    //Store data to EEPROM
} 

// read EEPROM memory;
timezone0 = EEPROM.read(adresa);  // timezone +12
timezone1 = timezone0;
TIMEZONE = (float)diferenta[timezone0];  // convert in hours
DST = EEPROM.read(adresa+1);
DST0 = DST;
h1224 = EEPROM.read(adresa+2);
intensitate11 = EEPROM.read(adresa+3);
intensitate10 = intensitate11;
intensitate21 = EEPROM.read(adresa+4);
intensitate20 = intensitate21;
infoclock = EEPROM.read(adresa+5);

if (h1224 > 3) h1224 = 0;  // if hour format is invalit set to "auto"
if (h1224 == 0) formatora = " auto  ";
if (h1224 == 1) formatora = "12-hour";
if (h1224 == 2) formatora = "24-hour";

if (intensitate11 > 7) intensitate11 = 2; // typical value
if (intensitate21 > 7) intensitate21 = 0; // typical value

if (infoclock > 2) infoclock = 1; // typical value
if (infoclock == 0) infoclock1 = "never (just clock)";
if (infoclock == 1) infoclock1 = "just on daytime";
if (infoclock == 2) infoclock1 = "always (day/night)";

            
if (DST == 0) oravara = "off"; 
else oravara = "on"; 
Serial.println("==============");
Serial.print("TimeZone: ");
Serial.print(TIMEZONE);
Serial.print(" / ");
Serial.println(diferenta[timezone0]);
Serial.print("DST: ");
Serial.println(DST);
Serial.println("==============");

//WiFiManager
//Local intialization. Once its business is done, there is no need to keep it around
WiFiManager wifiManager;

Serial.println("Wi-Fi: AutoConnectAP");
Serial.println("IP: 192.168.4.1     ");

 lc.setIntensity(0,intensitate21);   // Set the brightness:
//BRIGHT_TYPICAL = 2, BRIGHT_DARKEST = 0,BRIGHTEST = 15;

lc.clearDisplay(0);

// Autodetect is not working reliable, don't use the following line
// dht.setup(17);
// use this instead: 
dht.setup(DHTPIN, DHTesp::DHT22); // Connect DHT sensor to GPIO 17

/*
 *   lc.setDigit(0,7,2,false); // 1st display, 7th digit (leftside), number 2, no dp
 *   lc.setDigit(0,6,3,true);  // 1st display, 6th digit (leftside), number 3, with dp
 *   lc.setRow(0,1,B00001110); // 1st display, 1st digit (righside), no dp,a,b,c,d,e,f,g
 */

   lc.setRow(0,7,B00000000); // free digit
   lc.setRow(0,6,B00000000); // free digit
   lc.setRow(0,5,B00000000); // free digit
   lc.setRow(0,4,B01110111); // A
   lc.setRow(0,3,B01100111); // P 
   lc.setRow(0,2,B00000000); // free digit
   lc.setRow(0,1,B00000000); // free digit
   lc.setRow(0,0,B00000000); // free digit 
delay(100);

Serial.println("------- ");

  //reset settings - for testing
  //wifiManager.resetSettings();

  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  wifiManager.setTimeout(180);
  
  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if(!wifiManager.autoConnect("AutoConnectAP")) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  } 

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");

    // Print local IP address
  Serial.println("\nWiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

ip = WiFi.localIP().toString();
lungip = ip.length();

 Serial.print(lungip);
 Serial.println("chars");

aipi[lungip+1];

j = 3;
k = 3;
 
for (byte i=lungip; i>0; i--)  
{ 
  if (ip[i] == '.')
  {
    aipi[i] = 18;
    j--;
    k = 3;
    Serial.print(".");
  }
  aipi2[j][k] = ip[i] - 48;
  Serial.print(ip[i] - 48);
  Serial.print(" j = "); 
  Serial.print(j); 
  Serial.print(" k  = "); 
  Serial.print(k);  
  Serial.println(" "); 
  k--;
 delay(1);
}

aipi2[0][0] = ip[0]-48;  //ASCII table
Serial.print(ip[0]);
Serial.print(" j = "); 
Serial.print(0); 
Serial.print(" k  = "); 
Serial.print(0);  
Serial.println(" "); 

Serial.println("-------//--------"); 

//delay(2000);

for (byte j=0; j<4; j++)  
{ 
 for (byte k=0; k<3; k++)
{  
Serial.print(aipi2[j][k]);
delay(4);
}
Serial.print("-");
}
Serial.println("recovered IP");
//delay(3000);

 lc.clearDisplay(0);  
  
k = 0;
for (byte j=0; j<4; j++)  
{ 
   lc.setRow(0,7,B00000000); // free digit
   lc.setRow(0,6,B00000000); // free digit
   lc.setRow(0,5,B00000000); // free digit
   lc.setRow(0,4,B00000000); // free digit
   lc.setRow(0,3,B00000000); // free digit
   if (aipi2[j][k] == 0)
    lc.setRow(0,2,B00000000); // free digit
     else
    lc.setDigit(0,2,aipi2[j][k],false);  
   if ((aipi2[j][k] == 0) and (aipi2[j][k+1] == 0))
    lc.setRow(0,1,B00000000); // free digit
     else
    lc.setDigit(0,1,aipi2[j][k+1],false);  
   lc.setDigit(0,0,aipi2[j][k+2],true);
delay(3000);
}

  timeClient.begin();
  if (DST == 1) 
  timeClient.setTimeOffset((TIMEZONE+1)*3600); // Offset time from the GMT standard
  else
  timeClient.setTimeOffset(TIMEZONE*3600); // Offset time from the GMT standard
  timeClient.update(); // Update the latest time from the NTP server
  server.begin(); // Start web server!

iaOra();
delay(1000);
iaData();
 lc.clearDisplay(0);      // Clear display register
 humidity = dht.getHumidity();
 temperature = dht.getTemperature();
Soare();
luminita();
}

void loop(){

  timeClient.update(); // Update the latest time from the NTP server
  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    currentTime = millis();
    previousTime = currentTime;
    while (client.connected() && currentTime - previousTime <= timeoutTime) { // loop while the client is connected
      currentTime = millis();         
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          if (currentLine.length() == 0) { // If the current line is blank, you got two newline characters in a row. That's the end of the client HTTP request, so send a response:
            client.println("HTTP/1.1 200 OK"); // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            client.println("Content-type:text/html"); // and a content-type so the client knows what's coming, then a blank line:
            client.println("Connection: close");
            client.println();
            
            if (header.indexOf("GET /vara/on") >= 0) { // If the user clicked the alarm's on button
              Serial.println("Daylight saving time (DST) was activated !");
              oravara = "on";
              lc.clearDisplay(0); 
              timeClient.setTimeOffset((TIMEZONE+1)*3600); // Offset time from the GMT standard
              client.println("<meta http-equiv='Refresh' content=0;url=//" + WiFi.localIP().toString()+ ":80/>");
              iaOra();
            } 
            else if (header.indexOf("GET /vara/off") >= 0) { // If the user clicked the alarm's off button
              Serial.println("Daylight saving time (DST) was deactivated !");
              oravara = "off";
              lc.clearDisplay(0);  
              timeClient.setTimeOffset((TIMEZONE+0)*3600); // Offset time from the GMT standard
              client.println("<meta http-equiv='Refresh' content=0;url=//" + WiFi.localIP().toString()+ ":80/>");
              iaOra();
            }
              if (header.indexOf("GET /TZplus") >= 0) { // If the user clicked the TimeZone change button
              Serial.println("TimeZone was changed !");
              timezone0 = timezone0 + 1;
              lc.clearDisplay(0);  
              if (timezone0 > 37) timezone0 = 0;
              TIMEZONE = (float)diferenta[timezone0];  // convert in hours
              timeClient.setTimeOffset((TIMEZONE+DST)*3600); // Offset time from the GMT standard
            client.println("<meta http-equiv='Refresh' content=0;url=//" + WiFi.localIP().toString()+ ":80/>");
            } 

            if (header.indexOf("GET /TZminus") >= 0) { // If the user clicked the TimeZone change button
              Serial.println("TimeZone was changed !");
              timezone0 = timezone0 - 1;
              lc.clearDisplay(0);  
              if (timezone0 < 0) timezone0 = 37;
              TIMEZONE = (float)diferenta[timezone0];  // convert in hours
              timeClient.setTimeOffset((TIMEZONE+DST)*3600); // Offset time from the GMT standard
              client.println("<meta http-equiv='Refresh' content=0;url=//" + WiFi.localIP().toString()+ ":80/>");
            }

            if (header.indexOf("GET /HourFormat") >= 0) { // If the user clicked the HourFormat change button
              Serial.println("HourFormat was changed !");
              h1224 = h1224 + 1;
              lc.clearDisplay(0); 
              if (h1224 > 2) h1224 = 0;
              if (h1224 == 0) formatora = " auto  ";
              if (h1224 == 1) formatora = "12-hour";
              if (h1224 == 2) formatora = "24-hour";
              client.println("<meta http-equiv='Refresh' content=0;url=//" + WiFi.localIP().toString()+ ":80/>");
            } 

            if (header.indexOf("GET /infos") >= 0) { // If the user clicked the HourFormat change button
              Serial.println("HourFormat was changed !");
              infoclock = infoclock + 1;
              lc.clearDisplay(0); 
              if (infoclock > 2) infoclock = 0;
              if (infoclock == 0) infoclock1 = "never (just clock)";
              if (infoclock == 1) infoclock1 = "just on daytime";
              if (infoclock == 2) infoclock1 = "always (day/night)";
              client.println("<meta http-equiv='Refresh' content=0;url=//" + WiFi.localIP().toString()+ ":80/>");
            } 

              if (header.indexOf("GET /Refreshinfo") >= 0) { // If the user clicked the "Refresh Info" button
              Serial.println("Clock was updated !");
            //  client.println("<meta http-equiv='Refresh' content=0; url=//" + WiFi.localIP().toString()+ ":80/>");
            }
            
            else if (header.indexOf("GET /time") >= 0) { // If the user submitted the time input form
              // Strip the data from the GET request
              int index = header.indexOf("GET /time");
              String timeData = header.substring(index + 15, index + 22);
              lc.clearDisplay(0);  
      
              Serial.println(timeData);
              // Update our alarm DateTime with the user selected time, using the current date.
              // Since we just compare the hours and minutes on each loop, I do not think the day or month matters.
              DateTime temp = DateTime(timeClient.getEpochTime()); 
         //     client.println("<meta http-equiv='Refresh' content=0;url=//" + WiFi.localIP().toString()+ ":80/>");
            }

            if (header.indexOf("GET /IZminus") >= 0) { // If the user clicked the Brightness change button
              Serial.println("Day Brightness was changed !");
              intensitate11 = intensitate11 - 1;
              if (intensitate11 > 15) intensitate11 = 0;
              lc.clearDisplay(0); 
              lc.setIntensity(0,intensitate11);   // Set the brightness
              tpcitire4 = millis();
              client.println("<meta http-equiv='Refresh' content=0;url=//" + WiFi.localIP().toString()+ ":80/>");
            }

            if (header.indexOf("GET /IZplus") >= 0) { // If the user clicked the Brightness change button
              Serial.println("Day Brightness was changed !");
              intensitate11 = intensitate11 + 1;
              if (intensitate11 > 15) intensitate11 = 15;
              lc.clearDisplay(0); 
              lc.setIntensity(0,intensitate11);   // Set the brightness
              tpcitire4 = millis();
              client.println("<meta http-equiv='Refresh' content=0;url=//" + WiFi.localIP().toString()+ ":80/>");
            }

            if (header.indexOf("GET /INminus") >= 0) { // If the user clicked the Brightness change button
              Serial.println("Night Brightness was changed !");
              intensitate21 = intensitate21 - 1;
              if (intensitate21 > 15) intensitate21 = 0;
              lc.clearDisplay(0); 
              lc.setIntensity(0,intensitate21);   // Set the brightness
              tpcitire4 = millis();
              client.println("<meta http-equiv='Refresh' content=0;url=//" + WiFi.localIP().toString()+ ":80/>");
            }

            if (header.indexOf("GET /INplus") >= 0) { // If the user clicked the Brightness change button
              Serial.println("Night Brightness was changed !");
              intensitate21 = intensitate21 + 1;
              if (intensitate21 > 15) intensitate21 = 15;
              lc.clearDisplay(0); 
              lc.setIntensity(0,intensitate21);   // Set the brightness
              tpcitire4 = millis();
              client.println("<meta http-equiv='Refresh' content=0;url=//" + WiFi.localIP().toString()+ ":80/>");
            }

          
            // Display the HTML web page
            // Head
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=0\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            client.println("<link rel=\"stylesheet\" href=\"//stackpath.bootstrapcdn.com/bootstrap/4.4.1/css/bootstrap.min.css\">"); // Bootstrap
          //  client.println("<meta http-equiv='Refresh' content=0;url=//" + WiFi.localIP().toString()+ ":80/>");
            client.println("</head>");
            
            // Body
            client.println("<body>");
            client.println("<h1 class=\"text-center mt-3\"NTP / DSP Clock</h1>"); // Title

            // Current Time
            client.print("<h1 class=\"text-center\">"); 
            client.print(timeClient.getFormattedTime());
            client.println("</h1>");
            client.print("<h4 class=\"text-center\">"); 
            client.print("(last update for clock)"); 
            client.println("<a href=\"/Refreshinfo\"><button class=\"btn btn-sm btn-warning\">Refresh info (clock)</button></a></p>");
            client.println("</h4>");
            
            // Display sunrise / sunset
               client.print("<h3 class=\"text-center\">Sunrise: " + String(hr1));
               client.println(":" + String(mn1));
               client.println(" / Sunset: " + String(hr2));
               client.println(":" + String(mn2) + "</h3>");

              client.println("</br>");
              client.println("</br>");
              
             // Display current state, and ON/OFF buttons for DST 
            client.println("<h2 class=\"text-center\">Daylight Saving Time - " + oravara + "</h2>");
            if (oravara=="off") {
              client.println("<p class=\"text-center\"><a href=\"/vara/on\"><button class=\"btn btn-sm btn-danger\">ON</button></a></p>");
              DST = 0;
            }
            else {
              client.println("<p class=\"text-center\"><a href=\"/vara/off\"><button class=\"btn btn-success btn-sm\">OFF</button></a></p>");
              DST = 1;
            }

            // Display TimeZone state, and button for increase TimeZone 
            client.println("<h2 class=\"text-center\">TimeZone = " + String(TIMEZONE) + "</h2>");
            client.println("<p class=\"text-center\"><a href=\"/TZminus\"><button class=\"btn btn-sm btn-info\">decrease TimeZone (-)</button></a>");
            client.println("<a href=\"/TZplus\"><button class=\"btn btn-sm btn-warning\">increase TimeZone (+)</button></a></p>");

            // Display 12/24-hour format state, and button for increase 12/24-hout format 
            client.println("<h2 class=\"text-center\">HourFormat = " + formatora + "</h2>"); 
            client.println("<p class=\"text-center\"><a href=\"/HourFormat\"><button class=\"btn btn-sm btn-info\">change 12/24-hour format</button></a></p>");

            // Display brightness state, and button for increase/decrease brightness 
            client.println("<h2 class=\"text-center\">Day Brightness = " + String(intensitate11) + "</h2>");
            client.println("<p class=\"text-center\"><a href=\"/IZminus\"><button class=\"btn btn-sm btn-info\">decrease brightness (-)</button></a>");
            client.println("<a href=\"/IZplus\"><button class=\"btn btn-sm btn-warning\">increase brightness (+)</button></a></p>");
            client.println("<h2 class=\"text-center\">Night Brightness = " + String(intensitate21) + "</h2>");
            client.println("<p class=\"text-center\"><a href=\"/INminus\"><button class=\"btn btn-sm btn-info\">decrease brightness (-)</button></a>");
            client.println("<a href=\"/INplus\"><button class=\"btn btn-sm btn-warning\">increase brightness (+)</button></a></p>");

            // DHT sensor info
            client.println("<h2 class=\"text-center\">DHT22 sensor: ");
            client.println(temperature);
            client.println("<sup>0</sup>C / ");
            client.println(humidity);
            client.println("%RH</h2>");      

            // Display extra info (date/year,temperature,humidity)
            client.println("<h2 class=\"text-center\">Extra info (date/year,temperature,humidity) = " + infoclock1 + "</h2>");
            client.println("<p class=\"text-center\"><a href=\"/infos\"><button class=\"btn btn-sm btn-info\">change extra info format</button></a></p>");

            client.println("</body></html>");
            client.println(); // The HTTP response ends with another blank line
            break; // Break out of the while loop
            
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    
    header = ""; // Clear the header variable
    client.stop(); // Close the connection
    Serial.println("Client disconnected.");
    Serial.println("");
  }

if(DST == 1)
timeClient.setTimeOffset((TIMEZONE+1)*3600); // Offset time from the GMT standard
else
timeClient.setTimeOffset(TIMEZONE*3600); // Offset time from the GMT standard

if (millis() - tpcitire > 1000)
{
iaOra();
tpcitire = millis();
}

if (millis() - tpcitire4 > 5000)
{
luminita();
tpcitire4 = millis();
}

if ((minut == 0) and (citire3 == 0))  // read day, mounth and year every hour
{
iaData();
Soare();
citire3 = 1;
lc.clearDisplay(0);
}
if ((minut > 1) and (citire3 == 1))  // reseting variable for reading the data every hours
{
  citire3 = 0;
}

// 0 = just clock, 1 - auto (just clock in night / full info just in day), 2 - full (day/night)

if (infoclock == 1)
{
if (noapte == 0)  // day
  extrainfo = 1; 
else              // night
  extrainfo = 0;
}
if (infoclock == 2) extrainfo = 1;
if (infoclock == 0) extrainfo = 0;

if (extrainfo == 1)
{
if (secunda <= 50)  // hour
 {
  if (h1224 == 0)
  {
  if (minut % 2 == 0) displayTime();
     else displayTime12();
  }
  else 
  if (h1224 == 1) displayTime12();
  else  displayTime();
 }
else
 if ((secunda > 50) and (secunda <= 55))  // date or humidity
 {
 if (minut % 2 == 0)
 {
  if (h1224 == 0)
  {
  if (minut % 2 == 0) displayTime();
     else displayTime12();
  }
  else 
  if (h1224 == 1) displayTime12();
  else  displayTime();
 }
  else                                    // temperature
  {
    afisareT();
    if (millis() - tpcitire5 > 100000)
     {
      tpcitire5 = millis();
      humidity = dht.getHumidity();
      temperature = dht.getTemperature();
     }
  }
 }
else
 {
  if (minut % 2 == 0) displayDate();
  else afisareH();
  stergere = 0;
 }
}
else
{
  if (h1224 == 0)
  {
  if (minut % 2 == 0) displayTime();
     else displayTime12();
  }
  else 
  if (h1224 == 1) displayTime12();
  else  displayTime(); 
}
 
if ((stergere == 0) and (secunda == 0))
{
    lc.clearDisplay(0);
    stergere = 1;
}

if (DST0 != DST)
{
timeClient.setTimeOffset((TIMEZONE+DST)*3600);
EEPROM.write(adresa + 1, DST);  // 1 => summer format, 0 => winter format 
EEPROM.commit();    //Store data to EEPROM 
Serial.print("Write in EEPROM: DST = ");
Serial.println(DST);
iaData();
Serial.println("==============");
Serial.print("TimeZone: ");
Serial.print(TIMEZONE);
Serial.print(" / ");
Serial.println(diferenta[timezone0]);
Serial.print("DST: ");
Serial.println(DST);
Serial.println("==============");
Soare();
}
DST0 = DST;

if (h12240 != h1224)
{
EEPROM.write(adresa + 2, h1224);  // 0 => auto 12/24-hour format, 1 => 12-hour format, 2 - 24-hour format
EEPROM.commit();    //Store data to EEPROM 
Serial.print("Write in EEPROM: h1224 = "); 
Serial.println(h1224);
}
h12240 = h1224;

if (infoclock0 != infoclock)
{
EEPROM.write(adresa + 5, infoclock);  // 0 = just clock, 1 - auto (just clock in night / full info just in day), 2 - full (day/night)
EEPROM.commit();    //Store data to EEPROM 
Serial.print("Write in EEPROM: infoclock = "); 
Serial.println(infoclock);
}
infoclock0 = infoclock;


if (timezone1 != timezone0)
{
EEPROM.write(adresa, timezone0);  // new time zone
EEPROM.commit();    //Store data to EEPROM 
Serial.print("Write in EEPROM: timezone = ");
Serial.println(timezone0);
iaData();
Serial.println("==============");
Serial.print("TimeZone: ");
Serial.print(TIMEZONE);
Serial.print(" / ");
Serial.println(diferenta[timezone0]);
Serial.print("DST: ");
Serial.println(DST);
Serial.println("==============");
Soare();  
}
timezone1 = timezone0;

if (intensitate10 != intensitate11)
{
EEPROM.write(adresa + 3, intensitate11);  // brightness
EEPROM.commit();    //Store data to EEPROM 
Serial.print("Write in EEPROM: day brightness = "); 
Serial.println(intensitate11);
Soare();
}
intensitate10 = intensitate11;

if (intensitate20 != intensitate21)
{
EEPROM.write(adresa + 4, intensitate21);  // brightness
EEPROM.commit();    //Store data to EEPROM 
Serial.print("Write in EEPROM: night brightness = "); 
Serial.println(intensitate21);
Soare();
}
intensitate20 = intensitate21;

}   //End of main program loop


void displayTime(){
    byte tensHour = ora / 10; //Extract the individual digits
    byte unitsHour = ora % 10;
    byte tensMin = minut / 10;
    byte unitsMin = minut % 10;
    byte tensSec = secunda / 10;
    byte unitsSec = secunda % 10;

  if(tensHour == 0)  // hour >= 10
  {
   lc.setRow(0,7,B00000000); // free digit 
  }
  else
  {
   lc.setDigit(0,7,tensHour,false);   
  }
   lc.setDigit(0,6,unitsHour,false); 
   if (secunda%2 == 0)
     lc.setRow(0,5,B00000001); // - 
    else
     lc.setRow(0,5,B00000000); // none 
   lc.setDigit(0,4,tensMin,false); 
   lc.setDigit(0,3,unitsMin,false); 
   if (secunda%2 == 0)
     lc.setRow(0,2,B00000001); // - 
    else
     lc.setRow(0,2,B00000000); // none 
   lc.setDigit(0,1,tensSec,false); 
   lc.setDigit(0,0,unitsSec,false);  
}

void displayTime12(){
    byte tensHour = ora / 10; //Extract the individual digits
    byte unitsHour = ora % 10;
    byte tensMin = minut / 10;
    byte unitsMin = minut % 10;
    byte tensSec = secunda / 10;
    byte unitsSec = secunda % 10;

H = ora;
if (ora > 12)
  {
    H = H%12;
    pm = 1;
  }
  else
  {
    pm = 0;
  }
  if (H == 0) H = 12;

tensHour = H / 10; //Extract the individual digits
unitsHour = H % 10;

  if(tensHour == 0)  // hour >= 10
  {
   lc.setRow(0,7,B00000000); // free digit 
  }
  else
  {
   lc.setDigit(0,7,tensHour,false);   
  }
   lc.setDigit(0,6,unitsHour,false); 
   if (secunda%2 == 0)
     lc.setRow(0,5,B00000001); // - 
    else
     lc.setRow(0,5,B00000000); // none 
   lc.setDigit(0,4,tensMin,false); 
   lc.setDigit(0,3,unitsMin,false); 
   if (secunda%2 == 0)
     lc.setRow(0,2,B00000001); // - 
    else
     lc.setRow(0,2,B00000000); // none 
   lc.setDigit(0,1,tensSec,false); 
   if (pm == 1)
    lc.setDigit(0,0,unitsSec,true);
    else 
    lc.setDigit(0,0,unitsSec,false);
}

void displayDate(){
    byte tensDate = ziua / 10; //Extract the individual digits
    byte unitsDate = ziua % 10;
    byte tensMon = luna / 10;
    byte unitsMon = luna % 10;
    byte tensYear = an / 10;
    byte unitsYear = an % 10;    

   lc.setDigit(0,7,tensDate,false); 
   lc.setDigit(0,6,unitsDate,true); 
   lc.setDigit(0,5,tensMon,false); 
   lc.setDigit(0,4,unitsMon,true); 
   lc.setDigit(0,3,2,false); 
   lc.setDigit(0,2,0,false);   
   lc.setDigit(0,1,tensYear,false); 
   lc.setDigit(0,0,unitsYear,false);  
}


void iaData()
{
  timeClient.update();
  epochTime = timeClient.getEpochTime();
  //Get a time structure
  struct tm *ptm = gmtime ((time_t *)&epochTime); 

  ziua = ptm->tm_mday;
  Serial.print("Month day: ");
  Serial.println(ziua);

  luna = ptm->tm_mon+1;
  Serial.print("Month: ");
  Serial.println(luna);

  an = ptm->tm_year+1900-2000;
  Serial.print("Year: ");
  Serial.println(an);
}

void iaOra()
{
  timeClient.update();

epochTime = timeClient.getEpochTime();
//  Serial.print("Epoch Time: ");
//  Serial.println(epochTime);
  
  String formattedTime = timeClient.getFormattedTime();
//  Serial.print("Formatted Time: ");
//  Serial.println(formattedTime);  
  
    ora = timeClient.getHours();
  //  Serial.print("ora = ");
  //  Serial.println(ora);
    minut = timeClient.getMinutes();
    secunda = timeClient.getSeconds();
}

// Rounded HH:mm format
char * hoursToString(double h, char *str)
{
  int m = int(round(h * 60));
  int hr = (m / 60) % 24;
  int mn = m % 60;

  str[0] = (hr / 10) % 10 + '0';
  str[1] = (hr % 10) + '0';
  str[2] = ':';
  str[3] = (mn / 10) % 10 + '0';
  str[4] = (mn % 10) + '0';
  str[5] = '\0';
  return str;
}

void Soare()
{
   // Calculate the times of sunrise, transit, and sunset, in hours (UTC)
  calcSunriseSunset(2000+an, luna, ziua, latitude, longitude, transit, sunrise, sunset);

m1 = int(round((sunrise + TIMEZONE+DST) * 60));
hr1 = (m1 / 60) % 24;
mn1 = m1 % 60;

m2 = int(round((sunset + TIMEZONE+DST) * 60));
hr2 = (m2 / 60) % 24;
mn2 = m2 % 60;

  Serial.print("Sunrise = ");
  Serial.print(sunrise+TIMEZONE+DST);
  Serial.print(" = ");
  Serial.print(hr1);
  Serial.print(":");
  Serial.print(mn1);
  Serial.print(" -> ");
  Serial.println(hoursToString(sunrise + TIMEZONE+DST, str));
  Serial.print("Sunset = ");
  Serial.print(sunset+TIMEZONE+DST);
  Serial.print(" = ");
  Serial.print(hr2);
  Serial.print(":");
  Serial.print(mn2);
  Serial.print(" -> ");
  Serial.println(hoursToString(sunset + TIMEZONE+DST, str));
}

void luminita()
{
  ora1 = 100*hr1 + mn1;  // rasarit 
  ora2 = 100*hr2 + mn2;  // apus
  oraactuala = 100*ora + minut;  // ora actuala
  if ((oraactuala > ora1) and (oraactuala < ora2))  // zi
  {
     lc.setIntensity(0,intensitate11);   // Set the brightness
     noapte = 0;
  }
  else  // noapte
  {
     lc.setIntensity(0,intensitate21);   // Set the brightness
     noapte = 1;
  }
}


void afisareT()
{
digitoneT = temperature / 10;
digittwoT = temperature % 10;
   lc.setRow(0,7,B00000000); // free digit
if (temperature < 0) 
   lc.setRow(0,6,B00000001); // -
else
   lc.setRow(0,6,B00000000); // free digit  
if (digitoneT == 0)
   lc.setRow(0,5,B00000000); // free digit
else
   lc.setDigit(0,5,digitoneT,false); 

   lc.setDigit(0,4,digittwoT,false); 
   lc.setRow(0,3,B01100011); // degree sign
   lc.setRow(0,2,B01001110); // C
   lc.setRow(0,1,B00000000); // free digit
   lc.setRow(0,0,B00000000); // free digit  
}

void afisareH()
{
digitoneH = humidity / 10;
digittwoH = humidity % 10; 

   lc.setRow(0,7,B00000000); // free digit
   lc.setRow(0,6,B00000000); // free digit
   lc.setDigit(0,5,digitoneH,false); 
   lc.setDigit(0,4,digittwoH,false); 
   lc.setRow(0,3,B01100011); // degree sign
   lc.setRow(0,2,B00011101); // o
   lc.setRow(0,1,B00000000); // free digit
   lc.setRow(0,0,B00000000); // free digit      
}
