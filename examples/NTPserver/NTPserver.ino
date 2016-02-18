/*
 * Arduino ENC28J60 EtherCard NTP client demo
 */

#include <avr/pgmspace.h> // cf. https://www.arduino.cc/en/Reference/PROGMEM
#include <EtherCard.h>

#define SECS_YR_1900_2000  (3155673600UL) // Jan 1 

static byte mymac[6] = { 0x4C,0x61,0x65,0x74,0x75,0x65 };

// IP and netmask allocated by DHCP
static byte myip[4] = { 0,0,0,0 };
static byte mynetmask[4] = { 0,0,0,0 };
static byte gwip[4] = { 0,0,0,0 };
static byte dnsip[4] = { 0,0,0,0 };
static byte dhcpsvrip[4] = { 0,0,0,0 };

static int currentTimeserver = 0;

// Find list of servers at http://support.ntp.org/bin/view/Servers/StratumTwoTimeServers
// Please observe server restrictions with regard to access to these servers.
// This number should match how many NTP time server strings we have
#define NUM_TIMESERVERS 4

// Create an entry for each timeserver to use
const char ntp0[] PROGMEM = "0.pool.ntp.org";
const char ntp1[] PROGMEM = "1.pool.ntp.org";
const char ntp2[] PROGMEM = "2.pool.ntp.org";
const char ntp3[] PROGMEM = "3.pool.ntp.org";

// Now define another array in PROGMEM for the above strings
const char * const ntpList[] PROGMEM = { ntp0, ntp1, ntp2, ntp3 };
  
// Packet buffer, must be big enough to packet and payload
#define BUFFER_SIZE 550
byte Ethernet::buffer[BUFFER_SIZE];
byte clientPort = 123;

// The next part is to deal with converting time received from NTP servers
// to a value that can be displayed. This code was taken from somewhere that
// I can't remember. Apologies for no acknowledgement.

unsigned long lastUpdate = 0;
unsigned long timeLong;
// Number of seconds between 1-Jan-1900 and 1-Jan-1970, 
// NTP time starts 1900 and UNIX time starts 1970.
#define GETTIMEOFDAY_TO_NTP_OFFSET 2208988800UL

#define  EPOCH_YR  1970
#define SECS_DAY  86400UL  //(24L * 60L * 60L)
#define LEAPYEAR(year)  (!((year) % 4) && (((year) % 100) || !((year) % 400)))
#define YEARSIZE(year)  (LEAPYEAR(year) ? 366 : 365)

static const char day_abbrev[] PROGMEM = "SunMonTueWedThuFriSat";
// isleapyear = 0-1
// month=0-11
// return: how many days a month has

byte monthlen(byte isleapyear,byte month)
{
  const byte mlen[2][12] = 
  {
    { 
      31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
    ,
    { 
      31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
  };
  return(mlen[isleapyear][month]);
}


// gmtime -- converts calendar time (sec since 1970) into broken down time
// returns something like Fri 2007-10-19 in day and 01:02:21 in clock
// The return values is the minutes as integer. This way you can update
// the entire display when the minutes have changed and otherwise just
// write current time (clock). That way an LCD display needs complete
// re-write only every minute.
byte gmtime(const unsigned long time,char *day, char *clock)
{
  char dstr[4];
  byte i;
  unsigned long dayclock;
  unsigned int dayno;
  unsigned int tm_year = EPOCH_YR;
  byte tm_sec,tm_min,tm_hour,tm_wday,tm_mon;

  dayclock = time % SECS_DAY;
  dayno = time / SECS_DAY;

  tm_sec = dayclock % 60UL;
  tm_min = (dayclock % 3600UL) / 60;
  tm_hour = dayclock / 3600UL;
  tm_wday = (dayno + 4) % 7;  /* day 0 was a thursday */
  while (dayno >= YEARSIZE(tm_year)) 
  {
    dayno -= YEARSIZE(tm_year);
    tm_year++;
  }
  tm_mon = 0;
  while (dayno >= monthlen(LEAPYEAR(tm_year),tm_mon)) 
  {
    dayno -= monthlen(LEAPYEAR(tm_year),tm_mon);
    tm_mon++;
  }
  i=0;
  while (i<3)
  {
    dstr[i]= pgm_read_byte(&(day_abbrev[tm_wday*3 + i])); 
    i++;
  }
  dstr[3]='\0';
  sprintf_P(day,PSTR("%s %u-%02u-%02u"),dstr,tm_year,tm_mon+1,dayno + 1);
  sprintf_P(clock,PSTR("%02u:%02u:%02u"),tm_hour,tm_min,tm_sec);
  return(tm_min);
}


void setup()
{
  Serial.begin(9600);
  Serial.println( F("EtherCard NTP Client" ) );
  
  currentTimeserver = 0;

  byte rev = ether.begin(sizeof Ethernet::buffer, mymac);
  if ( rev == 0) 
  {
    Serial.println( F( "Failed to access Ethernet controller" ) );
  }

  Serial.println( F( "Setting up DHCP" ));
  if (!ether.dhcpSetup())
  {
    Serial.println( F( "DHCP failed" ));
  }
  
  ether.printIp("My IP: ", ether.myip);
  ether.printIp("Netmask: ", ether.netmask);
  ether.printIp("GW IP: ", ether.gwip);
  ether.printIp("DNS IP: ", ether.dnsip);

  lastUpdate = millis();
}

void loop()
{
  unsigned int dat_p;
  char day[22];
  char clock[22];
  int plen = 0;
  
  // Main processing loop now we have our addresses
  
  // handle ping and wait for a tcp packet
  plen = ether.packetReceive();
  dat_p=ether.packetLoop(plen);
  // Has unprocessed packet response
  if (plen > 0) 
  {
    timeLong = 0L;
   
    if (ether.ntpProcessAnswer(&timeLong,clientPort)) 
    {
      Serial.print( F( "Time:" ));
      Serial.println(timeLong); // secs since year 1900
     
      if (timeLong) 
      {
        timeLong -= GETTIMEOFDAY_TO_NTP_OFFSET;
        gmtime(timeLong,day,clock);
        Serial.print( day );
        Serial.print( " " );
        Serial.println( clock );
      }
    }
  }
  
  // Request an update every 20s
  if( lastUpdate + 20000L < millis() ) 
  {
    // time to send request
    lastUpdate = millis();
    Serial.print( F("\nTimeSvr: " ) );
    Serial.println( currentTimeserver, DEC );

    if (!ether.dnsLookup( (char*)pgm_read_word(&(ntpList[currentTimeserver])) )) 
    {
      Serial.println( F("DNS failed" ));
    } 
    else 
    {
      ether.printIp("SRV: ", ether.hisip);
      
      Serial.print( F("Send NTP request " ));
      Serial.println( currentTimeserver, DEC );
    
      ether.ntpRequest(ether.hisip, ++clientPort);
      Serial.print( F("clientPort: "));
      Serial.println(clientPort, DEC );
    }
    if( ++currentTimeserver >= NUM_TIMESERVERS )
    {
      currentTimeserver = 0;
    } 
  }
}

