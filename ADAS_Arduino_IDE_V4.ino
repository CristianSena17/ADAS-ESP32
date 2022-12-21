//Declaração da Task para o processamento no segundo núcleo
TaskHandle_t Task1;

//Bibliotecas
#include <ESP32Servo.h>
#include "BluetoothSerial.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

//Pins
const int servoPin = GPIO_NUM_12;    //Pino de sinal do servo motor
const int motorFrente = GPIO_NUM_27; //Pino de sinal PWM pra frente
const int motorTras = GPIO_NUM_25;   //Pino de sinal PWM pra tras
const int pinTrigger = GPIO_NUM_5;   //Pino do sensor ultrasonico
const int pinEcho = GPIO_NUM_18;     //Pino de sensor ultrasonico

//Declaração dos objetos
Servo servo;
BluetoothSerial SerialBT;

//Declaração das variaveis globais
bool paraFrente = true;
bool travaDist = false;

//Prototypes:
void ControleServo(int ang);
void Task1code(void * pvParameters);


/*
   Descricao: Controla a posição do servo motor com base em um angulo
*/
void ControleServo(int ang){
  Serial.printf("Servo: %i\n", ang);
  
  if (ang >= -65 && ang <=65){
    ang = map(ang, -65, 65, 8, 48);
    servo.write(ang);
  }
}

/*
   Descricao: Controla a aceleracao do motor com base em um angulo
*/
void ControlePonteH(int ang){    
    ang = map(ang, 91, 180, 0, 255);
    if(paraFrente ){
      if(!travaDist){
        ledcWrite(14, ang);                            //canal 14 do PWM (para frente)
        Serial.printf("ponte h p frente: %i\n", ang);
      }else{
        ledcWrite(14, 0); 
        //ledcWrite(15, 0); 
      }
    }else{
      ledcWrite(15, ang);                              //canal 15 do PWM (para tras)
      Serial.printf("ponte h p tras: %i\n", ang);
    }

}

/*
   Descricao: Inicia o sensor ultasonico
*/
void sonarBegin (byte trig, byte echo){
  #define divisor 58.0
  #define intervaloMedida 35
  #define qtdMedida 20

  pinMode(trig, OUTPUT);
  pinMode(echo, INPUT);

  digitalWrite(trig, LOW);
  delayMicroseconds(500);
}


/*
   Descricao: Procedimento paralelo que verifica a distancia lida pelo sensor ultrasolico e controla a trava de distacia
*/

void Task1code( void * pvParameters ){
  Serial.print("Task1 running on core ");
  Serial.println(xPortGetCoreID());
  long duration;
  float distanceCm;
  float distanceInch;

  for(;;){ 
    digitalWrite(pinTrigger, LOW);
    delayMicroseconds(2);
    digitalWrite(pinTrigger, HIGH);
    delayMicroseconds(10);
    digitalWrite(pinTrigger, LOW);
    
    duration = pulseIn(pinEcho, HIGH);
    distanceCm = duration * 0.034/2;
    
    Serial.print("Distance (cm): ");
    Serial.println(distanceCm);
    
    if(distanceCm <= 40 && distanceCm != 0 && !travaDist){
      travaDist = true;
      Serial.println("travaDist = true");
    }else if(distanceCm > 40 && travaDist){
      travaDist = false;
      Serial.println("travaDist = false");
    }
    
    delay(150);
  } 
}


void setup() {
  Serial.begin(115200);
  Serial.println("ADAS_ESP32");
  SerialBT.begin("ADAS_ESP32"); //Bluetooth device name

  //Inicia o servo motor
  servo.attach(servoPin);

  pinMode(motorFrente, OUTPUT);
  pinMode(motorTras, OUTPUT);

  ledcSetup(14, 500, 8);          //Para o PWM: canal, frequencia e resolucao
  ledcSetup(15, 500, 8);          //Para o PWM: canal, frequencia e resolucao
  ledcAttachPin(motorFrente, 14); //Seta o pino motorFrente no canal 1 do PWM
  ledcAttachPin(motorTras, 15);   //Seta o pino motorTras no canal 2 do PWM

  
  //Configuração da tarefa no core 0
  xTaskCreatePinnedToCore(
                    Task1code,   /* função que implementa a tarefa */
                    "Task1",     /* nome da tarefa */
                    10000,       /* número de palavras a serem alocadas para uso com a pilha da tarefa */
                    NULL,        /* parâmetro de entrada para a tarefa (pode ser NULL) */
                    1,           /* prioridade da tarefa (0 a N) */
                    &Task1,      /* Task handle to keep track of created task */
                    0);          /* Núcleo que executará a tarefa */ 

  servo.write(28); //Centraliza a direção

  sonarBegin(pinTrigger, pinEcho); //Inicia o sensor ultasonico
  
  delay(500);
}


void loop() {
  String text = "";
  int num;

  if (SerialBT.available() > 0) {
    text = SerialBT.readStringUntil(',');
    num = atoi(text.c_str());
    //Serial.println(num);

    if(num >= -65 && num <= 65){
      ControleServo(num);
    }else if(num >= 91 && num <= 180){
      ControlePonteH(num);
    }else if(num == 777){
      paraFrente = true;
    }else if(num == 888){
      paraFrente = false;
    }
  }
  
  delay(10);
}
