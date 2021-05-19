
#include "scr_def.h"

#include <stdio.h>
#include <WiFi.h>
#include <WebServer.h>
#include "page.h"

const char *ssid = "ESP32";
const char *password = "12345678";

WebServer server(80);

std::string code; 
bool waiting = true;

void sendPage()
{
	server.send(200, "text/html", page);
}

void text()
{
	server.send(200, "text/plain", "ok");
  code = server.arg(0).c_str();
  waiting = false;
}

const int DataPin = 15;
const int IRQpin =  2;

void setup() {
  Serial.begin(115200);
  delay(750);

  LangState ls;

  /* Создание точки доступа
  широковещательный wifi, 6-й канал, 1 клиент */
  WiFi.softAP(ssid, password, 6, 0, 1);
	server.on("/", sendPage);
	server.on("/text", text);
	// Запуск сервера
	server.begin();

wait_for_uploading:
  // Ожидание клиента
  while (waiting) {
    delay(10);
    server.handleClient();
  }

  unsigned long time1, time2;
  int vmcode;

  ls.setHeapSize(16384);

  if (ls.compileString(code) == 0) {
    time1 = micros();
    vmcode = ls.run();
    time2 = micros() - time1;
    PRINTF("\nПрограмма завершилась с кодом 0x%08x\n", vmcode);
    PRINTF("Время выполнения: %u мс (%u мкс)\n", time2 / 1000, time2);
  }
  
  if (SD.begin()) {
    PRINTF("LangState::CompileToFile\n");
    ls.compileToFile(LEX_INPUT_STRING, SD, code, "/TEST2.txt");
    SD.end();
  }
  
  if (SD.begin()) {
    PRINTF("LangState::ExecuteFile\n");
    time1 = micros();
    vmcode = ls.executeFile(SD, "/TEST2.txt");
    time2 = micros() - time1;
    PRINTF("\nПрограмма завершилась с кодом 0x%08x\n", vmcode);
    PRINTF("Время выполнения: %u мс (%u мкс)\n", time2 / 1000, time2);
    SD.end();
  }


  waiting = true;
  goto wait_for_uploading;
}

void loop() {}
