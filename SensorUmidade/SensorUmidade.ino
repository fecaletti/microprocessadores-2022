/* Universidade Estadual do Rio Grande do Sul */
/* Curso de Engenharia de Controle e Automação */
/* Aluno: Fernando A. C. de Barros */
/* Disciplina: Microprocessadores */
/* Atividade: Armazenar leituras em memória EEPROM para recuperação posterior. */
/* Microcontrolador utilizado: Arduino Nano. */

#include <EEPROM.h>

#define MEDICOES_POR_REGISTRO 10
#define BYTES_POR_REGISTRO 13
#define INTERVALO_PROCESSO_MS 3500
#define PINO_AD 10
//#define DEBUG_MODE

void(* resetFunc) (void) = 0;

typedef union ByteConverter {
  float fData;
  int iData;
  unsigned char bytes[4];
};

unsigned int IntervaloSelecionado = 0;
unsigned int MedicaoAD = 0;

void setup() 
{
  Serial.begin(115200);
  pinMode(PINO_AD, INPUT);  
  SetupTimer1(INTERVALO_PROCESSO_MS);
}

void loop() 
{
  Serial.println("Medidor analogico. Opcoes: [t - Alterar taxa de leitura AD]");
  if(Serial.available())
  {
    char command = Serial.read();
    Serial.flush();
    switch (command)
    {
      case 't':
        RecebeValorTaxaAD();
        break;
      
      default:
        Serial.println("Comando invalido...");
        break;
    }
  }
  delay(5000);
}

unsigned int CalculateCounterByInterval(unsigned int intervalMillis)
{
  return 65536 - (intervalMillis * 16000) / 1024;
}

void SetTimer1Counter()
{
  TCNT1 = CalculateCounterByInterval(IntervaloSelecionado);
}

void SetupTimer1(unsigned int intervaloMillis) //Max: 4194ms
{
  IntervaloSelecionado = intervaloMillis;

  TCCR1A = 0;
  TCCR1B = 0;
  TCCR1B |= (1 << CS10) | (1 << CS12); //Prescaler: 1024 - Max value
  //SetTimer1Counter();
  TCNT1 = 0;
  TIMSK1 |= (1 << OCR1A); //Habilita interrupção timer 1
  
}

ISR(TIMER1_OVF_vect) //Registra callback para interrupção do timer1
{
  SetTimer1Counter();
  ProcessoAD();  
}

void ProcessoAD()
{
  Serial.println("Processo AD - Iniciando");
  MedicaoAD = analogRead(PINO_AD);
  Serial.print("Valor obtido: ");
  Serial.println(MedicaoAD);
  Serial.println("Processo AD - Fim");
}

void RecebeValorTaxaAD()
{
  int setandoValor = 1;
  Serial.print("Insira o valor desejado em ms (Max. 4000): ");
  String output = "";

  while(Serial.available())
  {
    output += Serial.read();
  }
  unsigned int result = output.toInt();
  if((result > 0) && (result <= 4000))
    SetupTimer1(result);
}
