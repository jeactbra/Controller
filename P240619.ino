#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include "max6675.h"
#include <SPI.h>
#include <SD.h>
#include <Keypad.h> // Biblioteca do código

LiquidCrystal_I2C lcd(0x27, 16, 2); //I2C address is 
unsigned long ultimoTempoExibicao = 0;
unsigned long key_tempo = 0;
unsigned long key_taxa = 0;
unsigned long energy_count = 0;
unsigned long start_exp = 0;

// ___________________________________________________________________________________________________________
// TECLADO CODIGO
// ___________________________________________________________________________________________________________

const byte LINHAS = 4; // Linhas do teclado
const byte COLUNAS = 4; // Colunas do teclado

const char TECLAS_MATRIZ[LINHAS][COLUNAS] = { // Matriz de caracteres (mapeamento do teclado)
  {'1', '2', '3', 'A'},
  {'4', '5', '6', '-'},
  {'7', '8', '9', '*'},
  {'.', '0', '=', 'D'} // Substitui '=' por '/' para divisão
};

const byte PINOS_LINHAS[LINHAS] = {9, 8, 7, 6}; // Pinos de conexão com as linhas do teclado
const byte PINOS_COLUNAS[COLUNAS] = {5, 4, 3, 2}; // Pinos de conexão com as colunas do teclado

Keypad teclado_personalizado = Keypad(makeKeymap(TECLAS_MATRIZ), PINOS_LINHAS, PINOS_COLUNAS, LINHAS, COLUNAS); // Inicia teclado

String inputBuffer1 = ""; // Buffer para armazenar o primeiro número digitado
String inputBuffer2 = ""; // Buffer para armazenar o segundo número digitado
String inputBuffer3 = ""; // Buffer para armazenar o segundo número digitado
bool Key_temperatura = false; // Variável para indicar se o primeiro valor foi digitado
bool Key_rate = false; // Variável para indicar se o segundo valor foi digitado
bool Key_time = false; // Variável para indicar se o segundo valor foi digitado

// ___________________________________________________________________________________________________________
// CRIAÇÃO DE CARACTERES
// ___________________________________________________________________________________________________________
// Cria o caracter de grau para aprecer no display lcd
byte grau[8] = {
  0b00111,
  0b00101,
  0b00111,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000
};

// ___________________________________________________________________________________________________________
// NOME DO ARQUIVO NO CARTÃO SD
// ___________________________________________________________________________________________________________
const char* baseFileName = "dados";
const char* fileExtension = ".txt";
char fileName[13]; // Nome do arquivo final (8 caracteres base + 3 caracteres extensão + 1 caractere nulo)

// ___________________________________________________________________________________________________________
// VARIAVEIS DE TEMPO
// ___________________________________________________________________________________________________________
unsigned long tempoInicial;
unsigned long tempoInicial_Contador;
unsigned long segundosPassados = 0;
unsigned long minutos_exp = 0;
unsigned long segundos_exp = 0;

// ___________________________________________________________________________________________________________
// PINOS DO MÓDULO ADPTADOR SD
// ___________________________________________________________________________________________________________
const int chipSelect = 53;

// ___________________________________________________________________________________________________________
// PINOS DOS SENSORES DE TEMPERATURA
// ___________________________________________________________________________________________________________
const int thermo1_SO = 45;
const int thermo1_CS = 47;
const int thermo1_CLK = 49;
MAX6675 thermo1(thermo1_CLK, thermo1_CS, thermo1_SO);

const int thermo2_SO = 44;
const int thermo2_CS = 46;
const int thermo2_CLK = 48;
MAX6675 thermo2(thermo2_CLK, thermo2_CS, thermo2_SO);

const int thermo3_SO = 39;
const int thermo3_CS = 41;
const int thermo3_CLK = 43;
MAX6675 thermo3(thermo3_CLK, thermo3_CS, thermo3_SO);

const int thermo4_SO = 38;
const int thermo4_CS = 40;
const int thermo4_CLK = 42;
MAX6675 thermo4(thermo4_CLK, thermo4_CS, thermo4_SO);

const int thermo5_SO = 33;
const int thermo5_CS = 35;
const int thermo5_CLK = 37;
MAX6675 thermo5(thermo5_CLK, thermo5_CS, thermo5_SO);

