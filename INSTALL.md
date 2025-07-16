# Инструкции по установке ShiwaPTPTool

**Copyright (c) 2024 SHIWA NETWORK - All rights reserved**

## Обзор

ShiwaPTPTool требует установки различных зависимостей в зависимости от того, какую версию вы хотите использовать (CLI, GUI или обе).

## Системные требования

### Минимальные требования:
- Linux с ядром 3.0 или выше
- Поддержка PTP в ядре
- Компилятор C++ с поддержкой C++17
- Права root для работы с PTP устройствами

### Рекомендуемые требования:
- Linux с ядром 4.0 или выше
- Qt5 для GUI версии
- libevent для сетевых функций

## Установка зависимостей

### Ubuntu/Debian

#### Для CLI версии:
```bash
sudo apt-get update
sudo apt-get install build-essential
sudo apt-get install libevent-dev
sudo apt-get install pkg-config
```

#### Для GUI версии (дополнительно):
```bash
sudo apt-get install qt5-default
sudo apt-get install libqt5core5a libqt5widgets5a libqt5gui5a
sudo apt-get install qtbase5-dev
```

#### Полная установка (CLI + GUI):
```bash
sudo apt-get update
sudo apt-get install build-essential libevent-dev pkg-config
sudo apt-get install qt5-default libqt5core5a libqt5widgets5a libqt5gui5a qtbase5-dev
```

### CentOS/RHEL/Fedora

#### Для CLI версии:
```bash
sudo yum groupinstall "Development Tools"
sudo yum install libevent-devel
sudo yum install pkgconfig
```

#### Для GUI версии (дополнительно):
```bash
sudo yum install qt5-qtbase-devel
sudo yum install qt5-qtbase
```

#### Полная установка (CLI + GUI):
```bash
sudo yum groupinstall "Development Tools"
sudo yum install libevent-devel pkgconfig
sudo yum install qt5-qtbase-devel qt5-qtbase
```

### Arch Linux

#### Для CLI версии:
```bash
sudo pacman -S base-devel
sudo pacman -S libevent
sudo pacman -S pkg-config
```

#### Для GUI версии (дополнительно):
```bash
sudo pacman -S qt5-base
```

#### Полная установка (CLI + GUI):
```bash
sudo pacman -S base-devel libevent pkg-config
sudo pacman -S qt5-base
```

### openSUSE

#### Для CLI версии:
```bash
sudo zypper install patterns-devel-C-C++
sudo zypper install libevent-devel
sudo zypper install pkg-config
```

#### Для GUI версии (дополнительно):
```bash
sudo zypper install libqt5-qtbase-devel
sudo zypper install libqt5-qtbase
```

#### Полная установка (CLI + GUI):
```bash
sudo zypper install patterns-devel-C-C++ libevent-devel pkg-config
sudo zypper install libqt5-qtbase-devel libqt5-qtbase
```

## Проверка установки зависимостей

### Проверка компилятора:
```bash
g++ --version
```

### Проверка libevent:
```bash
pkg-config --exists libevent
echo $?
# Должно вернуть 0
```

### Проверка Qt5 (для GUI):
```bash
pkg-config --exists Qt5Core Qt5Widgets Qt5Gui
echo $?
# Должно вернуть 0
```

## Проверка поддержки PTP

### Проверка наличия PTP устройств:
```bash
ls /dev/ptp*
```

### Проверка поддержки PTP в ядре:
```bash
modprobe ptp
echo $?
# Должно вернуть 0
```

### Проверка сетевых интерфейсов с поддержкой PTP:
```bash
# Список интерфейсов с поддержкой PTP
sudo ethtool -T eth0 2>/dev/null | grep -i ptp || echo "PTP не поддерживается на eth0"
```

## Установка ShiwaPTPTool

### 1. Клонирование репозитория:
```bash
git clone <repository-url>
cd PTPtool
```

### 2. Сборка:

#### Сборка обеих версий:
```bash
make all
```

#### Сборка только CLI версии:
```bash
make ptptool-cli
```

#### Сборка только GUI версии:
```bash
make ptptool-gui
```

### 3. Проверка сборки:
```bash
# Проверка CLI версии
./ptptool-cli --help

# Проверка GUI версии (если собрана)
./ptptool-gui
```

### 4. Установка в систему (опционально):
```bash
sudo make install
```

## Устранение проблем при установке

### Ошибка "libevent not found":
```bash
# Ubuntu/Debian
sudo apt-get install libevent-dev

# CentOS/RHEL
sudo yum install libevent-devel

# Arch Linux
sudo pacman -S libevent
```

### Ошибка "Qt5 not found":
```bash
# Ubuntu/Debian
sudo apt-get install qt5-default

# CentOS/RHEL
sudo yum install qt5-qtbase-devel

# Arch Linux
sudo pacman -S qt5-base
```

### Ошибка компиляции C++17:
```bash
# Убедитесь, что используется g++ версии 7 или выше
g++ --version

# Если версия ниже 7, обновите компилятор
sudo apt-get install g++-7  # Ubuntu/Debian
```

### Ошибка "permission denied":
```bash
# Убедитесь, что у вас есть права на запись в директорию
ls -la

# Если нужно, измените права
chmod +w .
```

## Настройка окружения разработки

### Для разработки CLI версии:
```bash
# Установка дополнительных инструментов
sudo apt-get install clang-format  # Ubuntu/Debian
sudo yum install clang-tools-extra # CentOS/RHEL
sudo pacman -S clang              # Arch Linux
```

### Для разработки GUI версии:
```bash
# Установка Qt Creator (опционально)
sudo apt-get install qtcreator     # Ubuntu/Debian
sudo yum install qt-creator        # CentOS/RHEL
sudo pacman -S qt-creator         # Arch Linux
```

## Проверка работоспособности

### Тест CLI версии:
```bash
# Проверка справки
./ptptool-cli -h

# Проверка доступных устройств
ls /dev/ptp*

# Тест получения времени (требует root)
sudo ./ptptool-cli -d 0 -g
```

### Тест GUI версии:
```bash
# Запуск GUI
./ptptool-gui

# Проверка подключения к устройству через GUI
# (требует запуска с правами root для полной функциональности)
```

## Дополнительные настройки

### Настройка автозапуска PTP сервиса:
```bash
# Создание systemd сервиса (опционально)
sudo tee /etc/systemd/system/ptptool.service << EOF
[Unit]
Description=ShiwaPTPTool Server
After=network.target

[Service]
Type=simple
ExecStart=/usr/bin/ptptool-cli -G
Restart=always
User=root

[Install]
WantedBy=multi-user.target
EOF

# Включение автозапуска
sudo systemctl enable ptptool.service
sudo systemctl start ptptool.service
```

### Настройка прав доступа:
```bash
# Создание группы для PTP доступа
sudo groupadd ptpusers

# Добавление пользователя в группу
sudo usermod -a -G ptpusers $USER

# Настройка прав на PTP устройства
sudo tee /etc/udev/rules.d/99-ptp.rules << EOF
KERNEL=="ptp*", GROUP="ptpusers", MODE="0660"
EOF

# Перезагрузка правил
sudo udevadm control --reload-rules
sudo udevadm trigger
```

## Заключение

После успешной установки всех зависимостей и сборки ShiwaPTPTool, вы можете использовать как CLI, так и GUI версии для управления PTP часами.

Для получения дополнительной помощи обратитесь к основному README.md файлу или создайте issue в репозитории проекта.