#include <Arduino.h>
#include <string>
#include <string.h>
#include <WiFiNINA.h>
#include <utility/wifi_drv.h>
#include <ArduinoBLE.h>
#include <SPI.h>
#include <SD.h>
#include <DS3231.h>
#include <Wire.h>
#include <DHT.h>

/*
used for tensorflow that running on arduino; However, it wastes too much ram and flash
so we will just use the equation with the coeffiction that trained by the module;
This is just for illustration that tensorflow model can be run on Arduino
#include <EloquentTinyML.h>
#include "model.h"   //modle trained by concentration
#define NUM_INPUTS 5
#define NUM_OUTPUTS 1
#define Tensor_area 5*1024
Eloquent::TinyML::TfLite<NUM_INPUTS,NUM_OUTPUTS,Tensor_area> my_model;
*/

#define MODULE_USE_CONCENTRATION
// #define MODULE_USE_CONCENTRATION_WO_TEMPHUM
// #define MODULE_USE_LOWRATIO
// #define MODULE_USE_LOWRATIO_WO_TEMPHUM

#define STRINGSIZE 10
#define PM1 0
#define PM2 5
#define PM3 3
#define DHTpin 7 
#define RED 25
#define GREEN 26
#define BLUE 27
#define sampletime_ms 500  //delay some time for value to stablize
#define defualtLEDDelay 10 



#define DHTTYPE DHT11
#define SD_MAX_BLOCK    3900000  //for 1 GB : 10^9 / (25*#SD_BLOCK_WRITE_COUNT)
//#define SD_MAX_BLOCK    7900000  //for 2 GB : 2*10^9 /(25*#SD_BLOCK_WRITE_COUNT)
#define SD_BLOCK_WRITE_COUNT 30 //一个block含5条


/*filter features*/
#define windowSize 5
#define medianArrayLength 3

/******************************************************Global variables*****************************************************************/
/*Bluetooth service*/
BLEService PMservice("ef12c126-80cf-11ec-a8a3-0242ac120002"); // create service
BLECharacteristic PMCharacteristic("76b0499a-80ea-11ec-a8a3-0242ac120002", BLERead | BLENotify, "stringValue"); //characteristic for PM value
BLECharacteristic FreqCharacteristic("36612c92-80ea-11ec-a8a3-0242ac120002", BLEWrite, "stringValue"); //characteristic for change frequency
BLECharacteristic dateCharacteristic("f37e1b98-afdc-11ec-b909-0242ac120002", BLEWrite, "stringValue"); //characteristic for update time for RTC



/*define DHT temp&humid sensor port (D7) and type*/
DHT THsensor(DHTpin, DHTTYPE);



long LEDOffDelay = defualtLEDDelay; // LED off time. Initialized to 2999680 us (3s - 320us), 320us comes from sensor LED on delay + read delay

/*Bluetooth send out string*/
char serialPM[STRINGSIZE] = {0};


//median array index counter
int medianIndex = 0;
//moving avg filtering window size
int windowIndex = 0;

/*Arrays used to find median*/
float PM1MedianArray[medianArrayLength];
float PM2MedianArray[medianArrayLength];
float PM3MedianArray[medianArrayLength];
float TempMedianArray[medianArrayLength];
float HumidMedianArray[medianArrayLength];

/*arays used to calculate moving */
float PM1WindowArray[windowSize];
float PM2WindowArray[windowSize];
float PM3WindowArray[windowSize];
float TempWindowArray[windowSize];
float HumidWindowArray[windowSize];
float weightFactor = 0.5;


/*MRL and compensated PM values*/
float finalPM = 0;


/*structure to store data*/
typedef struct{
  float PMAnalog[3];
  float temp;
  float humid;
}Data;


typedef struct{
  float PMvalue;
  String time;
}SD_data;

Data PMD;
DS3231 myRTC;

String user_time;


/*SD config*/
SD_data      sd;
File         myfile;
uint32_t     entry_count;
uint32_t     dummytime; //for test
SD_data      RAM_BUFFER[SD_BLOCK_WRITE_COUNT];
uint32_t     BLOCK_COUNT;  
bool         hot_swap;   //false when sd card is removed, true is sd card is installed
uint32_t     file_pointer;
File dataset;


