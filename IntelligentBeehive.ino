/*
  This code is meant to run on the Arduino Yun. It will incorporate data acquired from
    * RHT03 sensor (relative humidity and temperature sensor)
    * SparkFun Electret Microphone
    * Walgreens Digital Glass Scale (connections instructions in other document)

  This sketch will make use of the Temboo library to append data to a Google Spreadsheet. 

  With a Google account, and a spreadsheet, you can constantly be uploading data in real 
  time. The columns in the spreadsheet must be labeled. The labels do not matter. 

  You will also need a Temboo account and can update your Temboo account information in 
  the accompanying TembooAccount.h file. For more information about how to do this, visit 
  https://www.temboo.com/arduino/update-google-spreadsheet.

  The current code has been calibrated and works with a Walgreens Digital Glass Scale. A 
  full document on how to connect the scale to the Arduino can be found online at 
  http://github.com/glenmeyerowitz/readScale. The scale will need to be calibrated before 
  use. Calibration instructions are also online. 

  March 17, 2014
  Glen Meyerowitz

  This code is in the public domain and part of Yale Bee Space intelligent beehive project. 
*/

#include <Bridge.h>
#include <dht.h>
#include <Temboo.h>
#include "TembooAccount.h"

double readScale();        // returns double that is equal to the weight of the objecto on the scale
int readMic();             // returns int that is equal to the analog value of the microphone signal
dht readRHT(int thisPin);  // returns DHT object that contains information about temperature and humidity 

const String GOOGLE_USERNAME = "your-google-username";
const String GOOGLE_PASSWORD = "your-google-password";
const String SPREADSHEET_TITLE = "your-spreadsheet-title";
String startString;
int scalePowerPin = 2;  // scale control signal from digital pin 2
int tempPowerPin = 3;   // power for the RHT sensors from pin 3
int rht1Pin = 4;        // first RHT sensor connected to digital pin 4
int rht2Pin = 5;        // second RHT sensor connected to digital pin 5
int micPin = A0;        // microphone connected to analog pin 0 
int scalePin = A1;      // scale connected to analog pin 1

void setup() {
  Serial.begin(9600);

  // bridge startup
  Serial.print("Initializing the bridge...");
  Bridge.begin();
  Serial.println("Done");

  pinMode(scalePowerPin, OUTPUT);
  pinMode(tempPowerPin, OUTPUT);
  pinMode(rht1Pin, INPUT);
  pinMode(rht2Pin, INPUT);
  pinMode(micPin, INPUT);
  pinMode(scalePin, INPUT);

  // get the time that this sketch started:
  Process startTime;
  startTime.runShellCommand("date");
  while(startTime.available()) {
    char c = startTime.read();
    startString += c;
  }
}

void loop() {
  // turn on pin for RHT power
  digitalWrite(tempPowerPin, HIGH);

  // initialize varialbles for the process
  double weight = 0;
  int soundLevel = 0;
  dht DHT;

  Serial.println("Getting sensor value...");

  // get the current time from the server:
  Process time;
  time.runShellCommand("date");
  String timeString = "";
  while(time.available()) {
    char c = time.read();
    timeString += c;
  }
 
  String rowData(timeString);
  rowData += ",";
      
  DHT = readRHT(rht1Pin);
  rowData += DHT.humidity;
  rowData += ",";
  rowData += DHT.temperature;
  rowData += ",";

  DHT = readRHT(rht2Pin);
  rowData += DHT.humidity;
  rowData += ",";
  rowData += DHT.temperature;
  rowData += ",";

  soundLevel = analogRead(micPin);
  rowData += soundLevel;
  rowData += ",";

  weight = readScale();
  rowData += weight;

  Serial.println(rowData);

  // initialize Temboo objects and variables
  TembooChoreo AppendRowChoreo;

  AppendRowChoreo.begin();

  AppendRowChoreo.setAccountName(TEMBOO_ACCOUNT);
  AppendRowChoreo.setAppKeyName(TEMBOO_APP_KEY_NAME);
  AppendRowChoreo.setAppKey(TEMBOO_APP_KEY);

  AppendRowChoreo.setChoreo("/Library/Google/Spreadsheets/AppendRow");

  AppendRowChoreo.addInput("Username", GOOGLE_USERNAME);
  AppendRowChoreo.addInput("Password", GOOGLE_PASSWORD);

  AppendRowChoreo.addInput("SpreadsheetTitle", SPREADSHEET_TITLE);

  AppendRowChoreo.addInput("RowData", rowData);

  // run the Choreo and wait for the results
  // the return code (returnCode) will indicate success or failure
  unsigned int returnCode = AppendRowChoreo.run();

  // return code of zero (0) means success
  if (returnCode == 0) {
    Serial.println("Success! Appended " + rowData);
    Serial.println("");
  } else {
    // return code of anything other than zero means failure
    // read and display any error messages
    while (AppendRowChoreo.available()) {
      char c = AppendRowChoreo.read();
      Serial.print(c);
    }
  }

  AppendRowChoreo.close();

  delay(1000*60*15);  // this will check every 15 minutes
}

dht readRHT(int rhtPin){
  // read in data from RHT03 sensor
  dht DHT;
  int chk = DHT.read21(rhtPin);

  switch (chk)
  {
    case DHTLIB_OK:  
		Serial.print("OK,\t"); 
		break;
    case DHTLIB_ERROR_CHECKSUM: 
		Serial.print("Checksum error,\t"); 
		break;
    case DHTLIB_ERROR_TIMEOUT: 
		Serial.print("Time out error,\t"); 
		break;
    default: 
		Serial.print("Unknown error,\t"); 
		break;
  }

  return DHT;
}

double readScale(){
  double scaleValue = 0;       // the analog signal from the pin
  double weight = 0;           // stores the weight of the scale
  int iter = 75;               // number of iterations
  analogReference(EXTERNAL);   // use the AREF pin as a voltage reference. AREF should be shorted to 3.3V

  // turn scale on before data collection
  digitalWrite(scalePowerPin, HIGH);
  delay(1000);
  
  // average lots of readings together to get a stable value
  for(int i=1; i<=iter; i++){
      scaleValue += analogRead(scalePin);
      scaleValue -= 545.25;    // this value is used in calabrating the scale to find what analog signal is zero pounds
      delay(15);
  }

  // change the following code to calibrate the scale
  scaleValue = scaleValue / iter;
  weight = 0.001*scaleValue*scaleValue + 4.5899*scaleValue + 0.0007;

  // turn scale off after data collection
  digitalWrite(scalePowerPin, LOW);

  return weight;
}

int readMic(){
      int soundLevel = analogRead(micPin);
      return soundLevel;
}
