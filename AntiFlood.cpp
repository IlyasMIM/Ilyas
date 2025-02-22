#include <WiFi.h>
#include <UniversalTelegramBot.h>

// Замените эти значения на ваши реальные токены и идентификаторы
#define BOT_TOKEN "8089902347:AAHqNmexNF_7aD6fn1c7R-rNUbBs71sKxYI"
#define CHAT_ID "-1002028211077"

// Проверяем наличие новых сообщений каждые 1 секунду
int botRequestDelay = 1000;  // Задержка между запросами к боту
unsigned long lastTimeBotRan;  // Время последнего запуска бота

// Выходы для светодиодов и реле
const int ledGREEN = 2;  // Номер пина, к которому подключен LED при успешном подключении к WiFi
const int ledYELLOW = 4;  // Номер пина, к которому подключен LED при отсутствии WiFi сети
const int ledRED = 16;  // Номер пина, к которому подключен LED при аварии
const int Rellay_HVSO = 17;  // Номер пина, к которому подключено реле холодной воды открыть
const int Rellay_HVSC = 5;  // Номер пина, к которому подключено реле холодной воды закрыть
const int Rellay_GVSO = 18;  // Номер пина, к которому подключено реле горячей воды открыть
const int Rellay_GVSC = 19;  // Номер пина, к которому подключено реле горячей воды закрыть

// Входы для датчиков
const int WaterSensor1 = 21;  // Номер пина для датчика 1

// Переменные
bool StateRellay = LOW;  // Начальное состояние реле
bool emergencyAlertSent = false;  // Флаг для отслеживания отправки сообщения об аварии

WiFiClient wifiClient;  // Создаем объект WiFiClient
UniversalTelegramBot bot(BOT_TOKEN, wifiClient);  // Передаем объект в конструктор
void handleNewMessages(int numNewMessages);  // Объявление функции

// Обработка новых сообщений
void handleNewMessages(int numNewMessages) {
  Serial.println("handleNewMessages");  // Сообщение о начале обработки новых сообщений
  Serial.println(String(numNewMessages));  // Вывод количества новых сообщений

  for (int i = 0; i < numNewMessages; i++) {  // Проходим по всем новым сообщениям
    String chat_id = String(bot.messages[i].chat_id);  // Получаем идентификатор чата отправителя
    if (chat_id != CHAT_ID) {  // Проверяем, авторизован ли пользователь
      bot.sendMessage(chat_id, "Неавторизованный пользователь", "");  // Отправляем сообщение об ошибке
      continue;  // Переходим к следующему сообщению
    }

    String text = bot.messages[i].text;  // Получаем текст сообщения
    Serial.println(text);  // Выводим текст сообщения в консоль

    // Обработка команд
    if (text == "Старт") {
      String welcome = "Добро пожаловать!\n";
      welcome += "Список команд для использования:\n";
      welcome += "/Open_cold - Открыть холодную воду\n";
      welcome += "/Close_cold - Закрыть холодную воду\n";
      welcome += "/Open_hot - Открыть горячую воду\n";
      welcome += "/Close_hot - Закрыть горячую воду\n";
      welcome += "/Close_water - Закрыть холодную и горячую воду\n";
      welcome += "/Status - Проверка состояния\n";
      bot.sendMessage(chat_id, welcome, "");
    }

    if (text == "/Status") {
      if (digitalRead(WaterSensor1)) {
        bot.sendMessage(chat_id, "Датчик 1: Протечка обнаружена!", "");
      } else {
        bot.sendMessage(chat_id, "Датчик 1: Протечка не обнаружена.", "");
      }
    }

    if (text == "/Open_cold") {
      bot.sendMessage(chat_id, "Открыта холодная вода", "");
      StateRellay = HIGH;
      digitalWrite(Rellay_HVSO, StateRellay);
      delay(10000);
      StateRellay = LOW;
      digitalWrite(Rellay_HVSO, StateRellay);
    }

    if (text == "/Close_cold") {
      bot.sendMessage(chat_id, "Закрыта холодная вода", "");
      StateRellay = HIGH;
      digitalWrite(Rellay_HVSC, StateRellay);
      delay(10000);
      StateRellay = LOW;
      digitalWrite(Rellay_HVSC, StateRellay);
    }

    if (text == "/Open_hot") {
      bot.sendMessage(chat_id, "Открыта горячая вода", "");
      StateRellay = HIGH;
      digitalWrite(Rellay_GVSO, StateRellay);
      delay(10000);
      StateRellay = LOW;
      digitalWrite(Rellay_GVSO, StateRellay);
    }

    if (text == "/Close_hot") {
      bot.sendMessage(chat_id, "Закрыта горячая вода", "");
      StateRellay = HIGH;
      digitalWrite(Rellay_GVSC, StateRellay);
      delay(10000);
      StateRellay = LOW;
      digitalWrite(Rellay_GVSC, StateRellay);
    }

    if (text == "/Close_water") {
      bot.sendMessage(chat_id, "Закрыта холодная и горячая вода", "");
      StateRellay = HIGH;
      digitalWrite(Rellay_HVSC, StateRellay);
      digitalWrite(Rellay_GVSC, StateRellay);
      delay(10000);
      StateRellay = LOW;
      digitalWrite(Rellay_HVSC, StateRellay);
      digitalWrite(Rellay_GVSC, StateRellay);
    }
  }
}

