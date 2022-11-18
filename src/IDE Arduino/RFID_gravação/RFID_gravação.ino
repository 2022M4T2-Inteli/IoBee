#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#define I2C_SDA  47
#define I2C_SCL  48
 
//Pinos Reset e SS módulo MFRC522
#define SS_PIN 21
#define RST_PIN 14
MFRC522 mfrc522(SS_PIN, RST_PIN);
 
LiquidCrystal_I2C lcd(0x27, 16, 2);

#define pino_botao_le 41
#define pino_botao_gr 39
#define led_az_le     2
#define led_ve_gr     5
#define led_sis       4
#define buzzer        37
 
MFRC522::MIFARE_Key key;
 
void setup()
{
  pinMode(pino_botao_le, INPUT);
  pinMode(pino_botao_gr, INPUT);
  pinMode(led_az_le, OUTPUT);
  pinMode(led_ve_gr, OUTPUT);
  pinMode(led_sis, OUTPUT);
  pinMode(buzzer, OUTPUT);
 
  Serial.begin(9600);   //Inicia a serial
  SPI.begin();      //Inicia  SPI bus
  mfrc522.PCD_Init();   //Inicia MFRC522
 
  //Inicializa o LCD 16x2
  // lcd.begin(16, 2);
  Wire.begin(I2C_SDA, I2C_SCL);
  lcd.init();
  lcd.backlight();
  lcd.print("Bem-Vindo");

  //Mostra visualmente a Energização do Sistema
  digitalWrite(led_az_le, HIGH);
  digitalWrite(led_ve_gr, HIGH);
  digitalWrite(led_sis, HIGH);
  delay(1000);
  mensageminicial();
  digitalWrite(led_az_le, LOW);
  digitalWrite(led_ve_gr, LOW); 
 
  //Prepara chave - padrao de fabrica = FFFFFFFFFFFFh
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;
}
 
void loop()
{
  //Verifica se o botao modo leitura foi pressionado
  int modo_le = digitalRead(pino_botao_le);
  if (modo_le != 0)
  {
    lcd.clear();
    Serial.println("Modo leitura selecionado");
    lcd.setCursor(2, 0);
    lcd.print("Modo leitura");
    lcd.setCursor(3, 1);
    lcd.print("selecionado");
    digitalWrite(led_az_le, HIGH);
    delay(2000);
    digitalWrite(led_az_le, LOW);
    while (digitalRead(pino_botao_le) == 1) {}
    delay(200);
    modo_leitura();
  }
  //Verifica se o botao modo gravacao foi pressionado
  int modo_gr = digitalRead(pino_botao_gr);
  if (modo_gr != 0)
  {
    lcd.clear();
    Serial.println("Modo gravacao selecionado");
    lcd.setCursor(2, 0);
    lcd.print("Modo gravacao");
    lcd.setCursor(3, 1);
    lcd.print("selecionado");
    digitalWrite(led_ve_gr, HIGH);
    delay(2000);
    digitalWrite(led_ve_gr, LOW);
    while (digitalRead(pino_botao_gr) == 1) {}
    delay(200);
    modo_gravacao();
  }
}
void mensageminicial()
{
  digitalWrite(led_az_le, LOW);
  digitalWrite(led_ve_gr, LOW);
  Serial.println("nSelecione o modo leitura ou gravacao...");
  Serial.println();
  lcd.clear();
  lcd.print("Selecione o modo");
  lcd.setCursor(0, 1);
  lcd.print("leitura/gravacao");
}
 
void mensagem_inicial_cartao(int resposta)
{
  Serial.println("Aproxime o seu cartao do leitor...");
  lcd.clear();
  lcd.print(" Aproxime o seu");
  lcd.setCursor(0, 1);
  lcd.print("cartao do leitor");
  while ( ! mfrc522.PICC_IsNewCardPresent()){
    if(resposta == 1){
    digitalWrite(led_az_le, HIGH);
    delay(80);
    digitalWrite(led_az_le, LOW);
    delay(80);
    } else {
    digitalWrite(led_ve_gr, HIGH);
    delay(80);
    digitalWrite(led_ve_gr, LOW);
    delay(80);
    }
  }
}
 
void modo_leitura()
{
  int leitura = 1;
  mensagem_inicial_cartao(leitura);
  //Aguarda cartao
  while ( ! mfrc522.PICC_IsNewCardPresent())
  {
    delay(100);
  }
  if ( ! mfrc522.PICC_ReadCardSerial())
  {
    return;
  }
  //Mostra UID na serial
  Serial.print("UID da tag : ");
  String conteudo = "";
  byte letra;
  for (byte i = 0; i < mfrc522.uid.size; i++)
  {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    conteudo.concat(String(mfrc522.uid.uidByte[i]<0x10 ? " 0" : " "));
    conteudo.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  Serial.println();
 
  //Obtem os dados do setor 1, bloco 4 = Nome
  byte sector         = 1;
  byte blockAddr      = 4;
  byte trailerBlock   = 7;
  byte status;
  byte buffer[18];
  byte size = sizeof(buffer);
 
  //Autenticacao usando chave A
  status=mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A,
                                  trailerBlock, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("PCD_Authenticate() failed: "));
    Serial.println(mfrc522.GetStatusCodeName((MFRC522::StatusCode)status));
    return;
  }
  status = mfrc522.MIFARE_Read(blockAddr, buffer, &size);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE_Read() failed: "));
    Serial.println(mfrc522.GetStatusCodeName((MFRC522::StatusCode)status));
  }
  //Mostra os dados do nome no Serial Monitor e LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  for (byte i = 1; i < 16; i++)
  {
    Serial.print(char(buffer[i]));
    lcd.write(char(buffer[i]));
  }
  Serial.println();
 
  //Obtem os dados do setor 0, bloco 1 = Sobrenome
  sector         = 0;
  blockAddr      = 1;
  trailerBlock   = 3;
 
  //Autenticacao usando chave A
  status=mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A,
                                  trailerBlock, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK)
  {
    Serial.print(F("PCD_Authenticate() failed: "));
    Serial.println(mfrc522.GetStatusCodeName((MFRC522::StatusCode)status));
    return;
  }
  status = mfrc522.MIFARE_Read(blockAddr, buffer, &size);
  if (status != MFRC522::STATUS_OK)
  {
    Serial.print(F("MIFARE_Read() failed: "));
    Serial.println(mfrc522.GetStatusCodeName((MFRC522::StatusCode)status));
  }
  //Mostra os dados do sobrenome no Serial Monitor e LCD
  lcd.setCursor(0, 1);
  for (byte i = 0; i < 16; i++)
  {
    Serial.print(char(buffer[i]));
    lcd.write(char(buffer[i]));
  }
  Serial.println();
  digitalWrite(led_az_le, HIGH);
 
  // Halt PICC
  mfrc522.PICC_HaltA();
  // Stop encryption on PCD
  mfrc522.PCD_StopCrypto1();
  delay(2000);
  mensageminicial();
}
 
