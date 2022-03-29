#include <Arduino.h>
#include <string>
#include <WiFiNINA.h>
#include <utility/wifi_drv.h>
#include <ArduinoBLE.h>
#include <SPI.h>
#include <SD.h>
#include <DS3231.h>
#include <Wire.h>
#include <DHT.h>


#define STRINGSIZE 10
#define LEDControl1 0
#define LEDControl2 1
#define LEDControl3 2
#define PM1 A0
#define PM2 A1
#define PM3 A2
#define RED 25
#define GREEN 26
#define BLUE 27
#define LEDDelay 280  //delay some time for value to stablize
#define ReadDelay 40  //time delayed after read;
#define defualtLEDDelay 500000 //2999680
#define SD_ENTRY_SIZE   25
#define DHTTYPE DHT11

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

/*define DHT temp&humid sensor port (D5) and type*/
DHT THsensor(5, DHTTYPE);



// int readInterval = 3; // reading interval (one read per how many seconds)
long LEDOffDelay = defualtLEDDelay; // LED off time. Initialized to 2999680 us (3s - 320us), 320us comes from sensor LED on delay + read delay

/*Bluetooth send out string*/
char serialPM[STRINGSIZE] = {0};


//median array index counter
int medianArrayLength = 5;
int medianIndex = 0;

//moving avg filtering window size
int windowSize = 10;
int windowIndex = 0;

/*Arrays used to find median*/
float PM1MedianArray[5];
float PM2MedianArray[5];
float PM3MedianArray[5];
float TempMedianArray[5];
float HumidMedianArray[5];

/*arays used to calculate moving */
float PM1WindowArray[10];
float PM2WindowArray[10];
float PM3WindowArray[10];
float TempWindowArray[10];
float HumidWindowArray[10];
float weightFactor = 0.5;

/*calibration factors*/
float aFactor = 0.17;
float bIntercept = 0.153;
float tempFactor = 0.0031; //0.0031
float humidFactor = 0.00064; //0.00064
float tempReference = 2;
float humidReference = 12;

/*filtered and compensated PM values*/
float roughPM1 = 0;
float roughPM2 = 0;
float roughPM3 = 0;
float finalPM = 0;


/*structure to store data*/
typedef struct{
  float PMAnalog[3];
  float PMPhys[3];
  float temp;
  float humid;
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
    analogReadResolution(12);

    //waits for serial connection with a port
    //while (!Serial);

    THsensor.begin();

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

/*Quick sort Algorithm for medianArray. Algorith inspired from Algolist.net, viewd on March.26, 2022, available at: https://www.algolist.net/Algorithms/Sorting/Quicksort */
void quickSort(float arr[], int left, int right) {
  int i = left, j = right;
  float tmp;
  float pivot = arr[(left + right) / 2];

  /* partition */
  while (i <= j) {
    while ((arr[i] - pivot) < 0){
      i++;
    }
    while ((arr[j] - pivot) > 0){
      j--;
    }   
    if (i <= j) {
      tmp = arr[i];
      arr[i] = arr[j];
      arr[j] = tmp;
      i++;
      j--;
    }
  }
  
  /* recursion */
  if (left < j){
    quickSort(arr, left, j);
  }
  if (i < right){
    quickSort(arr, i, right);
  }
}

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
    Serial.print("Rough PM1= " + String(roughPM1) + "ug/m3 ");
    Serial.print("Rough PM2 = " + String(roughPM1) + "ug/m3 ");
    Serial.println("Rough PM3 = " + String(roughPM1) + "ug/m3 ");

    for (int i=1; i<4; i++){
    //Serial.print("fine PM" + String(i) + " Voltage = " + String(PMD.PMAnalog[i-1]) + "V ");
    Serial.print("fine PM" + String(i) + " Density = " + String(PMD.PMPhys[i-1]) + "ug/m3 ");
    }
    Serial.println("");
    Serial.println("");


}


/*serialize the data so that it could be send through BLE*/
inline void serialize(){
  bool flag = false;
  String realtime = String(myRTC.getYear()) + "." + String(myRTC.getMonth(flag)) + "."+ String(myRTC.getDate()) + "  "+ String(myRTC.getHour(flag,flag))+":"+String(myRTC.getMinute())+":"+String(myRTC.getSecond());
  for (int i=0; i<3; i++){
    sprintf(serialPM,"%f",PMD.PMPhys[i]);
  }
  
  sd.time = realtime;
  sd.PMvalue = finalPM;
}


