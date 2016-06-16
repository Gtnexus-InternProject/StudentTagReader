#include <Adafruit_CC3000.h>
#include <Adafruit_CC3000_Server.h>
#include <ccspi.h>

//Import the libraries required
#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>
#include <string.h>
#include "utility/debug.h"
#include <SoftwareSerial.h>
#include <LiquidCrystal.h>

//Define the constants that will be used during the program
#define ADAFRUIT_CC3000_IRQ   3
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10
#define WLAN_SSID       "corp-wifi"
#define WLAN_PASS       "!cloudblue5"
#define WLAN_SECURITY   WLAN_SEC_WPA2
#define IDLE_TIMEOUT_MS  3000
#define WEBSITE      "www.gtnexus.com"
#define WEB_HOST_IP      cc3000.IP2U32(192, 168, 253, 157)
#define WEBPAGE      "/users/student/rfidvalidate/"
#define WEBPAGEE      "/users/student/rfidvalidate/24853484854666952526567483"
#define WEBPORT      3000
#define SYNC_INTERVAL  20000
#define  C     1912    // 523 Hz 
// Define a special note, 'R', to represent a rest
#define  R     0
int melody[] = {C, C, C, 0, C, C, C, C, C, 0, C, C};
int beats[]  = { 16, 16, 16,  8,  8,  16, 32, 16, 16, 16, 8, 8 };
int MAX_COUNT = sizeof(melody) / 2; // Melody length, for looping.

long tempo = 10000;

int pause = 100;

int rest_count = 100; //<-BLETCHEROUS HACK; See NOTES

// Initialize core variables
int tone_ = 0;
int beat = 0;
long duration  = 0;
Adafruit_CC3000_Client www;
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT,
                         SPI_CLOCK_DIVIDER);
uint32_t ip;
LiquidCrystal lcd(52, 53, 38, 36, 34, 32);
SoftwareSerial RFID(2, 3); // RX and TX
int data1 = 0;
int ok = -1;
int yes = 30;
int no = 31;
int wait = 24;
int speakerOut = 26;
String  stringCurr = "";
String  stringPrev = "";

void setup() {
  Serial.begin(115200);
  lcd.begin(20, 4);
  pinMode(speakerOut, OUTPUT);
  digitalWrite(speakerOut, LOW);
  //        lcd.setCursor(0, 1);
  lcd.print("Testing");
  connectToWiFi();
  setupRFID();
}

bool  connectToWiFi() {

  Serial.println(F("Hello, CC3000!\n"));
  Serial.print("Free RAM: "); Serial.println(getFreeRam(), DEC);
  Serial.println(F("\nInitializing..."));
  if (!cc3000.begin())
  {
    Serial.println(F("Couldn't begin()! Check your wiring?"));
    while (1);
  }
  Serial.print(F("\nAttempting to connect to ")); Serial.println(WLAN_SSID);
  if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
    Serial.println(F("Failed!"));
    while (1);
  }
  Serial.println(F("Connected!"));
  Serial.println(F("Request DHCP"));
  while (!cc3000.checkDHCP())
  {
    delay(100);
  }
  while (! displayConnectionDetails()) {
    delay(1000);
  }
  return true;
}

bool displayConnectionDetails(void) {
  uint32_t ipAddress, netmask, gateway, dhcpserv, dnsserv;

  if (!cc3000.getIPAddress(&ipAddress, &netmask, &gateway, &dhcpserv, &dnsserv))
  {
    Serial.println(F("Unable to retrieve the IP Address!\r\n"));
    return false;
  }
  else
  {
    Serial.print(F("\nIP Addr: ")); cc3000.printIPdotsRev(ipAddress);
    Serial.print(F("\nNetmask: ")); cc3000.printIPdotsRev(netmask);
    Serial.print(F("\nGateway: ")); cc3000.printIPdotsRev(gateway);
    Serial.print(F("\nDHCPsrv: ")); cc3000.printIPdotsRev(dhcpserv);
    Serial.print(F("\nDNSserv: ")); cc3000.printIPdotsRev(dnsserv);
    Serial.println();
    return true;
  }
}

void setupRFID() {
  RFID.begin(9600);    // start serial to RFID reader
  Serial3.begin(9600); // start serial to PC
  pinMode(yes, OUTPUT); // for status LEDs
  pinMode(no, OUTPUT);
  pinMode(wait, OUTPUT);
  // pinMode(wait,OUTPUT);
  for (int i = 0; i < 3; i++) {
    digitalWrite(yes, HIGH);
    delay(300);
    digitalWrite(yes, LOW);
  }
  Serial.println("1");
}
//read the tags............