void modo_gravacao()
{
  //Variável que diferencia as cores dos leds durante a chamada da mensagem.
  int leitura = 0;
  mensagem_inicial_cartao(leitura);
  //Aguarda cartao
  while ( ! mfrc522.PICC_IsNewCardPresent()) {
    digitalWrite(led_ve_gr, HIGH);
    delay(100);
  }
  if ( ! mfrc522.PICC_ReadCardSerial())    return;
 
  //Mostra UID na serial
  Serial.print(F("UID do Cartao: "));    //Dump UID
  for (byte i = 0; i < mfrc522.uid.size; i++)
  {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
  }
  //Mostra o tipo do cartao
  Serial.print(F("nTipo do PICC: "));
  byte piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
  // Serial.println(mfrc522.PICC_GetTypeName(piccType));
 
  byte buffer[34];
  byte block;
  byte status, len;
 
  Serial.setTimeout(20000L) ;
  Serial.println(F("Digite o sobrenome,em seguida o caractere #"));
  lcd.clear();
  lcd.print("Digite o sobreno");
  lcd.setCursor(0, 1);
  lcd.print("me + #");
  len = Serial.readBytesUntil('#', (char *) buffer, 30);
  for (byte i = len; i < 30; i++) buffer[i] = ' ';

  block = 1;
  //Serial.println(F("Autenticacao usando chave A..."));
  status=mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A,
                                    block, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("PCD_Authenticate() failed: "));
    Serial.println(mfrc522.GetStatusCodeName((MFRC522::StatusCode)status));
    return;
  }
 
  //Grava no bloco 1
  status = mfrc522.MIFARE_Write(block, buffer, 16);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE_Write() failed: "));
    Serial.println(mfrc522.GetStatusCodeName((MFRC522::StatusCode)status));
    return;
  }
 
  block = 2;
  //Serial.println(F("Autenticacao usando chave A..."));
  status=mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A,
                                    block, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("PCD_Authenticate() failed: "));
    Serial.println(mfrc522.GetStatusCodeName((MFRC522::StatusCode)status));
    return;
  }
 
  //Grava no bloco 2
  status = mfrc522.MIFARE_Write(block, &buffer[16], 16);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE_Write() failed: "));
    Serial.println(mfrc522.GetStatusCodeName((MFRC522::StatusCode)status));
    return;
  }
    for(int i = 0; i < 10; i++){
      digitalWrite(led_ve_gr, LOW);
      delay(40);
      digitalWrite(led_ve_gr, HIGH);
      delay(40);
    };

  Serial.println(F("Digite o nome, em seguida o caractere #"));
  lcd.clear();
  lcd.print("Digite o nome e");
  lcd.setCursor(0, 1);
  lcd.print("em seguida #");
  len = Serial.readBytesUntil('#', (char *) buffer, 20) ;
  for (byte i = len; i < 20; i++) buffer[i] = ' ';
 
  block = 4;
  //Serial.println(F("Autenticacao usando chave A..."));
  status=mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A,
                                    block, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("PCD_Authenticate() failed: "));
    Serial.println(mfrc522.GetStatusCodeName((MFRC522::StatusCode)status));
    return;
  }
 
  //Grava no bloco 4
  status = mfrc522.MIFARE_Write(block, buffer, 16);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE_Write() failed: "));
    Serial.println(mfrc522.GetStatusCodeName((MFRC522::StatusCode)status));
    return;
  }
 
  block = 5;
  //Serial.println(F("Authenticating using key A..."));
  status=mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A,
                                    block, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("PCD_Authenticate() failed: "));
    Serial.println(mfrc522.GetStatusCodeName((MFRC522::StatusCode)status));
    return;
  }
 
  //Grava no bloco 5
  status = mfrc522.MIFARE_Write(block, &buffer[16], 16);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE_Write() failed: "));
    Serial.println(mfrc522.GetStatusCodeName((MFRC522::StatusCode)status));
    //return;
  }
  else
  {
    for(int i = 0; i < 10; i++){
      digitalWrite(led_ve_gr, LOW);
      delay(40);
      digitalWrite(led_ve_gr, HIGH);
      delay(40);
    };
    Serial.println(F("Dados gravados com sucesso!"));
    lcd.clear();
    lcd.print("Gravacao OK!");
  }
 
  mfrc522.PICC_HaltA(); // Halt PICC
  mfrc522.PCD_StopCrypto1();  // Stop encryption on PCD
  delay(2000);
  mensageminicial();
}