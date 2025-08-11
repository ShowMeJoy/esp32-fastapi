# ESP32 FastAPI Logger

IoT-проект на базе ESP32. Измеряет температуру, влажность, давление и отправляет их на локальный сервер FastAPI.

---

## Технологии:
* Микроконтроллер ESP32
* Сенсор DHT22 для считывания температуры и влажности
* Сенсор BMP180

## Описание:
ESP32 отправляет данные на локальный сервер, реализованный с помощью **FastAPI**
Сервер принимает данные и сохраняет их в `.db` файл

## Структура проекта:
```
esp32-fastapi/
├── backend/               # FastAPI-сервер
│   ├── main.py
│   └── database.db
├── firmware/              # Код для ESP32
│   ├── components
│   │   ├── bmp180
│   │   └── dht
│   ├── main
│   │   └── logger.c
│   ├── include
│   │   └── secrets.h (в .gitignore)
├── .gitignore
├── README.md
└── CHANGELOG.md
```
