#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include <math.h>

#define BTN 2

#define NTC_PIN A0
#define TDS_PIN A1
#define TURB_PIN A2

#define FATOR_CALIBRACAO 2.0

// 10 floats = 40 bytes
#define BYTES_REGISTRO 40

// 24 registros completos
#define MAX_REGISTROS 24

float V_LIMPA = 4.75;
float V_TURVA = 3.40;

LiquidCrystal_I2C lcd(0x27,16,2);

bool ativo = false;
bool finalizado = false;
bool aguardandoSalvar = false;

bool contagemInicial = false;

unsigned long tempoInicioContagem = 0;

unsigned long tTela = 0;
unsigned long tMed = 0;
unsigned long tGravacao = 0;
unsigned long tFinal = 0;

unsigned long tempoBotao = 0;

bool pressionando = false;

int tela = 0;
int telaFinal = 0;

int ultimoSegundo = -1;

float tempF = 25;
float tdsF = 0;
float turbF = 0;

float hist[10];
int idxHist = 0;

float histTurb[10];
int idxTurb = 0;

float tdsMin = 9999;
float tdsMax = 0;

float turbMin = 9999;
float turbMax = 0;

int ponteiroEEPROM = 0;

void printLinha(int c,int l,String t)
{
  lcd.setCursor(c,l);

  String texto = t;

  while(texto.length() < 16)
    texto += " ";

  lcd.print(texto);
}

float ema(float ant,float novo,float k)
{
  return ant*(1-k)+novo*k;
}

void limparEEPROM()
{
  for(int i=0;i<EEPROM.length();i++)
    EEPROM.write(i,0);
}

int qualidade(float tds,float turb,float temp)
{
  int score = 100;

  if(tds > 150)
    score -= (tds - 150) * 0.1;

  if(turb > 20)
    score -= (turb - 20) * 0.7;

  if(temp > 30 || temp < 5)
    score -= 10;

  if(score < 0)
    score = 0;

  return score;
}

String classe(int q)
{
  if(q > 80) return "BOA";
  if(q > 50) return "MEDIA";

  return "RUIM";
}

float lerTemp()
{
  int adc = analogRead(NTC_PIN);

  if(adc == 0)
    adc = 1;

  float r = 10000.0 * (1023.0 / adc - 1.0);

  float s = log(r / 10000.0) / 3950.0 + 1.0 / (25 + 273.15);

  return 1.0 / s - 273.15;
}

float lerTDS(float t)
{
  float soma = 0;

  for(int i=0;i<8;i++)
    soma += analogRead(TDS_PIN);

  float v = (soma / 8.0) * (5.0 / 1023.0);

  float comp = v / (1 + 0.02 * (t - 25));

  float val =
    (133.42 * pow(comp,3)
    -255.86 * pow(comp,2)
    +857.39 * comp) * 0.5;

  return val * FATOR_CALIBRACAO;
}

float lerTurb()
{
  float soma = 0;

  for(int i=0;i<15;i++)
    soma += analogRead(TURB_PIN);

  float v = (soma / 15.0) * (5.0 / 1023.0);

  float p =
    (V_LIMPA - v) /
    (V_LIMPA - V_TURVA) * 100.0;

  if(p < 0) p = 0;
  if(p > 100) p = 100;

  return p;
}

void barra(int p)
{
  int b = map(p,0,100,0,16);

  lcd.setCursor(0,1);

  for(int i=0;i<b;i++)
    lcd.write(byte(255));

  for(int i=b;i<16;i++)
    lcd.print(" ");
}

