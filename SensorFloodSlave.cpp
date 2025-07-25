#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <esp_sleep.h>

#define WIFI_SSID "OPPO A53"
#define WIFI_PASSWORD "123Police"
#define BOT_TOKEN "8089902347:AAHqNmexNF_7aD6fn1c7R-rNUbBs71sKxYI"
#define CHAT_ID "-1001631198478"

// Период отправки статуса (24 часа в микросекундах)
#define STATUS_INTERVAL 24 * 60 * 60 * 1000000ULL

// Параметры батареи
#define BATTERY_PIN 34          // Пин для измерения напряжения батареи
#define VOLTAGE_DIVIDER 2.0     // Коэффициент делителя напряжения (если используется)
#define MAX_BATTERY_VOLTAGE 4.2 // Максимальное напряжение полностью заряженной батареи
#define MIN_BATTERY_VOLTAGE 3.0 // Минимальное напряжение перед разрядом

const unsigned long BOT_MTBS = 1000;
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);
unsigned long bot_lasttime;
bool R = false;  // Флаг состояния (false = сухо, true = тонем)
bool wakeFromSleep = false; // Флаг пробуждения от сна
uint64_t lastStatusTime = 0; // Время последней отправки статуса

float readBatteryVoltage() {
  int rawValue = analogRead(BATTERY_PIN);
  float voltage = rawValue * (3.3 / 4095.0) * VOLTAGE_DIVIDER;
  return voltage;
}

int calculateBatteryPercentage(float voltage) {
  // Простая линейная аппроксимация для Li-ion
  voltage = constrain(voltage, MIN_BATTERY_VOLTAGE, MAX_BATTERY_VOLTAGE);
  int percentage = (int)((voltage - MIN_BATTERY_VOLTAGE) / (MAX_BATTERY_VOLTAGE - MIN_BATTERY_VOLTAGE) * 100);
  return percentage;
}

void handleNewMessages(int numNewMessages) {
  Serial.print("handleNewMessages ");
  Serial.println(numNewMessages);

  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = bot.messages[i].chat_id;
    String text = bot.messages[i].text;
    String from_name = bot.messages[i].from_name;
    
    if (from_name == "") from_name = "Guest";

    // Если пришла любая команда - сбрасываем флаг "Тонем"
    if (text.length() > 0) {
      R = false;
      Serial.println("Обнулили флаг 'Тонем'");
      bot.sendMessage(CHAT_ID, "Обнулили 😀", "");
    }
  }
}

void connectWiFi() {
  Serial.begin(9600);
  Serial.println();
  Serial.print("Connecting to WiFi SSID ");
  Serial.print(WIFI_SSID);
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
  
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  
  Serial.print("\nWiFi connected. IP address: ");
  Serial.println(WiFi.localIP());
  
  Serial.print("Retrieving time: ");
  configTime(0, 0, "pool.ntp.org");
  time_t now = time(nullptr);
  while (now < 24 * 3600) {
    Serial.print(".");
    delay(100);
    now = time(nullptr);
  }
  Serial.println(now);
}

void sendStatus() {
  float voltage = readBatteryVoltage();
  int percentage = calculateBatteryPercentage(voltage);
  
  String status = R ? "⚠️ Статус: Тонем! 😭" : "✅ Статус: Всё в порядке 😊";
  status += "\n🔋 Батарея: " + String(voltage, 2) + "V (" + String(percentage) + "%)";
  
  bot.sendMessage(CHAT_ID, status, "");
  lastStatusTime = esp_timer_get_time();
}

void goToSleep() {
  Serial.println("Переход в глубокий сон...");
  
  // Настройка пробуждения по прерыванию от пина 5 (LOW уровень)
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_5, LOW);
  
  // Настройка таймера для пробуждения через 24 часа
  uint64_t sleepTime = STATUS_INTERVAL;
  if (lastStatusTime > 0) {
    uint64_t timeSinceLastStatus = esp_timer_get_time() - lastStatusTime;
    if (timeSinceLastStatus < STATUS_INTERVAL) {
      sleepTime = STATUS_INTERVAL - timeSinceLastStatus;
    }
  }
  esp_sleep_enable_timer_wakeup(sleepTime);
  
  // Переход в глубокий сон
  esp_deep_sleep_start();
}

void setup() {
  // Определяем причину пробуждения
  wakeFromSleep = (esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_UNDEFINED);
  
  pinMode(5, INPUT_PULLUP);  // Кнопка/датчик на GPIO5 (подтяжка к VCC)
  pinMode(BATTERY_PIN, INPUT); // Пин для измерения напряжения батареи
  
  if (wakeFromSleep) {
    // Если проснулись от прерывания
    if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0) {
      R = true; // Устанавливаем флаг "Тонем"
    }
    
    // Подключаемся к WiFi только если нужно отправить сообщение
    connectWiFi();
    
    // Если проснулись по таймеру - отправляем статус
    if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER) {
      sendStatus();
    }
    
    // Если проснулись от датчика - отправляем сообщение о затоплении
    if (R) {
      bot.sendMessage(CHAT_ID, "Тонем! 😭", "");
    }
  } else {
    // Первый запуск (не из сна)
    connectWiFi();
    sendStatus();
  }
}

void loop() {
  // Проверяем новые сообщения от Telegram
  if (millis() - bot_lasttime > BOT_MTBS) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    if (numNewMessages > 0) {
      handleNewMessages(numNewMessages);
    }
    bot_lasttime = millis();
  }

  // Проверяем состояние датчика/кнопки (если не проснулись от него)
  if (!wakeFromSleep || esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_EXT0) {
    bool sensorState = digitalRead(5);  // Читаем GPIO5
    
    if (!R && sensorState == LOW) {  // Если было "сухо", но датчик сработал (LOW, т.к. PULLUP)
      Serial.println("Тонем!");
      R = true;  // Устанавливаем флаг
      bot.sendMessage(CHAT_ID, "Тонем! 😭", "");
    }
  }

  // Если с момента последнего статуса прошло более 24 часов - отправляем новый
  if (esp_timer_get_time() - lastStatusTime >= STATUS_INTERVAL) {
    sendStatus();
  }

  // Если не было активности в течение 30 секунд - переходим в сон
  static unsigned long lastActivity = millis();
  if (millis() - lastActivity > 30000) {
    goToSleep();
  }
  lastActivity = millis();
}
