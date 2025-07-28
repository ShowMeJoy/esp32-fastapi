# Changelog

Все заметные изменения буду записывать в этот файл.

## [Unreleased] - 2025-07-19
### Added
- Перешел с PlatformIO на  ESP-IDF, пересобрал logger.c, дописал wifi_connect и чтение данных с DHT22
- Эндпоинт `/log/` выдает данные на сервере

## [0.1.0] - 2025-07-28
### Added
- Базовая архитектура FastAPI + SQLite
- `POST /submit/` & `GET /log/`
- Сбор данных с ESP32
### Changed
- Отказался от дисплея ssd1306. Новый `firmware/` без SSD1306
