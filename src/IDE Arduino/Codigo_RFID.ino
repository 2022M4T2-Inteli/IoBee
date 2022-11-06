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

// Variável para controle de funcionamento do sistema
int iniciar = 0;

// Definição das propriedades utilizadas no leitor de RFID
MFRC522 rfidBase = MFRC522(RFID_SS_SDA, RFID_RST);

// Definição da rede e senha utilizada para conexão via WIFI com o ESP32 S3
const char* ssid = "SHARE-RESIDENTE";
const char* password =  "Share@residente";

// Quando o leitor RFID inicia a leitura de uma tag, cartão ou etiqueta
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
    // Valores de regulagem e escolha das portas no ESP32
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
        Serial.println("Cartao presente");
        // Habilitação do led verde quando cartão for lido
          while (iniciar != 0){
            digitalWrite(led_verde, HIGH);
            delay(80);
            digitalWrite(led_verde, LOW);
            delay(80);
            iniciar -= 1;
          }
        cartaoDetectado = 1;
        if (rfid->PICC_ReadCardSerial()) { // Quando cartão, tag ou etiqueta já for lido
          Serial.println("Cartao lido");
          digitalWrite(led_verde, HIGH); // Led verde quando cartão, tag ou etiqueta já for lido ficar com luz contínua
          cartaoJaLido = 1;
          processaCodigoLido();
          rfid->PICC_HaltA(); 
          rfid->PCD_StopCrypto1(); 
          tone(buzzer, 3000, 500);
          delay(1000);
        }
      }else{
        cartaoDetectado = 0;
        iniciar = 10;
        digitalWrite(led_verde, LOW); // Apagar led verde
      }
    };
    char *cartaoLido(){
      // Conexão do ESP 32 S3 com o WIFI
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
      iniciar = 10;
    }
    // Passos para processamento de leitura
    void listI2CPorts(){
      Serial.println("\nI2C Scanner"); // Pronto para escaniar 
      byte error, address;
      int nDevices;
      Serial.println("Scanning..."); // Escaniando 
      nDevices = 0;
      for(address = 1; address < 127; address++ ) {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();
        if (error == 0) {
          Serial.print("I2C device found at address 0x"); // Leitura realizada
          if (address<16) {
            Serial.print("0");
          }
          Serial.println(address,HEX);
          nDevices++;
        }
        else if (error==4) {
          Serial.print("Unknow error at address 0x"); // Leitura com erro
          if (address<16) {
            Serial.print("0");
          }
          Serial.println(address,HEX);
        }
      }
      if (nDevices == 0) {
        Serial.println("No I2C devices found\n"); // Leitura não concluída
      }
      else {
        Serial.println("done\n");
      }
    };
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
  // Led aceso para mostrar que o sistema está funcionando 
  digitalWrite(led_vermelho, HIGH);

  leitor = new LeitorRFID(&rfidBase);

}
// Exibir qual foi o cartão lido, sua identificação
void loop() {
  Serial.println("Lendo Cartao:");
  leitor->leCartao();
  if(leitor->cartaoFoiLido()){
    Serial.println(leitor->tipoCartao());
    Serial.println(leitor->cartaoLido());
    leitor->resetarLeitura();
    delay(1000);
  }
}