/*algorithm for PM conversion*/
void calcPM(){


  roughPM1 = float(PMD.PMAnalog[0]*aFactor - bIntercept);
  roughPM2 = float(PMD.PMAnalog[1]*aFactor - bIntercept);
  roughPM3 = float(PMD.PMAnalog[2]*aFactor - bIntercept);
  
  //0.0031 (V/celcius) is the temperature factor that converts temp diff to a voltage diff, the value came from reference[2]
  //0.00064 (V/RH%) is the humidity factor that converts humidity diff to a voltage diff, the value came from refernece[2]
  //24 is the base temperature (from reference[2] based on our intepretation)
  //30 is the base relative humidity (from reference[2] based on our intepretation)
  for (int i=0; i<3; i++){
    PMD.PMPhys[i] =  float(aFactor*(PMD.PMAnalog[i] + tempFactor*(PMD.temp - tempReference) + humidFactor*(PMD.humid - humidReference))- bIntercept);
  }

  /*merge results over here*/
    //final PM = ;

  serialize();
  printData();
}


/*Send sensor's value out to smartphone*/
void sendData(){

  //serialize();
  
  //****note:2.22 bug potential*****/
  PMCharacteristic.writeValue(serialPM);
  //clear string
  memset(serialPM, '0',STRINGSIZE);
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

/*Activate and read values from the sensor*/
void sense(){
    
    
    //turn sensor LED on, according to manual, optimum pulsewidth is 32ms
    digitalWrite(LEDControl1,HIGH);
    delayMicroseconds(LEDDelay);
    //read photodiode value at 28ms (peak value according to manual)
    PMD.PMAnalog[0] = analogRead(PM1);
    //Serial.println("Read complete");
    //let sensor LED stay on for another 4ms to complete the 32ms pulse
    delayMicroseconds(ReadDelay);
    //Serial.println("Read delay complete");
    //turn sensor LED off, light up onboard LED to indicate that a read is complete
    digitalWrite(LEDControl1,LOW);
    
    digitalWrite(LEDControl2,HIGH);
    delayMicroseconds(LEDDelay);
    PMD.PMAnalog[1] = analogRead(PM2);
    delayMicroseconds(ReadDelay);
    digitalWrite(LEDControl2,LOW);

    digitalWrite(LEDControl3,HIGH);
    delayMicroseconds(ReadDelay);
    PMD.PMAnalog[2] = analogRead(PM3);
    delayMicroseconds(ReadDelay);
    digitalWrite(LEDControl3,LOW);

    //reading takes 250ms, data refresh takes 2s in the sensor 
    float currentTemp = THsensor.readTemperature();
    float currentHum = THsensor.readHumidity();

    //if the temperature & humidity sensors are not reading values
    if (isnan(currentTemp) || isnan(currentHum)){
      Serial.println("invalid temperature or humidity");
    } else {
      PMD.temp = currentTemp;
      PMD.humid = currentHum;      
    }

    //convert ADC reading to voltage
    PM1MedianArray[medianIndex] = PMD.PMAnalog[0]/4096*5;
    PM2MedianArray[medianIndex] = PMD.PMAnalog[1]/4096*5;
    PM3MedianArray[medianIndex] = PMD.PMAnalog[2]/4096*5;

    //update the arrays for temperature and humidity
    TempMedianArray[medianIndex] = PMD.temp;
    HumidMedianArray[medianIndex] = PMD.humid;

    Serial.print("sensed PM 1 voltage = " + String(PM1MedianArray[medianIndex]) + "V ");
    Serial.print("sensed PM 2 voltage = " + String(PM2MedianArray[medianIndex]) + "V ");
    Serial.println("sensed PM 3 voltage = " + String(PM3MedianArray[medianIndex]) + "V ");
    Serial.print("sensed temperature = " + String(PMD.temp) + "C ");
    Serial.print("sensed humidity = " + String(PMD.humid) + "% ");
    Serial.println("");
    
    //find the medians
    if (medianIndex >= medianArrayLength-1){
      quickSort(PM1MedianArray,0, medianArrayLength-1);
      quickSort(PM2MedianArray,0, medianArrayLength-1);
      quickSort(PM3MedianArray,0, medianArrayLength-1);
      quickSort(TempMedianArray,0, medianArrayLength-1);
      quickSort(HumidMedianArray,0, medianArrayLength-1);

      PMD.PMAnalog[0] = PM1MedianArray[(medianArrayLength-1)/2];
      PMD.PMAnalog[1] = PM2MedianArray[(medianArrayLength-1)/2];
      PMD.PMAnalog[2] = PM3MedianArray[(medianArrayLength-1)/2];
      PMD.temp = TempMedianArray[(medianArrayLength-1)/2];
      PMD.humid = HumidMedianArray[(medianArrayLength-1)/2];


      Serial.print("Median PM1 voltage = " + String(PMD.PMAnalog[0]) + "V ");
      Serial.print("Median PM2 voltage = " + String(PMD.PMAnalog[1]) + "V ");
      Serial.print("Median PM3 voltage = " + String(PMD.PMAnalog[2]) + "V ");
      Serial.print("Median temperature = " + String(PMD.temp) + "C ");
      Serial.print("Median humidity = " + String(PMD.humid) + "% ");
      Serial.println("");
      
      
      //update the moving average window,if the number of data is < 10, skip the filtering process
      if (windowIndex < windowSize-1){

        PM1WindowArray[windowIndex] = PMD.PMAnalog[0];
        PM2WindowArray[windowIndex] = PMD.PMAnalog[1];
        PM3WindowArray[windowIndex] = PMD.PMAnalog[2];
        TempWindowArray[windowIndex] = PMD.temp;
        HumidWindowArray[windowIndex] = PMD.humid;

        windowIndex ++;
      }
      
      //if the number of data >= 10
      else{
        //shift all elements left by 1 in the window
        float PM1Sum = 0;
        float PM2Sum = 0;
        float PM3Sum = 0;
        float TempSum = 0;
        float HumidSum = 0;

        for (int i = 1; i <= windowSize; i++){
          PM1WindowArray[i-1] = PM1WindowArray[i];
          PM2WindowArray[i-1] = PM2WindowArray[i];
          PM3WindowArray[i-1] = PM3WindowArray[i];
          TempWindowArray[i-1] = TempWindowArray[i];
          HumidWindowArray[i-1] = HumidMedianArray[i];
        }

        //update the window's last element by current reading
        PM1WindowArray[windowSize-1] = PMD.PMAnalog[0];
        PM2WindowArray[windowSize-1] = PMD.PMAnalog[1];
        PM3WindowArray[windowSize-1] = PMD.PMAnalog[2];
        TempWindowArray[windowSize-1] = PMD.temp;
        HumidWindowArray[windowSize-1] = PMD.humid;

        //calculate the sum of the previous 9 values
        for (int j = 0; j < windowSize-1; j++){
          PM1Sum += PM1WindowArray[j];
          PM2Sum += PM2WindowArray[j];
          PM3Sum += PM3WindowArray[j];
          TempSum += TempWindowArray[j];
          HumidSum += HumidWindowArray[j];
        }

      //calculate the moving avg
        PMD.PMAnalog[0] = float((PM1WindowArray[windowSize-1]*weightFactor)+((PM1Sum/float(windowSize-1))*(1-weightFactor)));
        PMD.PMAnalog[1] = float((PM2WindowArray[windowSize-1]*weightFactor)+((PM2Sum/float(windowSize-1))*(1-weightFactor)));
        PMD.PMAnalog[2] = float((PM3WindowArray[windowSize-1]*weightFactor)+((PM3Sum/float(windowSize-1))*(1-weightFactor)));
        PMD.temp = float((TempWindowArray[windowSize-1]*weightFactor)+(TempSum/float(windowSize-1))*(1-weightFactor));
        PMD.humid = float((HumidWindowArray[windowSize-1]*weightFactor)+(HumidSum/float(windowSize-1))*(1-weightFactor));

      Serial.println("");
      Serial.print("filtered PM1 voltage = " + String(PMD.PMAnalog[0]) + "V ");
      Serial.print("filtered PM2 voltage = " + String(PMD.PMAnalog[1]) + "V ");
      Serial.print("filtered PM3 voltage = " + String(PMD.PMAnalog[2]) + "V ");
      Serial.print("filtered temperature = " + String(PMD.temp) + "C ");
      Serial.print("filtered humidity = " + String(PMD.humid) + "% ");
      Serial.println("");
      Serial.println("");
      }

      //go to PM calculation algorithm and write the value to SD card
      calcPM();
      SD_write();
      
      medianIndex = 0;
    }
  else{
    medianIndex ++;
  }
    
  delayMicroseconds(LEDOffDelay);

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
          Serial.println("");
        }
      }
      //after connection is finished
      Serial.println("************Bluetooth disconected from central:");
      Serial.println(central.address());
    }
    else{
      sense();
      sendData();
      //Serial.println("");
      //Serial.println("Central not connected");
    }

    LEDOffDelay = defualtLEDDelay;

}