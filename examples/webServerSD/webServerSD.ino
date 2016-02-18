/* 
 * SD card webserver sketch for Gate Shield. 
 * 
 * Hardware : ENC28J60'SS on pin 10
 *            SD card'SS on pin 7
 *            
 * Website stored in a file named index.htm located on SD card
 * 
 * LIBRARIES : EtherCard : https://github.com/Snootlab/EtherCard
 *             SdFat : https://github.com/greiman/SdFat
 * 
 * IDE 1.6.6
 * 
 * Copyleft Snootlab 2016
 */

/* ETHERCARD SETTINGS */
#include <EtherCard.h>

// Ethernet interface MAC address, must be unique on the LAN
static byte mymac[6] = { 0x4C,0x61,0x65,0x74,0x75,0x65 };
static byte myip[4] = { 192,168,10,89 };

byte Ethernet::buffer[400];
BufferFiller bfill;

/* SD CARD SETTINGS */
#include <SdFat.h>
#define SD_CS 7

SdFat sd;
SdFile myFile;

void setup() 
{
  Serial.begin(9600);
  
  // Init Ethernet
  if (ether.begin(sizeof Ethernet::buffer, mymac) == 0)
  {
    Serial.println(F("Failed to access Ethernet controller"));
    return;
  }
  ether.staticSetup(myip);
  
  // Initialize SdFat or print a detailed error message and halt
  // Use half speed like the native library.
  // change to SPI_FULL_SPEED for more performance.
  if (!sd.begin(SD_CS, SPI_HALF_SPEED))   sd.initErrorHalt();

  // Check if the file containing our website is present
  if(!sd.exists("index.htm"))
  {
    Serial.println("File not found !");
    return;
  }
}

word homePage() 
{
  // Open the file for reading:
  if (!myFile.open("index.htm", O_READ)) 
  {
    sd.errorHalt("Opening index.htm for read failed");
  }

  char website[250] = ""; // storage

  while (myFile.available())
  {
    // read each character one after another
    char c = myFile.read();
    // and save them for later
    sprintf(website, "%s%c", website, c); 
  }
  // close the file:
  myFile.close();
  Serial.println("Sending webpage...");

  bfill = ether.tcpOffset();

  // Actually send the packet
  bfill.emit_p(PSTR(
    "HTTP/1.0 200 OK\r\n"
    "Content-Type: text/html\r\n"
    "Pragma: no-cache\r\n"
    "\r\n"
    "<title>Gate webserver</title>" 
    "$S"),
      website);
      
  return bfill.position();
}

void loop () 
{
  word len = ether.packetReceive();
  word pos = ether.packetLoop(len);
  
  if (pos)  // check if valid TCP data is received
  {
    ether.httpServerReply(homePage()); // send web page data
  }
}
