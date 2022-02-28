#include <Arduino.h>
#include <string>
#include <WiFiNINA.h>
#include <utility/wifi_drv.h>
#include <ArduinoBLE.h>
#include <SPI.h>
#include <SD.h>
#include <DS3231.h>
#include <Wire.h>


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
#define defualtLEDDelay 2999680
#define SD_ENTRY_SIZE   25

//#define SD_MAX_BLOCK    3900000  //for 1 GB : 10^9 / (25*#SD_BLOCK_WRITE_COUNT)
//#define SD_MAX_BLOCK    7900000  //for 2 GB : 2*10^9 /(25*#SD_BLOCK_WRITE_COUNT)
#define SD_MAX_BLOCK    5//for test 最多写5个block

//#define SD_BLOCK_WRITE_COUNT 100;  //block write
#define SD_BLOCK_WRITE_COUNT 5 //一个block含5条


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

// int readInterval = 3; // reading interval (one read per how many seconds)
long LEDOffDelay = defualtLEDDelay; // LED off time. Initialized to 2999680 us (3s - 320us), 320us comes from sensor LED on delay + read delay

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


typedef struct{
  float PMvalue;
  String time;
}SD_data;

Data PMD;
DS3231 myRTC;


/*SD config*/
// char *       FILE_NAME;
SD_data      sd;
File         myfile;
uint32_t     entry_count;
uint32_t     dummytime; //for test
SD_data      RAM_BUFFER[SD_BLOCK_WRITE_COUNT];
uint32_t     BLOCK_COUNT;  
bool         hot_swap;   //false when sd card is removed, true is sd card is installed
uint32_t     file_pointer;

/****************************************************************Setup*****************************************************************/
void setup() {

  /*Initialize sensors */
    //Serial.begin(115200);
    Serial.begin(9600);

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



    /*initialize SD module*/
    if (!SD.begin(4)) {
      Serial.println("SD initialization failed!");
      //while (1);
    }
    myfile = SD.open("PM.csv",O_READ | O_WRITE|O_CREAT);
    if(myfile) Serial.println("file created\n");
    myfile.close();
    entry_count = 0;


    /*initializing RTC Module*/
    Wire.begin();
    /***for testing only*******/
    myRTC.setClockMode(false); 
    myRTC.setYear(22);
    myRTC.setMonth(2);
    myRTC.setDate(28);
    myRTC.setHour(14);
    myRTC.setMinute(54);
    myRTC.setSecond(48);
    /***for testing only*******/
    



    /*initializing BLE Module*/
    if (!BLE.begin()) {
      Serial.println("starting BLE failed!");
      while (1);
    }


    BLE.setLocalName("PM sensor");
    BLE.setAdvertisedService(PMservice);
    Serial.println("PMsensor active, waiting for connections...");

    PMservice.addCharacteristic(PMCharacteristic);
    PMservice.addCharacteristic(FreqCharacteristic);

    BLE.addService(PMservice);
    BLE.advertise();

    Serial.println("BLE PM Peripheral");
  

}


/********************************************Methods************************************************/

/*Change the PM sensor reading freqeuncy to request frequency*/
inline void updateReadInterval(){
  Serial.println("");
  char inputInterval[10] = {0};
  FreqCharacteristic.readValue((void *)inputInterval,10);
  Serial.println("input interval = " + String(inputInterval) + "s        ");
  //readInterval = atoi(inputInterval);
  LEDOffDelay = (atoi(inputInterval) * 1000000) - 320;
  Serial.print("Current read interval = " + String((LEDOffDelay + 320)/(1000000)) + "s        ");
  Serial.print("Current off delay = " + String(LEDOffDelay) + "us");
  Serial.println("");
  Serial.println("");
}


/*print values to serial port*/
void printData(){
    //convert then store the read analog voltage to physical PM density for each sensor, print both values to serial port
    for (int i=1; i<4; i++){
    Serial.print("PM" + String(i) + " Voltage = " + String(PMD.PMAnalog[i-1]) + "V ");
    Serial.print("PM" + String(i) + " Density = " + String(PMD.PMPhys[i-1]) + "ug/m3 ");
    }
    Serial.println("");
    Serial.println("Avergaed PM = " + String(roughPM) + "ug/m3 ");
    
}


/*serialize the data so that it could be send through BLE*/
inline void serialize(){
  bool flag = false;
  String realtime = String(myRTC.getYear()) + "." + String(myRTC.getMonth(flag)) + "."+ String(myRTC.getDate()) + "  "+ String(myRTC.getHour(flag,flag))+":"+String(myRTC.getMinute())+":"+String(myRTC.getSecond());
  sprintf(serialPM,"%f",roughPM);
  //Serial.println("hello");
  //Serial.println(realtime);
  sd.time = realtime;
  sd.PMvalue = roughPM;
}


