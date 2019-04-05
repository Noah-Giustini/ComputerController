#include <FS.h>                 
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <BlynkSimpleEsp8266.h>
//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <SimpleTimer.h>
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson


#define BLYNK_PRINT Serial    // Comment this out to disable prints and save space
//#define BLYNK_DEBUG

char blynk_token[34] = "YOUR_BLYNK_TOKEN";

bool shouldSaveConfig = true;
int power = 12;
int reset = 13;
int powerinput = 15;
WidgetLED led(V4);

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println();

  pinMode(power, OUTPUT);   //initilize pin as output
  pinMode(reset, OUTPUT);   //initilize pin as output
  pinMode(powerinput, INPUT_PULLUP); //initilize pin to be read
 

  //clean FS, for testing
  //SPIFFS.format();

  //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");

          //strcpy(mqtt_server, json["mqtt_server"]);
          //strcpy(mqtt_port, json["mqtt_port"]);
          strcpy(blynk_token, json["blynk_token"]);

        } else {
          Serial.println("failed to load json config");
        }
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  //end read

  WiFiManagerParameter custom_blynk_token("blynk", "blynk token", blynk_token, 33);

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  wifiManager.addParameter(&custom_blynk_token);

  //reset settings - for testing
  //wifiManager.resetSettings();

  if (!wifiManager.autoConnect("Wifi_Manager", "password")) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");

  strcpy(blynk_token, custom_blynk_token.getValue());

  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["blynk_token"] = blynk_token;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
  }


  Blynk.config(blynk_token);
  bool result = Blynk.connect();

if (result != true)
{
  Serial.println("BLYNK Connection Fail");
  Serial.println(blynk_token);
  wifiManager.resetSettings();
  ESP.reset();
  delay (5000);
}
else
{
  Serial.println("BLYNK Connected");
}

}

/* power relay 
 * pulses pin high then low 
 * else always low
 */
BLYNK_WRITE(V1){  
    if (param.asInt()){       
        digitalWrite(power, HIGH);
        delay(100);
        digitalWrite(power, LOW);
    } else {
        digitalWrite(power, LOW);

    }  
}

/* reset relay 
 * pulses pin high then low 
 * else always low
 */
BLYNK_WRITE(V2){  
    if (param.asInt()){       
        digitalWrite(reset, HIGH);
        delay(100);
        digitalWrite(reset, LOW);
    } else {
        digitalWrite(reset, LOW);

    }
}


void checkPin()
{
  /* Read pin through app using gpio 14
   * pin v3 is to allow webhook to read via shortcuts app on ios
   * Led widget on pin V4 to show in blynk app using led instead of HIGH LOW display
   */

  int state = digitalRead(powerinput);
  if (state == 1){
    Blynk.virtualWrite(V3,HIGH);
    led.on();
  }else {
    Blynk.virtualWrite(V3,LOW);
    led.off();
  }
  
}

void loop() {
  Blynk.run();
  checkPin();
}
