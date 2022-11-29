#include <WiFi.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#define I2C_SDA 47 
#define I2C_SCL 48 

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
      //Serial.printf("L1: %s\n",texto);
      lcd_i2c->setCursor(0, 0); 
      lcd_i2c->printf(texto);
    };
    void mostraL1Dest(char *texto){
     // Serial.printf("L1 Destino: %s\n",texto);
      lcd_i2c->setCursor(0, 0); 
      lcd_i2c->printf("->%s ",texto);
    };
    void mostraL1IP(char *ip){
   //   Serial.printf("IP: %s\n",ip);
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
/////////////////////////////

//Vetores com nomes de rede e senhas dos Access Points
char* SSIDS[3]={"iobeeG3T21","iobeeG3T22","iobeeG3T23"};
char* PWD[3]={"iobeeG3T21","iobeeG3T22","iobeeG3T23"};
//Variável que continua ou não o MENU 2

void DadosConexao(){
  Serial.printf("IP:%s SubMask %s GWay:%s DNS:%s BCast:%s MAC:%s NetID:%s  PSK:%s BSSID:%s RSSI:%i \n", 
    WiFi.localIP().toString().c_str(),     WiFi.subnetMask().toString().c_str(), WiFi.gatewayIP().toString().c_str(), WiFi.dnsIP().toString().c_str(),     
    WiFi.broadcastIP().toString().c_str(), WiFi.macAddress().c_str(), WiFi.networkID().toString().c_str(), WiFi.psk().c_str(), 
    WiFi.BSSIDstr().c_str(),    WiFi.RSSI());
}
char strIP[50]  = "";
void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);
  pinMode(38,OUTPUT);
  digitalWrite(38,LOW);
  //WiFi.mode(WIFI_STA); 
  //------------------------//
  lcd = new MostradorLCD(&lcd_i2cBase);
  //------------------------//
  WiFi.softAP(SSIDS[2],PWD[2], 1, 0, 4, true);
  IPAddress IP = WiFi.softAPIP();
  strcpy(strIP,IP.toString().c_str());
  lcd->mostraL1IP(strIP);
  lcd->mostraL2(SSIDS[2]);
  DadosConexao();
  
}
void loop() {

}