void readTags() {
  if (Serial3.available() > 0)
  {
    delay(100); // needed to allow time for the data to come in from the serial buffer.
    for (int z = 0 ; z < 14 ; z++) // read the rest of the tag
    {
      data1 = Serial3.read();
      stringCurr.concat(data1);
    }
    Serial3.flush();
    if (stringCurr.indexOf('-') == -1) {
      if (stringCurr.equals(stringPrev)) {
        Serial.println("same");
      } else {
        pinMode(wait, HIGH);
        Serial.println("differ");
        checkTag(stringCurr);
      }
    }
    stringPrev = stringCurr;
    stringCurr = "";
  }

}
 
void checkTag(String tagID) {
  Serial.println("checking");
  resolveHostDetails();
  sendRequest(tagID);
  String resp = processResponse();
  Serial.println("86: " + resp);
  if (resp.indexOf('1') != -1) {
    Serial.println("AAAAAAA");
    lightControl(true);

  }
  if (resp.indexOf('0') != -1) {
    Serial.println("BBBBBBB");
    lightControl(false);
  }
}

void lightControl(bool val) {
  pinMode(wait, LOW);
  if (val) // if we had a match
  {
    Serial.println("Accepted");
    lcd.setCursor(0, 1);
    lcd.print("Accepted");
    digitalWrite(yes, HIGH);

    for (int i = 0; i < MAX_COUNT; i++) {
      tone_ = melody[i];
      beat = beats[i];
      duration = beat * tempo; // Set up timing
      playTone();
      delayMicroseconds(pause);
    }
    delay(1000);
    digitalWrite(yes, LOW);
  }
  else // if we didn't have a match
  {
    Serial.println("Rejected");
    lcd.setCursor(0, 1);
    lcd.print("Rejected");

    digitalWrite(speakerOut, HIGH);
    for (int i = 0; i < 4; i++) {
      digitalWrite(no, HIGH);
      delay(500);
      digitalWrite(no, LOW);
    }
    digitalWrite(speakerOut, LOW);

  }
}




void closeWificonnection() {
  Serial.println(F("\n\nDisconnecting"));
  cc3000.disconnect();
}

void resolveHostDetails() {
  ip = WEB_HOST_IP;
  //This happens only if there is no ip specified
  while (ip == 0) {
    if (! cc3000.getHostByName(WEBSITE, &ip)) {
      Serial.println(F("Couldn't resolve!"));
    }
    delay(500);
  }

}
void sendRequest(String tagID) {
  Serial.println("sending");
  String url = WEBPAGE + tagID;
  int x = url.length() + 1;
  Serial.println("len " + x);
  char test[x];
  url.toCharArray(test, x);
  Serial.println("XXXXXXXXXXXXXX");
  for (int i = 0; i < x; i++) {
    Serial.print(test[i]);
  }
  Serial.println("");
  Serial.println("XXXXXXXXXXXXXX");
  www = cc3000.connectTCP(ip, WEBPORT);
  if (www.connected()) {
    www.fastrprint(F("GET "));
    www.fastrprint(test);
    www.fastrprint(F(" HTTP/1.1\r\n"));
    www.fastrprint(F("Host: "));
    //www.fastrprint(WEBSITE);
    www.fastrprint(F("\r\n"));
    www.fastrprint(F("\r\n"));
    www.println();
  } else {
    Serial.println(F("Connection failed"));
    return ;
  }
  memset(test, 0, sizeof(test));
}

String processResponse() {
  Serial.println("processing");

  /* Read data until either the connection is closed, or the idle timeout is reached. */
  unsigned long lastRead = millis();
  String response = "";
  while (www.connected() && (millis() - lastRead < IDLE_TIMEOUT_MS)) {
    while (www.available()) {
      char c = www.read();
      //Serial.print(c);
      response += c;
      lastRead = millis();
    }
  }
  www.close();
  return formatResponse(response);
}

String formatResponse(String response) {
  Serial.println("formatting");
  int begin = response.indexOf('{');
  int end = response.indexOf('}');
  String req = response.substring(begin, end + 1);
  return req;
}

void playTone() {
  long elapsed_time = 0;
  if (tone_ > 0) { // if this isn't a Rest beat, while the tone has
    //  played less long than 'duration', pulse speaker HIGH and LOW
    while (elapsed_time < duration) {

      digitalWrite(speakerOut, HIGH);
      delayMicroseconds(tone_ / 2);

      // DOWN
      digitalWrite(speakerOut, LOW);
      delayMicroseconds(tone_ / 2);

      // Keep track of how long we pulsed
      elapsed_time += (tone_);
    }
  }
  else { // Rest beat; loop times delay
    for (int j = 0; j < rest_count; j++) { // See NOTE on rest_count
      delayMicroseconds(duration);
    }
  }
}

void loop() {
  ok = -1;
  readTags();
}


