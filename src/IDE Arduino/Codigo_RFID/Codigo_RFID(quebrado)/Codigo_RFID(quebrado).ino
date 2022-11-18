// Inclusão das bibliotecas
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <MFRC522.h>
#include <SPI.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Definição das portas de entradas e saídas
#define RFID_SS_SDA   21
#define RFID_RST      14
#define led_vermelho  2
#define led_verde     5
#define buzzer        40
#define I2C_SDA       47
#define I2C_SCL       48

// Variável para controle de funcionamento do Led Verde (Utilizada quando o sensor RFID de está lendo uma tag, quando conclui a leitura e na função de reset)
int iniciar = 0;
int lcdLinha =2;
int lcdColuna = 16;
LiquidCrystal_I2C lcd(0x27, lcdColuna, lcdLinha);  

// Definição das propriedades utilizadas no leitor de RFID
MFRC522 rfidBase = MFRC522(RFID_SS_SDA, RFID_RST);

// Definição da rede e senha utilizada para conexão via WIFI com o ESP32 S3
// const char* ssid = "Inteli-COLLEGE";
// const char* password =  "QazWsx@123";
const char* ssid = "SHARE-RESIDENTE";
const char* password = "Share@residente";

// Objeto Sensor RFID, contendo todas as suas funções de utilização
class LeitorRFID{
  private:

    // Variáveis para controle de leitura
    char codigoRFIDLido[100] = "";
    char codigoRFIDEsperado[100] = "";
    int cartaoDetectado = 0;
    int cartaoJaLido = 0;

    MFRC522 *rfid = NULL;

   // Processamento de leitura e regulagem tempo da operação 
    void processaCodigoLido(){
      char codigo[3*rfid->uid.size+1];
      codigo[0] = 0;
      char temp[10];
      for(int i=0; i < rfid->uid.size; i++){
        sprintf(temp,"%X",rfid->uid.uidByte[i]);
        strcat(codigo,temp);
      }
      codigo[3*rfid->uid.size+1] = 0;
      // Resultado da leitura
      strcpy(codigoRFIDLido,codigo);
      Serial.println(codigoRFIDLido);
    }
  public:
    // Valores de regulagem e escolha das portas do ESP32 S3 para a utilização do Sensor RFID
    LeitorRFID(MFRC522 *leitor){
      rfid = leitor;
      rfid->PCD_Init();
      Serial.printf("MOSI: %i MISO: %i SCK: %i SS: %i\n",MOSI,MISO,SCK,SS);
    };
    // Resultados do tipo de leitura
    char *tipoCartao(){
      MFRC522::PICC_Type piccType = rfid->PICC_GetType(rfid->uid.sak);
      Serial.println(rfid->PICC_GetTypeName(piccType));
      return((char *)rfid->PICC_GetTypeName(piccType));
    };
    // Para cartão, tag ou etiqueta detectada
    int cartaoPresente(){
      return(cartaoDetectado);
    };
    // Para cartão, tag ou etiqueta já processadas 
    int cartaoFoiLido(){
      return(cartaoJaLido);
    };
    // Comando para ler o cartão, tag ou etiqueta 
    void leCartao(){
      if (rfid->PICC_IsNewCardPresent()) { // Nova tag, cartão ou etiqueta for habilitada 
        iniciar = 7;
        lcd.clear(); 
        lcd.print("Lendo...");
        // Serial.println("Cartao presente");
        // Habilitação do led verde piscando até que a leitura do cartão, tag ou etiqueta estiver concluída.
          while (iniciar != 0){
            digitalWrite(led_verde, HIGH);
            delay(40);
            digitalWrite(led_verde, LOW);
            delay(80);
            iniciar -= 1;
          }
          lcd.clear(); 
        cartaoDetectado = 1;
        if (rfid->PICC_ReadCardSerial()) { // Quando cartão, tag ou etiqueta já for lido.
          lcd.clear(); 
            lcd.print("Leitura Completa");
            lcd.setCursor(0,1);
            lcd.print(codigoRFIDLido);
          // Serial.println("Cartao lido");
          digitalWrite(led_verde, HIGH); // Led verde quando cartão, tag ou etiqueta já for lido ficar com luz contínua.
          cartaoJaLido = 1;
          processaCodigoLido();
          rfid->PICC_HaltA(); 
          rfid->PCD_StopCrypto1(); 
          tone(buzzer, 3000, 500); // Buzzer é acionado, indicando a conclusão da leitura do cartão, tag ou etiqueta.
          delay(1000);
        }
      }else{
        // Reset das variáveis caso o cartão, tag ou etiqueta tenha sido retirado antes da conclusão da leitura.
        cartaoDetectado = 0;
        iniciar = 10;
        digitalWrite(led_verde, LOW);
        lcd.clear(); 
      }
    };
    char *cartaoLido(){
      // Conexão do ESP 32 S3 com o WIFI e uma requisão para enviar informações para um endereço.
          if ((WiFi.status() == WL_CONNECTED))
          {
          long rnd = random (1,10);
          HTTPClient client;
          client.begin("https://maker.ifttt.com/trigger/iobee_request/with/key/eefv4KI3yu2Xx7QRcutzc1AKbKq-STjZQ5YAHDo5dEN");
          int httpCode = client.GET();
          }
      return(codigoRFIDLido);
    };
    // Variáveis de reset para outra leitura do sensor RFID
    void resetarLeitura(){
      cartaoDetectado = 0;
      cartaoJaLido = 0;
      iniciar = 7;
    }
};

LeitorRFID *leitor = NULL;

void setup() {
  Serial.begin(115200);
 WiFi.begin(ssid, password);
 while (WiFi.status() != WL_CONNECTED) {
  delay(500);
  Serial.println("Connecting to WiFi.."); // Conectando esp32 s3 com o wifi
 }
 Serial.println("Connected to the WiFi network"); // Wifi conectado
  SPI.begin();
  // Setando os leds como saída
  pinMode(led_vermelho, OUTPUT);
  pinMode(led_verde, OUTPUT);
  pinMode(buzzer, OUTPUT);
  // Led aceso para mostrar que o sistema está energizado e em funcionamento
  digitalWrite(led_vermelho, HIGH);

  //LCD
  Wire.begin(I2C_SDA, I2C_SCL);
  lcd.init();
  lcd.backlight();

  leitor = new LeitorRFID(&rfidBase);

}
// No loop, será chamda todas as função quando requisitadas. Enquanto espera até que um cartão, tag ou etiqueta for encontrado, o programa vai mostrar "Lendo Cartão"
void loop() {
  lcd.setCursor(0, 0);
  lcd.print("Aproxime Cartao");
  // Serial.println("Lendo Cartao:");
  leitor->leCartao();
  if(leitor->cartaoFoiLido()){
    Serial.println(leitor->tipoCartao());
    Serial.println(leitor->cartaoLido());
    leitor->resetarLeitura();
    delay(1000);
  }
}