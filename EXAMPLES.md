# Примеры использования ShiwaPTPTool

**Copyright (c) 2024 SHIWA NETWORK - All rights reserved**

## Обзор

Этот документ содержит подробные примеры использования ShiwaPTPTool для различных сценариев работы с PTP часами.

## Базовые операции

### 1. Получение информации о PTP устройстве

#### Проверка доступных устройств:
```bash
ls /dev/ptp*
```

#### Получение возможностей устройства:
```bash
sudo shiwaptptool-cli -d 0 -c
```

**Пример вывода:**
```
/dev/ptp0
capabilities:
  1000000 maximum frequency adjustment (ppb)
  0 programmable alarms
  2 external time stamp channels
  1 programmable periodic signals
  1 pulse per second
  2 programmable pins
  1 cross timestamping
```

### 2. Работа с временем

#### Получение текущего времени PTP часов:
```bash
sudo shiwaptptool-cli -d 0 -g
```

**Пример вывода:**
```
Clock time: 1640995200.123456789 or Wed Jan 01 12:00:00 2022
```

#### Установка времени PTP из системного времени:
```bash
sudo shiwaptptool-cli -d 0 -s
```

#### Установка системного времени из PTP:
```bash
sudo shiwaptptool-cli -d 0 -S
```

#### Сдвиг времени на 10 секунд:
```bash
sudo shiwaptptool-cli -d 0 -t 10
```

#### Установка времени в конкретное значение:
```bash
sudo shiwaptptool-cli -d 0 -T 1640995200
```

### 3. Настройка частоты

#### Настройка частоты на +1000 ppb (частей на миллиард):
```bash
sudo shiwaptptool-cli -d 0 -f 1000
```

#### Настройка частоты на -500 ppb:
```bash
sudo shiwaptptool-cli -d 0 -f -500
```

## Измерение смещения

### Базовое измерение смещения:
```bash
sudo shiwaptptool-cli -d 0 -k 5
```

**Пример вывода:**
```
System and phc clock time offset request okay

Sample 1:
  System time: 1640995200.123456789
  PHC    time: 1640995200.123456800
  System time: 1640995200.123456790
  System/phc clock time offset is 5 ns
  System     clock time delay  is 1 ns

Sample 2:
  System time: 1640995200.123456791
  PHC    time: 1640995200.123456802
  System time: 1640995200.123456792
  System/phc clock time offset is 6 ns
  System     clock time delay  is 1 ns
```

### Измерение с большим количеством сэмплов:
```bash
sudo shiwaptptool-cli -d 0 -k 25
```

## Работа с пинами

### Просмотр конфигурации пинов:
```bash
sudo shiwaptptool-cli -d 0 -l
```

**Пример вывода:**
```
Name AUX0 index 0 func 0 chan 0
Name AUX1 index 1 func 0 chan 0
```

### Настройка пина для внешних временных меток:
```bash
sudo shiwaptptool-cli -d 0 -i 0 -L 0,1
```

### Настройка пина для периодического вывода:
```bash
sudo shiwaptptool-cli -d 0 -i 0 -L 1,2
```

## Сетевые операции

### Запуск сервера для приема временных меток:
```bash
shiwaptptool-cli -G
```

**Пример вывода:**
```
Starting PTP server on *:9001
Server started. Listening for PTP messages...
```

### Отправка временных меток на удаленный сервер:
```bash
sudo shiwaptptool-cli -d 0 -e 10 -E 192.168.1.100 -n myhost
```

### Запуск сервера с указанием хоста:
```bash
shiwaptptool-cli -G -n server1
```

## Периодические операции

### Настройка однократного таймера на 30 секунд:
```bash
sudo shiwaptptool-cli -d 0 -a 30
```

### Настройка периодического таймера каждые 60 секунд:
```bash
sudo shiwaptptool-cli -d 0 -A 60
```

### Настройка периодического вывода с периодом 1 секунда:
```bash
sudo shiwaptptool-cli -d 0 -i 0 -p 1000000000
```

## Внешние временные метки

### Чтение 5 внешних временных меток:
```bash
sudo shiwaptptool-cli -d 0 -i 0 -e 5
```

**Пример вывода:**
```
External time stamp request okay
Event index 0 at 1640995200.123456789
Event index 0 at 1640995200.223456789
Event index 0 at 1640995200.323456789
Event index 0 at 1640995200.423456789
Event index 0 at 1640995200.523456789
```

## PPS управление

### Включение PPS для системного времени:
```bash
sudo shiwaptptool-cli -d 0 -P 1
```

### Отключение PPS:
```bash
sudo shiwaptptool-cli -d 0 -P 0
```

## GUI примеры

### Запуск GUI:
```bash
shiwaptptool-gui
```

