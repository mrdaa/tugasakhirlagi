#include <DHT.h>
#include <ESP8266WiFi.h>
#include <SocketIoClient.h>
#include <EEPROM.h>
#include <GravityTDS.h>


SocketIoClient socket;

//Setup Global Variable
String id = "1471984882";
bool IsConnect = false;

//define your sensors here
#define DHTTYPE DHT11
#define dhtPin D2

#define relay1 D3
#define relay2 D4

#define TdsSensorPin A0
GravityTDS gravityTds;

DHT dht(dhtPin, DHTTYPE);

const int trigPin = D6;
const int echoPin = D7;
long duration = 0;
float distance = 0;

float temperature = 25, tdsValue = 0;

// Setting WiFi
char ssid[] = "a";
char pass[] = "1234567890";

//Setting Server
char SocketServer[] = "139.180.189.208";
int port = 4000;

void setup()
{
    //setup pins and sensor

    pinMode(relay1, OUTPUT);
    pinMode(relay1, HIGH);
    pinMode(relay2, OUTPUT);
    pinMode(relay2, HIGH);

    gravityTds.setPin(TdsSensorPin);
    gravityTds.setAref(5.0);      //reference voltage on ADC, default 5.0V on Arduino UNO
    gravityTds.setAdcRange(1024); //1024 for 10bit ADC;4096 for 12bit ADC
    gravityTds.begin();           //initialization

    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);

    dht.begin();

    //setup tombol realtime aplikasi
    SetupRelayAplikasi();

    Serial.begin(115200);

    //Setup WiFi
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
    }
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.println("");

    socket.begin(SocketServer, port);

    //listener for socket io start

    socket.on("connect", konek);
    socket.on("rwl", RelayWl);
    socket.on("rtds", RelayTds);
    socket.on("rtemp", RelayTemp);
    socket.on("rhum", RelayHum);
    socket.on("disconnect", diskonek);

    //listener for socket io end
}
void loop()
{
    socket.loop();
    // sensor suhu dan humidity
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    String temp = TangkapNilaiSensor(t);
    String hum = TangkapNilaiSensor(h);

    WlSensor(); //panggil function WaterLevel
    String wlval = TangkapNilaiSensor(distance);

    TdsSensor(); // panggil Function TDS
    String tdsVal = TangkapNilaiSensor(tdsValue);
    if (IsConnect)
    {
        // kirim Data Sensor Disini
        KirimSocket("temp", temp);
        KirimSocket("hum", hum);
        KirimSocket("tds", tdsVal);
        KirimSocket("wl", wlval);

    }
    delay(1000);
}

void WlSensor() // function water Level
{

    // Sets the trigPin on HIGH state for 10
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    const float maxH = 10;
    // Reads the echoPin, returns the sound wave travel time in microseconds
    duration = pulseIn(echoPin, HIGH);

    // Calculating the distance
    distance = (duration * 0.034) / 2;
    distance = maxH - distance;
    distance = ((distance / maxH) * 100);
}
// end water level

void TdsSensor() // function tds sensor
{
    gravityTds.setTemperature(temperature); // set the temperature and execute temperature compensation
    gravityTds.update();                    //sample and calculate
    tdsValue = gravityTds.getTdsValue();    // then get the value
    // Serial.print(tdsValue,0);
    // Serial.println("ppm");
}
// end tds sensor

//Function Function Penting Di Bawah

void konek(const char *payload, size_t length)
{
    socket.emit("new user", "\"P1471984882\"");
    Serial.println("Made Socket Connection");
    digitalWrite(relay1, HIGH);
    digitalWrite(relay2, HIGH);
    IsConnect = true;
    SetupRelayAplikasi();
}

void JalankanRelay(const char *payload, String NamaSocket, uint8_t pin)
{
    String value = String(payload);
    if (value == "true")
    {
        digitalWrite(pin, LOW);
        KirimSocket(NamaSocket, "true");
        Serial.println("its true");
    }
    else
    {
        digitalWrite(pin, HIGH);
        KirimSocket(NamaSocket, "false");
        Serial.println("its false");
    }
}

void RelayWl(const char *payload, size_t length)
{
    Serial.println(payload);
    JalankanRelay(payload, "resWl", relay1);
}

void RelayTemp(const char *payload, size_t length)
{
    Serial.println(payload);
    JalankanRelay(payload, "resTemp", relay2);
}

void RelayHum(const char *payload, size_t length)
{
    Serial.println(payload);
    JalankanRelay(payload, "resHum", relay1);
}

void diskonek(const char *payload, size_t length)
{
    digitalWrite(relay1, HIGH);
    digitalWrite(relay2, HIGH);
    IsConnect = false;
}

void RelayTds(const char *payload, size_t length)
{
    JalankanRelay(payload, "resTds", relay1);
}

void KirimSocket(String nama, String val)
{
    String Data = "{\"_id\":\"" + id + "\",\"_val\":\"" + val + "\"}";
    socket.emit(nama.c_str(), Data.c_str());
}

String TangkapNilaiSensor(float sensor)
{
    char Var[20];
    dtostrf(sensor, 1, 2, Var);
    String hasil = String(Var);
    return hasil;
}

void SetupRelayAplikasi()
{
    KirimSocket("resTemp", "false");
    KirimSocket("resHum", "false");
    KirimSocket("resTds", "false");
    KirimSocket("resWl", "false");
    KirimSocket("Mode", "true");
    //  KirimSocket("Mode","false");
}
