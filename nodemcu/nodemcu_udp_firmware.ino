#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

const int RECV_PORT=2777;
const int LED_PIN=D0;
const int MAX_SPEED=1023;

unsigned long now=0;

void set_led(bool state)
{
  digitalWrite(LED_PIN,state?HIGH:LOW);
}

void led_pattern(int dur)
{
  int d=now/dur;
  set_led((d&1)>0);
}


const char* ssid="236927";
const char* password="redrobot";
WiFiUDP Udp;
byte mac[6];
char mac_str[32];
enum NetStatus { NS_INIT, NS_CONNECTING, NS_FAILED, NS_CONNECTED } nstatus=NS_INIT;
const int IN_BUFFER_SIZE=256;
char incoming[IN_BUFFER_SIZE];
unsigned long last_drive=0;
int  drive=0;

void stop_motors()
{
  digitalWrite(D1,LOW);
  digitalWrite(D2,LOW);
  digitalWrite(D3,LOW);
  digitalWrite(D4,LOW);
}

void process_motor(int speed, int pin1, int pin2)
{
  if (speed>=0)
  {
    //digitalWrite(pin1,LOW);
    analogWrite(pin1,0);
    analogWrite(pin2,speed);
  }
  else
  {
    //digitalWrite(pin2,LOW);
    analogWrite(pin2,0);
    analogWrite(pin1,-speed);
  }
}

void drive_speed(int left, int right)
{
  process_motor(left,D1,D2);
  process_motor(right,D3,D4);
}

void process_command(char* buffer)
{
  char* left_cmd=strtok(buffer," ");
  if (left_cmd==NULL) return;
  int left=atoi(left_cmd);
  char* right_cmd=strtok(NULL," ");
  if (right_cmd==NULL) return;
  int right=atoi(right_cmd);
  if (left<-MAX_SPEED || left>MAX_SPEED || right<-MAX_SPEED || right>MAX_SPEED) return;
  last_drive=millis();
  drive_speed(left,right);
}

void handle_incoming()
{
  if (nstatus!=NS_CONNECTED) return;
  int packetSize = Udp.parsePacket();
  if (packetSize)
  {
    int len = Udp.read(incoming, IN_BUFFER_SIZE);
    if (len > 0)
    {
      incoming[len] = 0;
      IPAddress ip=Udp.remoteIP();
      uint32_t addr=ip;
      addr=(addr>>24)&0xFF;
      if (addr<190) 
      {
        //Serial.print("Received: '");
        //Serial.print(incoming);
        //Serial.println("'");
        process_command(incoming);
      }
    }
  }
}

void generate_mac_str()
{
  // convert mac bytes into readable string
  char* ptr=mac_str;
  for(int i=0;i<6;++i)
  {
    if (i>0) *ptr++ = '-';
    sprintf(ptr,"%02X",mac[i]);
    ptr+=2;
  }
  *ptr=0;
}

void connect_to_wifi()
{
  nstatus=NS_CONNECTING;
  WiFi.begin(ssid, password);
  int tries=0;
  for(tries=0;tries<20 && WiFi.status()!=WL_CONNECTED;++tries)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  if (tries>=20) 
  {
    nstatus=NS_FAILED;
    Serial.println("Failed to connect to WiFi");
  }
  else
  {
    nstatus=NS_CONNECTED;
    WiFi.macAddress(mac);
    generate_mac_str();
    Serial.print("WiFi connected: MAC=");
    Serial.println(mac_str);
    Udp.begin(RECV_PORT);
  }
}


void setup() {
  pinMode(D1,OUTPUT);
  pinMode(D2,OUTPUT);
  pinMode(D3,OUTPUT);
  pinMode(D4,OUTPUT);
  stop_motors();
  Serial.begin(115200);
  while (!Serial) delay(10);
  pinMode(LED_PIN,OUTPUT);
  digitalWrite(LED_PIN,LOW);
  Serial.println("Starting WiFi");
  connect_to_wifi();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED)
    nstatus=NS_FAILED;
  switch (nstatus) {
  case NS_CONNECTING:       led_pattern(32);  return;
  case NS_CONNECTED:        set_led(true);    break;
  case NS_FAILED:           led_pattern(256); return;
  }
  handle_incoming();
  now=millis();
  if ((now-last_drive)>200)
  {
    last_drive=now;
    drive_speed(0,0);
  }
}