// ___________________________________________________________________________________________________________
// PINOS DE ALIMENTAÇÃO DOS RELÉS
// ___________________________________________________________________________________________________________
const int vccPin = A14; // Tensão para o relé
const int gndPin = 22; // Gera um gnd para o relé

const int vccSDcard = A5; //Tensão para o cartão SD
const int vccThermo_heater = A4; //Tensão para o termopar da resistencia
const int vccThermo_reactor = A3; //Tensão para o termopar do reator
const int vccThermo_exit = A2; //Tensão para o termopar na saida do reator
const int vccThermo_outside = A1; //Tensão para o termopar na manta
const int vccThermo_outside2 = A0; //Tensão para o termopar da resistencia

// ___________________________________________________________________________________________________________
// PARAMETROS DE ENTRADA
// ___________________________________________________________________________________________________________
float temperaturaMaximaReator = 0;  // Temperatura máxima reator (em graus Celsius)
float tempo_experimental = 0; // tempo total de experimento (em minutos)
float taxa_aquecimento = 0; // Taxa de temperatura que o reator vai aquecer em ºC/min

float temperaturaMaximaForno = 0;  // Temperatura máxima forno (em graus Celsius)
float temperaturaMinima = 0;  // Calcula a temperatura mínima

// _-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-
// _-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-
// _-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-
// _-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-

void setup() {

  lcd.init();
  lcd.clear();
  lcd.backlight();

  // ___________________________________________________________________________________________________________
  // CRIA OS CARACTERES PARA MOSTRAR NO DISPLAY
  // ___________________________________________________________________________________________________________

  lcd.createChar(3, grau); // criar caracter de grau

  // ___________________________________________________________________________________________________________
  // LIMPA O DISPLAY
  // ___________________________________________________________________________________________________________

  lcd.clear(); // limpa display
  lcd.setCursor(0, 0); // move o cursor para (0, 0)
  lcd.print("Ligando..."); // print message
  lcd.setCursor(0, 1);
  lcd.print("Aguarde!");
  lcd.setCursor(9, 1);
  lcd.write((byte)0);  // printa o caracter customizado 0

  // ___________________________________________________________________________________________________________
  // ATIVADO OS PINOS
  // ___________________________________________________________________________________________________________

  pinMode(vccPin, OUTPUT);
  pinMode(gndPin, OUTPUT);
  pinMode(vccThermo_heater, OUTPUT);
  pinMode(vccThermo_reactor, OUTPUT);
  pinMode(vccThermo_exit, OUTPUT);
  pinMode(vccThermo_outside, OUTPUT);
  pinMode(vccThermo_outside2, OUTPUT);
  pinMode(vccSDcard, OUTPUT);

  lcd.setCursor(0, 1);
  lcd.print("Pin Configurados");
  delay(2500); //delay(2000)

  // ___________________________________________________________________________________________________________
  // INICIALIZA A COMUNICAÇÃO SPI
  // ___________________________________________________________________________________________________________

  pinMode(chipSelect, OUTPUT);
  digitalWrite(chipSelect, HIGH); // Desativa o cartão SD

  Serial.begin(9600);

  // ___________________________________________________________________________________________________________
  // DETECTA SE O CARTÃO SD ESTÁ CONECTADO (DISPLAY + CONDIÇÃO)
  // ___________________________________________________________________________________________________________

  if (!SD.begin(chipSelect)) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Falha de leitura");
    lcd.setCursor(3, 1);
    lcd.print("cartao SD");
    delay(2500); //delay(50);

    lcd.clear();
    lcd.setCursor(3, 0);
    lcd.print("ADICIONE O");
    lcd.setCursor(3, 1);
    lcd.print("cartao SD");
    delay(500000000); //28800000
    return;
  }

  // ___________________________________________________________________________________________________________
  // INICIA O CARTÃO SD (DISPLAY)
  // ___________________________________________________________________________________________________________

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Cartao iniciado");
  lcd.setCursor(2, 1);
  lcd.print("com sucesso");
  delay(2500); //delay(2000);  

  // ___________________________________________________________________________________________________________
  // CRIA O ARQUIVO SE NÃO EXISTIR
  // ___________________________________________________________________________________________________________

  if (!SD.exists(fileName)) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("File nao existe!");
    lcd.setCursor(3, 1);
    lcd.print("Criando...");
    delay(1000); //delay(2000);

    generateFileName(); // Gera um novo arquivo
    writeHeader(); // Adicione esta função para escrever um cabeçalho, se necessário

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Arquivo criado!");
    lcd.setCursor(0, 1);
    lcd.print(fileName);    
    
    delay(3000); //delay(15000);
  }

  // ___________________________________________________________________________________________________________
  // INICIALIZAÇÃO
  // ___________________________________________________________________________________________________________
  
  //tempoInicial = millis() / 1000; // incio da contagem

  lcd.clear();
  lcd.setCursor(0, 0); // move o cursor para (0, 0)
  lcd.print("Dados de Entrada"); // print message
  lcd.setCursor(0, 1);
  lcd.print("Temperatura: ");
}