/*algorithm for PM conversion*/
void calcPM(){

  for(int i = 0; i < 3; i++){
    PMD.PMPhys[i] = float(((PMD.PMAnalog[i]/1024) - 0.0356)*12000*0.035);
    PMD.PMAnalog[i] = PMD.PMAnalog[i]/1024*5;
    roughPM = PMD.PMPhys[i] + roughPM;
  }

  roughPM = roughPM/3;
  sd.PMvalue = roughPM;
  serialize();
  //printData();

}


/*Send sensor's value out to smartphone*/
void sendData(){

  //serialize();
  
  //****note:2.22 bug potential*****/
  PMCharacteristic.writeValue(serialPM);
  //clear string
  memset(serialPM, '0',STRINGSIZE);
}



/*Activate and read values from the sensor*/
void sense(){
    //turn sensor LED on, according to manual, optimum pulsewidth is 32ms
    digitalWrite(LEDControl1,HIGH);
    digitalWrite(LEDControl2,HIGH);
    digitalWrite(LEDControl3,HIGH);
    delayMicroseconds(LEDDelay);
    //Serial.println("LED ON");

    //read photodiode value at 28ms (peak value according to manual)
    PMD.PMAnalog[0] = analogRead(PM1);
    PMD.PMAnalog[1] = analogRead(PM2);
    PMD.PMAnalog[2] = analogRead(PM3);
    //Serial.println("Read complete");

    //let sensor LED stay on for another 4ms to complete the 32ms pulse
    delayMicroseconds(ReadDelay);
    //Serial.println("Read delay complete");

    //turn sensor LED off, light up onboard LED to indicate that a read is complete
    digitalWrite(LEDControl1,LOW);
    digitalWrite(LEDControl2,LOW);
    digitalWrite(LEDControl3,LOW);
    //Serial.println("LED Off");

    calcPM();
    SD_write();

    //turning on-board LED on and off to indicate a read has finished. WiFiDrv::analogWrite does not work, maybe in conflict with BLE library 
    /*WiFiDrv::analogWrite(RED, 255);
    delay(400);
    WiFiDrv::analogWrite(RED, 0);
    delayMicroseconds(5680);*/

    //delay for sensor LED = off
    delayMicroseconds(LEDOffDelay);
    //Serial.println("Off delay complete");
}


void SD_write(){

  RAM_BUFFER[entry_count] = sd;
  entry_count++;


  //if ram buffer is full; move data from ram to sd card
  if(entry_count == SD_BLOCK_WRITE_COUNT){



    if(!SD.begin(4)){
        entry_count = 0;
        Serial.println("WARNING: SD card is not present!\n");
        return;
    }else{
        if(SD.exists("PM.csv")){
          myfile = SD.open("PM.csv",O_READ | O_WRITE);
          myfile.seek(file_pointer);
          Serial.println("log: file opened");
        }else{
          myfile = SD.open("PM.csv",O_READ | O_WRITE | O_CREAT);
          myfile.seek(0);
          Serial.println("log: file created");
          BLOCK_COUNT = 0;
        }
    }



    if(myfile){
      //If full, use circular buffer, write from beginning;
      if(BLOCK_COUNT==SD_MAX_BLOCK){
        myfile.seek(0);
        //Serial.println(myfile.position());
        BLOCK_COUNT = 0;
        Serial.println("log: circular buffer wrap around");
      }

      for(int i = 0; i<SD_BLOCK_WRITE_COUNT;i++){
          String buf = RAM_BUFFER[i].time + "," + String(RAM_BUFFER[i].PMvalue);
          //dummytime++;
          //String buf = String(dummytime)+","+String(RAM_BUFFER[i].PMvalue);
          //Serial.println(myfile.position());
          myfile.println(buf);
      }
      file_pointer = myfile.position();
      //next update!!!!    don't delete, when rtc is ready
      //prevent hot swap during writing. writing is not finished
      //file_pointer = file_pointer - file_pointer%SD_ENTRY_SIZE;

      entry_count = 0;
      BLOCK_COUNT++;
      myfile.close();
      Serial.println("log : WRITE DONE ********\n");
    }else{
      Serial.println("WARNING: file open error\n");
    }
  }
}
/*********************************************Main loop***************************************************/

void loop() {

    /*to be implemented */
    /*set back to defualt(no ble connection frequency*/
    sense();

    //listen for BLE peripheral devices to connect
    BLEDevice central = BLE.central();

    // if a central is connected to smartphone
    if (central) {
      //print central's MAC address
      Serial.println("");
      Serial.print("**************Connected to central: ");
      Serial.println(central.address());
      Serial.println("");

      // When arduino is still connected to smartphone
      while (central.connected()) {
        
        if (FreqCharacteristic.written()) {
          updateReadInterval();
        }else{
          //send data out to smartphone
          sense();
          sendData();
          roughPM = 0;
          Serial.println("");
        }
      }
      //after connection is finished
      Serial.println("************Bluetooth disconected from central:");
      Serial.println(central.address());
    }
    else{
      Serial.println("Central not connected");
    }

    LEDOffDelay = defualtLEDDelay;
    roughPM = 0; 

}
