#if CONFIG_FREERTOS_UNICORE
static const BaseType_t cpu = 0;
#else
static const BaseType_t cpu = 1;
#endif

#define buzzer 4
#define led 0
#define trig 5
#define echo 18
#define SOUND_SPEED 0.034

static volatile int delayTime = 500;

#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27,20,4);

static TimerHandle_t timer;
static QueueHandle_t queue;

void timeFunction(TimerHandle_t xTimer){
  digitalWrite(led, LOW);
  digitalWrite(buzzer, LOW);
}

float distanceCal(){
  float duration;
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(2);
  digitalWrite(trig, LOW);

  duration = pulseIn(echo ,HIGH, 30000);

  return duration * SOUND_SPEED / 2;
}

void send(void* parameter){
  static float dis;
  while(1){
    dis = distanceCal();
    xQueueSend(queue, &dis, portMAX_DELAY);

    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

void receive(void* parameter){
  static float dis;
  while(1){
    if(xQueueReceive(queue, &dis, portMAX_DELAY) == pdTRUE){
      lcd.setCursor(3, 1);
      lcd.printf("Distance: %.2f", dis);
    }
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

void warning(void* parameter){
  static float dis;
  while(1){
    if(xQueueReceive(queue, &dis, portMAX_DELAY) == pdTRUE){
      if(dis < 50){
        lcd.setCursor(5, 3);
        lcd.printf("Waring");
        digitalWrite(led, HIGH);
        digitalWrite(buzzer, HIGH);

        if (dis > 40) delayTime = 500;
        else if(dis > 30) delayTime = 400;
        else if(dis > 20) delayTime = 300;
        else if(dis > 10) delayTime = 200;
        else if (dis > 5) delayTime = 100;
        else if (dis > 2) delayTime = 70;
      }
      else{
        lcd.setCursor(5, 3);
        lcd.printf("      ");
      }

      if(xTimerIsTimerActive(timer) == pdFALSE){
        xTimerChangePeriod(timer, delayTime / portTICK_PERIOD_MS, portMAX_DELAY);
        xTimerStart(timer, portTICK_PERIOD_MS);
      }
    }
  }
}

void setup(){
  pinMode(buzzer, OUTPUT);
  pinMode(led, OUTPUT);
  pinMode(trig, OUTPUT);
  pinMode(echo, INPUT);

  Serial.begin(9600);

  lcd.init();
  lcd.backlight();

  timer = xTimerCreate("timer", delayTime / portTICK_PERIOD_MS, pdFALSE, (void*) 0, timeFunction);

  queue = xQueueCreate(5, sizeof(float));

  xTaskCreatePinnedToCore(send, "send Function", 2048, NULL, 5, NULL, cpu);
  xTaskCreatePinnedToCore(receive, "receive Function", 2048, NULL, 5, NULL, cpu);
  xTaskCreatePinnedToCore(warning, "warning Function", 2048, NULL, 5, NULL, cpu);

}

void loop(){}
