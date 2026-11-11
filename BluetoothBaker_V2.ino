#include <SPI.h>
#include <RF24.h>
#include <esp_bt.h>
#include <esp_wifi.h>

// ---- VSPI (radio A) ----
// #define VSPI_SCK 18
// #define VSPI_MISO 19
// #define VSPI_MOSI 23
// #define VSPI_CSN 21
// #define VSPI_CE 22

// // ---- HSPI (radio B) ----
// #define HSPI_SCK 14
// #define HSPI_MISO 27
// #define HSPI_MOSI 13
// #define HSPI_CSN 15
// #define HSPI_CE 25

class Radio {
  public:
    Radio(uint8_t ce, uint8_t csn, uint8_t spiBus, uint32_t spiSpeed = 16000000);
    bool init();
    void jamAll(int steps);
    void hop();

    int getChannel() const; // read the channel
    SPIClass* getBus() const ;      // hand back the bus ptr

  private:
    RF24 radio;        // the module driver 
    uint8_t busId;     // which controller: HSPI / VSPI
    SPIClass *bus;     // the per-instance bus. 
    int channel;       // current channel for this radio
    unsigned int flag;

};

Radio::Radio(uint8_t ce, uint8_t csn, uint8_t spiBus, uint32_t spiSpeed)
  : radio(ce, csn, spiSpeed),
    busId(spiBus),
    bus(nullptr),
    channel(45),
    flag(0)
{

}

void Radio::jamAll(int step){
  flag == 0 ? channel += step : channel -= step;
  if (channel >79 && flag == 0){
    flag = 1;
  } else if (channel < 2 && flag ==1){
    flag = 0;
  }
  radio.setChannel(channel);
}

void Radio::hop(){
  radio.setChannel(random(81));
}

int Radio::getChannel() const {
  return channel;
}

SPIClass* Radio::getBus() const {
  return bus;
}


void jamSetup(){
  esp_bt_controller_deinit();
  esp_wifi_stop();
  esp_wifi_deinit();
  esp_wifi_disconnect();
}

bool Radio::init(){
  bus = new SPIClass(busId);
  bus->begin();
  if (!radio.begin(bus)) return false;

  radio.setAutoAck(false);
  radio.stopListening();
  radio.setRetries(0,0);
  radio.setDataRate(RF24_2MBPS);       
  radio.setPALevel(RF24_PA_MAX, true);
  radio.setCRCLength(RF24_CRC_DISABLED);
  radio.startConstCarrier(RF24_PA_MAX, channel);
  return true;
}

Radio vp(22, 21, VSPI);
Radio hp(25, 15, HSPI);

void setup() {
  Serial.begin(115200);
  jamSetup();
  if(!hp.init()){
    Serial.println("hp didnt start");
  }
  
  if(!vp.init()){
    Serial.println("vp didnt start");
  }
}

void loop() {
  hp.jamAll(2);
  vp.jamAll(4);
}
