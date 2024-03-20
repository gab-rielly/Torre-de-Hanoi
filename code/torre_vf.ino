#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);  //LiquidCrystal_I2C lcd(endereco, colunas, linhas); //INSTANCIANDO OBJETOS lcd

// Números dos Pinos: RESET + SDAs
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

// Número de leitores
#define NR_OF_READERS 3

// Vetor de Tags UIDs que representam os discos (do menor para o maior)
const String tagarray[3] = { "0493f3d2a21190", "0493f2d2a21190", "0493f1d2a21190" };

// Vetor de conexões sda
byte ssPins[] = { SS_1_PIN, SS_2_PIN, SS_3_PIN };

MFRC522 mfrc522[NR_OF_READERS];

int rfidOrigem = 0;
int rfidAuxiliar = 1;
int rfidDestino = 2;

// Armazena o último disco lido no leitor RFID
String ultimoDiscoInseridoRFID0 = "";
String ultimoDiscoInseridoRFID1 = "";
String ultimoDiscoInseridoRFID2 = "";

// Armazena o atual disco lido no leitor RFID
String discoInseridoRFID0 = "";
String discoInseridoRFID1 = "";
String discoInseridoRFID2 = "";

//verifica o preenchimento das torres
String vetRFID0[3] = "";
String vetRFID1[3] = "";
String vetRFID2[3] = "";

bool entrouDiscoRFID0 = false;  //verifica movimento de entrada T-T
bool entrouDiscoRFID1 = false;  //verifica movimento de entrada T-T
bool entrouDiscoRFID2 = false;  //verifica movimento de entrada T-T

int contMovimentos = 0;  // Contador para rastrear o número de movimentos