// _-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-
// _-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-
// _-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-
// _-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-

void loop() {  

  // ___________________________________________________________________________________________________________
  // ENTRADA DE DADOS DE TEMPERATURA, TEMPO E TAXA
  // ___________________________________________________________________________________________________________
  char keyPressed = teclado_personalizado.getKey(); // Atribui a variável a leitura do teclado

  if (keyPressed != NO_KEY && start_exp == 0) { // Se alguma tecla foi pressionada
    if (isdigit(keyPressed) || keyPressed == '.') { // Se a tecla pressionada for um dígito ou ponto
      if (!Key_temperatura) {
        inputBuffer1 += keyPressed; // Adiciona o dígito ou ponto ao buffer do primeiro valor
        lcd.setCursor(12, 1);
        lcd.print(inputBuffer1);
      } else if (!Key_rate) {
        inputBuffer2 += keyPressed; // Adiciona o dígito ou ponto ao buffer do segundo valor
        lcd.setCursor(6, 1);
        lcd.print(inputBuffer2);
      } else if (!Key_time) {
        inputBuffer3 += keyPressed; // Adiciona o dígito ou ponto ao buffer do segundo valor
        lcd.setCursor(5, 1);
        lcd.print(inputBuffer3);
      }
    } else if (keyPressed == 'A') { // Se a tecla pressionada for o sinal de igual
      if (!Key_temperatura) {
        Key_temperatura = true; // Indica que o primeiro valor foi digitado
        lcd.clear();
        lcd.setCursor(0, 0); // move o cursor para (0, 0)
        lcd.print("Dados de Entrada"); // print message
        lcd.setCursor(0, 1);
        lcd.print("Tempo: ");
      } else if (Key_temperatura && !Key_rate && !Key_time) {
        Key_rate = true; // Indica que o segundo valor foi digitado
        lcd.clear();
        lcd.setCursor(0, 0); // move o cursor para (0, 0)
        lcd.print("Dados de Entrada"); // print message
        lcd.setCursor(0, 1);
        lcd.print("Taxa: ");
      } else if (Key_temperatura && Key_rate && !Key_time) {
        Key_time = true; // Indica que o segundo valor foi digitado
      }
    }
  }

  if (Key_temperatura && Key_rate && Key_time && start_exp == 0) { // Se os dois valores foram digitados
    temperaturaMaximaReator = inputBuffer1.toFloat(); // Converte o primeiro valor do buffer para float
    tempo_experimental = inputBuffer2.toFloat(); // Converte o segundo valor do buffer para float
    taxa_aquecimento = inputBuffer3.toFloat(); // Converte o segundo valor do buffer para float

    //Aparecer dados adicionados no display

    lcd.clear();
    lcd.setCursor(0, 0); // move o cursor para (0, 0)
    lcd.print("Valide condicoes"); // print message
    lcd.setCursor(0, 1);
    lcd.print("Escolhidas");
    delay(2500); //delay(2000);

    lcd.clear();
    lcd.setCursor(0, 0); // move o cursor para (0, 0)
    lcd.print("T:"); // print message
    lcd.setCursor(2, 0); // move o cursor para (0, 0)
    lcd.print(inputBuffer1);
    lcd.setCursor(5, 0); // move o cursor para (0, 0)
    lcd.write((byte)3);
    lcd.setCursor(6, 0); // move o cursor para (0, 0)
    lcd.print("C");

    lcd.setCursor(8, 0); // move o cursor para (0, 0)
    lcd.print("t:"); // print message
    lcd.setCursor(10, 0); // move o cursor para (0, 0)
    lcd.print(inputBuffer2);
    lcd.setCursor(13, 0); // move o cursor para (0, 0)
    lcd.print("min");

    lcd.setCursor(0, 1); // move o cursor para (0, 0)
    lcd.print("r:"); // print message
    lcd.setCursor(2, 1); // move o cursor para (0, 0)
    lcd.print(inputBuffer3);
    lcd.setCursor(4, 1); // move o cursor para (0, 0)
    lcd.write((byte)3);
    lcd.setCursor(5, 1); // move o cursor para (0, 0)
    lcd.print("C/min");

    delay(5000);

    tempoInicial = millis() / 1000;

    start_exp = 1;

    temperaturaMaximaForno = temperaturaMaximaReator + 20;  // Temperatura máxima forno (em graus Celsius)
    temperaturaMinima = temperaturaMaximaReator + 10;  // Calcula a temperatura mínima

    lcd.clear();
    lcd.setCursor(0, 0); // move o cursor para (0, 0)
    lcd.print("Iniciando..."); // print message

    delay(2500);

    lcd.clear();
    
  }

  if (start_exp != 0) {
  // ___________________________________________________________________________________________________________
  // ABRE O ARQUIVO NO MODE DE ESCRITA
  // ___________________________________________________________________________________________________________
    File dataFile = SD.open(fileName, FILE_WRITE);

    // ___________________________________________________________________________________________________________
    // ACIONA AS TENSÕES PARA O AQUECEDOR E REATOR 
    // ___________________________________________________________________________________________________________
    digitalWrite(vccThermo_reactor, HIGH);
    digitalWrite(vccThermo_heater, HIGH);
    digitalWrite(vccThermo_exit, HIGH);
    digitalWrite(vccThermo_outside, HIGH);
    digitalWrite(vccThermo_outside2, HIGH);
    digitalWrite(vccSDcard, HIGH);
    delay(300);

    unsigned long tempoAtual = millis() / 1000;
    segundosPassados = tempoAtual - tempoInicial;

    // ___________________________________________________________________________________________________________
    // COLETA OS DADOS DE TEMPERATURA DO FORNO E DO REATOR
    // ___________________________________________________________________________________________________________
    float temp_forno_1 = thermo1.readCelsius();
    delay(100);
    float temp_forno_2 = thermo2.readCelsius();
    delay(100);
    float temp_out = thermo3.readCelsius();
    delay(100);
    float temp_reator = thermo4.readCelsius();
    delay(100);
    float temp_out2 = thermo5.readCelsius();
    delay(100);

    float temp_forno = ( temp_forno_1 + temp_forno_2 ) / 2;
    float temp_media = ( temp_forno + temp_reator ) / 2;

    // ___________________________________________________________________________________________________________
    // CALCULA O TEMPO PARA PODER MOSTRAR: RESISTÊNCIA (LIGADO/DESLIGADO) + TEMPERATURA ESCOLHIDA (DISPLAY)
    // ___________________________________________________________________________________________________________
    if (((segundosPassados) - ultimoTempoExibicao >= 7) && ((segundosPassados) - ultimoTempoExibicao < 10)) {
      lcd.setCursor(0, 1);
      lcd.print("                ");
      
      if (minutos_exp <= tempo_experimental) {    
        lcd.setCursor(2, 1);
        lcd.print(minutos_exp);
        lcd.setCursor(5, 1);
        lcd.print("min");
      }
      else {
        lcd.setCursor(3, 1);
        lcd.print("FIM!");
      }

      // ___________________________________________________________________________________________________________
      // CONVERTE SEGUNDOS PARA HORAS, MINUTOS E SEGUNDOS
      // ___________________________________________________________________________________________________________
      unsigned long horas = segundosPassados / 3600;
      unsigned long minutos = (segundosPassados % 3600) / 60;
      unsigned long segundos = segundosPassados % 60;

      // ___________________________________________________________________________________________________________
      // FORMATAÇÃO DAS PARTES INDIVIDUAIS DO TEMPO (DISPLAY)
      // ___________________________________________________________________________________________________________
      String formattedTime = "";
      if (horas < 10) {
        formattedTime += "0";
      }
      formattedTime += String(horas) + ":";

      if (minutos < 10) {
        formattedTime += "0";
      }
      formattedTime += String(minutos) + ":";

      if (segundos < 10) {
        formattedTime += "0";
      }
      formattedTime += String(segundos);

      // Exibe o tempo decorrido no formato hh:mm:ss no LCD
      lcd.setCursor(0, 0);
      lcd.print(formattedTime);

      lcd.setCursor(8, 0);
      lcd.print(" Tf:   C");
      lcd.setCursor(12, 0);
      lcd.print(int(temp_forno));

      lcd.setCursor(8, 1);
      lcd.print("  T:   C");
      lcd.setCursor(12, 1);
      lcd.print(int(temperaturaMaximaReator));

    }
    else if (((segundosPassados) - ultimoTempoExibicao < 8)) {
      // ___________________________________________________________________________________________________________
      // INFORMAÇÕES DE TEMPERATURA NO DISPLAY (DISPLAY)
      // ___________________________________________________________________________________________________________
      lcd.setCursor(0, 0);
      lcd.print("T1:   C ");
      lcd.setCursor(3, 0);
      lcd.print(temp_forno_1, 0);

      lcd.setCursor(8, 0);
      lcd.print(" T2:   C");
      lcd.setCursor(12, 0);
      lcd.print(temp_forno_2, 0);

      lcd.setCursor(0, 1);
      lcd.print("Tr:   C ");
      lcd.setCursor(3, 1);
      lcd.print(temp_reator, 0);

      lcd.setCursor(8, 1);
      lcd.print(" Tm:   C");
      lcd.setCursor(12, 1);
      lcd.print(temp_media, 0);
    }

    else if (((segundosPassados) - ultimoTempoExibicao >= 10)) {
      ultimoTempoExibicao = segundosPassados;
    }

    // ___________________________________________________________________________________________________________
    // INFORMAÇÕES DE TNSÃO NO RELÉ
    // ___________________________________________________________________________________________________________

    if (digitalRead(vccPin) == HIGH) {
      energy_count = 1;
      }
      else {
        energy_count = 0;
      }

      // ___________________________________________________________________________________________________________
      // SALVAR OS DADOS NO ARQUIVO
      // ___________________________________________________________________________________________________________

      // Verifica se o arquivo foi aberto com sucesso
    if (dataFile) {

      // Escreve os dados no arquivo
      dataFile.print(temperaturaMaximaReator);
      dataFile.print("\t");
      dataFile.print(tempo_experimental);
      dataFile.print("\t");
      dataFile.print(taxa_aquecimento);
      dataFile.print("\t");
      dataFile.print(segundosPassados);
      dataFile.print("\t");
      dataFile.print(segundos_exp);
      dataFile.print("\t");
      dataFile.print(temp_forno_1);
      dataFile.print("\t");
      dataFile.print(temp_forno_2);
      dataFile.print("\t");
      dataFile.print(temp_forno);
      dataFile.print("\t");
      dataFile.print(temp_reator);
      dataFile.print("\t");
      dataFile.print(temp_media);
      dataFile.print("\t");
      dataFile.print(temp_out);
      dataFile.print("\t");
      dataFile.print(temp_out2);
      dataFile.print("\t");
      dataFile.print(energy_count);
      dataFile.print("\t");
      dataFile.println();

      // Fecha o arquivo
      dataFile.close();
    }
    // ___________________________________________________________________________________________________________
    // TAXA DE AQUECIMENTO
    // ___________________________________________________________________________________________________________

    float minutosPassados = segundosPassados / 60;
    float Temperatura_Set = 10 + taxa_aquecimento * segundosPassados / 60;

    // ___________________________________________________________________________________________________________
    // CONDIÇÕES PARA ACIONAMENTO DE PINOS DO RELÉ
    // ___________________________________________________________________________________________________________

    if (temp_reator <= temperaturaMaximaReator) {
  
      // Liga ou desliga os relés - COM RELAÇÃO A TEMPERATURA DO FORNO
      if ((temp_forno <= Temperatura_Set) && (key_taxa == 0) && (Temperatura_Set < temperaturaMaximaReator) && (minutos_exp <= tempo_experimental)) {
        digitalWrite(vccPin, HIGH);
        digitalWrite(gndPin, LOW);

      }
      else if ((temp_forno > Temperatura_Set) && (key_taxa == 0) && (minutos_exp <= tempo_experimental)) {
        digitalWrite(vccPin, LOW);
        digitalWrite(gndPin, HIGH);

      }
      else if  ((Temperatura_Set >= temperaturaMaximaReator) && (key_taxa == 0) && (minutos_exp <= tempo_experimental)) {
        key_taxa += 1;

      }
      else if ((temp_forno <= temperaturaMinima) && (key_taxa == 1) && (minutos_exp <= tempo_experimental)) {
        digitalWrite(vccPin, HIGH);
        digitalWrite(gndPin, LOW);
      } 
      else if ((temp_forno > temperaturaMaximaForno) && (minutos_exp <= tempo_experimental)) {
        digitalWrite(vccPin, LOW);
        digitalWrite(gndPin, HIGH);
      }
      else if ((minutos_exp > tempo_experimental)) {
        digitalWrite(vccPin, LOW);
        digitalWrite(gndPin, HIGH);
      }
    }
    else {
        digitalWrite(vccPin, LOW);
        digitalWrite(gndPin, HIGH);
    }

    // ___________________________________________________________________________________________________________
    // CALCULA O TEMPO DE REAÇÃO - APÓS ATINGIR A TEMPERATURA SETADA
    // ___________________________________________________________________________________________________________

    if ((key_tempo == 0) && (temp_media >= temperaturaMaximaReator) && (segundosPassados) > 10) {
      key_tempo += 1;
      tempoInicial_Contador = segundosPassados;

    }
    else if (key_tempo == 1) {
      segundos_exp = (segundosPassados - tempoInicial_Contador);
      minutos_exp = segundos_exp / 60; 
    }

      delay(500);
  }
}

