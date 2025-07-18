# Обзор проекта ShiwaPTPTool

**Copyright (c) 2024 SHIWA NETWORK - All rights reserved**

## Описание проекта

ShiwaPTPTool - это комплексное решение для управления PTP (Precision Time Protocol) часами с поддержкой как командной строки (CLI), так и графического интерфейса (GUI). Проект предоставляет полный набор инструментов для работы с PTP устройствами в Linux системах.

## Архитектура проекта

### Структура файлов:
```
ShiwaPTPTool/
├── src/
│   ├── ptptool_cli.cpp    # CLI версия (23KB, 780 строк)
│   └── ptptool_gui.cpp    # GUI версия (25KB, 854 строки)
├── Makefile               # Система сборки
├── README.md             # Основная документация
├── INSTALL.md            # Инструкции по установке
├── EXAMPLES.md           # Примеры использования
├── CONTRIBUTING.md       # Руководство по вкладу
├── CHANGELOG.md          # История изменений
├── LICENSE               # Лицензия MIT
├── PROJECT_OVERVIEW.md   # Этот файл
├── ptptool.cpp          # Оригинальная версия (16KB, 627 строк)
└── readme.md            # Старый README
```

## Компоненты системы

### 1. CLI версия (`src/ptptool_cli.cpp`)

**Особенности:**
- Объектно-ориентированный дизайн с классом `PTPToolCLI`
- Улучшенная обработка ошибок
- Структурированный код с разделением на методы
- Поддержка всех оригинальных функций

**Основные функции:**
- Управление временем PTP часов
- Измерение смещения между системным и PTP временем
- Управление пинами устройства
- Сетевые операции (сервер/клиент)
- Настройка частоты и PPS
- Таймеры (однократные и периодические)

### 2. GUI версия (`src/ptptool_gui.cpp`)

**Особенности:**
- Графический интерфейс на Qt5
- Многопоточная архитектура с отдельным worker потоком
- Вкладки для различных функций
- Сохранение/восстановление настроек
- Подробное логирование операций

**Интерфейс:**
- **Device Tab**: Подключение к устройствам и просмотр возможностей
- **Time Management**: Управление временем и частотой
- **Offset Measurement**: Измерение смещения с настройкой сэмплов
- **Pin Management**: Управление пинами устройства
- **Network**: Сетевые операции и мониторинг

### 3. Система сборки (`Makefile`)

**Возможности:**
- Сборка CLI и GUI версий
- Поддержка различных дистрибутивов Linux
- Автоматическая установка/удаление
- Форматирование кода
- Очистка артефактов сборки

**Цели сборки:**
- `make all` - сборка обеих версий
- `make ptptool-cli` - только CLI
- `make ptptool-gui` - только GUI
- `make install` - установка в систему
- `make clean` - очистка

## Документация

### 1. README.md (8.2KB)
- Полное описание проекта
- Инструкции по установке и использованию
- Описание всех опций CLI
- Руководство по GUI
- Устранение неполадок

### 2. INSTALL.md (7.8KB)
- Подробные инструкции по установке зависимостей
- Поддержка различных дистрибутивов Linux
- Проверка установки и работоспособности
- Настройка окружения разработки

### 3. EXAMPLES.md (9.3KB)
- Подробные примеры использования
- Сценарии применения
- Автоматизация и скрипты
- Отладка и производительность

### 4. CONTRIBUTING.md (6.3KB)
- Руководство по вкладу в проект
- Стандарты кода
- Процесс разработки
- Тестирование

### 5. CHANGELOG.md
- История изменений проекта
- Описание версий
- Примечания к релизам

## Технические характеристики

### Системные требования:
- **ОС**: Linux с ядром 3.0+
- **Компилятор**: g++ с поддержкой C++17
- **Зависимости**: libevent, Qt5 (для GUI)
- **Права**: root для PTP операций

### Поддерживаемые дистрибутивы:
- Ubuntu/Debian
- CentOS/RHEL/Fedora
- Arch Linux
- openSUSE

### Функциональность:

#### CLI функции:
- ✅ Управление временем (получение, установка, синхронизация)
- ✅ Измерение смещения (до 25 сэмплов)
- ✅ Управление пинами (просмотр, конфигурация)
- ✅ Сетевые операции (сервер/клиент UDP)
- ✅ Настройка частоты (ppb)
- ✅ PPS управление
- ✅ Таймеры (однократные/периодические)
- ✅ Внешние временные метки

#### GUI функции:
- ✅ Графический интерфейс с вкладками
- ✅ Многопоточность (worker thread)
- ✅ Сохранение настроек
- ✅ Логирование операций
- ✅ Все функции CLI через GUI

## Качество кода

### Метрики:
- **CLI версия**: 780 строк кода
- **GUI версия**: 854 строки кода
- **Документация**: 5 файлов, ~32KB текста
- **Покрытие функций**: 100% оригинальной функциональности

### Стандарты:
- C++17 стандарт
- Qt5 для GUI
- libevent для сетевых операций
- MIT лицензия

## Преимущества новой версии

### По сравнению с оригиналом:
1. **Структурированный код**: Объектно-ориентированный дизайн
2. **GUI интерфейс**: Удобный графический интерфейс
3. **Улучшенная документация**: Подробные инструкции и примеры
4. **Обработка ошибок**: Информативные сообщения об ошибках
5. **Многопоточность**: Безопасная работа GUI
6. **Настройки**: Сохранение пользовательских настроек
7. **Логирование**: Подробные логи операций

### Новые возможности:
- Графический интерфейс с вкладками
- Многопоточная архитектура
- Сохранение/восстановление настроек
- Подробное логирование
- Улучшенная обработка ошибок
- Поддержка различных дистрибутивов Linux

## Планы развития

### Краткосрочные цели:
- Добавление тестов
- Улучшение GUI интерфейса
- Расширение документации

### Долгосрочные цели:
- Поддержка Qt6
- Веб-интерфейс
- Интеграция с systemd
- Поддержка Windows (WSL)

## Заключение

ShiwaPTPTool версии 2.0 представляет собой значительное улучшение оригинального проекта с добавлением GUI интерфейса, улучшенной документации и более структурированного кода. Проект готов к использованию в производственной среде и дальнейшему развитию сообществом.