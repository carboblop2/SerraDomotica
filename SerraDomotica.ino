#undef max
#undef min
#include <LCDI2C_Multilingual.h>
#include <DHT11.h>
#include <WiFiNINA.h>
#include <string>
#include "Seeed_BMP280.h"
#include <Wire.h>
#include <ArduinoJson.h>
// http://librarymanager/All#ArduinoJSON

// Replace with your network credentials
char ssid[] = "iPhone di Carbo";
char pass[] = "offognaa";

WiFiServer server(80);
WiFiClient client;

String header;

String output1State = "off";
String output2State = "off";

// GPIOs on MKR WiFi 1010
const int output1 = 0;
const int output2 = 1;

int sensorPin = A0;
int sensorValue = 0;
int menuIndex = 0;
const int totalMenus = 6;
const int pinAvanti=3;
const int pinIndietro=2;

unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 2000;

DHT11 dht11(7);
BMP280 bmp280;

#define INTERVAL 2000
#define  RECEIVE_TIMEOUT  (100)

double t0;
uint32_t MSL = 102009; // Mean Sea Level in Pa
uint8_t cmd_measure[9]= {0xFF,0x01,0x9C,0x00,0x00,0x00,0x00,0x00,0x63};
uint32_t PPM = 0;

LCDI2C_Generic lcd(0x27, 16, 2); // I2C address: 0x27; Display size: 16x2

float alt = 0.0;
float umid_terr = 0.0;
float pres = 0.0;
int temp = 0;
int umid = 0;
int result = 0;

void setup() {
if(!bmp280.init()){
    Serial.println("Device error!");
  }

lcd.init();
lcd.backlight();
lcd.print("Avvio...");

Serial.begin(9600);

pinMode(pinAvanti, INPUT_PULLUP);
pinMode(pinIndietro, INPUT_PULLUP);
pinMode(output1, OUTPUT);
pinMode(output2, OUTPUT);
digitalWrite(output1, LOW);
digitalWrite(output2, LOW);

PPM=0;

  for(int i=0;i<12;i++)
  {
    Serial.println(i);
    delay(1000);
  }
  
  Serial.println("Avvio");
  Serial1.begin(9600);  

// Check WiFi module
if (WiFi.status() == WL_NO_MODULE) {
Serial.println("WiFi module not found");
while (true);
}

// Connect to WiFi
while (WiFi.status() != WL_CONNECTED) {
Serial.print("Connecting to ");
Serial.println(ssid);
WiFi.begin(ssid, pass);
delay(10000);
}

Serial.println("WiFi connected.");
Serial.print("IP Address: ");
Serial.println(WiFi.localIP());

server.begin();
}

void loop() {

double t1 = millis();
  if (t1 - t0 > INTERVAL) {
    float p1;

    //get and print temperatures
    float t = bmp280.getTemperature();
   
    //get and print atmospheric pressure data
    p1 = bmp280.getPressure();
    pres=p1 / 100.0;

    //get and print altitude data
    alt = bmp280.calcAltitude(MSL, p1, t);
    t0 = millis();
  }
  if (Serial.available()) {
    char mb[255];
    uint8_t ix = 0;
    while (Serial.available()) {
      mb[ix++] = Serial.read();
    }
    mb[ix] = 0;
    Serial.println("Incoming:");
    Serial.println(mb);
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, mb);
    if (!error) {
      float newMSL = doc["MSL"];
      Serial.print("New MSL: "); Serial.println(newMSL);
      if (newMSL > 0.0) {
        MSL = newMSL * 100;
      }
    } else {
      Serial.println("Parse error!");
    }
  }

//misura c02
  measure();
  delay(200);
 
// Lettura temperatura e umidità
result = dht11.readTemperatureHumidity(temp, umid);

//Lettura umidità suolo
sensorValue = analogRead(sensorPin);
umid_terr = (sensorValue / 1023.0) * 100; //580 con 3v