### Основные операции в GUI:

1. **Подключение к устройству:**
   - Выберите устройство в выпадающем списке
   - Нажмите "Connect"

2. **Получение времени:**
   - Перейдите на вкладку "Time Management"
   - Нажмите "Get Current Time"

3. **Измерение смещения:**
   - Перейдите на вкладку "Offset Measurement"
   - Установите количество сэмплов
   - Нажмите "Measure Offset"

4. **Просмотр пинов:**
   - Перейдите на вкладку "Pin Management"
   - Нажмите "List Pin Configuration"

## Сценарии использования

### Сценарий 1: Настройка PTP мастера

```bash
# 1. Проверка устройства
sudo shiwaptptool-cli -d 0 -c

# 2. Синхронизация с системным временем
sudo shiwaptptool-cli -d 0 -s

# 3. Настройка частоты
sudo shiwaptptool-cli -d 0 -f 0

# 4. Включение PPS
sudo shiwaptptool-cli -d 0 -P 1

# 5. Запуск сервера для клиентов
shiwaptptool-cli -G
```

### Сценарий 2: Настройка PTP клиента

```bash
# 1. Проверка устройства
sudo shiwaptptool-cli -d 0 -c

# 2. Измерение смещения
sudo shiwaptptool-cli -d 0 -k 10

# 3. Настройка частоты на основе измерений
sudo shiwaptptool-cli -d 0 -f -100

# 4. Синхронизация с мастером
sudo shiwaptptool-cli -d 0 -S
```

### Сценарий 3: Мониторинг точности

```bash
# Создание скрипта для периодического мониторинга
cat > monitor_ptp.sh << 'EOF'
#!/bin/bash
while true; do
    echo "$(date): Measuring PTP offset..."
    sudo shiwaptptool-cli -d 0 -k 5
    sleep 60
done
EOF

chmod +x monitor_ptp.sh
./monitor_ptp.sh
```

### Сценарий 4: Настройка временных меток

```bash
# 1. Настройка пина для внешних меток
sudo shiwaptptool-cli -d 0 -i 0 -L 0,1

# 2. Чтение меток с отправкой на сервер
sudo shiwaptptool-cli -d 0 -i 0 -e 100 -E 192.168.1.100 -n client1
```

## Автоматизация

### Создание systemd сервиса для PTP сервера:

```bash
sudo tee /etc/systemd/system/ptptool-server.service << EOF
[Unit]
Description=ShiwaPTPTool Server
After=network.target

[Service]
Type=simple
ExecStart=/usr/bin/shiwaptptool-cli -G
Restart=always
RestartSec=5
User=root

[Install]
WantedBy=multi-user.target
EOF

sudo systemctl enable ptptool-server.service
sudo systemctl start ptptool-server.service
```

### Создание скрипта для автоматической синхронизации:

```bash
cat > sync_ptp.sh << 'EOF'
#!/bin/bash

# Автоматическая синхронизация PTP
DEVICE=${1:-0}
SYNC_INTERVAL=${2:-300}  # 5 минут по умолчанию

while true; do
    echo "$(date): Starting PTP synchronization..."
    
    # Измерение смещения
    sudo shiwaptptool-cli -d $DEVICE -k 5
    
    # Синхронизация с системным временем
    sudo shiwaptptool-cli -d $DEVICE -s
    
    echo "$(date): Synchronization completed. Waiting $SYNC_INTERVAL seconds..."
    sleep $SYNC_INTERVAL
done
EOF

chmod +x sync_ptp.sh
```

## Отладка

### Проверка доступности устройства:
```bash
ls -la /dev/ptp*
```

### Проверка прав доступа:
```bash
sudo shiwaptptool-cli -d 0 -c
```

### Проверка сетевого подключения:
```bash
# Тест UDP соединения
nc -u -z 192.168.1.100 9001
```

### Логирование операций:
```bash
# Запуск с подробным выводом
sudo shiwaptptool-cli -d 0 -g -v 2>&1 | tee ptp_operation.log
```

## Производительность

### Измерение точности:
```bash
# Многократное измерение для статистики
for i in {1..100}; do
    sudo shiwaptptool-cli -d 0 -k 1
    sleep 1
done
```

### Мониторинг дрейфа:
```bash
# Долгосрочный мониторинг
while true; do
    timestamp=$(date +%s)
    offset=$(sudo shiwaptptool-cli -d 0 -k 1 2>/dev/null | grep "offset is" | awk '{print $4}')
    echo "$timestamp,$offset" >> drift_log.csv
    sleep 60
done
```

## Заключение

Эти примеры демонстрируют основные возможности ShiwaPTPTool. Для получения дополнительной информации обратитесь к основному README.md файлу или используйте справку:

```bash
shiwaptptool-cli -h
```