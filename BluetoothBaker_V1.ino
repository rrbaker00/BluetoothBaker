#include <SPI.h>
#include "RF24.h"
#include "esp_bt.h"
#include "eps_wifi.h"



// keywords for wifi
//---------------------------------------------------------------------------------------
// WiFi.mode(WiFi_STA) -> set wifi to station mode
// WiFi.disconnect()
// WiFi.scanNetworks() -> scan for avalible networks
//--------------------------------------------------------------------------------------

// keywords for radios
//---------------------------------------------------------------------------------------
// power settings for radio
// radio.powerDown() / radio.powerUp() -> for saving power if a battery is used
// radio.startListining() / radio.stopListining() -> for switching between rx and tx

// radio.setPALevel()
// RF24_PA_MIN	−18 dBm	~7 mA
// RF24_PA_LOW	−12 dBm	~7.5 mA
// RF24_PA_HIGH	−6 dBm	~9 mA
// RF24_PA_MAX	0 dBm	~11.3 mA

// chanel switching
// radio.setChannel() -> for switching the channel

//sending data
//radio.openWritingPipe(address)
//----------------------------------------------------------------------------------------


// ---- VSPI (radio A) ----
#define VSPI_SCK 18
#define VSPI_MISO 19
#define VSPI_MOSI 23
#define VSPI_CSN 21
#define VSPI_CE 22

// ---- HSPI (radio B) ----
#define HSPI_SCK 14
#define HSPI_MISO 27
#define HSPI_MOSI 13
#define HSPI_CSN 15
#define HSPI_CE 25

SPIClass hspi(HSPI);
//create second bus for parallel running
RF24 radioA(VSPI_CE, VSPI_CSN, 16000000);  // (CE, CSN, CPU Speed)
RF24 radioB(HSPI_CE, HSPI_CSN, 16000000);

const char whiteNoise[32] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32 };
const uint8_t chanMax = 124;
uint8_t hopIndex_V = 0;
uint8_t hopIndex_H = 50;

void setup() {
  Serial.begin(115200);
  while (!Serial) {}
  initWifi();
  SPI.begin(VSPI_SCK, VSPI_MISO, VSPI_MOSI);
  hspi.begin(HSPI_SCK, HSPI_MISO, HSPI_MOSI);
  initRadio(radioA, nullptr);  // radio A on default VSPI (nullptr = no bus given)
  initRadio(radioB, &hspi);
}

void displayNetworks() {
  int n = getNetworks();
  if (n == 0) {
    Serial.println("no networks found");
    return;  // nothing more to do
  } else {
    Serial.print(n);
    Serial.println(" networks found");
  }

  int channelCount[14] = { 0 };

  for (int i = 0; i < n; i++) {
    int ch = WiFi.channel(i);
    if (ch >= 1 && ch <= 13) channelCount[ch]++;

    Serial.print("  ");
    Serial.print(WiFi.SSID(i));
    Serial.print("  | ch ");
    Serial.print(ch);
    Serial.print(" | ");
    Serial.print(WiFi.RSSI(i));
    Serial.println(" dBm");
  }

  // --- Congestion summary: runs ONCE, after the loop above ---
  Serial.println("\nChannel congestion (2.4 GHz):");
  for (int ch = 1; ch <= 13; ch++) {
    Serial.print("  ch ");
    if (ch < 10) Serial.print(' '); 
    Serial.print(ch);
    Serial.print(": ");
    for (int j = 0; j < channelCount[ch]; j++) Serial.print('#');
    Serial.print("  (");
    Serial.print(channelCount[ch]);
    Serial.println(")");
  }
}

int getNetworks() {
  //return a list of all networks
  return WiFi.scanNetworks();
}

void initWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
} 

void initRadio(RF24 &r, SPIClass *bus) {
  bool ok;
  if (bus) ok = r.begin(bus);
  else ok = r.begin();

  if (!ok) {
    Serial.print("Radio init failed (code ");
    Serial.print(bus ? 1 : 2);
    Serial.println(") - check wiring/power");
    return;
  }

  r.setAutoAck(false);
  r.setDataRate(RF24_1MBPS);
  r.disableCRC();
  r.stopListening();
  r.setPALevel(RF24_PA_HIGH);
}

rf24_pa_dbm_e getPALevel() {
  //get and change value of power
}

// Send one message from a given radio on a given channel to a given address.
void sendMessage(RF24 &r, uint8_t channel, const uint8_t *address, const char *msg, uint8_t len) {
  r.setChannel(channel);  // tune to the channel for this send
  r.openWritingPipe(address);
  r.stopListening();  // make sure we're in TX mode
  r.write(msg, len); // actually send it
}

rf24_datarate_e getDataRate() {
  //set value of data rate for menu
}

int hop(int channelNum){
  if (channelNum == chanMax){
    return 0;
  }else {
    return channelNum+=1;
  }
}

// void jamAll() {
//   Serial.println("jamALL");
//   const uint8_t addressA[6] = "NODEA";
//   const uint8_t addressB[6] = "NODEB";
//   while(true) {
//     yield();
//     sendMessage(radioA, hopIndex_V, addressA, whiteNoise, sizeof(whiteNoise));
//     sendMessage(radioB, hopIndex_H, addressB, whiteNoise, sizeof(whiteNoise));
//     hopIndex_V = hop(hopIndex_V);
//     hopIndex_H = hop(hopIndex_H);

//   }
// }

