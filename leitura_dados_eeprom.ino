#include <EEPROM.h>
#include <math.h>

#define BYTES_REGISTRO 40
#define MAX_REGISTROS 25

String classeQualidade(int q)
{
  if(q > 80) return "BOA";
  if(q > 50) return "MEDIA";

  return "RUIM";
}

void setup()
{
  Serial.begin(9600);

  delay(2000);

  // CABEÇALHO CSV
  Serial.println(
    "Registro,"
    "Temperatura,"
    "Qualidade,"
    "Classe,"
    "TurbidezAtual,"
    "TurbidezMin,"
    "TurbidezMax,"
    "TurbidezMedia,"
    "TDSAtual,"
    "TDSMin,"
    "TDSMax,"
    "TDSMedia"
  );

  for(int i = 0; i < MAX_REGISTROS; i++)
  {
    int endereco = i * BYTES_REGISTRO;

    float temperatura;
    float qualidade;

    float turbAtual;
    float turbMin;
    float turbMax;
    float turbMedia;

    float tdsAtual;
    float tdsMin;
    float tdsMax;
    float tdsMedia;

    EEPROM.get(endereco + 0, temperatura);
    EEPROM.get(endereco + 4, qualidade);

    EEPROM.get(endereco + 8, turbAtual);
    EEPROM.get(endereco + 12, turbMin);
    EEPROM.get(endereco + 16, turbMax);
    EEPROM.get(endereco + 20, turbMedia);

    EEPROM.get(endereco + 24, tdsAtual);
    EEPROM.get(endereco + 28, tdsMin);
    EEPROM.get(endereco + 32, tdsMax);
    EEPROM.get(endereco + 36, tdsMedia);

    // IGNORA VAZIOS
    if(
      temperatura == 0 &&
      qualidade == 0 &&
      turbAtual == 0 &&
      tdsAtual == 0
    )
    {
      continue;
    }

    // IGNORA INVALIDOS
    if(
      isnan(temperatura) ||
      isnan(qualidade) ||
      isnan(turbAtual) ||
      isnan(tdsAtual)
    )
    {
      continue;
    }

    int q = (int)qualidade;

    // LINHA CSV
    Serial.print(i + 1);
    Serial.print(",");

    Serial.print(temperatura,1);
    Serial.print(",");

    Serial.print(q);
    Serial.print(",");

    Serial.print(classeQualidade(q));
    Serial.print(",");

    Serial.print(turbAtual,1);
    Serial.print(",");

    Serial.print(turbMin,1);
    Serial.print(",");

    Serial.print(turbMax,1);
    Serial.print(",");

    Serial.print(turbMedia,1);
    Serial.print(",");

    Serial.print(tdsAtual,1);
    Serial.print(",");

    Serial.print(tdsMin,1);
    Serial.print(",");

    Serial.print(tdsMax,1);
    Serial.print(",");

    Serial.println(tdsMedia,1);
  }

  Serial.println("FIM");
}

void loop()
{
}