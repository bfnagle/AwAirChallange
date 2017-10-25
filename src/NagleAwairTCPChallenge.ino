/*
 * Project NagleAwair
 * Description: Interview Challenge for Awair
 * Author: Brendan Nagle
 * Date: 10/21/2017
 */

 #include <adafruit-sht31.h>

Adafruit_SHT31 sht31 = Adafruit_SHT31();
const uint8_t vOC = A0;
char publishString[100] = "\0";

Thread* adcThread;
Thread* shtThread;
Thread* serialThread;
Thread* libratoThread;

volatile float temp = 0;
volatile float humid = 0;
int histVOC[50] = {0};
volatile int avgVOC = 0;
volatile int minVOC = 0;
volatile int maxVOC = 0;
volatile uint16_t vOCValue = 0;
const int red = 255;
const int blue = 0;
const int green = 0;
uint16_t brightness = 255;

//TCPServer server = TCPServer(22); //SSH
//TCPClient client;

void setup()
{
  RGB.control(true);
  RGB.color(red, blue, green);
  RGB.brightness(brightness);

  //server.begin();

  Serial.begin(115200);

  Serial.println("SHT31 test");
  if (!sht31.begin(0x44))
  {
      Serial.println("Couldn't find SHT31\n");
  }

  pinMode(vOC,INPUT);

  adcThread = new Thread("ADC",readADC);
  shtThread = new Thread("SHT31",readSHT31);
  serialThread = new Thread("JSON",processAndPrint);
  libratoThread = new Thread("Librato",publishToLibrato);
}

os_thread_return_t readADC()
{
  for(;;)
  {
    vOCValue = analogRead(vOC);
    int sum = vOCValue;
    int i = 0;
    minVOC = vOCValue;
    maxVOC = vOCValue;
    for(i = 0; i < 49; i++)
    {
      histVOC[i] = histVOC[i+1];
      sum += histVOC[i];
      if(histVOC[i] > maxVOC)
      {
        maxVOC = histVOC[i];
      }
      if(histVOC[i] < minVOC)
      {
        minVOC = histVOC[i];
      }
    }
    histVOC[49] = vOCValue;
    avgVOC = sum/50;
    brightness = vOCValue;
    brightness = brightness >> 4;
    RGB.brightness(brightness);
    delay(200);
  }
}

os_thread_return_t readSHT31()
{
  for(;;)
  {
    temp = sht31.readTemperature();
    humid = sht31.readHumidity();
    delay(3000);
  }
}

os_thread_return_t processAndPrint()
{
  for(;;)
  {
    sprintf(publishString,"{\"VOC min\": %d, \"VOC max\": %d, \"VOC avg\": %d, \"Temperature\": %.2f, \"Humidity\": %d}",minVOC,maxVOC,avgVOC,temp,static_cast<int>(humid));
    Serial.println(publishString);
    //if (client.connected()) {
      //while (client.available()) {
        //server.print(publishString);
      //}
    //} else {
    //  client = server.available();
    //}
    //server.print(publishString);
    delay(10000);
  }
}

os_thread_return_t publishToLibrato()
{
  for(;;)
  {
    char VOCString[5] = "\0";       //5 characters because biggest value is 4095, and add NULL terminator
    char tempString[7] = "\0";      //7 characters because biggest value is hundreds of degrees with 2 decimal places (e.g. 100.23) plus a NULL terminator
    char humidString[4] = "\0";     //4 characters because biggest value is 100 with a NULL terminator
    
    sprintf(VOCString,"%d",analogRead(vOC));
    sprintf(tempString,"%.2f",sht31.readTemperature());
    sprintf(humidString,"%d",static_cast<int>(sht31.readHumidity()));
    
    Particle.publish("VOC",VOCString);
    Particle.publish("Temperature",tempString);
    Particle.publish("Humidity",humidString);
    //Particle.publish("AwairData",publishString);
    delay(60000);
  }
}

void loop()
{
}