// _-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-
// _-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-
// _-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-
// _-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-

// ___________________________________________________________________________________________________________
// GERAR NOME DO ARQUIVO (CASO REPETIDO)
// ___________________________________________________________________________________________________________

void writeHeader() {
  // Função para escrever um cabeçalho no arquivo, se necessário
  File dataFile = SD.open(fileName, FILE_WRITE);
  if (dataFile) {
    // Escreve o cabeçalho no arquivo
    dataFile.print("Temperatura exp (C)");
    dataFile.print("\t");
    dataFile.print("Tempo exp (min)");
    dataFile.print("\t");
    dataFile.print("Taxa (C/min)");
    dataFile.print("\t");
    dataFile.print("Tempo (s)");
    dataFile.print("\t");
    dataFile.print("Tempo de Isoterma (s)");
    dataFile.print("\t");
    dataFile.print("Tforno 1 (C)");
    dataFile.print("\t");
    dataFile.print("Tforno 2 (C)");
    dataFile.print("\t");
    dataFile.print("Tforno medio (C)");
    dataFile.print("\t");
    dataFile.print("Treator (C)");
    dataFile.print("\t");
    dataFile.print("Tmedia");
    dataFile.print("\t");
    dataFile.print("Tforno out (C)");
    dataFile.print("\t");
    dataFile.print("Tgas out (C)");
    dataFile.print("\t");
    dataFile.println("Rele (ON/OFF)");

    dataFile.close();
    //Serial.println("Cabeçalho do arquivo criado");
  } else {
    //Serial.println("Erro ao abrir o arquivo para escrever o cabeçalho");
  }
}

void generateFileName() {
  // Gera um novo nome de arquivo único com um número incremental
  for (int i = 1; i <= 999; i++) {
    sprintf(fileName, "%s%d%s", baseFileName, i, fileExtension);

    if (!SD.exists(fileName)) {
      // Nome de arquivo único encontrado
      //Serial.print("Novo nome do arquivo: ");
      //Serial.println(fileName);
      return;
    }
  }

  //Serial.println("Erro: Não foi possível gerar um novo nome de arquivo único");
}
