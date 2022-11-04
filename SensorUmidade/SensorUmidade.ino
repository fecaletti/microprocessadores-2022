/* Universidade Estadual do Rio Grande do Sul */
/* Curso de Engenharia de Controle e Automação */
/* Aluno: Fernando A. C. de Barros */
/* Disciplina: Microprocessadores */
/* Atividade: Armazenar leituras em memória EEPROM para recuperação posterior. */
/* Microcontrolador utilizado: Arduino Nano. */

#include <EEPROM.h>

#define INTERVALO_PROCESSO_MS 3500
#define PINO_AD 10
#define LIMITE_SUP_TIMER 4000
#define LIMITE_INF_TIMER 0

#define SELECTED_PRESCALER 1024
#define BOARD_CLK 16000000
#define MAIN_LOOP_INTERVAL_MS 5000

void(* resetFunc) (void) = 0;

typedef union ByteConverter {
  float fData;
  int iData;
  unsigned char bytes[4];
};

unsigned int IntervaloSelecionado = 0;
unsigned int MedicaoAD = 0;
unsigned long LoopMillis = 0;

void setup() 
{
  Serial.begin(115200);
  pinMode(PINO_AD, INPUT);  
  SetupTimer1(INTERVALO_PROCESSO_MS);
}

void loop() 
{
  unsigned long currentMillis = millis();
  if((currentMillis - LoopMillis) >= MAIN_LOOP_INTERVAL_MS)
  {  
    LoopMillis = currentMillis;
    Serial.print("Medidor analogico. Taxa atual: ");
    Serial.print(IntervaloSelecionado);
    Serial.println(" ms");
    Serial.println("Opcoes: [t - Alterar taxa de leitura AD]");
  }

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

  delay(10);
}

unsigned int CalculateCounterByInterval(unsigned int intervalMillis)
{
  return ((double) intervalMillis / 1000) / ((double) SELECTED_PRESCALER / (double) BOARD_CLK);
}

void SetTimer1Counter()
{
  TCNT1 = 0;
}

void SetupTimer1(unsigned int intervaloMillis) //Max: 4194ms
{
  IntervaloSelecionado = intervaloMillis;

  TCCR1A = 0;
  TCCR1B = 0;
  TCCR1B |= (1 << CS10) | (1 << CS12); //Prescaler: 1024 - Max value
  SetTimer1Counter();
  TIMSK1 |= (1 << OCIE1A); //Habilita interrupção timer 1
  OCR1A = CalculateCounterByInterval(IntervaloSelecionado);
}

ISR(TIMER1_COMPA_vect) //Registra callback para interrupção do timer1
{
  ProcessoAD();  
  SetTimer1Counter();
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
  TIMSK1 = 0; //Desabilita interrupção TIM1
  int setandoValor = 1;
  Serial.println("Insira o valor desejado em ms (Max. 4000): ");
  String output = "";

  while(!Serial.available()) { }

  output = Serial.readString();
  unsigned int result = output.toInt();
  if((result > LIMITE_INF_TIMER) && (result <= LIMITE_SUP_TIMER))
    SetupTimer1(result);
}