/****************************************************************Setup*****************************************************************/
void setup() {  //version for no ble connection

  Serial.begin(9600);

  /*Initialize sensors */
  pinMode(PM1,INPUT);
  pinMode(PM2,INPUT);
  pinMode(PM3,INPUT);

  //waits for serial connection with a port
  while (!Serial);

  THsensor.begin();
  //my_model.begin(model);

  /*initialize SD module*/
  if (!SD.begin(4)) {
    Serial.println("SD initialization failed!");
    //while (1);
  }else{
    Serial.println("SD initialization done!!!!!");
      myfile = SD.open("PM.csv",O_READ | O_WRITE|O_CREAT);
      if(myfile) Serial.println("file created\n");
      myfile.close();
      entry_count = 0;
  }

  /*initializing RTC Module*/
  Wire.begin();
  /***for no ble connection only*******/
  myRTC.setClockMode(false); 
  myRTC.setYear(22);
  myRTC.setMonth(3);
  myRTC.setDate(28);
  myRTC.setHour(14);
  myRTC.setMinute(54);
  myRTC.setSecond(48);
  /***for testing only*******/
}

void setup1() {   //version for BLE connection
  Serial.begin(9600);
  pinMode(A0,INPUT);
  pinMode(A1,INPUT);


  THsensor.begin();

  if (!SD.begin(4)) {
    Serial.println("SD initialization failed!");
    //while (1);
  }else{
    Serial.println("SD initialization done!!!!!");
  }
  myfile = SD.open("PM.csv",O_READ | O_WRITE|O_CREAT);
  if(myfile) Serial.println("file created\n");
  myfile.close();
  entry_count = 0;


      /*initializing BLE Module*/
  if (!BLE.begin()) {
    Serial.println("starting BLE failed!");
    //while (1); //xiaolei testing
  }else{
        BLE.setLocalName("PM sensor");
        BLE.setAdvertisedService(PMservice);
        Serial.println("PMsensor active, waiting for connections...");

        PMservice.addCharacteristic(PMCharacteristic);
        PMservice.addCharacteristic(FreqCharacteristic);
        PMservice.addCharacteristic(dateCharacteristic);

        BLE.addService(PMservice);
        BLE.advertise();

        Serial.println("BLE PM Peripheral");
  }

    //for testing:
  dataset = SD.open("PMvalue.csv",O_READ | O_WRITE|O_CREAT);
  dataset.close();
    /*initializing RTC Module*/
  Wire.begin();
  /***for no ble connection only*******/
  myRTC.setClockMode(false); 
  myRTC.setYear(22);
  myRTC.setMonth(2);
  myRTC.setDate(28);
  myRTC.setHour(14);
  myRTC.setMinute(54);
  myRTC.setSecond(48);
  /***for testing only*******/


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
  LEDOffDelay = (atoi(inputInterval) * 1000) - sampletime_ms;
  Serial.print("Current read interval = " + String((LEDOffDelay + sampletime_ms)/(1000)) + "s        ");
  Serial.print("Current off delay = " + String(LEDOffDelay) + "us");
  Serial.println("");
  Serial.println("");
}

/*update the time for rtc*/
void updateRTCTime(){
  char usertime[25] = {0};
  String timedata;
  int time_date[6] = {0};
  dateCharacteristic.readValue((void *)usertime,25);
  timedata = usertime;

  //Serial.println(usertime);

  time_date[0] = timedata.substring(0,2).toInt();
  time_date[1] = timedata.substring(2,4).toInt();
  time_date[2] = timedata.substring(4,6).toInt();
  time_date[3] = timedata.substring(6,8).toInt();
  time_date[4] = timedata.substring(8,10).toInt();
  time_date[5] = timedata.substring(10,12).toInt();


  myRTC.setClockMode(false); 
  myRTC.setYear(time_date[0]);
  myRTC.setMonth(time_date[1]);
  myRTC.setDate(time_date[2]);
  myRTC.setHour(time_date[3]);
  myRTC.setMinute(time_date[4]);
  myRTC.setSecond(time_date[5]);

  bool flag = false;
  Serial.println(String(myRTC.getYear()) + "-" + myRTC.getMonth(flag) + "-" + myRTC.getDate()+ "  "+myRTC.getHour(flag,flag)+ ":"+ myRTC.getMinute()+ ":"+myRTC.getSecond());
}