client = server.available();
  if (client) {
    currentTime = millis();
    previousTime = currentTime;
    String currentLine = "";

    while (client.connected() && currentTime - previousTime <= timeoutTime) {
      currentTime = millis();
      if (client.available()) {
        char c = client.read();
        header += c;

        if (c == '\n') {
          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            if (header.indexOf("GET /1/on") >= 0) {
              Serial.println("POMPA DELL'ACQUA 1 ON");
              output1State = "on";
              digitalWrite(output1, HIGH);
            } else if (header.indexOf("GET /1/off") >= 0) {
              Serial.println("POMPA DELL'ACQUA 1 OFF");
              output1State = "off";
              digitalWrite(output1, LOW);
            } else if (header.indexOf("GET /2/on") >= 0) {
              Serial.println("POMPA DELL'ACQUA 2 ON");
              output2State = "on";
              digitalWrite(output2, HIGH);
            } else if (header.indexOf("GET /2/off") >= 0) {
              Serial.println("POMPA DELL'ACQUA 2 OFF");
              output2State = "off";
              digitalWrite(output2, LOW);
            }

            String mercuryColor = "green";
            if (temp < 15 || temp > 35) mercuryColor = "red";
            else if ((temp >= 15 && temp < 20) || (temp > 30 && temp <= 35)) mercuryColor = "yellow";

            int thermoHeight = map(temp, 0, 50, 0, 200);
            thermoHeight = constrain(thermoHeight, 0, 200);

            client.println("<!DOCTYPE html>");
            client.println("<html>");
            client.println("<head>");
            client.println("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<meta http-equiv=\"refresh\" content=\"10\">");
            client.println("<title>WEB SERVER MEUCCI</title>");
            client.println("<style>");
            client.println("body { font-family: Arial, sans-serif; background-color: #f4f4f4; margin: 0; padding: 20px; }");
            client.println("h1 { color: #333; text-align: center; margin-bottom: 20px; }");
            client.println(".card { background-color: #fff; padding: 20px; border-radius: 8px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); margin-bottom: 20px; }");
            client.println(".card-title { font-size: 1.4em; font-weight: bold; margin-bottom: 10px; }");
            client.println(".button { padding: 10px 20px; font-size: 0.9em; margin: 5px; }");
            client.println(".on { background-color: #4CAF50; }");
            client.println(".off { background-color: #f44336; }");
            client.println(".thermometer { width: 30px; height: 200px; background: #ddd; border-radius: 15px; position: relative; overflow: hidden; }");
            client.println(".mercury { width: 100%; position: absolute; bottom: 0; border-radius: 15px 15px 0 0; transition: height 0.5s ease; }");
            
            // Stili umidità aria
            client.println(".humidity-icon {");
            client.println("background-image: url(\"data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 32 32'%3E%3Cpath fill='%23808080' d='M24.5 28a5.385 5.385 0 0 1-5.5-5.249c0-1.057.295-2.058.874-2.831L23.49 14.538a1.217 1.217 0 0 1 2.02 0L29.06 19.815A5.492 5.492 0 0 1 30 22.751 5.385 5.385 0 0 1 24.5 28zm0-11.38l-2.936 4.367A3.359 3.359 0 0 0 21 22.751a3.51 3.51 0 0 0 7 0 3.436 3.436 0 0 0-.63-1.867z'/%3E%3Ccircle cx='5' cy='13' r='1' fill='%23808080'/%3E%3Ccircle cx='11' cy='19' r='1' fill='%23808080'/%3E%3Ccircle cx='15' cy='25' r='1' fill='%23808080'/%3E%3Ccircle cx='17' cy='15' r='1' fill='%23808080'/%3E%3Ccircle cx='13' cy='11' r='1' fill='%23808080'/%3E%3Ccircle cx='27' cy='11' r='1' fill='%23808080'/%3E%3Ccircle cx='9' cy='27' r='1' fill='%23808080'/%3E%3Ccircle cx='3' cy='21' r='1' fill='%23808080'/%3E%3Crect x='2' y='6' width='28' height='2' fill='%23808080'/%3E%3C/svg%3E\");");
            client.println("background-size: contain; width: 60px; height: 60px; margin: 10px auto; position: relative;");
            client.println("}");
            
            client.println(".humidity-fill {");
            client.println("position: absolute; top: 0; left: 0; width: 100%; height: 100%; background: #2196F3;");
            client.println("-webkit-mask-image: url(\"data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 32 32'%3E%3Cpath fill='%23fff' d='M24.5 28a5.385 5.385 0 0 1-5.5-5.249c0-1.057.295-2.058.874-2.831L23.49 14.538a1.217 1.217 0 0 1 2.02 0L29.06 19.815A5.492 5.492 0 0 1 30 22.751 5.385 5.385 0 0 1 24.5 28zm0-11.38l-2.936 4.367A3.359 3.359 0 0 0 21 22.751a3.51 3.51 0 0 0 7 0 3.436 3.436 0 0 0-.63-1.867z'/%3E%3Ccircle cx='5' cy='13' r='1' fill='%23fff'/%3E%3Ccircle cx='11' cy='19' r='1' fill='%23fff'/%3E%3Ccircle cx='15' cy='25' r='1' fill='%23fff'/%3E%3Ccircle cx='17' cy='15' r='1' fill='%23fff'/%3E%3Ccircle cx='13' cy='11' r='1' fill='%23fff'/%3E%3Ccircle cx='27' cy='11' r='1' fill='%23fff'/%3E%3Ccircle cx='9' cy='27' r='1' fill='%23fff'/%3E%3Ccircle cx='3' cy='21' r='1' fill='%23fff'/%3E%3Crect x='2' y='6' width='28' height='2' fill='%23fff'/%3E%3C/svg%3E\");");
            client.println("mask-image: url(\"data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 32 32'%3E%3Cpath fill='%23fff' d='M24.5 28a5.385 5.385 0 0 1-5.5-5.249c0-1.057.295-2.058.874-2.831L23.49 14.538a1.217 1.217 0 0 1 2.02 0L29.06 19.815A5.492 5.492 0 0 1 30 22.751 5.385 5.385 0 0 1 24.5 28zm0-11.38l-2.936 4.367A3.359 3.359 0 0 0 21 22.751a3.51 3.51 0 0 0 7 0 3.436 3.436 0 0 0-.63-1.867z'/%3E%3Ccircle cx='5' cy='13' r='1' fill='%23fff'/%3E%3Ccircle cx='11' cy='19' r='1' fill='%23fff'/%3E%3Ccircle cx='15' cy='25' r='1' fill='%23fff'/%3E%3Ccircle cx='17' cy='15' r='1' fill='%23fff'/%3E%3Ccircle cx='13' cy='11' r='1' fill='%23fff'/%3E%3Ccircle cx='27' cy='11' r='1' fill='%23fff'/%3E%3Ccircle cx='9' cy='27' r='1' fill='%23fff'/%3E%3Ccircle cx='3' cy='21' r='1' fill='%23fff'/%3E%3Crect x='2' y='6' width='28' height='2' fill='%23fff'/%3E%3C/svg%3E\");");
            client.println("-webkit-mask-size: contain; mask-size: contain;");
            client.println("clip-path: inset(calc(100% - (100% * var(--fill-level))) 0 0 0);");
            client.println("}");
            
            // Stili umidità terreno
            client.println(".soil-icon {");
            client.println("background-image: url(\"data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24'%3E%3Cpath fill='none' d='M11.964 6.97s-3.075.307-4.685-1.035C5.668 4.593 6.036 2.03 6.036 2.03s3.075-.306 4.685 1.036c1.61 1.342 1.243 3.904 1.243 3.904z' stroke='%23808080' stroke-width='1' stroke-linecap='round' stroke-linejoin='round'/%3E%3Cpath fill='none' d='M12.036 6.97s3.075.307 4.685-1.035c1.61-1.342 1.242-3.905 1.242-3.905s-3.075-.306-4.685 1.036c-1.61 1.342-1.242 3.904-1.242 3.904z' stroke='%23808080' stroke-width='1' stroke-linecap='round' stroke-linejoin='round'/%3E%3Cpath fill='none' d='M4 11a1 1 0 0 1 1-1h14a1 1 0 0 1 1 1v2a1 1 0 0 1-1 1H5a1 1 0 0 1-1-1v-2z' stroke='%23808080' stroke-width='1' stroke-linecap='round' stroke-linejoin='round'/%3E%3Cpath fill='none' d='M5 14h14l-2 8H7l-2-8z' stroke='%23808080' stroke-width='1' stroke-linecap='round' stroke-linejoin='round'/%3E%3Cpath fill='none' d='M12 16c1.105 0 2-.895 2-2 0-1.333-2-3-2-3s-2 1.667-2 3c0 1.105.895 2 2 2z' stroke='%23808080' stroke-width='1' stroke-linecap='round' stroke-linejoin='round' transform='translate(4, 8.5) scale(0.7)'/%3E%3C/svg%3E\");");
            client.println("background-size: contain; width: 60px; height: 60px; margin: 10px auto; position: relative;");
            client.println("background-color: transparent;");  // Aggiunto questa linea
            client.println("}");
            
            client.println(".soil-fill {");
            client.println("position: absolute; top: 0; left: 0; width: 100%; height: 100%; background: #8B4513;");
            client.println("-webkit-mask-image: url(\"data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24'%3E%3Cpath d='M11.964 6.97s-3.075.307-4.685-1.035C5.668 4.593 6.036 2.03 6.036 2.03s3.075-.306 4.685 1.036c1.61 1.342 1.243 3.904 1.243 3.904z' stroke='%23fff' stroke-width='1' stroke-linecap='round' stroke-linejoin='round'/%3E%3Cpath d='M12.036 6.97s3.075.307 4.685-1.035c1.61-1.342 1.242-3.905 1.242-3.905s-3.075-.306-4.685 1.036c-1.61 1.342-1.242 3.904-1.242 3.904z' stroke='%23fff' stroke-width='1' stroke-linecap='round' stroke-linejoin='round'/%3E%3Cpath d='M4 11a1 1 0 0 1 1-1h14a1 1 0 0 1 1 1v2a1 1 0 0 1-1 1H5a1 1 0 0 1-1-1v-2z' stroke='%23fff' stroke-width='1' stroke-linecap='round' stroke-linejoin='round'/%3E%3Cpath d='M5 14h14l-2 8H7l-2-8z' stroke='%23fff' stroke-width='1' stroke-linecap='round' stroke-linejoin='round'/%3E%3Cpath d='M12 16c1.105 0 2-.895 2-2 0-1.333-2-3-2-3s-2 1.667-2 3c0 1.105.895 2 2 2z' stroke='%23fff' stroke-width='1' stroke-linecap='round' stroke-linejoin='round' transform='translate(4, 8.5) scale(0.7)'/%3E%3C/svg%3E\");");
            client.println("mask-image: url(\"data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24'%3E%3Cpath d='M11.964 6.97s-3.075.307-4.685-1.035C5.668 4.593 6.036 2.03 6.036 2.03s3.075-.306 4.685 1.036c1.61 1.342 1.243 3.904 1.243 3.904z' stroke='%23fff' stroke-width='1' stroke-linecap='round' stroke-linejoin='round'/%3E%3Cpath d='M12.036 6.97s3.075.307 4.685-1.035c1.61-1.342 1.242-3.905 1.242-3.905s-3.075-.306-4.685 1.036c-1.61 1.342-1.242 3.904-1.242 3.904z' stroke='%23fff' stroke-width='1' stroke-linecap='round' stroke-linejoin='round'/%3E%3Cpath d='M4 11a1 1 0 0 1 1-1h14a1 1 0 0 1 1 1v2a1 1 0 0 1-1 1H5a1 1 0 0 1-1-1v-2z' stroke='%23fff' stroke-width='1' stroke-linecap='round' stroke-linejoin='round'/%3E%3Cpath d='M5 14h14l-2 8H7l-2-8z' stroke='%23fff' stroke-width='1' stroke-linecap='round' stroke-linejoin='round'/%3E%3Cpath d='M12 16c1.105 0 2-.895 2-2 0-1.333-2-3-2-3s-2 1.667-2 3c0 1.105.895 2 2 2z' stroke='%23fff' stroke-width='1' stroke-linecap='round' stroke-linejoin='round' transform='translate(4, 8.5) scale(0.7)'/%3E%3C/svg%3E\");");
            client.println("-webkit-mask-size: contain; mask-size: contain;");
            client.println("clip-path: inset(calc(100% - (100% * var(--fill-level))) 0 0 0);");
            client.println("}");
            
            client.println(".data-container { display: flex; flex-wrap: wrap; justify-content: space-around; }");
            client.println(".data-item { width: 45%; margin-bottom: 20px; }");
            client.println("</style>");
            client.println("</head>");
            client.println("<body>");
            client.println("<h1>WEB SERVER MEUCCI</h1>");
            client.println("<div class=\"data-container\">");

            // Sezione Temperatura
            client.println("<div class=\"data-item\">");
            client.println("<div class=\"card\">");
            client.println("<div class=\"card-title\">Temperatura</div>");
            client.println("<div class=\"thermometer\">");
            client.println("<div class=\"mercury\" style=\"height: " + String(thermoHeight) + "px; background-color: " + mercuryColor + ";\"></div>");
            client.println("</div>");
            client.println("<p>" + String(temp) + " &deg;C</p>");
            client.println("</div>");
            client.println("</div>");

            // Sezione Umidità Aria
            client.println("<div class=\"data-item\">");
            client.println("<div class=\"card\">");
            client.println("<div class=\"card-title\">Umidita' Aria</div>");
            client.println("<div class=\"humidity-icon\" style=\"--fill-level:" + String((float)umid / 100.0) + ";\">");
            client.println("<div class=\"humidity-fill\"></div>");
            client.println("</div>");
            client.println("<p>" + String(umid) + "%</p>");
            client.println("</div>");
            client.println("</div>");

            // Sezione Umidità Terreno
            client.println("<div class=\"data-item\">");
            client.println("<div class=\"card\">");
            client.println("<div class=\"card-title\">Umidita' Terreno</div>");
            client.println("<div class=\"soil-icon\" style=\"--fill-level:" + String(umid_terr / 100.0) + ";\">");
            client.println("<div class=\"soil-fill\"></div>");
            client.println("</div>");
            client.println("<p>" + String(umid_terr) + "%</p>");
            client.println("</div>");
            client.println("</div>");

            // Altitudine
            client.println("<div class=\"data-item\">");
            client.println("<div class=\"card\">");
            client.println("<div class=\"card-title\">Altitudine</div>");
            client.println("<p>" + String(alt) + " m</p>");
            client.println("</div>");
            client.println("</div>");

            client.println("<div class=\"data-item\">");
            client.println("<div class=\"card\">");
            client.println("<div class=\"card-title\">CO2</div>");
            client.println("<p>" + String(PPM) + " PPM</p>");
            client.println("</div>");
            client.println("</div>");

            client.println("<div class=\"data-item\">");
            client.println("<div class=\"card\">");
            client.println("<div class=\"card-title\">Pressione</div>");
            client.println("<p>" + String(pres) + " HPa</p>");
            client.println("</div>");
            client.println("</div>");

            client.println("</div>");

            // Controllo Pompa
            client.println("<div class=\"card\">");
            client.println("<div class=\"card-title\">CONTROLLO POMPE</div>");
            client.println("<div style=\"display: flex; justify-content: space-around; margin: 15px 0;\">");
            
            // Pompa 1
            client.println("<div style=\"text-align: center;\">");
            client.println("<p>Pompa 1: " + output1State + "</p>");
            client.println(output1State == "off" ? 
              "<a href=\"/1/on\"><button class=\"button on\">ACCENDI</button></a>" : 
              "<a href=\"/1/off\"><button class=\"button off\">SPEGNI</button></a>");
            client.println("</div>");
            
            // Pompa 2
            client.println("<div style=\"text-align: center;\">");
            client.println("<p>Pompa 2: " + output2State + "</p>");
            client.println(output2State == "off" ? 
              "<a href=\"/2/on\"><button class=\"button on\">ACCENDI</button></a>" : 
              "<a href=\"/2/off\"><button class=\"button off\">SPEGNI</button></a>");
            client.println("</div>");
            
            client.println("</div>"); // chiude il flex container
            client.println("</div>"); // chiude la card

            client.println("</body>");
            client.println("</html>");
            client.println();
            break;
          } else currentLine = "";
        } else if (c != '\r') currentLine += c;
      }
    }
    header = "";
    client.stop();
  }
  delay(2000);

  bool btnAvanti = digitalRead(pinAvanti) == LOW; // pulsante premuto (pull-up)
  bool btnIndietro = digitalRead(pinIndietro) == LOW; // pulsante premuto

  static unsigned long lastButtonPress = 0;
  unsigned long currentTime = millis();


  if (currentTime - lastButtonPress > 20) {
    if (btnAvanti) {
      menuIndex = (menuIndex + 1) % totalMenus;
      lastButtonPress = currentTime;
    } else if (btnIndietro) {
      menuIndex = (menuIndex - 1 + totalMenus) % totalMenus;
      lastButtonPress = currentTime;
    }
  }


  lcd.clear();
  switch (menuIndex) {
    case 0:
      // Temperatura
      lcd.setCursor(0, 0);
      lcd.print("Temperatura:");
      lcd.setCursor(0, 1);
      lcd.print(temp);
      lcd.print(" \xDF"); // simbolo gradi
      break;
    case 1:
      // Umidità
      lcd.setCursor(0, 0);
      lcd.print("Umidita':");
      lcd.setCursor(0, 1);
      lcd.print(umid);
      lcd.print(" %");
      break;
    case 2:
      // Altezza (Alt)
      lcd.setCursor(0, 0);
      lcd.print("Altitudine:");
      lcd.setCursor(0, 1);
      lcd.print(alt);
      lcd.print(" m");
      break;
    case 3:
      // Terreno
      lcd.setCursor(0, 0);
      lcd.print("Umidita' terreno:");
      lcd.setCursor(0, 1);
      lcd.print(umid_terr);
      lcd.print(" %");
      break;
    case 4:
      // Pressione (Pres)
      lcd.setCursor(0, 0);
      lcd.print("Pressione:");
      lcd.setCursor(0, 1);
      lcd.print(pres);
      lcd.print(" HPa");
      break;
    case 5:
      // CO2 (PPM)
      lcd.setCursor(0, 0);
      lcd.print("CO2:");
      lcd.setCursor(0, 1);
      lcd.print(PPM);
      lcd.print(" PPM");
    break;
  }

  delay(100);
}

uint8_t measure()
{
    uint8_t i;
    uint8_t buf[9];
    uint32_t start = millis();

    //Serial1.flush();

    for (i=0; i<9; i++) 
    {
        Serial1.write(cmd_measure[i]);
    }
  
    for (i=0; i<9;) 
    {
        if (Serial1.available()) 
        {
            buf[i++] = Serial1.read();
        }

        if (millis() - start > RECEIVE_TIMEOUT) 
        {
            return false;
        }
    }

    if (parse(buf)) 
    {
        return true;
    }

    return false;
}

uint8_t parse(uint8_t *pbuf)
{
    uint8_t i;
    uint8_t checksum = 0;

    for (i=0; i<9; i++) 
    {
        checksum += pbuf[i];
    }

    if (pbuf[0] == 0xFF && pbuf[1] == 0x9C && checksum == 0xFF)
    {
        PPM = (uint32_t)pbuf[2] << 24 | (uint32_t)pbuf[3] << 16 | (uint32_t)pbuf[4] << 8 | pbuf[5];
        return true;
    } 
    else 
    {
        return false;
    }
}