void gravarEEPROM()
{
  int endereco = ponteiroEEPROM * BYTES_REGISTRO;

  int q = qualidade(tdsF, turbF, tempF);

  float mediaTDS = 0;

  for(int i=0;i<10;i++)
    mediaTDS += hist[i];

  mediaTDS /= 10.0;

  float mediaTurb = 0;

  for(int i=0;i<10;i++)
    mediaTurb += histTurb[i];

  mediaTurb /= 10.0;

  EEPROM.put(endereco + 0, tempF);
  EEPROM.put(endereco + 4, (float)q);

  EEPROM.put(endereco + 8, turbF);
  EEPROM.put(endereco + 12, turbMin);
  EEPROM.put(endereco + 16, turbMax);
  EEPROM.put(endereco + 20, mediaTurb);

  EEPROM.put(endereco + 24, tdsF);
  EEPROM.put(endereco + 28, tdsMin);
  EEPROM.put(endereco + 32, tdsMax);
  EEPROM.put(endereco + 36, mediaTDS);

  Serial.println();
  Serial.println("========== DADOS SALVOS ==========");

  Serial.print("Registro: ");
  Serial.println(ponteiroEEPROM + 1);

  Serial.print("Temperatura Atual: ");
  Serial.print(tempF,1);
  Serial.println(" C");

  Serial.print("Qualidade Atual: ");
  Serial.print(q);
  Serial.print(" - ");
  Serial.println(classe(q));

  Serial.print("Turbidez Atual: ");
  Serial.print(turbF,0);
  Serial.println(" %");

  Serial.print("Turbidez Min: ");
  Serial.print(turbMin,0);
  Serial.println(" %");

  Serial.print("Turbidez Max: ");
  Serial.print(turbMax,0);
  Serial.println(" %");

  Serial.print("Turbidez Media: ");
  Serial.print(mediaTurb,0);
  Serial.println(" %");

  Serial.print("TDS Atual: ");
  Serial.print(tdsF,0);
  Serial.println(" ppm");

  Serial.print("TDS Min: ");
  Serial.print(tdsMin,0);
  Serial.println(" ppm");

  Serial.print("TDS Max: ");
  Serial.print(tdsMax,0);
  Serial.println(" ppm");

  Serial.print("TDS Media: ");
  Serial.print(mediaTDS,0);
  Serial.println(" ppm");

  Serial.println("==================================");
  Serial.println();

  ponteiroEEPROM++;

  if(ponteiroEEPROM >= MAX_REGISTROS)
    ponteiroEEPROM = MAX_REGISTROS;
}

void setup()
{
  pinMode(BTN,INPUT_PULLUP);

  Serial.begin(9600);

  Serial.println();
  Serial.println("======================================");
  Serial.println("     PROJETO MONITORAMENTO AGUA");
  Serial.println("======================================");
  Serial.println();

  Serial.println("Aluno: Andre Victor Rego");
  Serial.println("RGM: 47059893");

  Serial.println();
  Serial.println("Sensores Utilizados:");
  Serial.println("- Sensor TDS");
  Serial.println("- Sensor Turbidez");
  Serial.println("- Sensor Temperatura NTC");

  Serial.println();
  Serial.println("Sistema de monitoramento");
  Serial.println("e armazenamento em EEPROM");

  Serial.println();
  Serial.println("Inicializando sistema...");
  Serial.println("======================================");
  Serial.println();

  lcd.init();
  lcd.backlight();

  printLinha(0,0,"Projeto Monitor");
  printLinha(0,1,"Qualidade Agua");

  delay(3000);

  printLinha(0,0,"Andre V Rego");
  printLinha(0,1,"RGM 47059893");

  delay(3000);

  printLinha(0,0,"Aperte para");
  printLinha(0,1,"Iniciar");
}

