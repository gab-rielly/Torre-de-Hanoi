#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2); 

#define RST_PIN 9
#define SS_1_PIN 10
#define SS_2_PIN 8
#define SS_3_PIN 6
#define BUZZER 7

#define NOTE_C5 523
#define NOTE_D5 587
#define NOTE_E5 659
#define NOTE_F5 698
#define NOTE_G5 784
#define NOTE_A5 880


#define NR_OF_READERS 3

const String tagarray[3] = { "0493f3d2a21190", "0493f2d2a21190", "0493f1d2a21190" };


byte ssPins[] = { SS_1_PIN, SS_2_PIN, SS_3_PIN };

MFRC522 mfrc522[NR_OF_READERS];

int rfidOrigem = 0;
int rfidAuxiliar = 1;
int rfidDestino = 2;


String ultimoDiscoInseridoRFID0 = "";
String ultimoDiscoInseridoRFID1 = "";
String ultimoDiscoInseridoRFID2 = "";


String discoInseridoRFID0 = "";
String discoInseridoRFID1 = "";
String discoInseridoRFID2 = "";


String vetRFID0[3] = "";
String vetRFID1[3] = "";
String vetRFID2[3] = "";

bool entrouDiscoRFID0 = false;  
bool entrouDiscoRFID1 = false;  
bool entrouDiscoRFID2 = false;  

int contMovimentos = 0;  


volatile bool inicioTempo = false;
int cont = 0;
int horas = 0;
int minutos = 0;
int segundos = 0;
int decimas = 0;
long milisegundos = 0;
long ult = 0;
long atual = 0;
int conta = 3;

int flagTemDisco = false;

