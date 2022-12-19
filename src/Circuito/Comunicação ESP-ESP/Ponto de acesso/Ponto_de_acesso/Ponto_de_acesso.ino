#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <esp_wifi.h>
#define I2C_SDA 47 
#define I2C_SCL 48 
#define NB_APS 3

LiquidCrystal_I2C  lcd_i2cBase(0x27, 16, 2);
//////////////////////////////
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
    void mostraDistSSID(float d,char *ssid){
      lcd_i2c->setCursor(0, 0); 
      lcd_i2c->printf("SSID:%11s",ssid);
      lcd_i2c->setCursor(0, 1); 
      lcd_i2c->printf("Dist: %000.1f m   ",d);
    };
    void mostraConectandoSSID(int tentativa,char *ssid){
      lcd_i2c->setCursor(0, 0); 
      lcd_i2c->printf("SSID:%11s",ssid);
      lcd_i2c->setCursor(0, 1); 
      lcd_i2c->printf("CONECTANDO:%4i ",tentativa);
    };
    void mostraConectedFTM(char*ssid,int fc,int bp){
      lcd_i2c->setCursor(0, 0); 
      lcd_i2c->printf("Connected:%6s",ssid);
      lcd_i2c->setCursor(0, 1); 
      lcd_i2c->printf("FTM:%2ifc-%3ims ",fc,bp);     
    }
    void mostraSSIDAchado(int rede,char *ssid){
      lcd_i2c->setCursor(0, 0); 
      lcd_i2c->printf("Achada rede %4i",rede);
      lcd_i2c->setCursor(0, 1); 
      lcd_i2c->printf("SSID:%11s",ssid);
    };
    void mostraMediaBeacons(float a, float b, float c){
      lcd_i2c->setCursor(0, 0); 
      lcd_i2c->printf("Media distancias");
      lcd_i2c->setCursor(0, 1); 
      lcd_i2c->printf("%2.1f  %2.1f  %2.1f",a,b,c);
    };
};
MostradorLCD *lcd = NULL;
/////////////////////////////


//Vetores com nomes de rede e senhas dos Access Points
const char* SSIDS[NB_APS] = {"iobeeG3T21","iobeeG3T22","iobeeG3T23"};
const char* PWD[NB_APS]   = {"iobeeG3T21","iobeeG3T22","iobeeG3T23"};
char *SSIDEx[10] = {"DIR-842-24G","Inteli-COLLEGE"};
char *PWDEx[10]  = {"iobeeG3T2","QazWsx@123"};
int   hab[NB_APS]   = {1,1,1};
int   processingDist[NB_APS] = {1,0,0};
int   finishedProcessingDist[NB_APS] = {0,0,0};
int   processingAP = 0;
      
float distancia[NB_APS]={0,0,0};
int indice=0;
int distanciaObtidaSucesso = 0;

void postDataToServer() {
   Serial.println("Posting JSON data to server...");
    HTTPClient http;        
    http.begin("https://ur524n-3000.preview.csb.app/teobaldo");  
    http.addHeader("Content-Type", "application/json");         
    StaticJsonDocument<200> doc;

    doc["sensor"] = "gps";
    doc["time"] = 1351824120;

    JsonArray data = doc.createNestedArray("data");
    for(int i=0; i<NB_APS; i++)  {
      data.add(distancia[i]);
    }  
    String requestBody;
    serializeJson(doc, requestBody);
    int httpResponseCode = http.POST(requestBody);
    if(httpResponseCode>0){
      String response = http.getString();                       
      Serial.println(httpResponseCode);   
      Serial.println(response);
    }
    else {
       Serial.printf("Error occurred while sending HTTP POST: %s\n", http.errorToString(httpResponseCode).c_str());
    }
     
}