/*serialize the data so that it could be send through BLE*/
inline void serialize(){
  bool flag = false;
  String realtime = String(myRTC.getYear()) + "." + String(myRTC.getMonth(flag)) + "."+ String(myRTC.getDate()) + "  "+ String(myRTC.getHour(flag,flag))+":"+String(myRTC.getMinute())+":"+String(myRTC.getSecond());
  
  sd.time = realtime;
  sd.PMvalue = finalPM;
}

void SD_write(){

  serialize();
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

/*algorithm for PM conversion      Machine Learning goes here*/
void calcPM(){
  
  /*prediction by using TinyML
    float input[NUM_INPUTS] = {PMD.PMAnalog[0],PMD.PMAnalog[1],PMD.PMAnalog[2],PMD.humid,PMD.temp};
    finalPM = my_model.predict(input);
  */

#if defined(MODULE_USE_CONCENTRATION)

  finalPM = 0.6501466*PMD.PMAnalog[0] + 0.00596511* PMD.PMAnalog[1] + 0.88111126 * PMD.PMAnalog[2] + 25.065847 * PMD.humid + (-11.856685) * PMD.temp + 6.8888903;

#elif defined(MODULE_USE_CONCENTRATION_WO_TEMPHUM) 

  finalPM = 0.6501466*PMD.PMAnalog[0] + 0.00596511* PMD.PMAnalog[1] + 0.88111126 * PMD.PMAnalog[2] + 25.065847 * PMD.humid + (-11.856685) * PMD.temp + 6.8888903;

#elif defined(MODULE_USE_LOWRATIO)

  finalPM = 0.6501466*PMD.PMAnalog[0] + 0.00596511* PMD.PMAnalog[1] + 0.88111126 * PMD.PMAnalog[2] + 25.065847 * PMD.humid + (-11.856685) * PMD.temp + 6.8888903;

#elif defined(MODULE_USE_LOWRATIO_WO_TEMPHUM)

  finalPM = 0.6501466*PMD.PMAnalog[0] + 0.00596511* PMD.PMAnalog[1] + 0.88111126 * PMD.PMAnalog[2] + 25.065847 * PMD.humid + (-11.856685) * PMD.temp + 6.8888903;

#else 
  Serial.println("please select a module for multivariable linear regresion");
  while(1);
#endif


  
  //最后需要放开展示，关闭为了采集数据集
  Serial.print("the final PM after learning is:  ");
  Serial.print(finalPM);
  Serial.println("    ug/m3");

}


/*Send sensor's value out to smartphone*/
void sendData(){
  sprintf(serialPM,"%f",finalPM);
  PMCharacteristic.writeValue(serialPM);
  memset(serialPM, '0',STRINGSIZE);
}




void gen_data_set(){

  String buf = String(PMD.PMAnalog[0])+ "," + String(PMD.PMAnalog[1])+ "," + String(PMD.PMAnalog[2])+ "," + String(PMD.humid)+ "," + String(PMD.temp)+ ",";
  Serial.println(buf);

}


void filter(){
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

            
      //update the moving average window,if the number of data is < 10, skip the filtering process
      if (windowIndex < windowSize-1){

        PM1WindowArray[windowIndex] = PMD.PMAnalog[0];
        PM2WindowArray[windowIndex] = PMD.PMAnalog[1];
        PM3WindowArray[windowIndex] = PMD.PMAnalog[2];
        TempWindowArray[windowIndex] = PMD.temp;
        HumidWindowArray[windowIndex] = PMD.humid;

        windowIndex ++;
      }else{
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


        // Serial.println("");
        // Serial.print("filtered PM1 low ratio = " + String(PMD.PMAnalog[0]) + "%  ");
        // Serial.print("filtered PM2 low ratio = " + String(PMD.PMAnalog[1]) + "%  ");
        // Serial.print("filtered PM3 low ratio = " + String(PMD.PMAnalog[2]) + "%  ");
        // Serial.print("filtered temperature = " + String(PMD.temp) + "c   ");
        // Serial.print("filtered humidity = " + String(PMD.humid) + "%");
        // Serial.println("");

        gen_data_set();
        calcPM();
        //SD_write();    //关闭为了采集数据

      }

      medianIndex = 0;
   }else{
    medianIndex ++;
  }
}