void jamAllV2(){
  while(true){
    yield();
    radioA.startConstCarrier(RF24_PA_MAX, hopIndex_V);
    radioB.startConstCarrier(RF24_PA_MAX, hopIndex_H);
    hopIndex_V = hop(hopIndex_V);
    hopIndex_H = hop(hopIndex_H);
  }
}

void menu() {
  jamAllV2();
}

void haltERR(int num) {
  Serial.println("yo dawg it aint responding");
  if (num) {  // !num is true only when num == 0
    Serial.println("heres some help,");
    Serial.print(num);  // ...which prints 0
  }
  while (1) {}
}

void loop() {
  //ui will be done later
  //display menu
  menu();
}



/////// not mine //////////////////////////////////////////////////////////////////////////////
//credit to https://github.com/smoochiee/Noisy-boy-esp32-Bluetooth-jammer   for the code used below
#include "RF24.h"
#include <SPI.h>
#include <ezButton.h>
#include "esp_bt.h"
#include "esp_wifi.h"


SPIClass *sp = nullptr;
SPIClass *hp = nullptr; 

RF24 radio(16, 15, 16000000);   //HSPI CAN SET SPI SPEED TO 16000000 BY DEFAULT ITS 10000000
RF24 radio1(22, 21, 16000000);  //VSPI CAN SET SPI SPEED TO 16000000 BY DEFAULT ITS 10000000


//HSPI=SCK = 14, MISO = 12, MOSI = 13, CS = 15 , CE = 16
//VSPI=SCK = 18, MISO =19, MOSI = 23 ,CS =21 ,CE = 22

unsigned int flag = 0;   //HSPI// Flag variable to keep track of direction
unsigned int flagv = 0;  //VSPI// Flag variable to keep track of direction
int ch = 45;    // Variable to store value of ch
int ch1 = 45;   // Variable to store value of ch

ezButton toggleSwitch(33);




void two() {
  if (flagv == 0) {  // If flag is 0, increment ch by 4 and ch1 by 1

    ch1 += 4;
  } else {  // If flag is not 0, decrement ch by 4 and ch1 by 1

    ch1 -= 4;
  }

  if (flag == 0) {  // If flag is 0, increment ch by 4 and ch1 by 1
    ch += 2;

  } else {  // If flag is not 0, decrement ch by 4 and ch1 by 1
    ch -= 2;
  }

  // Check if ch1 is greater than 79 and flag is 0
  if ((ch1 > 79) && (flagv == 0)) {
    flagv = 1;                             // Set flag to 1 to change direction
  } else if ((ch1 < 2) && (flagv == 1)) {  // Check if ch1 is less than 2 and flag is 1
    flagv = 0;                             // Set flag to 0 to change direction
  }

  // Check if ch is greater than 79 and flag is 0
  if ((ch > 79) && (flag == 0)) {
    flag = 1;                            // Set flag to 1 to change direction
  } else if ((ch < 2) && (flag == 1)) {  // Check if ch is less than 2 and flag is 1
    flag = 0;                            // Set flag to 0 to change direction
  }
  radio.setChannel(ch);
  radio1.setChannel(ch1);
  /*Serial.print("SP:");
  Serial.println(ch1);
  Serial.print("\tHP:");
  Serial.println(ch);*/
}


void one() {
  ////RANDOM CHANNEL
  radio1.setChannel(random(80));
  radio.setChannel(random(80));
  delayMicroseconds(random(60));//////REMOVE IF SLOW


 /*  YOU CAN DO -----SWEEP CHANNEL
  for (int i = 0; i < 79; i++) {
    radio.setChannel(i);
*/

}



void setup() {

  Serial.begin(115200);
  esp_bt_controller_deinit();
  esp_wifi_stop();
  esp_wifi_deinit();
  esp_wifi_disconnect();
  toggleSwitch.setDebounceTime(50);

  initHP();
  initSP();
}

void initSP() {
  sp = new SPIClass(VSPI);
  sp->begin();
  if (radio1.begin(sp)) {
    Serial.println("SP Started !!!");
    radio1.setAutoAck(false);
    radio1.stopListening();
    radio1.setRetries(0, 0);
    radio1.setPALevel(RF24_PA_MAX, true);
    radio1.setDataRate(RF24_2MBPS);
    radio1.setCRCLength(RF24_CRC_DISABLED);
    radio1.printPrettyDetails();
    radio1.startConstCarrier(RF24_PA_MAX, ch1);
  } else {
    Serial.println("SP couldn't start !!!");
  }
}
void initHP() {
  hp = new SPIClass(HSPI);
  hp->begin();
  if (radio.begin(hp)) {
    Serial.println("HP Started !!!");
    radio.setAutoAck(false);
    radio.stopListening();
    radio.setRetries(0, 0);
    radio.setPALevel(RF24_PA_MAX, true);
    radio.setDataRate(RF24_2MBPS);
    radio.setCRCLength(RF24_CRC_DISABLED);
    radio.printPrettyDetails();
    radio.startConstCarrier(RF24_PA_MAX, ch);
  } else {
    Serial.println("HP couldn't start !!!");
  }
}

void loop() {


  toggleSwitch.loop();  // MUST call the loop() function first

  /* if (toggleSwitch.isPressed())
    Serial.println("one");
  if (toggleSwitch.isReleased())
    Serial.println("two");*/

  int state = toggleSwitch.getState();


  if (state == HIGH)
    two();

  else {
    one();
  }
}
