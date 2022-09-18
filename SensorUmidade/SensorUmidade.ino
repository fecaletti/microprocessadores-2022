/* Universidade Estadual do Rio Grande do Sul */
/* Curso de Engenharia de Controle e Automação */
/* Aluno: Fernando A. C. de Barros */
/* Disciplina: Microprocessadores */
/* Atividade: Armazenar leituras em memória EEPROM para recuperação posterior. */
/* Microcontrolador utilizado: Arduino Nano. */

#include <EEPROM.h>

#define MEDICOES_POR_REGISTRO 10
#define BYTES_POR_REGISTRO 13

//#define DEBUG_MODE

void(* resetFunc) (void) = 0;

typedef struct RegistroMedida
{
  float Media;
  int Maximo;
  int Minimo;
  unsigned char Tipo;
};

typedef union ByteConverter {
  float fData;
  int iData;
  unsigned char bytes[4];
};

int MedicoesTemperatura[10] = {0};
int MedicoesUmidade[10] = {0};
int ContadorMedicoes = 0;
int ContadorRegistros = 0;

void setup() 
{
  Serial.begin(9600);
  ContadorRegistros = leContadorRegistros();
}

void loop() 
{
  Serial.println("Sensor de Umidade. Opcoes: [p - Print historico | c - Limpa memória | r - Print valores na EEPROM]");
  Serial.print("Contador registros: ");
  Serial.println(ContadorRegistros);
  if(Serial.available())
  {
    char command = Serial.read();
    Serial.flush();
    switch (command)
    {
      case 'p':
        imprimeHistorico();
        break;
        
      case 'c':
        Serial.println("Zerando contador de registros.");
        zeraContadorRegistros();
        break;

      case 'r':
        imprimeMemoriaRaw();
        break;
      
      default:
        Serial.println("Wrong command...");
        break;
    }
  }

  MedicoesUmidade[ContadorMedicoes] = leituraSensorUmidade();
  MedicoesTemperatura[ContadorMedicoes++] = leituraSensorTemperatura();
  
  if(ContadorMedicoes >= MEDICOES_POR_REGISTRO)
  {
    registraMedicoes();
    ContadorMedicoes = 0;
  }
  
  delay(1000);
}

void registraMedicoes()
{
  RegistroMedida medidaTemperatura;
  calculaRegistro(MedicoesTemperatura, &medidaTemperatura);
  medidaTemperatura.Tipo = 0;
  imprimeRegistro(medidaTemperatura, 0, ContadorRegistros);
  RegistroMedida medidaUmidade;
  calculaRegistro(MedicoesUmidade, &medidaUmidade);
  medidaUmidade.Tipo = 1;
  imprimeRegistro(medidaUmidade, 1, ContadorRegistros + 1);

  registraMedicao(medidaTemperatura);
  registraMedicao(medidaUmidade);
}

/* Montagem de Registro */
/* Byte index inicial - Byte index final - Valor */
/* 0 - 3 - Media  */
/* 4 - 7 - Maximo */
/* 8 - 7 - Minimo */
void registraMedicao(RegistroMedida medida)
{
  unsigned char registro[BYTES_POR_REGISTRO] = {0};
  ByteConverter converter;

  converter.fData = medida.Media;
  registro[0] = converter.bytes[0];
  registro[1] = converter.bytes[1];
  registro[2] = converter.bytes[2];
  registro[3] = converter.bytes[3];

  converter.iData = medida.Maximo;
  registro[4] = converter.bytes[0];
  registro[5] = converter.bytes[1];
  registro[6] = converter.bytes[2];
  registro[7] = converter.bytes[3];

  converter.iData = medida.Minimo;
  registro[8] = converter.bytes[0];
  registro[9] = converter.bytes[1];
  registro[10] = converter.bytes[2];
  registro[11] = converter.bytes[3];

  registro[12] = medida.Tipo;

  if(ContadorRegistros >= (int)(254 / BYTES_POR_REGISTRO))
    ContadorRegistros = 0;

  unsigned char startingAddress = calculaEnderecoAtualEeprom();
  for (unsigned char i = 0; i < BYTES_POR_REGISTRO; i++)
  {
    #ifdef DEBUG_MODE
    Serial.print("Writing on EEPROM - ");
    Serial.print(startingAddress);
    Serial.print(" - ");
    Serial.println(registro[i]);
    #endif
    EEPROM.write(startingAddress + i, registro[i]);
  }
  ContadorRegistros++;
  salvaContadorRegistros(ContadorRegistros);
}