void setup() {
  Serial.begin(9600);  
  while (!Serial)
    ;  

  SPI.begin();  

  for (uint8_t leitor = 0; leitor < NR_OF_READERS; leitor++) {
    mfrc522[leitor].PCD_Init(ssPins[leitor], RST_PIN);
    Serial.print(F("Leitor #"));
    Serial.print(leitor);
    Serial.print(F(" Inicializado no pino"));
    Serial.print(String(ssPins[leitor]));
    Serial.print(F(". Potência da antena: "));
    Serial.print(mfrc522[leitor].PCD_GetAntennaGain());
    Serial.print(F(". Versão: "));
    mfrc522[leitor].PCD_DumpVersionToSerial();
    delay(100);
  }
  Serial.println(F("---FIM DA INICIALIZAÇÃO---"));
  Serial.println();

  lcd.init();
  lcd.setBacklight(HIGH);
  lcd.setCursor(0, 0);
  lcd.print("00:00:00");
  lcd.setCursor(0, 1);
  lcd.print("Movi:00");

  preenche(vetRFID0);
  mostraVet(vetRFID0);
  Serial.println();
}
void preenche(String vet[3]) {
  for (int i; i < 3; i++) {
    vetRFID0[i] = tagarray[i];
  }
}
void loop() {

  for (uint8_t leitor = 0; leitor < NR_OF_READERS; leitor++) {
    if (mfrc522[leitor].PICC_IsNewCardPresent() && mfrc522[leitor].PICC_ReadCardSerial()) {
      inicioTempo = true;
      lcd.clear();

      Serial.print(F("Leitor "));
      Serial.print(leitor);
      Serial.print(F(": UID do Cartão: "));
      String cardUID = dump_byte_array(mfrc522[leitor].uid.uidByte, mfrc522[leitor].uid.size);

      if (leitor == rfidOrigem) {

        for (int i = 0; i < sizeof(tagarray) / sizeof(tagarray[0]); i++) {
          if (cardUID == tagarray[i]) {  
            entrouDiscoRFID0 = true;

            String discoInseridoRFID0 = cardUID;  
            Serial.println("discoArmazenado:" + discoInseridoRFID0);

            if (conta > 0) {
              flagTemDisco = true;

              if (flagTemDisco && discoInseridoRFID0 == vetRFID0[i]) {
                entrouDiscoRFID0 = false;
                vetRFID0[i] = "";
                discoInseridoRFID0 = "";
                Serial.println();
                mostraVet(vetRFID0);
                conta--;
                Serial.println(conta);
                flagTemDisco = false;
              }
            } else if (ultimoDiscoInseridoRFID0 == "") { 


              ultimoDiscoInseridoRFID0 = tagarray[i];
              contMovimentos++;
              vetRFID0[i] = ultimoDiscoInseridoRFID0;
              mostraVet(vetRFID0);

             
            } else if (ultimoDiscoInseridoRFID0 == discoInseridoRFID0) {  
             
              int posicaoAnterior = -1;

              for (int j = 0; j < sizeof(tagarray) / sizeof(tagarray[0]); j++) {
                if (vetRFID0[j] == ultimoDiscoInseridoRFID0) {
                  posicaoAnterior = j + 1;
                  if (vetRFID0[posicaoAnterior] == "") {
                    posicaoAnterior = j + 2;
                  }
                  break;
                }
              }
              if (posicaoAnterior != -1) {
                ultimoDiscoInseridoRFID0 = vetRFID0[posicaoAnterior];
                vetRFID0[i] = "";
                discoInseridoRFID0 = "";
                mostraVet(vetRFID0);
                Serial.print("Movimentos: ");
                Serial.println(contMovimentos);
              }
              entrouDiscoRFID0 = false;
              discoInseridoRFID0 = ""; 
              mostraVet(vetRFID0);
              Serial.print("Movimentos: ");
              Serial.println(contMovimentos);
            } else {                      
              int posicaoAtual = i;        
              int posicaoAntecessor = -1;  
              
              for (int j = 0; j < sizeof(tagarray) / sizeof(tagarray[0]); j++) {
                if (ultimoDiscoInseridoRFID0 == tagarray[j]) {
                  posicaoAntecessor = j;  
                  break;
                }
              }                                                                                                        
              if (verificaPosicaoNoRFID(discoInseridoRFID0, ultimoDiscoInseridoRFID0, posicaoAtual, posicaoAntecessor)) 
              {
                vetRFID0[posicaoAtual] = tagarray[i];           
                ultimoDiscoInseridoRFID0 = discoInseridoRFID0;  
                mostraVet(vetRFID0);
                contMovimentos++;
                Serial.print("Movimentos: ");
                Serial.println(contMovimentos);

              } else {  
                resetarJogo();
              }
            }
            discoInseridoRFID0 = "";
          }
        }
      }

      if (leitor == rfidAuxiliar) {

        for (int i = 0; i < sizeof(tagarray) / sizeof(tagarray[0]); i++) {

          if (cardUID == tagarray[i]) {  

            entrouDiscoRFID1 = true;

            String discoInseridoRFID1 = cardUID; 
            Serial.println("discoArmazenado:" + discoInseridoRFID1);

            if (ultimoDiscoInseridoRFID1 == "") { 

              ultimoDiscoInseridoRFID1 = tagarray[i];
              contMovimentos++;
              vetRFID1[i] = ultimoDiscoInseridoRFID1;
              mostraVet(vetRFID1);

            } else if (ultimoDiscoInseridoRFID1 == discoInseridoRFID1) {
            
              entrouDiscoRFID1 = false;
              int posicaoAnterior = -1;

              for (int j = 0; j < sizeof(tagarray) / sizeof(tagarray[0]); j++) {
                if (vetRFID1[j] == ultimoDiscoInseridoRFID1) {
                  posicaoAnterior = j + 1;
                  if (vetRFID1[posicaoAnterior] == "") {
                    posicaoAnterior = j + 2;
                  }
                  break;
                }
              }

              if (posicaoAnterior != -1) {
                ultimoDiscoInseridoRFID1 = vetRFID1[posicaoAnterior];

                vetRFID1[i] = "";
                discoInseridoRFID1 = "";
                mostraVet(vetRFID1);
                Serial.print("Movimentos: ");
                Serial.println(contMovimentos);
              }
              discoInseridoRFID1 = ""; 

            }else {                       
              int posicaoAtual = i;        
              int posicaoAntecessor = -1; 
              
              for (int j = 0; j < sizeof(tagarray) / sizeof(tagarray[0]); j++) {
                if (ultimoDiscoInseridoRFID1 == tagarray[j]) {
                  posicaoAntecessor = j;  
                  break;
                }
              }                                                                                                          
              if (verificaPosicaoNoRFID(discoInseridoRFID1, ultimoDiscoInseridoRFID1, posicaoAtual, posicaoAntecessor))  
              {
                vetRFID1[posicaoAtual] = tagarray[i];          
                ultimoDiscoInseridoRFID1 = discoInseridoRFID1;  
                mostraVet(vetRFID1);
                contMovimentos++;
                Serial.print("Movimentos: ");
                Serial.println(contMovimentos);

              } else {  
                resetarJogo();
              }
            }
            discoInseridoRFID1 = "";

            if(percorreTorre(vetRFID1)==true){
              ultimoDiscoInseridoRFID1="";
            }
          }
        }
      }

      if (leitor == rfidDestino) {
        for (int i = 0; i < sizeof(tagarray) / sizeof(tagarray[0]); i++) {
          if (cardUID == tagarray[i]) { 

            entrouDiscoRFID2 = true;
            String discoInseridoRFID2 = cardUID;  
            Serial.println("discoArmazenado:" + discoInseridoRFID2);

            if (ultimoDiscoInseridoRFID2 == "") { 

              ultimoDiscoInseridoRFID2 = tagarray[i];

              vetRFID2[i] = ultimoDiscoInseridoRFID2;
              contMovimentos = contMovimentos + 1;
              mostraVet(vetRFID2);

              Serial.println("Movimentos: " + contMovimentos);
              

            } else if (ultimoDiscoInseridoRFID2 == discoInseridoRFID2) { 

              int posicaoAnterior = -1;
              for (int j = 0; j < sizeof(tagarray) / sizeof(tagarray[0]); j++) {
                if (vetRFID2[j] == ultimoDiscoInseridoRFID2) {
                  posicaoAnterior = j + 1;
                  if (vetRFID2[posicaoAnterior] == "") {
                    posicaoAnterior = j + 2;
                  }
                  break;
                }
              }

              if (posicaoAnterior != -1) {
                ultimoDiscoInseridoRFID2 = vetRFID2[posicaoAnterior];
                vetRFID2[i] = "";
                discoInseridoRFID2 = "";
                mostraVet(vetRFID2);
                Serial.print("Movimentos: ");
                Serial.println(contMovimentos);
              }
              entrouDiscoRFID2 = false;
              discoInseridoRFID2 = "";  


            } else {                       
              int posicaoAtual = i;        
              int posicaoAntecessor = -1;  
              
              for (int j = 0; j < sizeof(tagarray) / sizeof(tagarray[0]); j++) {
                if (ultimoDiscoInseridoRFID2 == tagarray[j]) {
                  posicaoAntecessor = j;  
                  break;
                }
              }                                                                                                          
              if (verificaPosicaoNoRFID(discoInseridoRFID2, ultimoDiscoInseridoRFID2, posicaoAtual, posicaoAntecessor))
              {
                vetRFID2[posicaoAtual] = tagarray[i];           
                ultimoDiscoInseridoRFID2 = discoInseridoRFID2;  
                mostraVet(vetRFID2);
                contMovimentos++;

                Serial.print("Movimentos: ");
                Serial.println(contMovimentos);

              } else {  
                resetarJogo();
              }
            }
            discoInseridoRFID2 = "";
          }
        }
      }
      contador();
      if (verificarVitoria(vetRFID2)) {
        cardUID = "";
        mostraVitoria();
        inicioTempo = false;
        resetarJogo();
        lcd.clear();
        break;
      }
      continue;
    }
    mfrc522[leitor].PICC_HaltA();
    mfrc522[leitor].PCD_StopCrypto1();
  }
  if (inicioTempo == true) {
    contaTempo();
  } else {
    resetaTempo();
  }
}