//cronometro
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
  Serial.begin(9600);  // Inicializar a comunicação serial com o PC
  while (!Serial)
    ;  // Não fazer nada se nenhuma porta serial estiver aberta (adicionado para Arduinos baseados em ATMEGA32U4)

  SPI.begin();  // Iniciar o barramento SPI

  /* Procurar pelos leitores MFRC522*/
  //attachInterrupt(digitalPinToInterrupt(pinoBotao), buttonPressed, RISING);
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

  preenche(vetRFID0);//rfif origem inicia com os tres discos
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
       /*******************************************
        rfid de origem
        *******************************************/
        for (int i = 0; i < sizeof(tagarray) / sizeof(tagarray[0]); i++) {
          if (cardUID == tagarray[i]) {  //verifica se a tag esta contida no vetor das tags
            entrouDiscoRFID0 = true;

            String discoInseridoRFID0 = cardUID;  //disco que acabou de ser lido
            Serial.println("discoArmazenado:" + discoInseridoRFID0);

            if (conta > 0) {//gambiarra pra retirar os 3 discos da torre origem 
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
              /*Se estiver vazia vai receber o disco que foi inserido atualmente para ser comparado depois*/

              ultimoDiscoInseridoRFID0 = tagarray[i];
              contMovimentos++;
              vetRFID0[i] = ultimoDiscoInseridoRFID0;
              mostraVet(vetRFID0);

              //preenche nessa mesma posição-> implica que o disco dessa posição tá lá no vetor;

            } else if (ultimoDiscoInseridoRFID0 == discoInseridoRFID0) {  
              //O disco que prrencheu o vetor do RFID0 é igual ao disco q foi inserido então eh um movimento de saida
              int posicaoAnterior = -1;
              //quem é a posicao anterior do ultimo disco que foi retirado?
              //isso aqui é escificamente para 3 discos "j-1" e "j-2"
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
              discoInseridoRFID0 = "";  // Reinicializa a variável para evitar leituras não relacionadas a essa verificação
              mostraVet(vetRFID0);
              Serial.print("Movimentos: ");
              Serial.println(contMovimentos);
            } else {                       //e se tiver um disco na torre?
              int posicaoAtual = i;        //a posicao de quem tá sendo lindo atualmente
              int posicaoAntecessor = -1;  //a posicao de quem tá laá na torre q alias n sei quem eh?
              // Encontre a posição do último disco inserido no vetor tagarray
              for (int j = 0; j < sizeof(tagarray) / sizeof(tagarray[0]); j++) {
                if (ultimoDiscoInseridoRFID0 == tagarray[j]) {
                  posicaoAntecessor = j;  //encontrei a posicao do antecessor
                  break;
                }
              }                                                                                                          // Agora a gente quer ter certeza de que o maior n vai ficar sobre o menor
              if (verificaPosicaoNoRFID(discoInseridoRFID0, ultimoDiscoInseridoRFID0, posicaoAtual, posicaoAntecessor))  //caso vdd o movimento tá correto
              {
                vetRFID0[posicaoAtual] = tagarray[i];           //então pode atualizar verdadeira a posição no vetor
                ultimoDiscoInseridoRFID0 = discoInseridoRFID0;  // isso aq é pq vamos ter outra comparacao depois
                mostraVet(vetRFID0);
                contMovimentos++;
                Serial.print("Movimentos: ");
                Serial.println(contMovimentos);

              } else {  //caso mentira o movimento tá INcorreto então reinicia TUDOOH
                resetarJogo();
              }
            }
            discoInseridoRFID0 = "";
          }
          /*******************************************
            fim do bloco do rfid de origem
          *******************************************/
        }
      }
      /*RFID AUX*/
      if (leitor == rfidAuxiliar) {

        for (int i = 0; i < sizeof(tagarray) / sizeof(tagarray[0]); i++) {

          if (cardUID == tagarray[i]) {  //verifica se a tag esta contida no vetor das tags

            /*******************************************
              rfid de auxxx
            *******************************************/
            entrouDiscoRFID1 = true;

            String discoInseridoRFID1 = cardUID;  //disco que acabou de ser lido
            Serial.println("discoArmazenado:" + discoInseridoRFID1);

            if (ultimoDiscoInseridoRFID1 == "") { /*Se estiver vazia vai receber o disco que foi inserido atualmente para ser comparado depois*/

              ultimoDiscoInseridoRFID1 = tagarray[i];
              contMovimentos++;
              vetRFID1[i] = ultimoDiscoInseridoRFID1;
              mostraVet(vetRFID1);

              //preenche nessa mesma posição-> implica que o disco dessa posição tá lá no vetor;

            } else if (ultimoDiscoInseridoRFID1 == discoInseridoRFID1) {
              //O disco que preencheu o vetor do RFID1 é igual ao disco q foi inserido então eh um movimento de saida
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
              discoInseridoRFID1 = "";  // Reinicializa a variável para evitar leituras não relacionadas a essa verificação

            }else {                       //e se tiver um disco na torre?
              int posicaoAtual = i;        //a posicao de quem tá sendo lindo atualmente
              int posicaoAntecessor = -1;  //a posicao de quem tá laá na torre q alias n sei quem eh?
              // Encontre a posição do último disco inserido no vetor tagarray
              for (int j = 0; j < sizeof(tagarray) / sizeof(tagarray[0]); j++) {
                if (ultimoDiscoInseridoRFID1 == tagarray[j]) {
                  posicaoAntecessor = j;  //encontrei a posicao do antecessor
                  break;
                }
              }                                                                                                          // Agora a gente quer ter certeza de que o maior n vai ficar sobre o menor
              if (verificaPosicaoNoRFID(discoInseridoRFID1, ultimoDiscoInseridoRFID1, posicaoAtual, posicaoAntecessor))  //caso vdd o movimento tá correto
              {
                vetRFID1[posicaoAtual] = tagarray[i];           //então pode atualizar verdadeira a posição no vetor
                ultimoDiscoInseridoRFID1 = discoInseridoRFID1;  // isso aq é pq vamos ter outra comparacao depois
                mostraVet(vetRFID1);
                contMovimentos++;
                Serial.print("Movimentos: ");
                Serial.println(contMovimentos);

              } else {  //caso mentira o movimento tá INcorreto então reinicia TUDOOH
                resetarJogo();
              }
            }
            discoInseridoRFID1 = "";

            if(percorreTorre(vetRFID1)==true){
              ultimoDiscoInseridoRFID1="";
            }
          }
          /*******************************************
            fim do bloco do rfid de auxiliar
          *******************************************/
        }
      }

      if (leitor == rfidDestino) {
        for (int i = 0; i < sizeof(tagarray) / sizeof(tagarray[0]); i++) {
          if (cardUID == tagarray[i]) {  //verifica se a tag esta contida no vetor das tags

            entrouDiscoRFID2 = true;
            String discoInseridoRFID2 = cardUID;  //disco que acabou de ser lido
            Serial.println("discoArmazenado:" + discoInseridoRFID2);

            if (ultimoDiscoInseridoRFID2 == "") { /*Se estiver vazia vai receber o disco que foi inserido atualmente para ser comparado depois*/

              ultimoDiscoInseridoRFID2 = tagarray[i];

              vetRFID2[i] = ultimoDiscoInseridoRFID2;
              contMovimentos = contMovimentos + 1;
              mostraVet(vetRFID2);

              Serial.println("Movimentos: " + contMovimentos);
              //preenche nessa mesma posição-> implica que o disco dessa posição tá lá no vetor;

            } else if (ultimoDiscoInseridoRFID2 == discoInseridoRFID2) {  //O disco que prrencheu o vetor do RFID0 é igual ao disco q foi inserido então eh um movimento de saida

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
              discoInseridoRFID2 = "";  // Reinicializa a variável para evitar leituras não relacionadas a essa verificação


            } else {                       //e se tiver um disco na torre?
              int posicaoAtual = i;        //a posicao de quem tá sendo lindo atualmente
              int posicaoAntecessor = -1;  //a posicao de quem tá laá na torre q alias n sei quem eh?
              // Encontre a posição do último disco inserido no vetor tagarray
              for (int j = 0; j < sizeof(tagarray) / sizeof(tagarray[0]); j++) {
                if (ultimoDiscoInseridoRFID2 == tagarray[j]) {
                  posicaoAntecessor = j;  //encontrei a posicao do antecessor
                  break;
                }
              }                                                                                                          // Agora a gente quer ter certeza de que o maior n vai ficar sobre o menor
              if (verificaPosicaoNoRFID(discoInseridoRFID2, ultimoDiscoInseridoRFID2, posicaoAtual, posicaoAntecessor))  //caso vdd o movimento tá correto
              {
                vetRFID2[posicaoAtual] = tagarray[i];           //então pode stualizar verdadeira a posição no vetor
                ultimoDiscoInseridoRFID2 = discoInseridoRFID2;  // isso aq é pq vamos ter outra comparacao depois
                mostraVet(vetRFID2);
                contMovimentos++;

                Serial.print("Movimentos: ");
                Serial.println(contMovimentos);

              } else {  //caso mentira o movimento tá INcorreto então reinicia TUDOOH
                resetarJogo();
              }
            }
            discoInseridoRFID2 = "";
          }


          /*******************************************
            fim do bloco do rfid destino
          *******************************************/
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
  return false;  // Todas as posições estão preenchidas, retorna verdadeiro

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
  //Serial.println(milisegundos);

  if ((atual - ult) >= 1) {  //Entrar apenas se tiver passado o décimo de segundo
    ult = atual;

    decimas++;
    if (decimas == 60) {  //Quando passou 10 décimos de segundo, conta um segundo
      decimas = 0;
      segundos++;
    }
    if (segundos == 60) {
      // Após 60 segundos, conta um minuto
      segundos = 0;
      minutos++;
    }
    if (minutos == 60) {  // Depois de 60 minutos, conta uma hora
      minutos = 0;
      //horas++;
    }
    //exibição no LCD
    lcd.setCursor(0, 0);
    if (minutos < 10) {  //se os minutos forem menor que 10, acrescenta um 0 na frente
      lcd.print("0");
    }
    lcd.print(minutos);
    lcd.print(":");
    lcd.setCursor(3, 0);

    if (segundos < 10) {  //se os segundos forem menor que 10, acrescenta um 0 na frente
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
      return false;  // Se alguma posição estiver vazia, retorna falso
    }
  }
  return true;  // Todas as posições estão preenchidas, retorna verdadeiro
}
bool verificaPosicaoNoRFID(String atual, String antecessor, int posicaoAtual, int posicaoAntecessor) {
  if ((posicaoAtual == 2 && posicaoAntecessor == 0) || (posicaoAtual == 1 && posicaoAntecessor == 0)) {
    buzzer();
    Serial.println("Erro: Disco na posição " + String(posicaoAtual) + " não pode ser empilhado acima do disco na posição " + String(posicaoAntecessor));
    //error( posicaoAtual, posicaoAntecessor);
    return false;
  } else if (posicaoAtual == 2 && posicaoAntecessor == 1) {
    buzzer();
    Serial.println("Erro: Disco na posição " + String(posicaoAtual) + " não pode ser empilhado acima do disco na posição " + String(posicaoAntecessor));
    //error(posicaoAtual, posicaoAntecessor);
    return false;
  } else if (posicaoAtual == 1 && posicaoAntecessor == 0) {
    buzzer();
    Serial.println("Erro: Disco na posição " + String(posicaoAtual) + " não pode ser empilhado acima do disco na posição " + String(posicaoAntecessor));
    //error(posicaoAtual, posicaoAntecessor);
    return false;

  } else {
    Serial.println("O empilhamento está correto.");
    return true;  //tudo bem gracas a Deus
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
  // Armazena o último disco lido
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
