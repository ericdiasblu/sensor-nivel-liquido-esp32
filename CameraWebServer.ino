#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>

#define CAMERA_MODEL_AI_THINKER
#define FLASH_PIN 4  // GPIO do LED do flash
#include "board_config.h"

// WiFi
const char *ssid = "Arataca";
const char *password = "olapodeusar155";

// Servidor de destino
const char *serverName = "http://192.168.0.17:5000/upload";  // <- Troque pela sua URL

void setup() {
  Serial.begin(115200);
  pinMode(FLASH_PIN, OUTPUT);  // <-- Adiciona isso
  WiFi.begin(ssid, password);

  Serial.print("Conectando ao Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado!");

  // Configuração da câmera
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;

  // Alterações aqui para melhorar qualidade
  config.frame_size = FRAMESIZE_SVGA;    // 800x600 - maior resolução
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_LATEST;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 5;                // Qualidade JPEG melhor (0-63, menor é melhor)
  config.fb_count = 2;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Erro ao iniciar a câmera: 0x%x\n", err);
    while(true);  // Trava aqui se der erro
  }

  // Ajustes finos opcionais (pode testar e ajustar depois)
  sensor_t * s = esp_camera_sensor_get();
  s->set_brightness(s, 0);   // -2 a 2
  s->set_contrast(s, 1);     // -2 a 2
  s->set_saturation(s, 0);   // -2 a 2
  s->set_gainceiling(s, (gainceiling_t)6);

  Serial.println("ESP32-CAM pronto! Pressione 'T' no Serial Monitor para tirar a foto.");
}

void loop() {
  if (Serial.available() > 0) {
    char c = Serial.read();
    if (c == 'T' || c == 't') {
      Serial.println("Tecla T detectada! Capturando e enviando foto...");
      sendPhoto();
      Serial.println("Pronto! Aperte 'T' para tirar outra foto.");
    } else {
      Serial.printf("Tecla '%c' ignorada. Aperte 'T' para tirar foto.\n", c);
    }
  }
  delay(100);
}

void sendPhoto() {
  digitalWrite(FLASH_PIN, HIGH);  // Liga o flash
  delay(200);                     // Espera um pouco para iluminar

  camera_fb_t *fb = esp_camera_fb_get();

  digitalWrite(FLASH_PIN, LOW);   // Desliga o flash após capturar

  if (!fb) {
    Serial.println("Erro ao capturar imagem");
    return;
  }

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverName);
    http.addHeader("Content-Type", "image/jpeg");

    int httpResponseCode = http.POST(fb->buf, fb->len);

    if (httpResponseCode > 0) {
      Serial.printf("Imagem enviada! Código HTTP: %d\n", httpResponseCode);
      String response = http.getString();
      Serial.println(response);
    } else {
      Serial.printf("Erro no envio: %d\n", httpResponseCode);
    }

    http.end();
  } else {
    Serial.println("Wi-Fi não conectado");
  }

  esp_camera_fb_return(fb);
}