void calculaRegistro(int* dados, RegistroMedida* output)
{
  int maior = 0;
  int menor = 10000;
  unsigned long long soma = 0;
  for(int i = 0; i < MEDICOES_POR_REGISTRO; i++)
  {
    if(*(dados + i) < menor)
      menor = *(dados + i);
    if(*(dados + i) > maior)
      maior = *(dados + i);

    soma += *(dados + i);
  }
  float media = ((double) soma) / ((double) MEDICOES_POR_REGISTRO);

  output->Media = media;
  output->Maximo = maior;
  output->Minimo = menor;
}

unsigned char calculaEnderecoAtualEeprom()
{
  return (ContadorRegistros * BYTES_POR_REGISTRO) + 1; //Qtdd reg. x Tam. reg. x 2 regs por medicao + endereco do contador.
}

void imprimeMemoriaRaw()
{
  Serial.println("----------------- Lendo EEPROM -------------");
  for (unsigned char i = 0; i < 255; i++)
  {
    Serial.print("\tI: ");
    Serial.print(i);
    Serial.print("\tV: ");
    Serial.println(EEPROM.read(i));
  }
  Serial.println("--------------------------------------------\n");
}

void imprimeHistorico()
{
  Serial.println("Historico de medicoes: ");
  for (unsigned char i = 0; i < ContadorRegistros; i++)
  {
    RegistroMedida registro = leRegistro((i * BYTES_POR_REGISTRO) + 1);
    imprimeRegistro(registro, registro.Tipo, i);
  }
}

void imprimeRegistro(RegistroMedida registro, unsigned char tipo, int contador)
{
  if(tipo == 0)
    Serial.print("--- Registro Temperatura #");
  if(tipo == 1)
    Serial.print("--- Registro Umidade #");
  Serial.print(contador);
  Serial.println(" ---");
  Serial.print("-- Media: ");
  Serial.print(registro.Media);
  Serial.print("\t Maximo: ");
  Serial.print(registro.Maximo);
  Serial.print("\t Minimo: ");
  Serial.print(registro.Minimo);
  Serial.println("\n-------------------");
}

RegistroMedida leRegistro(unsigned char endereco)
{
  ByteConverter converter;
  RegistroMedida registro;

  converter.bytes[0] = EEPROM.read(endereco);
  converter.bytes[1] = EEPROM.read(endereco + 1);
  converter.bytes[2] = EEPROM.read(endereco + 2);
  converter.bytes[3] = EEPROM.read(endereco + 3);
  registro.Media = converter.fData;

  converter.bytes[0] = EEPROM.read(endereco + 4);
  converter.bytes[1] = EEPROM.read(endereco + 5);
  converter.bytes[2] = EEPROM.read(endereco + 6);
  converter.bytes[3] = EEPROM.read(endereco + 7);
  registro.Maximo = converter.iData;

  converter.bytes[0] = EEPROM.read(endereco + 8);
  converter.bytes[1] = EEPROM.read(endereco + 9);
  converter.bytes[2] = EEPROM.read(endereco + 10);
  converter.bytes[3] = EEPROM.read(endereco + 11);
  registro.Minimo = converter.iData;

  registro.Tipo = EEPROM.read(endereco + 12);

  return registro;
}

void zeraContadorRegistros()
{
  salvaContadorRegistros(0);
  resetFunc();
}

void salvaContadorRegistros(int contadorAtual)
{
  return EEPROM.write(0, contadorAtual);
}

int leContadorRegistros()
{
  return EEPROM.read(0);
}

int leituraSensorUmidade()
{
  return 7;
}

int leituraSensorTemperatura()
{
  return 5;
}
