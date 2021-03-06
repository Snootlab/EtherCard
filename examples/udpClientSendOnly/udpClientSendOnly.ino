#include <EtherCard.h>

static byte mymac[] = { 0x4C, 0x61, 0x65, 0x74, 0x75, 0x65 };
byte Ethernet::buffer[700];
static uint32_t timer;

const char website[] PROGMEM = "server.example.net";
const int dstPort PROGMEM = 1234;

const int srcPort PROGMEM = 4321;

void setup () {
  Serial.begin(9600);

  if (ether.begin(sizeof Ethernet::buffer, mymac) == 0) 
    Serial.println( "Failed to access Ethernet controller");
  if (!ether.dhcpSetup())
    Serial.println("DHCP failed");

  ether.printIp("IP:  ", ether.myip);
  ether.printIp("GW:  ", ether.gwip);  
  ether.printIp("DNS: ", ether.dnsip);  

  if (!ether.dnsLookup(website))
    Serial.println("DNS failed");
    
  ether.printIp("SRV: ", ether.hisip);
}

char textToSend[] = "test 123";

void loop () {  
    if (millis() > timer) {
      timer = millis() + 5000;  
     //static void sendUdp (char *data,uint8_t len,uint16_t sport, uint8_t *dip, uint16_t dport);      
     ether.sendUdp(textToSend, sizeof(textToSend), srcPort, ether.hisip, dstPort );   
  }
}
