#include <Arduino.h>
#include <string>
#include <WiFiNINA.h>
#include <utility/wifi_drv.h>
#include <ArduinoBLE.h>


#define STRINGSIZE 10
#define LEDControl1 0
#define LEDControl2 1
#define LEDControl3 6
#define PM1 A0
#define PM2 A1
#define PM3 A2
#define RED 25
#define GREEN 26
#define BLUE 27
#define LEDDelay 280  //delay some time for value to stablize
#define ReadDelay 40  //time delayed after read;
#define LEDOffTime 100 //time when LED is off


/******************************************************Global variables*****************************************************************/
/*Bluetooth service*/
BLEService PMservice("ef12c126-80cf-11ec-a8a3-0242ac120002"); // create service
BLECharacteristic PMCharacteristic("76b0499a-80ea-11ec-a8a3-0242ac120002", BLERead | BLENotify, "stringValue"); //characteristic for PM value
BLECharacteristic FreqCharacteristic("36612c92-80ea-11ec-a8a3-0242ac120002", BLEWrite, "stringValue"); //characteristic for change frequency

/*Sensor variables*/
//float DensityAnalog1 = 0; //PM voltage value (after ADC, max 1023 = 5V, min 0 = 0V)
//float DensityPhys = 0; //converted real physical value
//int OutputCounter = 0;

float roughPM = 0;

int opFreq = 0; // reading frequency

/*Bluetooth send out string*/
char serialPM[STRINGSIZE] = {0};


/*structure to store data*/
typedef struct{
  float PMAnalog[3];
  float PMPhys[3];
  uint16_t temp;
  uint16_t hum;
  //int time;
}Data;

Data PMD;

/****************************************************************Setup*****************************************************************/
void setup() {
    // put your setup code here, to run once:
    Serial.begin(115200);

    pinMode(LEDControl1,OUTPUT);
    pinMode(LEDControl2,OUTPUT);
    pinMode(LEDControl3,OUTPUT);
    pinMode(PM1,INPUT);
    pinMode(PM2,INPUT);
    pinMode(PM3,INPUT);
    WiFiDrv::pinMode(RED,OUTPUT);
    WiFiDrv::pinMode(GREEN,OUTPUT);
    WiFiDrv::pinMode(BLUE,OUTPUT);

    //waits for serial connection with a port
    while (!Serial);

    //wait for bluetooth device to initialize successfully
    if (!BLE.begin()) {
      Serial.println("starting BLE failed!");
      while (1);
    }

    //set this device as PM sensor
    BLE.setLocalName("PM sensor");
    //set advertised service as PMservice
    BLE.setAdvertisedService(PMservice);

    PMservice.addCharacteristic(PMCharacteristic);
    PMservice.addCharacteristic(FreqCharacteristic);

    BLE.addService(PMservice);
    BLE.advertise();

    Serial.println("BLE PM Peripheral");
  

}


/********************************************Methods************************************************/

/*Change the PM sensor reading freqeuncy to request frequency*/
inline void updateFrequency(){
  char requestFreq[12] = {0};
  FreqCharacteristic.readValue((void *)requestFreq,10);
  opFreq = atoi(requestFreq);
  Serial.print("Current operating Frequency => ");
  Serial.println(opFreq);
}

void writeDataToSD(){
    /*to be implemented*/
}

inline void serialize(){
  
  sprintf(serialPM,"%f",roughPM);
}


/*Send sensor's value out to smartphone*/
void sendData(){

  calcPM();
  serialize();
  
  PMCharacteristic.writeValue(serialPM);
  //clear string
  memset(serialPM, '0',STRINGSIZE);
}

void calcPM(){

  for(int i = 0; i < 3; i++){
    roughPM = PMD.PMPhys[i] + roughPM;
  }

  roughPM = roughPM/3;
}

void detecting(){
    //turn sensor LED on, according to manual, optimum pulsewidth is 32ms
    digitalWrite(LEDControl1,HIGH);
    digitalWrite(LEDControl2,HIGH);
    digitalWrite(LEDControl3,HIGH);
    delayMicroseconds(LEDDelay);
    Serial.println("LED ON");

    //read photodiode value at 28ms (peak value according to manual)
    PMD.PMAnalog[0] = analogRead(PM1);
    PMD.PMAnalog[1] = analogRead(PM2);
    PMD.PMAnalog[2] = analogRead(PM3);
    Serial.println("Read complete");

    //let sensor LED stay on for another 4ms to complete the 32ms pulse
    delayMicroseconds(ReadDelay);
    Serial.println("Read delay complete");

    //turn sensor LED off, light up onboard LED to indicate that a read is complete
    digitalWrite(LEDControl1,LOW);
    digitalWrite(LEDControl2,LOW);
    digitalWrite(LEDControl3,LOW);
    Serial.println("LED Off");


    //turning on-board LED on and off to indicate a read has finished. WiFiDrv::analogWrite does not work, maybe in conflict with BLE library 
    /*WiFiDrv::analogWrite(RED, 255);
    delay(400);
    WiFiDrv::analogWrite(RED, 0);
    delayMicroseconds(5680);*/

    //delay for sensor LED = off
    delay(3000);
    Serial.println("Off delay complete");

    writeDataToSD();
    printData();

}

void printData(){
    //convert then store the read analog voltage to physical PM density for each sensor, print both values to serial port
    for (int i=1; i<4; i++){
    PMD.PMPhys[i-1] = float(((PMD.PMAnalog[i-1]/1024) - 0.0356)*12000*0.035);
    Serial.print("PM" + String(i) + " Voltage = " + String(PMD.PMAnalog[i-1]/1024*5) + "V ");
    Serial.print("PM" + String(i) + " Density = " + String(PMD.PMPhys[i-1]) + "ug/m3 ");
    }
    Serial.println("");
}

/*********************************************Main loop***************************************************/

void loop() {

    //listen for BLE peripheral devices to connect
    BLEDevice central = BLE.central();

    // if a central is connected to smartphone
    if (central) {
      //print central's MAC address
      Serial.print("Connected to central: ");
      Serial.println(central.address());

      // When arduino is still connected to smartphone
      while (central.connected()) {
        
        if (FreqCharacteristic.written()) {
          updateFrequency();
        }else{
          //send data out to smartphone
          detecting();
          sendData();
        }
      }
      //after connection is finished
      Serial.println("Bluetooth disconected from central:");
      Serial.println(central.address());
    } 

    /*to be implemented */
    /*set back to defualt(no ble connection frequency*/
    detecting();
    writeDataToSD();
    


}