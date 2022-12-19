#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <HTTPClient.h>
#define I2C_SDA 47 
#define I2C_SCL 48 
#define SSID  "123@IOBEE"
#define PASS  "123@IOBEE"
#define DESTINO  "192.168.4.1"
#define BASEURL "/mensagemap_in"
#define MINHAMENSAGEM "IOBEE"


LiquidCrystal_I2C  lcd_i2cBase(0x27, 16, 2);
HTTPClient httpBase;


class MostradorLCD {
  private:
    LiquidCrystal_I2C *lcd_i2c;
  public:
    MostradorLCD (LiquidCrystal_I2C *lcd){
      lcd_i2c = lcd;     
      lcd_i2c->init(); // initialize the lcd
      lcd_i2c->backlight();

    };
    void mostraL1(char *texto){
      lcd_i2c->setCursor(0, 0); 
      lcd_i2c->printf(texto);
    };
    void mostraL1Dest(char *texto){
      lcd_i2c->setCursor(0, 0); 
      lcd_i2c->printf("->%s ",texto);
    };
    void mostraL1IP(char *ip){
      lcd_i2c->setCursor(0, 0); 
      lcd_i2c->printf("IP:%s",ip);
    };
    void mostraL2(char *texto){
      lcd_i2c->setCursor(0, 1); 
      lcd_i2c->printf(texto);
    };
    void mostraL2Msg(char *texto){
      lcd_i2c->setCursor(0, 1); 
      lcd_i2c->printf("Msg: %s",texto);
    };
    void mostraL2Dist(float d){
      lcd_i2c->setCursor(0, 1); 
      lcd_i2c->printf("Dist: %000.1f     ",d);
    };

};

MostradorLCD *lcd = NULL;

class ClienteAoServidorRemoto{
  private:
    char  baseURL[500] = "";
    char* ssidGERAL = "";
    char* passGERAL = "";
    char  ipbase[50];
    char  mensagem[1000]="Valor inicial";
    HTTPClient *http;
  public:
    ClienteAoServidorRemoto(char *ssid,char *pass,char *baseip,HTTPClient *h){
      ssidGERAL = ssid;
      passGERAL = pass;
      WiFi.begin(ssidGERAL, passGERAL);
      strcpy(ipbase,baseip);
      strcat(baseURL,"http://");
      strcat(baseURL,baseip);
      strcat(baseURL,BASEURL);
      http = h;
    };
    void conectaRede(){
      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
      }
      Serial.println(WiFi.localIP());
    };
    void conectaEnvia(char *msg){
      char url[1000]="";
      strcat(url,baseURL);
      strcat(url,"?msg=");
      strcat(url,msg);
      http->begin(url);
      Serial.println(url);
      int respCode = http->GET();
      if (respCode>0) {
        Serial.print("HTTP Response code: ");
        Serial.println(respCode);
        sprintf(mensagem,"%-16s",http->getString().c_str());  
        Serial.println(mensagem);  
      } else {
        Serial.print("Error code: ");
        Serial.println(respCode);
      }
      // Free resources
      http->end();
    };
    char *getMensagem(){
      return(mensagem);
    };
};

ClienteAoServidorRemoto *clienteServidorRemoto = NULL;
float distanciaCalculada;

// FTM settings
// Number of FTM frames requested in terms of 4 or 8 bursts (allowed values - 0 (No pref), 16, 24, 32, 64)
const uint8_t FTM_FRAME_COUNT = 64;
// Requested time period between consecutive FTM bursts in 100’s of milliseconds (allowed values - 0 (No pref) or 2-255)
const uint16_t FTM_BURST_PERIOD = 5;
// Semaphore to signal when FTM Report has been received
xSemaphoreHandle ftmSemaphore;
// Status of the received FTM Report
bool ftmSuccess = true;
// FTM report handler with the calculated data from the round trip
void onFtmReport(arduino_event_t *event) {
  const char * status_str[5] = {"SUCCESS", "UNSUPPORTED", "CONF_REJECTED", "NO_RESPONSE", "FAIL"};
  wifi_event_ftm_report_t * report = &event->event_info.wifi_ftm_report;
  // Set the global report status
  ftmSuccess = report->status == FTM_STATUS_SUCCESS;
  if (ftmSuccess) {
    // The estimated distance in meters may vary depending on some factors (see README file)
    // Serial.printf("FTM Estimate: Distance: %.2f m, Return Time: %u ns\n", (float)report->dist_est / 100.0 - 43.0, report->rtt_est);
    Serial.printf("report->dist_est %.2f\n", (float)report->dist_est);
    Serial.printf("report->rtt_est %u\n", report->rtt_est);
    distanciaCalculada = (float)report->dist_est / 100.0 - 40.3, report->rtt_est;
     
    // Pointer to FTM Report with multiple entries, should be freed after use
    free(report->ftm_report_data);
  } else {
    Serial.print("FTM Error: ");
    Serial.println(status_str[report->status]);
  }
  // Signal that report is received
  xSemaphoreGive(ftmSemaphore);
}
// Initiate FTM Session and wait for FTM Report
bool getFtmReport(){
  if(!WiFi.initiateFTM(FTM_FRAME_COUNT, FTM_BURST_PERIOD)){
    Serial.println("FTM Error: Initiate Session Failed");
    return false;
  }
  // Wait for signal that report is received and return true if status was success
  return xSemaphoreTake(ftmSemaphore, portMAX_DELAY) == pdPASS && ftmSuccess;
}
void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);
  //------------------------//
  lcd = new MostradorLCD(&lcd_i2cBase);

  // Create binary semaphore (initialized taken and can be taken/given from any thread/ISR)
  ftmSemaphore = xSemaphoreCreateBinary();
  // Listen for FTM Report events
  WiFi.onEvent(onFtmReport, ARDUINO_EVENT_WIFI_FTM_REPORT);



  clienteServidorRemoto = new ClienteAoServidorRemoto(SSID,PASS,DESTINO,&httpBase);
  clienteServidorRemoto->conectaRede();

  Serial.print("Initiating FTM session with Frame Count ");
  Serial.print(FTM_FRAME_COUNT);
  Serial.print(" and Burst Period ");
  Serial.print(FTM_BURST_PERIOD * 100);
  Serial.println(" ms");



};
int i=0;
char msgEnvio[100] ="";
void loop() {
  lcd->mostraL1Dest(DESTINO);
  sprintf(msgEnvio,"%s%i",MINHAMENSAGEM,i++);
  clienteServidorRemoto->conectaEnvia(msgEnvio);
  Serial.println(msgEnvio);
  msgEnvio[0]=0;
  lcd->mostraL2(clienteServidorRemoto->getMensagem());
  getFtmReport();
  delay(1000);
  lcd->mostraL2Dist(distanciaCalculada);
  delay(1000);
  getFtmReport();
}
