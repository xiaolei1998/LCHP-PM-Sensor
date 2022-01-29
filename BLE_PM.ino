#include <ArduinoBLE.h>


#define STRINGSIZE 50



/**********************************************Global variable declaration************************************/
//Bluetooth service
BLEService PMservice("ef12c126-80cf-11ec-a8a3-0242ac120002"); // create service

BLECharacteristic PMCharacteristic("ef12c126-80cf-11ec-a8a3-0242ac120002", BLERead, "stringValue"); //characteristic for PM sensor 1 and 2
BLECharacteristic PM3Characteristic("76b0499a-80ea-11ec-a8a3-0242ac120002", BLERead, "stringValue"); //characteristic for PM sensor 3
BLECharacteristic THCharacteristic("e2295132-80e8-11ec-a8a3-0242ac120002", BLERead, "stringValue"); //characteristic for temp and Humidity
BLECharacteristic FreqCharacteristic("36612c92-80ea-11ec-a8a3-0242ac120002", BLEWrite, "stringValue"); //characteristic for change frequency



int opFreq = 0;

struct pmData{
    uint16_t pm1;
    uint16_t pm2;
    uint16_t pm3;
    uint16_t temp;
    uint16_t hum;
    //int time;
}PMD;


char serialPM[STRINGSIZE] = {0};
char serialPM3[STRINGSIZE] = {0};
char serialTH[STRINGSIZE] = {0};


/************************************************************************************************************/
void setup(){
    Serial.begin(9600);
    while (!Serial);
    
    if (!BLE.begin()) {
      Serial.println("starting BLE failed!");

      while (1);
    }

    BLE.setLocalName("PM sensor");
    BLE.setAdvertisedService(PMservice);
    
    PMservice.addCharacteristic(PMCharacteristic);
    PMservice.addCharacteristic(PM3Characteristic);
    PMservice.addCharacteristic(THCharacteristic);
    PMservice.addCharacteristic(FreqCharacteristic);

    BLE.addService(PMservice);


    BLE.advertise();

    Serial.println("BLE PM Peripheral");

}


void loop(){
      BLEDevice central = BLE.central();
      

      // if a central is connected to smartphone
      if (central) {
        Serial.print("Connected to central: ");
        Serial.println(central.address());
    
        // When arduino connects to smartphone
        while (central.connected()) {
          
          if (FreqCharacteristic.written()) {
            updateFrequency(); 

          }else{
            /*send data out to smartphone*/
            sendData();
            
          }
        }
    
        // when the central disconnects, print it out:
        //Serial.print(F("Disconnected from central: "));
        Serial.println(central.address());
      }

}

/*Change the PM sensor reading freqeuncy to request frequency*/
inline void updateFrequency(){
  char requestFreq[12] = {0};
  FreqCharacteristic.readValue((void *)requestFreq,10);
  opFreq = atoi(requestFreq);
  Serial.print("Current operating Frequency => ");
  Serial.println(opFreq);
}


/*Send sensor's value out to smartphone*/
void sendData(){
  testDataGen();
  serialize();
  
  PMCharacteristic.writeValue(serialPM);
  PM3Characteristic.writeValue(serialPM3);
  THCharacteristic.writeValue(serialTH);
  
  //clear string
  memset(serialPM, '0',STRINGSIZE);
  memset(serialPM3, '0',STRINGSIZE);
  memset(serialTH, '0',STRINGSIZE);
}


void testDataGen(){
  PMD.pm1 = 7000;
  PMD.pm2 = 8000;
  PMD.pm3 = 7500;
  PMD.temp = 2200;
  PMD.hum = 3300;

}

inline void serialize(){
  sprintf(serialPM,"%d,%d",PMD.pm1,PMD.pm2);
  sprintf(serialPM3,"%d",PMD.pm3);
  sprintf(serialTH,"%d,%d",PMD.temp,PMD.hum);

}