//Uma função para ler todos os dados da conexão WiFi
void DadosConexao(){
  Serial.printf("IP:%s SubMask %s GWay:%s DNS:%s BCast:%s MAC:%s NetID:%s  PSK:%s BSSID:%s RSSI:%i \n", 
    WiFi.localIP().toString().c_str(),     WiFi.subnetMask().toString().c_str(), WiFi.gatewayIP().toString().c_str(), WiFi.dnsIP().toString().c_str(),     
    WiFi.broadcastIP().toString().c_str(), WiFi.macAddress().c_str(), WiFi.networkID().toString().c_str(), WiFi.psk().c_str(), 
    WiFi.BSSIDstr().c_str(),    WiFi.RSSI());
}
/////////////////////////////////////////////////////////////////
/////////////////////////////// FTM /////////////////////////////
// Definições para o FTM
// Number of FTM frames requested in terms of 4 or 8 bursts (allowed values - 0 (No pref), 16, 24, 32, 64)
const uint8_t FTM_FRAME_COUNT = 16;
// Requested time period between consecutive FTM bursts in 100’s of milliseconds (allowed values - 0 (No pref) or 2-255)
const uint16_t FTM_BURST_PERIOD = 2;
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
    if(!finishedProcessingDist[processingAP]){
      // The estimated distance in meters may vary depending on some factors (see README file)
      distancia[indice] = abs(((float)report->dist_est - (float)4000)/(float)100);
      Serial.printf("FTM Estimate: Distance RAW: %.4f,Distance: %.4f m, Return Time: %u ns  %X-%X-%X-%X-%X-%X\n", (float)report->dist_est, (float)((float)report->dist_est - (float)4000) / (float)100, report->rtt_est,
                          report->peer_mac[0], report->peer_mac[1],report->peer_mac[2], report->peer_mac[3],report->peer_mac[4], report->peer_mac[5]);
      finishedProcessingDist[processingAP] = 1;
      // Pointer to FTM Report with multiple entries, should be freed after use
      // free(report->ftm_report_data);
    }
  } else {
    Serial.print("FTM Error: ");
    Serial.println(status_str[report->status]);
    distancia[indice] = 0;
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
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
//Função para conectar em APs sem medição FTM
void ConectarEnviarRede(int rede) {
  Serial.println("Conectando na rede: ");
  Serial.println(rede);
  WiFi.begin(SSIDEx[rede],PWDEx[rede]);
      while (WiFi.status() != WL_CONNECTED) {
        Serial.print("Tentando novamente!");
        delay(500);
      }
      Serial.println("WiFi connected");
      DadosConexao();
      postDataToServer();      
      WiFi.disconnect();
      Serial.println("Desconectei!");
}
//Função para conectar num AP sem medição FTM. Futuramente para conectar na internet e enviar 
//os dados dos sensores

void MedirDistancia(int rede){
  // Create binary semaphore (initialized taken and can be taken/given from any thread/ISR)
  ftmSemaphore = xSemaphoreCreateBinary();
  // Listen for FTM Report events
  WiFi.onEvent(onFtmReport, ARDUINO_EVENT_WIFI_FTM_REPORT);  
  // Connect to AP that has FTM Enabled
  Serial.println("Connecting to FTM Responder");
  WiFi.begin(SSIDS[rede], PWD[rede]);
  int tent = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(2000);
    Serial.print(".");
    lcd->mostraConectandoSSID(++tent, (char*)SSIDS[rede]);
    WiFi.disconnect();
    delay(500);
    WiFi.reconnect();
    delay(500); 
  }
  Serial.printf("WiFi Connected - FTM: FRAME_COUNT:%i %i ms Burst Period\n",FTM_FRAME_COUNT,FTM_BURST_PERIOD);
  lcd->mostraConectedFTM((char*)SSIDS[rede],FTM_FRAME_COUNT,FTM_BURST_PERIOD);
  delay(500);
  getFtmReport();
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA); 
  Wire.begin(I2C_SDA, I2C_SCL);
  //------------------------//
  lcd = new MostradorLCD(&lcd_i2cBase);
  //------------------------//
  Serial.println("Escaneando redes!");

  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  Serial.printf("%i networks found",n);
  for (int i = 0; i < n; ++i) {
    // Print SSID and RSSI for each network found
    if( WiFi.RSSI(i) > -60){
      Serial.printf("%i: %20s (%i) %s\n",i+1,WiFi.SSID(i),WiFi.RSSI(i),((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*"));      
      lcd->mostraSSIDAchado(i,(char*)WiFi.SSID(i).c_str());
      delay(100);
    }
  }
}

void loop() {
    Serial.println("Rotação de beacons!");
    if(hab[processingAP]){
      if(processingDist[processingAP]){
        Serial.printf("BEACON: %s\n",(char*)SSIDS[processingAP]);
        indice = processingAP;
        MedirDistancia(processingAP);
        if(finishedProcessingDist[processingAP]){
            lcd->mostraDistSSID(distancia[processingAP],(char*)SSIDS[processingAP]);
            delay(3000);
            if(processingAP < NB_APS){
                processingDist[processingAP] = 0;
                finishedProcessingDist[processingAP] = 0;
                processingAP++;
                WiFi.removeEvent(ARDUINO_EVENT_WIFI_FTM_REPORT);
                WiFi.disconnect();    
                esp_wifi_ftm_end_session();
            }
            if(processingAP == NB_APS){
              processingAP = 0;
              Serial.println("Conectar na internet e enviar dados para o servidor!"); 
             // ConectarEnviarRede(0);         
            }
            processingDist[processingAP] = 1;            
        }
      }
    }
}