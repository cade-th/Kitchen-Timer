

//#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>

const char* ssid = "Kitchen Timer IoT";
const char* password = "";

// The String below "webpage" contains the complete HTML code that is sent to the client whenever someone connects to the webserver
String webpage = "<!DOCTYPE html><html><head><meta charset='UTF-8'> <title>Kitchen Timer IoT</title> <style>.container-timer{display: flex;flex-direction: row;justify-content: left;align-items: center;margin: 1rem;}.container-text {max-width: 500px;margin: 0 auto;padding: 2rem;box-shadow: 0px 2px 6px rgba(0, 0, 0, 0.1);background-color: #fff;border-radius: 0.5rem;line-height: 1;font-size: 1.2rem;color: #333;text-align: justify;text-justify: inter-word;word-wrap: break-word;}input[type='text'] {width: 10%;padding: 0.5rem;font-size: 1.2rem;border: none;border-radius: 0.5rem;box-shadow: 0px 2px 6px rgba(0, 0, 0, 0.1);background-color: #f5f5f5;font-family: Arial, sans-serif;transition: box-shadow 0.2s ease-in-out;}input[type='text']:focus {outline: none;box-shadow: 0px 4px 8px rgba(0, 0, 0, 0.2);} /* Style for the number display */ #number-display { font-size: 5rem; text-align: center; padding: 1rem; background-color: #eee; border-radius: 0.5rem; margin-bottom: 2rem; } /* Styles for the buttons */ .button { display: inline-block; background-color: #4CAF50; color: white; padding: 1rem 2rem; text-align: center; text-decoration: none; font-size: 1.5rem; margin: 0 1rem; cursor: pointer; border-radius: 0.5rem; border: none; } .button:hover { background-color: #3e8e41; } </style></head><body><p><div id='number-display'>00:00</div><button class='button' id='BTN_INC'>Increment</button><button class='button' id='BTN_START'>Start</button><button class='button' id='BTN_STOP'>Stop</button></p><div class='container-timer'><p><button class='button' id='BTN_SET'>Set Timer</button></p><input type='text' id='SET_TIME' placeholder='01:30'></div><div class='container-text'><p>Version 1.0: April 2023</p><p>Set Timer Format: MM:SS</p><p><b>Note:</b> Set Timer should not exceed 59mins:59secs (e.g.: 59:59)</p><p>This version doesn't not check if Set Timer was introduce properly. Make sure that you follow the specified format.</p></div></body><script> var Socket; document.getElementById('BTN_INC').addEventListener('click', button_inc); document.getElementById('BTN_START').addEventListener('click', button_start); document.getElementById('BTN_STOP').addEventListener('click', button_stop); document.getElementById('BTN_SET').addEventListener('click', button_set); function init() { Socket = new WebSocket('ws://' + window.location.hostname + ':81/'); Socket.onmessage = function(event) { processCommand(event); }; } function button_set() {var textBox = document.getElementById('SET_TIME'); var msg = {type: 'button_set',state: 'pressed',value: textBox.value};Socket.send(JSON.stringify(msg)); } function button_inc() { var msg = {type: 'button_inc',state: 'pressed'};Socket.send(JSON.stringify(msg)); } function button_start() { var msg = {type: 'button_start',state: 'pressed'};Socket.send(JSON.stringify(msg)); } function button_stop() { var msg = {type: 'button_stop',state: 'pressed'};Socket.send(JSON.stringify(msg)); } function processCommand(event) { var obj = JSON.parse(event.data);document.getElementById('number-display').innerHTML = obj.time;console.log(obj.time); } window.onload = function(event) { init(); }</script></html>";

// The JSON library uses static memory, so this will need to be allocated:
// -> in the video I used global variables for "doc_tx" and "doc_rx", however, I now changed this in the code to local variables instead "doc" -> Arduino documentation recomends to use local containers instead of global to prevent data corruption

int interval = 1000;
unsigned long previousMillis = 0;

ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

#define BUFF_SIZE 20
char gIncomingChar;
char gCommsMsgBuff[BUFF_SIZE];
int iBuff = 0;
byte gPackageFlag = 0;
byte gProcessDataFlag = 0;

void setup() {
pinMode(LED_BUILTIN, OUTPUT);
digitalWrite(LED_BUILTIN, HIGH);

Serial.begin(9600);
WiFi.softAP(ssid, password);

server.on("/", {
server.send(200, "text/html", webpage);
});
server.begin();

webSocket.begin();
webSocket.onEvent(webSocketEvent);
}

void loop() {
server.handleClient();
webSocket.loop();

unsigned long now = millis();
if ((unsigned long)(now - previousMillis) > interval) {
Serial.print("$GET\n");
gIncomingChar = Serial.read();
if(gIncomingChar == '$') {
for(int i=0; i<BUFF_SIZE; i++) {
gCommsMsgBuff[i] = 0;
}
iBuff = 0;
gIncomingChar = Serial.read();
while(gIncomingChar != '\n') {
gCommsMsgBuff[iBuff] = gIncomingChar;
iBuff++;
gIncomingChar = Serial.read();
if(iBuff == BUFF_SIZE) {
gCommsMsgBuff[0] = 'x';
gCommsMsgBuff[1] = 'x';
gCommsMsgBuff[2] = ':';
gCommsMsgBuff[3] = 'x';
gCommsMsgBuff[4] = 'x';
break;
}
}
}
String jsonString = "";
StaticJsonDocument<200> doc;
JsonObject object = doc.to<JsonObject>();
object["time"] = String(gCommsMsgBuff);
serializeJson(doc, jsonString);
webSocket.broadcastTXT(jsonString);
previousMillis = now;
}
}

void webSocketEvent(byte num, WStype_t type, uint8_t * payload, size_t length) {
switch (type) {
case WStype_DISCONNECTED:
break;
case WStype_CONNECTED:
break;
case WStype_TEXT:
StaticJsonDocument<200> doc;
deserializeJson(doc, payload, length);
JsonObject obj = doc.as<JsonObject>();
String cmd = obj["command"];
if (cmd == "start") {
digitalWrite(LED_BUILTIN, LOW);
} else if (cmd == "stop") {
digitalWrite(LED_BUILTIN, HIGH);
}
break;
}
}