/*Activate and read values from the sensor*/
void sense(){

  /*Sensor variables*/
  unsigned long starttime;

  unsigned long duration1;
  unsigned long duration2;
  unsigned long duration3;
  unsigned long lowpulseoccupancy1 = 0;
  unsigned long lowpulseoccupancy2 = 0;
  unsigned long lowpulseoccupancy3 = 0;
  float ratio1 = 0;
  float ratio2 = 0;
  float ratio3 = 0;
  float concentration1 = 0;
  float concentration2 = 0;
  float concentration3 = 0;


  starttime = millis();
  while(1){
    duration1 = pulseIn(PM1, LOW);
    duration2 = pulseIn(PM2, LOW);
    duration3 = pulseIn(PM3, LOW);


    lowpulseoccupancy1 = lowpulseoccupancy1+duration1;
    lowpulseoccupancy2 = lowpulseoccupancy2+duration2;
    lowpulseoccupancy3 = lowpulseoccupancy3+duration3;

    if ((millis()-starttime) > sampletime_ms){//if the sampel time == 30s
      ratio1 = lowpulseoccupancy1/(sampletime_ms*10.0);  // Integer percentage 0=>100
      concentration1 = 1.1*pow(ratio1,3)-3.8*pow(ratio1,2)+520*ratio1+0.62; // using spec sheet curve

      ratio2 = lowpulseoccupancy2/(sampletime_ms*10.0);  // Integer percentage 0=>100
      concentration2 = 1.1*pow(ratio2,3)-3.8*pow(ratio2,2)+520*ratio2+0.62; 

      ratio3 = lowpulseoccupancy3/(sampletime_ms*10.0);  // Integer percentage 0=>100
      concentration2 = 1.1*pow(ratio2,3)-3.8*pow(ratio2,2)+520*ratio2+0.62; 

      // Serial.print("concentration1 = ");
      // Serial.print(concentration1);
      // Serial.print(" pcs/0.01cf  -  ");
      // Serial.print("concentration2 = ");
      // Serial.print(concentration2);
      // Serial.println(" pcs/0.01cf  -  ");
        
      lowpulseoccupancy1 = 0;
      lowpulseoccupancy2 = 0;
      lowpulseoccupancy3 = 0;

      break;
    }
  }

  
   //reading takes 250ms, data refresh takes 2s in the sensor 
  float currentTemp = THsensor.readTemperature();
  float currentHum = THsensor.readHumidity();

  
  if (isnan(currentTemp) || isnan(currentHum)){
    //if the temperature & humidity sensors are not reading values, use the mid;
    PMD.temp = TempMedianArray[medianIndex];  
    PMD.humid = HumidMedianArray[medianIndex];
  } else {
    PMD.temp = currentTemp;
    PMD.humid = currentHum; 
  }



#if defined(MODULE_USE_CONCENTRATION) || defined(MODULE_USE_CONCENTRATION_WO_TEMPHUM)
  PM1MedianArray[medianIndex] = concentration1;
  PM2MedianArray[medianIndex] = concentration2;
  PM3MedianArray[medianIndex] = concentration2;   //记得新sensor来了之后给改了！！！！！！！
#else
  PM1MedianArray[medianIndex] = ratio1;
  PM2MedianArray[medianIndex] = ratio2;
  PM3MedianArray[medianIndex] = ratio2;   //记得新sensor来了之后给改了！！！！！！！
#endif


  // //update the arrays for temperature and humidity
  TempMedianArray[medianIndex] = PMD.temp;
  HumidMedianArray[medianIndex] = PMD.humid;

  filter();


  delay(LEDOffDelay);

}


/*********************************************Main loop***************************************************/
void loop(){
  sense();
  //SD_write();   //xiaoleit testing for data set collection

}

void loop1() {
    
    /*to be implemented */
    /*set back to defualt(no ble connection frequency*/
    sense();
    SD_write();
    

    //listen for BLE peripheral devices to connect
    BLEDevice central = BLE.central();

    // if a central is connected to smartphone
    if (central) {
      //print central's MAC address
      Serial.println("");
      Serial.print("**************Connected to central: ");
      Serial.println(central.address());
      Serial.println("************");
      Serial.println("");

      // When arduino is still connected to smartphone
      while (central.connected()) {
        if (FreqCharacteristic.written()) {
          updateReadInterval();
        }

        if(dateCharacteristic.written()){
          updateRTCTime();
        }

        //send data out to smartphone
        sense();
        sendData();
        SD_write();
      }
      //after connection is finished
      Serial.println("************Bluetooth disconected from central:");
      Serial.println(central.address());
      Serial.println("************");
    }

    LEDOffDelay = defualtLEDDelay;    

}