bool percorreTorre(String vet[]) {
  int cont=0;
  for (int i = 0; i < 3; i++) {
    if (vet[i] == "") {
      cont++;
    }
  }

  if(cont==3){
    return true;
  }
  return false;  

void resetaTempo() {
  inicioTempo = false;
  cont = 0;
  horas = 0;
  minutos = 0;
  segundos = 0;
  decimas = 0;
  milisegundos = 0;
  ult = 0;
  atual = 0;
}
void contaTempo() {
  atual = millis();
  

  if ((atual - ult) >= 1) {  
    ult = atual;

    decimas++;
    if (decimas == 60) {  
      decimas = 0;
      segundos++;
    }
    if (segundos == 60) {
      
      segundos = 0;
      minutos++;
    }
    if (minutos == 60) {  
      minutos = 0;
      
    }

    lcd.setCursor(0, 0);
    if (minutos < 10) {  
      lcd.print("0");
    }
    lcd.print(minutos);
    lcd.print(":");
    lcd.setCursor(3, 0);

    if (segundos < 10) {  
      lcd.print("0");
    }
    lcd.print(segundos);
    lcd.print(":");
    lcd.setCursor(6, 0);

    if (decimas == 60) {
      lcd.print("00");
      delay(1);
      lcd.clear();
    }
    lcd.print(decimas);
  }
}
bool verificarVitoria(String vet[]) {
  for (int i = 0; i < 3; i++) {
    if (vet[i] == "") {
      return false;  
    }
  }
  return true; 
}
bool verificaPosicaoNoRFID(String atual, String antecessor, int posicaoAtual, int posicaoAntecessor) {
  if ((posicaoAtual == 2 && posicaoAntecessor == 0) || (posicaoAtual == 1 && posicaoAntecessor == 0)) {
    buzzer();
    Serial.println("Erro: Disco na posição " + String(posicaoAtual) + " não pode ser empilhado acima do disco na posição " + String(posicaoAntecessor));
  
    return false;
  } else if (posicaoAtual == 2 && posicaoAntecessor == 1) {
    buzzer();
    Serial.println("Erro: Disco na posição " + String(posicaoAtual) + " não pode ser empilhado acima do disco na posição " + String(posicaoAntecessor));
  
    return false;
  } else if (posicaoAtual == 1 && posicaoAntecessor == 0) {
    buzzer();
    Serial.println("Erro: Disco na posição " + String(posicaoAtual) + " não pode ser empilhado acima do disco na posição " + String(posicaoAntecessor));

    return false;

  } else {
    Serial.println("O empilhamento está correto.");
    return true; 
  }
}
void contador() {
  lcd.setCursor(0, 1);
  lcd.print("Movi:");
  if (contMovimentos < 10) {
    lcd.print("0");
    lcd.print(contMovimentos);
  } else {
    lcd.print(contMovimentos);
  }
}
void mostraVet(String vet[3]) {
  for (int i = 0; i < 3; i++) {
    Serial.print("posicao ");
    Serial.print(i);
    Serial.print(": ");
    Serial.println(vet[i]);
  }
}
String dump_byte_array(byte* buffer, byte bufferSize) {
  String result;
  for (byte i = 0; i < bufferSize; i++) {
    result += (buffer[i] < 0x10 ? "0" : "");
    result += String(buffer[i], HEX);
  }
  return result;
}
void buzzer() {
  int bips = 1;
  for (int j = 0; j < bips; j++) {
    tone(BUZZER, 392);
    delay(800);
    noTone(BUZZER);
    delay(200);
  }
}
void resetarJogo() {

  ultimoDiscoInseridoRFID0 = "";
  ultimoDiscoInseridoRFID1 = "";
  ultimoDiscoInseridoRFID2 = "";

  discoInseridoRFID0 = "";
  discoInseridoRFID1 = "";
  discoInseridoRFID2 = "";

  for (int i = 0; i < 3; i++) {
    vetRFID0[i] = "";
    vetRFID1[i] = "";
    vetRFID2[i] = "";
  }
  contMovimentos = 0;

  entrouDiscoRFID0 = false;
  entrouDiscoRFID1 = false;
  entrouDiscoRFID2 = false;
  inicioTempo = false;
  conta = 3;
  lcd.clear();
  lcd.setBacklight(HIGH);
  lcd.setCursor(0, 0);
  lcd.print("Reinicia Jogo.");
  delay(1000);
  lcd.clear();
  lcd.print("00:00:00");
  lcd.setCursor(0, 1);
}
void tocarMusicaVitoria() {
  int melody[] = {
    NOTE_C5, NOTE_D5, NOTE_E5, NOTE_F5, NOTE_G5, NOTE_A5, NOTE_G5, NOTE_F5
  };

  int noteDurations[] = {
    200, 200, 200, 200, 200, 200, 400, 400
  };

  int numNotas = sizeof(melody) / sizeof(melody[0]);

  for (int i = 0; i < numNotas; i++) {
    int duration = noteDurations[i];
    if (melody[i] == 0) {
      delay(duration);
    } else {
      tone(BUZZER, melody[i], duration);
      delay(duration * 1.3);
    }
    noTone(BUZZER);
  }
}
void mostraVitoria() {
  delay(500);
  lcd.clear();
  lcd.setBacklight(HIGH);
  lcd.setCursor(0, 0);

  lcd.print("Parabéns!");
  tocarMusicaVitoria();
  delay(1000);
  lcd.clear();
}