void setup() {
  Serial.begin(115200);  // Инициализация последовательного порта
  pinMode(ledGREEN, OUTPUT);  // Настройка пина LED
  pinMode(ledYELLOW, OUTPUT);
  pinMode(ledRED, OUTPUT);
  pinMode(Rellay_HVSO, OUTPUT);
  pinMode(Rellay_HVSC, OUTPUT);
  pinMode(Rellay_GVSO, OUTPUT);
  pinMode(Rellay_GVSC, OUTPUT);
  pinMode(WaterSensor1, INPUT);  // Настройка пина для датчика

  // Подключаемся к WiFi
  WiFi.begin("YOUR_SSID", "YOUR_PASSWORD");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Подключение к WiFi...");
  }
  Serial.println("Подключено к WiFi");
  digitalWrite(ledGREEN, HIGH);  // Включаем зеленый LED
   Serial.println(WiFi.localIP());  // Выводим локальный IP-адрес устройства
 
  bot.sendMessage(CHAT_ID, "Система запущена", "");  // Отправляем сообщение о запуске бота
}

void loop() {
  if (millis() > lastTimeBotRan + botRequestDelay) {  // Проверяем, прошло ли время задержки
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);  // Получаем новые сообщения

    while (numNewMessages) {  // Пока есть новые сообщения
      Serial.println("got response");  // Выводим сообщение о получении ответа
      handleNewMessages(numNewMessages);  // Обрабатываем новые сообщения
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);  // Получаем новые сообщения снова
    }
    lastTimeBotRan = millis();  // Обновляем время последнего запуска бота
  }

  // Проверка состояния датчика
  if (digitalRead(WaterSensor1) == HIGH && !emergencyAlertSent) {  // Если на пине 1 1 и сообщение об аварии еще не отправлено
    bot.sendMessage(CHAT_ID, "Датчик 1: Протечка!", "");  // Отправляем сообщение об аварии
    bot.sendMessage(CHAT_ID, "Закрыта холодная и горячая вода", "");  // Отправляем подтверждение
    // Закрываем холодную и горячую воду
    StateRellay = HIGH;
    digitalWrite(Rellay_HVSC, StateRellay);
    digitalWrite(Rellay_GVSC, StateRellay);
    delay(10000);
    StateRellay = LOW;
    digitalWrite(Rellay_HVSC, StateRellay);
    digitalWrite(Rellay_GVSC, StateRellay);
    emergencyAlertSent = true;  // Устанавливаем флаг, что сообщение об аварии было отправлено
  }

  // Для сброса аварии
// if (digitalRead(WaterSensor1) == LOW) {
 //   emergencyAlertSent = false;  // Сбрасываем флаг, чтобы можно было отправить сообщение снова при следующем срабатывании
 // }
}
"коммент можно удалять этот:::::JJJJJJJJещё один коммит"
"NEW_VETKA"