void loop()
{
  // SISTEMA FINALIZADO
  if(finalizado)
  {
    if(millis() - tFinal > 2000)
    {
      tFinal = millis();

      telaFinal++;

      if(telaFinal > 1)
        telaFinal = 0;

      if(telaFinal == 0)
      {
        printLinha(0,0,"Dados salvos");
        printLinha(0,1,"EEPROM OK");
      }
      else
      {
        printLinha(0,0,"Leitura");
        printLinha(0,1,"Finalizada");
      }
    }

    return;
  }

  // INICIO SISTEMA
  if(!ativo)
  {
    // APERTOU BOTAO
    if(digitalRead(BTN)==LOW && !contagemInicial)
    {
      delay(300);

      printLinha(0,0,"Limpando");
      printLinha(0,1,"EEPROM");

      limparEEPROM();

      delay(1500);

      contagemInicial = true;

      tempoInicioContagem = millis();

      ultimoSegundo = -1;

      Serial.println();
      Serial.println("Contagem iniciada...");
    }

    // CONTAGEM
    if(contagemInicial)
    {
      int restante =
        30 - ((millis() - tempoInicioContagem) / 1000);

      if(restante < 0)
        restante = 0;

      // ATUALIZA APENAS 1 VEZ
      if(restante != ultimoSegundo)
      {
        ultimoSegundo = restante;

        printLinha(0,0,"Iniciando em");
        printLinha(0,1,String(restante)+" segundos");
      }

      // TERMINOU CONTAGEM
      if(millis() - tempoInicioContagem >= 30000)
      {
        ativo = true;

        contagemInicial = false;

        printLinha(0,0,"Sistema");
        printLinha(0,1,"Iniciado");

        Serial.println();
        Serial.println("================================");
        Serial.println(" SISTEMA INICIADO");
        Serial.println("================================");
        Serial.println();

        Serial.println("Coletando dados...");

        aguardandoSalvar = true;

        delay(1500);
      }
    }

    return;
  }

  // FINALIZAR PELO BOTAO
  if(digitalRead(BTN) == LOW)
  {
    if(!pressionando)
    {
      pressionando = true;
      tempoBotao = millis();
    }

    if(millis() - tempoBotao >= 2000)
    {
      finalizado = true;

      printLinha(0,0,"Dados salvos");
      printLinha(0,1,"Finalizado");

      Serial.println();
      Serial.println("Sistema finalizado.");
    }
  }
  else
  {
    pressionando = false;
  }

  // LEITURAS
  if(millis()-tMed > 2000)
  {
    tMed = millis();

    float t = lerTemp();
    float td = lerTDS(t);
    float tu = lerTurb();

    tempF = ema(tempF,t,0.2);
    tdsF = ema(tdsF,td,0.2);
    turbF = ema(turbF,tu,0.2);

    hist[idxHist] = tdsF;

    idxHist++;

    if(idxHist >= 10)
      idxHist = 0;

    histTurb[idxTurb] = turbF;

    idxTurb++;

    if(idxTurb >= 10)
      idxTurb = 0;

    if(tdsF < tdsMin)
      tdsMin = tdsF;

    if(tdsF > tdsMax)
      tdsMax = tdsF;

    if(turbF < turbMin)
      turbMin = turbF;

    if(turbF > turbMax)
      turbMax = turbF;
  }

  // GRAVACAO EEPROM
  if(millis()-tGravacao > 9000)
  {
    tGravacao = millis();

    printLinha(0,0,"Gravando dados");
    printLinha(0,1,"EEPROM");

    gravarEEPROM();

    delay(1500);

    // PARADA AUTOMATICA
    if(ponteiroEEPROM >= 24)
    {
      finalizado = true;

      aguardandoSalvar = true;

      Serial.println();
      Serial.println("================================");
      Serial.println(" LIMITE DE MEDICOES ATINGIDO");
      Serial.println(" SISTEMA FINALIZADO");
      Serial.println("================================");
      Serial.println();

      printLinha(0,0,"24 medicoes");
      printLinha(0,1,"Finalizado");

      return;
    }

    aguardandoSalvar = false;
  }

  // SERIAL UMA VEZ
  if(!aguardandoSalvar)
  {
    Serial.println("Coletando dados...");

    aguardandoSalvar = true;
  }

  // TROCA TELAS
  if(millis()-tTela > 2000)
  {
    tTela = millis();

    tela++;

    if(tela > 4)
      tela = 0;
  }

  int q = qualidade(tdsF,turbF,tempF);

  // LCD
  if(tela == 0)
  {
    printLinha(0,0,"Temp "+String(tempF,1)+" C");

    barra(map(tempF,0,40,0,100));
  }
  else if(tela == 1)
  {
    printLinha(0,0,"TDS "+String(tdsF,0)+" ppm");

    barra(map(tdsF,0,500,0,100));
  }
  else if(tela == 2)
  {
    printLinha(0,0,"Turb "+String(turbF,0)+" %");

    barra(turbF);
  }
  else if(tela == 3)
  {
    printLinha(0,0,"Qualidade "+classe(q));

    barra(q);
  }
  else
  {
    float media = 0;

    for(int i=0;i<10;i++)
      media += hist[i];

    media /= 10.0;

    printLinha(
      0,
      0,
      "Min:"+String(tdsMin,0)+
      " Max:"+String(tdsMax,0)
    );

    barra(map(media,0,500,0,100));
  }
}