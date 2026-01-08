#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "esp_ota_ops.h"

// === ВАШИ НАСТРОЙКИ WI-FI ===
const char* ssid = "MyWiFi";        // ← ЗАМЕНИТЕ НА ВАШУ СЕТЬ!
const char* password = "12345678";  // ← ЗАМЕНИТЕ НА ВАШ ПАРОЛЬ!
// ===========================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
WebServer server(80);

void setupOLED() {
  Wire.begin(21, 22);
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("OLED не найден!");
    return;
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("OTA ACTIVATOR");
  display.display();
}

void setupWiFi() {
  Serial.print("Подключение к Wi-Fi");
  WiFi.begin(ssid, password);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
}

void setupServer() {
  server.on("/", HTTP_GET, []() {
    String html = "<h2>OTA ACTIVATOR</h2>";
    html += "<p>Эта прошивка принудительно активирует последнюю OTA-прошивку</p>";
    html += "<form method='POST' enctype='multipart/form-data' action='/update'>";
    html += "<input type='file' name='firmware' accept='.bin' required>";
    html += "<input type='submit' value='Обновить'>";
    html += "</form>";
    server.send(200, "text/html", html);
  });

  server.on("/update", HTTP_POST, []() {
    server.send(200, "text/plain", Update.hasError() ? "Ошибка" : "OK! Перезагрузка...");
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Update.begin(UPDATE_SIZE_UNKNOWN);
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      Update.write(upload.buf, upload.currentSize);
    } else if (upload.status == UPLOAD_FILE_END) {
      Update.end(true);
    }
  });

  server.begin();
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n=== OTA ACTIVATOR ===");
  
  setupOLED();
  setupWiFi();
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nУспешно! IP: " + WiFi.localIP().toString());
    
    // === КРИТИЧЕСКИ ВАЖНЫЙ КОД: ПРИНУДИТЕЛЬНАЯ АКТИВАЦИЯ ПРОШИВКИ ===
    const esp_partition_t* running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;
    
    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
      if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
        // Принудительно подтверждаем работоспособность прошивки
        esp_ota_mark_app_valid_cancel_rollback();
        Serial.println("✅ Прошивка принудительно активирована!");
        
        display.clearDisplay();
        display.setCursor(0,0);
        display.println("ACTIVATED!");
        display.println("Rebooting...");
        display.display();
        
        delay(2000);
        ESP.restart();
      }
    }
    // =================================================================
    
    setupServer();
  } else {
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("Wi-Fi ERROR");
    display.display();
  }
}

void loop() {
  server.handleClient();
  delay(1);
}
