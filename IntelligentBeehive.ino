/*
  This code is meant to run on the Arduino Yun. It will incorporate data acquired from
  * RHT03 sensor (relative humidity and temperature sensor)
  * SparkFun Electret Microphone

  Prepare your SD card with an empty folder in the SD root named "arduino" and a subfolder of that named "www". 
  This will ensure that the Yún will create a link to the SD to the "/mnt/sd" path.

  In this sketch folder is a basic webpage and a copy of zepto.js, a minimized version of jQuery.  
  When you upload your sketch, these files will be placed in the /arduino/www/TemperatureWebPanel folder on your SD card.

  You can then go to http://arduino.local/sd/IntelligentBeehive to see the output of this sketch.

  Feb 27, 2014
  Glen Meyerowitz
 
  This code is in the public domain and part of Yale Bee Space intelligent beehive project. 
*/
#include <Bridge.h>
#include <YunServer.h>
#include <YunClient.h>
#include <dht.h>

// Listen on default port 5555, the webserver on the Yun will forward there all the HTTP requests for us.
YunServer server;
String startString;
long hits = 0;
dht DHT;

int RHT1_Pin = 2;
int RHT2_Pin = 3;
int mic_Pin = A0;

void setup() {
  Serial.begin(9600);

  // Bridge startup
  pinMode(13,OUTPUT);
  digitalWrite(13, LOW);
  Bridge.begin();
  digitalWrite(13, HIGH);

  // Listen for incoming connection only from localhost (no one from the external network could connect)
  server.listenOnLocalhost();
  server.begin();

  // get the time that this sketch started:
  Process startTime;
  startTime.runShellCommand("date");
  while(startTime.available()) {
    char c = startTime.read();
    startString += c;
  }
}

void loop() {
  // Get clients coming from server
  YunClient client = server.accept();

  // There is a new client?
  if (client) {
    // read the command
    String command = client.readString();
    command.trim();        //kill whitespace
    Serial.println(command);
    // is "data" command?
    if (command == "data") {

      // get the time from the server:
      Process time;
      time.runShellCommand("date");
      String timeString = "";
      while(time.available()) {
        char c = time.read();
        timeString += c;
      }
      Serial.println(timeString);
      int soundLevel = analogRead(mic_Pin);
      client.print("Current time on the Yun: ");
      client.println(timeString);
      client.print("RHT sensor 1: \t");
      readRHT(RHT1_Pin, client);
      client.print("RHT sensor 2: \t");
      readRHT(RHT2_Pin, client);
      client.print("<br>Current sound level: ");
      client.print(soundLevel);
      client.print("<br>Hits so far: ");
      client.print(hits);
    }

    // Close connection and free resources.
    client.stop();
    hits++;
  }

  delay(100);  // this will check every 0.1s
}

void readRHT(int thisPin, YunClient client){
  // Read in data from RHT03 sensor
  int chk = DHT.read21(thisPin);
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
  // Print data to the client
  client.print("Current humidity:");
  client.print(DHT.humidity, 1);
  client.print("\tCurrent temperature:");
  client.print(DHT.temperature, 1);
}

