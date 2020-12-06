#include <AWS_IOT.h>
#include <WiFi.h>
#include <Arduino_JSON.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

Adafruit_BME280 bme;  // I2C
AWS_IOT hornbill;
const char* ssid = "SK_WiFiGIGA594C";
const char* password = "1903025786";
char HOST_ADDRESS[] = "a2zm0k5ovubv8z-ats.iot.ap-northeast-2.amazonaws.com";
char CLIENT_ID[]= "ymleeESP32"; // not necessary for this assignment
// 알아서 바꿔주쎄용
char sTOPIC_NAME[]= "$aws/things/smartBox/shadow/update/delta"; // subscribe topic name
char pTOPIC_NAME[]= "$aws/things/smartBox/shadow/update"; // publish topic name

//cds 센서 pinnum 설정
const int cdsPin = 35;
// millis 사용을 위해 선언
unsigned long previousMillis = 0;
const long publishInterval = 900000; // 15분
int keyword = 0; // 키워드 별로 번호를 부여 번호가 높을수록 보관 온도가 낮고 우선순위가 높음
int temp = 0;
int humid = 0;

int status = WL_IDLE_STATUS;
int msgCount = 0, msgReceived = 0;
char payload[512];
char rcvdPayload[512];
int delayTime;  // 10s

void mySubCallBackHandler(char *topicName, int payloadLen, char *payLoad) {
  strncpy(rcvdPayload,payLoad,payloadLen);
  rcvdPayload[payloadLen] = 0;
  msgReceived = 1;
}

void setup() {
  Serial.begin(115200);
  bool status;
  Serial.println(F("BME280 test"));
  status = bme.begin(0x76);  // bme280 I2C address = 0x76
  if (!status) {
    Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
    Serial.print("SensorID was: 0x");
    Serial.println(bme.sensorID(), 16);
    Serial.print("  ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
    Serial.print("  ID of 0x56-0x58 represents a BMP 280.\n");
    Serial.print("  ID of 0x60 represents a BMP 280.\n");
    Serial.print("  ID of 0x61 represents a BMP 680.\n");
    while (1) delay(10);
  }
  else Serial.println(F("BME280 valid"));
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to WiFi");
  if(hornbill.connect(HOST_ADDRESS,CLIENT_ID)== 0) {
    Serial.println("Connected to AWS");
    delay(1000);
    if(0==hornbill.subscribe(sTOPIC_NAME,mySubCallBackHandler)) {
      Serial.println("Subscribe Successful");
    }
    else {
      Serial.println("Subscribe Failed, Check the Thing Name and Certificates");
      while(1);
    }
  }
  else {
    Serial.println("AWS connection failed, Check the HOST Address");
    while(1);
  }
  delayTime = 10000;
  delay(2000);
}

void printValues() {
  Serial.print("Temperature = ");
  temp = bme.readTemperature();
  Serial.print(temp);
  Serial.println(" *C");
  
  Serial.print("Humidity = ");
  humid = bme.readHumidity();
  Serial.print(humid);
  Serial.println(" %");
}

// publish and subcsribe 확인
void publishOnlambda(){
  if(hornbill.publish(pTOPIC_NAME,payload) == 0) {
    sprintf(payload,"{\"state\":{\"reported\":{\"temp\":%d, \"humid\":%d, \"keyword\" : %d}}}", temp, humid, keyword);
    Serial.print("Publish Message: ");
    Serial.println(payload);
  }
  else Serial.println("Publish failed");
  if(msgReceived == 1) {
    msgReceived = 0;
    Serial.print("Received Message: ");
    Serial.println(rcvdPayload);
    // Parse JSON
    JSONVar myObj = JSON.parse(rcvdPayload);
    JSONVar state = myObj["state"];
    keyword = (const int)state["keyword"];
  }
}

void returnTime(int t) {
  float time = 0;
  time = 24 - ((tmep-t)*5/6);
  // time을 브라우져에 전달 해주어야함.
}

void loop() {
  unsigned long currentMillis = millis();
  int cValue = analogRead(cdsPin); // 조도센서 밝기
  printValues();
  delay(delayTime);
  // 15분 단위로 박스 내 택배가 있을 때 publish
  if ((currentMillis - previousMillis >=  publishInterval) && keyword != 0) {
    publishOnlambda();
    previousMillis = currentMillis;
  }
  
  // 택배가 와서 온도를 확인 하고 싶을 때, 또는 사물을 추가할 때 publish
  subscribe();
  if (msgReceived == 1) { /*&버튼이 눌려서 키워드에 숫자가 할당되었을 때*/ 
    int newKeyword = 0;
    Serial.print("Received Message: ");
    Serial.println(rcvdPayload);
    // Parse JSON
    JSONVar myObj = JSON.parse(rcvdPayload);
    JSONVar state = myObj["state"];
    // 형변환 확인좀
    newKeyword = (const int)state["keyword"];
    if (newKeyword != 0) {
      if (keyword < newKeyword) {
        keyword = newKeyword
        newKeyword = 0;
      }      
      publishOnlambda(cValue);
    } else {
      keyword = 0;
    }
  }
  // cds 센서를 통해 조도를 계속 측정
  if (cValue > 600) {
    //publish
    publishOnlambda(cValue);
  }
}
