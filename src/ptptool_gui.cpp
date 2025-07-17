/*
 * ShiwaPTPTool GUI - Precision Time Protocol Graphical User Interface
 * 
 * Copyright (c) 2024 SHIWA NETWORK
 * All rights reserved.
 * 
 * This software is provided as-is for educational and development purposes.
 * Use at your own risk.
 */

#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QTabWidget>
#include <QMessageBox>
#include <QTimer>
#include <QProgressBar>
#include <QTableWidget>
#include <QHeaderView>
#include <QStatusBar>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QSettings>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QTime>
#include <QCloseEvent>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QNetworkInterface>

#include <arpa/inet.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <event2/event-config.h>
#include <event2/event.h>
#include <event2/event_struct.h>
#include <event2/thread.h>
#include <event2/util.h>
#include <fcntl.h>
#include <getopt.h>
#include <inttypes.h>
#include <linux/ethtool.h>
#include <linux/ptp_clock.h>
#include <linux/sockios.h>
#include <math.h>
#include <net/if.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/timex.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <time.h>
#include <unistd.h>

#include <chrono>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>

// PTP Worker Thread Class
class PTPWorker : public QThread {
    Q_OBJECT

public:
    struct PTPData {
        int device = -1;
        clockid_t clkid;
        int fd = -1;
        bool isConnected = false;
    };

    struct Msg {
        uint64_t ptpNow;
        char name[64];
    };

signals:
    void timeUpdated(const QString &time);
    void capabilitiesUpdated(const QString &capabilities);
    void offsetMeasured(const QString &offset);
    void errorOccurred(const QString &error);
    void statusUpdated(const QString &status);

public slots:
    void getTime();
    void getCapabilities();
    void measureOffset(int samples);
    void adjustFrequency(int ppb);
    void adjustTime(int seconds);
    void setTimeFromSystem();
    void setSystemFromTime();
    void listPins();
    void connectDevice(int deviceIndex);
    void disconnectDevice();

private:
    PTPData ptpData;
    QMutex mutex;
    bool running = false;

    static clockid_t get_clockid(int fd) {
        #define CLOCKFD 3
        #define FD_TO_CLOCKID(fd) ((~(clockid_t)(fd) << 3) | CLOCKFD)
        return FD_TO_CLOCKID(fd);
    }

    static long ppb_to_scaled_ppm(int ppb) {
        return (long)(ppb * 65.536);
    }

    static int64_t pctns(struct ptp_clock_time *t) {
        return t->sec * 1000000000LL + t->nsec;
    }

    std::string getPHCFileName(int phc_index) {
        std::stringstream s;
        s << "/dev/ptp" << phc_index;
        return s.str();
    }

    bool openDevice(int deviceIndex);
    void closeDevice();
};

// PTP Server Class
class PTPServer : public QObject {
    Q_OBJECT

public:
    PTPServer(QObject *parent = nullptr);
    ~PTPServer();

public slots:
    void startServer(const QString &address, int port);
    void stopServer();

signals:
    void serverStarted(const QString &message);
    void serverStopped(const QString &message);
    void clientConnected(const QString &clientInfo);
    void clientDisconnected(const QString &clientInfo);
    void dataReceived(const QString &data);
    void errorOccurred(const QString &error);

private slots:
    void handleNewConnection();
    void handleClientDisconnected();
    void handleDataReceived();
    void handleUdpDataReceived();

private:
    QTcpServer *tcpServer;
    QUdpSocket *udpSocket;
    QList<QTcpSocket*> clients;
    QMutex mutex;
    bool isRunning;
    QString serverAddress;
    int serverPort;

    void sendPTPResponse(QTcpSocket *client, const QString &request);
    void sendUdpPTPResponse(const QHostAddress &address, quint16 port, const QString &request);
    QString generatePTPResponse(const QString &request);
};

// PTPServer Implementation
PTPServer::PTPServer(QObject *parent) : QObject(parent), isRunning(false) {
    tcpServer = new QTcpServer(this);
    udpSocket = new QUdpSocket(this);
    
    connect(tcpServer, &QTcpServer::newConnection, this, &PTPServer::handleNewConnection);
    connect(udpSocket, &QUdpSocket::readyRead, this, &PTPServer::handleUdpDataReceived);
}

PTPServer::~PTPServer() {
    stopServer();
}

void PTPServer::startServer(const QString &address, int port) {
    QMutexLocker locker(&mutex);
    
    if (isRunning) {
        emit errorOccurred("Server is already running");
        return;
    }
    
    serverAddress = address;
    serverPort = port;
    
    // Start TCP server
    QHostAddress hostAddress;
    if (address == "0.0.0.0" || address.isEmpty()) {
        hostAddress = QHostAddress::Any;
    } else {
        hostAddress = QHostAddress(address);
    }
    
    if (!tcpServer->listen(hostAddress, port)) {
        emit errorOccurred(QString("Failed to start TCP server: %1").arg(tcpServer->errorString()));
        return;
    }
    
    // Start UDP socket for PTP protocol (port 319 is standard PTP event port)
    int ptpPort = (port == 9001) ? 319 : port + 1; // Use PTP standard port or offset
    if (!udpSocket->bind(hostAddress, ptpPort)) {
        emit errorOccurred(QString("Failed to bind UDP socket to port %1: %2").arg(ptpPort).arg(udpSocket->errorString()));
        tcpServer->close();
        return;
    }
    
    isRunning = true;
    emit serverStarted(QString("PTP Server started on %1:%2 (TCP) and %3 (UDP)").arg(address).arg(port).arg(ptpPort));
}

void PTPServer::stopServer() {
    QMutexLocker locker(&mutex);
    
    if (!isRunning) {
        return;
    }
    
    // Close all client connections
    for (QTcpSocket *client : clients) {
        client->disconnectFromHost();
        client->deleteLater();
    }
    clients.clear();
    
    // Stop servers
    tcpServer->close();
    udpSocket->close();
    
    isRunning = false;
    emit serverStopped("PTP Server stopped");
}

void PTPServer::handleNewConnection() {
    while (tcpServer->hasPendingConnections()) {
        QTcpSocket *client = tcpServer->nextPendingConnection();
        clients.append(client);
        
        connect(client, &QTcpSocket::readyRead, this, &PTPServer::handleDataReceived);
        connect(client, &QTcpSocket::disconnected, this, &PTPServer::handleClientDisconnected);
        
        QString clientInfo = QString("%1:%2").arg(client->peerAddress().toString()).arg(client->peerPort());
        emit clientConnected(QString("Client connected: %1").arg(clientInfo));
    }
}

void PTPServer::handleClientDisconnected() {
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    if (client) {
        QString clientInfo = QString("%1:%2").arg(client->peerAddress().toString()).arg(client->peerPort());
        clients.removeAll(client);
        emit clientDisconnected(QString("Client disconnected: %1").arg(clientInfo));
        client->deleteLater();
    }
}

void PTPServer::handleDataReceived() {
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    if (client) {
        QByteArray data = client->readAll();
        QString request = QString::fromUtf8(data);
        emit dataReceived(QString("TCP Request from %1:%2: %3")
                         .arg(client->peerAddress().toString())
                         .arg(client->peerPort())
                         .arg(request.trimmed()));
        
        sendPTPResponse(client, request);
    }
}

void PTPServer::handleUdpDataReceived() {
    while (udpSocket->hasPendingDatagrams()) {
        QByteArray data;
        data.resize(udpSocket->pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;
        
        udpSocket->readDatagram(data.data(), data.size(), &sender, &senderPort);
        QString request = QString::fromUtf8(data);
        
        emit dataReceived(QString("UDP Request from %1:%2: %3")
                         .arg(sender.toString())
                         .arg(senderPort)
                         .arg(request.trimmed()));
        
        sendUdpPTPResponse(sender, senderPort, request);
    }
}

void PTPServer::sendPTPResponse(QTcpSocket *client, const QString &request) {
    QString response = generatePTPResponse(request);
    client->write(response.toUtf8());
    client->flush();
}

void PTPServer::sendUdpPTPResponse(const QHostAddress &address, quint16 port, const QString &request) {
    QString response = generatePTPResponse(request);
    udpSocket->writeDatagram(response.toUtf8(), address, port);
}

QString PTPServer::generatePTPResponse(const QString &request) {
    // Generate timestamp
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
    
    // Basic PTP-like response format
    QString response;
    
    if (request.trimmed().toLower().contains("time")) {
        response = QString("PTP_TIME_RESPONSE: %1 ns\n").arg(timestamp);
    } else if (request.trimmed().toLower().contains("sync")) {
        response = QString("PTP_SYNC_RESPONSE: %1 ns\n").arg(timestamp);
    } else if (request.trimmed().toLower().contains("delay")) {
        response = QString("PTP_DELAY_RESPONSE: %1 ns\n").arg(timestamp);
    } else {
        response = QString("PTP_RESPONSE: Server time %1 ns\n").arg(timestamp);
    }
    
    return response;
}

// Main GUI Window
class PTPToolGUI : public QMainWindow {
    Q_OBJECT

public:
    PTPToolGUI(QWidget *parent = nullptr);

private slots:
    void onGetTime();
    void onGetCapabilities();
    void onMeasureOffset();
    void onAdjustFrequency();
    void onAdjustTime();
    void onSetTimeFromSystem();
    void onSetSystemFromTime();
    void onListPins();
    void onConnectDevice();
    void onDisconnectDevice();
    void onStartServer();
    void onStopServer();
    void onTimeUpdated(const QString &time);
    void onCapabilitiesUpdated(const QString &capabilities);
    void onOffsetMeasured(const QString &offset);
    void onErrorOccurred(const QString &error);
    void onStatusUpdated(const QString &status);
    void onAbout();
    
    // Server slots
    void onServerStarted(const QString &message);
    void onServerStopped(const QString &message);
    void onClientConnected(const QString &clientInfo);
    void onClientDisconnected(const QString &clientInfo);
    void onServerDataReceived(const QString &data);
    void onServerError(const QString &error);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void setupUI();
    void setupMenuBar();
    void setupStatusBar();
    void setupDeviceTab();
    void setupTimeTab();
    void setupOffsetTab();
    void setupPinsTab();
    void setupNetworkTab();
    void loadSettings();
    void saveSettings();

    // UI Components
    QTabWidget *tabWidget;
    QTextEdit *logOutput;
    QStatusBar *statusBarWidget;
    
    // Device Tab
    QComboBox *deviceComboBox;
    QPushButton *connectButton;
    QPushButton *disconnectButton;
    QLabel *connectionStatus;
    
    // Time Tab
    QLabel *currentTimeLabel;
    QPushButton *getTimeButton;
    QPushButton *setTimeFromSystemButton;
    QPushButton *setSystemFromTimeButton;
    QSpinBox *adjustTimeSpinBox;
    QPushButton *adjustTimeButton;
    QSpinBox *adjustFreqSpinBox;
    QPushButton *adjustFreqButton;
    
    // Offset Tab
    QSpinBox *offsetSamplesSpinBox;
    QPushButton *measureOffsetButton;
    QTextEdit *offsetResults;
    
    // Pins Tab
    QPushButton *listPinsButton;
    QTextEdit *pinsResults;
    
    // Network Tab
    QLineEdit *serverAddressEdit;
    QSpinBox *serverPortSpinBox;
    QPushButton *startServerButton;
    QPushButton *stopServerButton;
    QTextEdit *networkLog;
    
    // Worker thread
    PTPWorker *worker;
    QThread *workerThread;
    
    // PTP Server
    PTPServer *ptpServer;
    QThread *serverThread;
};

// PTPWorker Implementation
void PTPWorker::getTime() {
    QMutexLocker locker(&mutex);
    
    if (!ptpData.isConnected) {
        emit errorOccurred("Device not connected");
        return;
    }

    struct timespec ts;
    if (clock_gettime(ptpData.clkid, &ts)) {
        emit errorOccurred(QString("clock_gettime failed: %1").arg(strerror(errno)));
        return;
    }

    QString timeStr = QString("Clock time: %1.%2 or %3")
                     .arg(ts.tv_sec)
                     .arg(ts.tv_nsec, 9, 10, QChar('0'))
                     .arg(ctime(&ts.tv_sec));
    emit timeUpdated(timeStr);
}

void PTPWorker::getCapabilities() {
    QMutexLocker locker(&mutex);
    
    if (!ptpData.isConnected) {
        emit errorOccurred("Device not connected");
        return;
    }

    struct ptp_clock_caps caps;
    if (ioctl(ptpData.fd, PTP_CLOCK_GETCAPS, &caps)) {
        emit errorOccurred(QString("PTP_CLOCK_GETCAPS failed: %1").arg(strerror(errno)));
        return;
    }

    QString capsStr = QString("/dev/ptp%1\n"
                             "capabilities:\n"
                             "  %2 maximum frequency adjustment (ppb)\n"
                             "  %3 programmable alarms\n"
                             "  %4 external time stamp channels\n"
                             "  %5 programmable periodic signals\n"
                             "  %6 pulse per second\n"
                             "  %7 programmable pins\n"
                             "  %8 cross timestamping")
                     .arg(ptpData.device)
                     .arg(caps.max_adj)
                     .arg(caps.n_alarm)
                     .arg(caps.n_ext_ts)
                     .arg(caps.n_per_out)
                     .arg(caps.pps)
                     .arg(caps.n_pins)
                     .arg(caps.cross_timestamping);
    
    emit capabilitiesUpdated(capsStr);
}

void PTPWorker::measureOffset(int samples) {
    QMutexLocker locker(&mutex);
    
    if (!ptpData.isConnected) {
        emit errorOccurred("Device not connected");
        return;
    }

    if (samples <= 0 || samples > 25) {
        emit errorOccurred("Samples should be between 1 and 25");
        return;
    }

    ptp_sys_offset sysoff = {};
    sysoff.n_samples = samples;

    if (ioctl(ptpData.fd, PTP_SYS_OFFSET, &sysoff)) {
        emit errorOccurred(QString("PTP_SYS_OFFSET failed: %1").arg(strerror(errno)));
        return;
    }

    QString offsetStr = "System and phc clock time offset request okay\n\n";
    struct ptp_clock_time *pct = &sysoff.ts[0];
    
    for (unsigned int i = 0; i < sysoff.n_samples; i++) {
        int64_t t1 = pctns(pct + 2 * i);
        int64_t tp = pctns(pct + 2 * i + 1);
        int64_t t2 = pctns(pct + 2 * i + 2);
        int64_t interval = t2 - t1;
        int64_t offset = (t2 + t1) / 2 - tp;

        offsetStr += QString("Sample %1:\n").arg(i + 1);
        offsetStr += QString("  System time: %1.%2\n")
                    .arg((pct + 2 * i)->sec)
                    .arg((pct + 2 * i)->nsec);
        offsetStr += QString("  PHC    time: %1.%2\n")
                    .arg((pct + 2 * i + 1)->sec)
                    .arg((pct + 2 * i + 1)->nsec);
        offsetStr += QString("  System time: %1.%2\n")
                    .arg((pct + 2 * i + 2)->sec)
                    .arg((pct + 2 * i + 2)->nsec);
        offsetStr += QString("  System/phc clock time offset is %1 ns\n")
                    .arg(offset);
        offsetStr += QString("  System     clock time delay  is %1 ns\n\n")
                    .arg(interval);
    }
    
    emit offsetMeasured(offsetStr);
}

void PTPWorker::adjustFrequency(int ppb) {
    QMutexLocker locker(&mutex);
    
    if (!ptpData.isConnected) {
        emit errorOccurred("Device not connected");
        return;
    }

    struct timex tx;
    memset(&tx, 0, sizeof(tx));
    tx.modes = ADJ_FREQUENCY;
    tx.freq = ppb_to_scaled_ppm(ppb);
    
    if (clock_adjtime(ptpData.clkid, &tx)) {
        emit errorOccurred(QString("clock_adjtime failed: %1").arg(strerror(errno)));
        return;
    }
    
    emit statusUpdated(QString("Frequency adjustment okay (%1 ppb)").arg(ppb));
}

void PTPWorker::adjustTime(int seconds) {
    QMutexLocker locker(&mutex);
    
    if (!ptpData.isConnected) {
        emit errorOccurred("Device not connected");
        return;
    }

    struct timex tx;
    memset(&tx, 0, sizeof(tx));
    tx.modes = ADJ_SETOFFSET;
    tx.time.tv_sec = seconds;
    tx.time.tv_usec = 0;
    
    if (clock_adjtime(ptpData.clkid, &tx) < 0) {
        emit errorOccurred(QString("clock_adjtime failed: %1").arg(strerror(errno)));
        return;
    }
    
    emit statusUpdated(QString("Time shift okay (%1 seconds)").arg(seconds));
}

void PTPWorker::setTimeFromSystem() {
    QMutexLocker locker(&mutex);
    
    if (!ptpData.isConnected) {
        emit errorOccurred("Device not connected");
        return;
    }

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    
    if (clock_settime(ptpData.clkid, &ts)) {
        emit errorOccurred(QString("clock_settime failed: %1").arg(strerror(errno)));
        return;
    }
    
    emit statusUpdated("Set time from system okay");
}

void PTPWorker::setSystemFromTime() {
    QMutexLocker locker(&mutex);
    
    if (!ptpData.isConnected) {
        emit errorOccurred("Device not connected");
        return;
    }

    struct timespec ts;
    clock_gettime(ptpData.clkid, &ts);
    
    if (clock_settime(CLOCK_REALTIME, &ts)) {
        emit errorOccurred(QString("clock_settime failed: %1").arg(strerror(errno)));
        return;
    }
    
    emit statusUpdated("Set system time from PTP okay");
}

void PTPWorker::listPins() {
    QMutexLocker locker(&mutex);
    
    if (!ptpData.isConnected) {
        emit errorOccurred("Device not connected");
        return;
    }

    struct ptp_clock_caps caps;
    struct ptp_pin_desc desc;
    int n_pins = 0;
    
    if (ioctl(ptpData.fd, PTP_CLOCK_GETCAPS, &caps)) {
        emit errorOccurred(QString("PTP_CLOCK_GETCAPS failed: %1").arg(strerror(errno)));
        return;
    } else {
        n_pins = caps.n_pins;
    }
    
    QString pinsStr = QString("Pin configuration for /dev/ptp%1:\n\n").arg(ptpData.device);
    
    for (int i = 0; i < n_pins; i++) {
        desc.index = i;
        if (ioctl(ptpData.fd, PTP_PIN_GETFUNC, &desc)) {
            pinsStr += QString("Error reading pin %1: %2\n").arg(i).arg(strerror(errno));
            break;
        }
        pinsStr += QString("Name %1 index %2 func %3 chan %4\n")
                   .arg(desc.name)
                   .arg(desc.index)
                   .arg(desc.func)
                   .arg(desc.chan);
    }
    
    emit statusUpdated(pinsStr);
}

void PTPWorker::connectDevice(int deviceIndex) {
    QMutexLocker locker(&mutex);
    
    if (ptpData.isConnected) {
        closeDevice();
    }
    
    if (openDevice(deviceIndex)) {
        ptpData.device = deviceIndex;
        ptpData.isConnected = true;
        emit statusUpdated(QString("Connected to /dev/ptp%1").arg(deviceIndex));
    }
}

void PTPWorker::disconnectDevice() {
    QMutexLocker locker(&mutex);
    closeDevice();
    emit statusUpdated("Device disconnected");
}

bool PTPWorker::openDevice(int deviceIndex) {
    if (geteuid() != 0) {
        emit errorOccurred("Root privileges required for PTP operations");
        return false;
    }
    
    std::string devicePath = getPHCFileName(deviceIndex);
    ptpData.fd = open(devicePath.c_str(), O_RDWR);
    if (ptpData.fd < 0) {
        emit errorOccurred(QString("Error opening %1: %2")
                          .arg(QString::fromStdString(devicePath))
                          .arg(strerror(errno)));
        return false;
    }
    
    ptpData.clkid = get_clockid(ptpData.fd);
    if (ptpData.clkid == -1) {
        emit errorOccurred("Failed to read clock id");
        close(ptpData.fd);
        ptpData.fd = -1;
        return false;
    }
    
    return true;
}

void PTPWorker::closeDevice() {
    if (ptpData.fd >= 0) {
        close(ptpData.fd);
        ptpData.fd = -1;
    }
    ptpData.isConnected = false;
}

// PTPToolGUI Implementation
PTPToolGUI::PTPToolGUI(QWidget *parent) : QMainWindow(parent) {
            setWindowTitle("ShiwaPTPTool GUI - Precision Time Protocol Management");
    setMinimumSize(800, 600);
    
    setupUI();
    setupMenuBar();
    setupStatusBar();
    
    // Create worker thread
    worker = new PTPWorker();
    workerThread = new QThread();
    worker->moveToThread(workerThread);
    
    // Connect signals
    connect(worker, &PTPWorker::timeUpdated, this, &PTPToolGUI::onTimeUpdated);
    connect(worker, &PTPWorker::capabilitiesUpdated, this, &PTPToolGUI::onCapabilitiesUpdated);
    connect(worker, &PTPWorker::offsetMeasured, this, &PTPToolGUI::onOffsetMeasured);
    connect(worker, &PTPWorker::errorOccurred, this, &PTPToolGUI::onErrorOccurred);
    connect(worker, &PTPWorker::statusUpdated, this, &PTPToolGUI::onStatusUpdated);
    
    workerThread->start();
    
    // Create PTP server thread
    ptpServer = new PTPServer();
    serverThread = new QThread();
    ptpServer->moveToThread(serverThread);
    
    // Connect server signals
    connect(ptpServer, &PTPServer::serverStarted, this, &PTPToolGUI::onServerStarted);
    connect(ptpServer, &PTPServer::serverStopped, this, &PTPToolGUI::onServerStopped);
    connect(ptpServer, &PTPServer::clientConnected, this, &PTPToolGUI::onClientConnected);
    connect(ptpServer, &PTPServer::clientDisconnected, this, &PTPToolGUI::onClientDisconnected);
    connect(ptpServer, &PTPServer::dataReceived, this, &PTPToolGUI::onServerDataReceived);
    connect(ptpServer, &PTPServer::errorOccurred, this, &PTPToolGUI::onServerError);
    
    serverThread->start();
    
    loadSettings();
}

void PTPToolGUI::setupUI() {
    QWidget *centralWidget = new QWidget();
    setCentralWidget(centralWidget);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    
    // Create tab widget
    tabWidget = new QTabWidget();
    mainLayout->addWidget(tabWidget);
    
    setupDeviceTab();
    setupTimeTab();
    setupOffsetTab();
    setupPinsTab();
    setupNetworkTab();
    
    // Log output
    logOutput = new QTextEdit();
    logOutput->setMaximumHeight(150);
    logOutput->setReadOnly(true);
    mainLayout->addWidget(logOutput);
}

void PTPToolGUI::setupMenuBar() {
    QMenuBar *menuBar = this->menuBar();
    
    // File menu
    QMenu *fileMenu = menuBar->addMenu("&File");
    QAction *exitAction = fileMenu->addAction("E&xit");
    connect(exitAction, &QAction::triggered, this, &QWidget::close);
    
    // Help menu
    QMenu *helpMenu = menuBar->addMenu("&Help");
    QAction *aboutAction = helpMenu->addAction("&About");
    connect(aboutAction, &QAction::triggered, this, &PTPToolGUI::onAbout);
}

void PTPToolGUI::setupStatusBar() {
    statusBarWidget = this->statusBar();
    statusBarWidget->showMessage("Ready");
}

void PTPToolGUI::setupDeviceTab() {
    QWidget *deviceTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(deviceTab);
    
    // Device selection
    QGroupBox *deviceGroup = new QGroupBox("Device Connection");
    QHBoxLayout *deviceLayout = new QHBoxLayout(deviceGroup);
    
    deviceLayout->addWidget(new QLabel("PTP Device:"));
    deviceComboBox = new QComboBox();
    for (int i = 0; i < 10; i++) {
        deviceComboBox->addItem(QString("/dev/ptp%1").arg(i));
    }
    deviceLayout->addWidget(deviceComboBox);
    
    connectButton = new QPushButton("Connect");
    disconnectButton = new QPushButton("Disconnect");
    deviceLayout->addWidget(connectButton);
    deviceLayout->addWidget(disconnectButton);
    
    connectionStatus = new QLabel("Not connected");
    deviceLayout->addWidget(connectionStatus);
    
    layout->addWidget(deviceGroup);
    
    // Capabilities
    QGroupBox *capGroup = new QGroupBox("Device Capabilities");
    QVBoxLayout *capLayout = new QVBoxLayout(capGroup);
    
    QPushButton *getCapabilitiesButton = new QPushButton("Get Capabilities");
    capLayout->addWidget(getCapabilitiesButton);
    
    layout->addWidget(capGroup);
    layout->addStretch();
    
    // Connect signals
    connect(connectButton, &QPushButton::clicked, this, &PTPToolGUI::onConnectDevice);
    connect(disconnectButton, &QPushButton::clicked, this, &PTPToolGUI::onDisconnectDevice);
    connect(getCapabilitiesButton, &QPushButton::clicked, this, &PTPToolGUI::onGetCapabilities);
    
    tabWidget->addTab(deviceTab, "Device");
}

void PTPToolGUI::setupTimeTab() {
    QWidget *timeTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(timeTab);
    
    // Current time
    QGroupBox *currentTimeGroup = new QGroupBox("Current Time");
    QVBoxLayout *currentTimeLayout = new QVBoxLayout(currentTimeGroup);
    
    currentTimeLabel = new QLabel("Not available");
    currentTimeLayout->addWidget(currentTimeLabel);
    
    getTimeButton = new QPushButton("Get Current Time");
    currentTimeLayout->addWidget(getTimeButton);
    
    layout->addWidget(currentTimeGroup);
    
    // Time operations
    QGroupBox *timeOpsGroup = new QGroupBox("Time Operations");
    QGridLayout *timeOpsLayout = new QGridLayout(timeOpsGroup);
    
    timeOpsLayout->addWidget(new QLabel("Adjust Time (seconds):"), 0, 0);
    adjustTimeSpinBox = new QSpinBox();
    adjustTimeSpinBox->setRange(-3600, 3600);
    timeOpsLayout->addWidget(adjustTimeSpinBox, 0, 1);
    adjustTimeButton = new QPushButton("Adjust Time");
    timeOpsLayout->addWidget(adjustTimeButton, 0, 2);
    
    timeOpsLayout->addWidget(new QLabel("Adjust Frequency (ppb):"), 1, 0);
    adjustFreqSpinBox = new QSpinBox();
    adjustFreqSpinBox->setRange(-1000000, 1000000);
    timeOpsLayout->addWidget(adjustFreqSpinBox, 1, 1);
    adjustFreqButton = new QPushButton("Adjust Frequency");
    timeOpsLayout->addWidget(adjustFreqButton, 1, 2);
    
    setTimeFromSystemButton = new QPushButton("Set PTP Time from System");
    timeOpsLayout->addWidget(setTimeFromSystemButton, 2, 0, 1, 2);
    
    setSystemFromTimeButton = new QPushButton("Set System Time from PTP");
    timeOpsLayout->addWidget(setSystemFromTimeButton, 2, 2);
    
    layout->addWidget(timeOpsGroup);
    layout->addStretch();
    
    // Connect signals
    connect(getTimeButton, &QPushButton::clicked, this, &PTPToolGUI::onGetTime);
    connect(adjustTimeButton, &QPushButton::clicked, this, &PTPToolGUI::onAdjustTime);
    connect(adjustFreqButton, &QPushButton::clicked, this, &PTPToolGUI::onAdjustFrequency);
    connect(setTimeFromSystemButton, &QPushButton::clicked, this, &PTPToolGUI::onSetTimeFromSystem);
    connect(setSystemFromTimeButton, &QPushButton::clicked, this, &PTPToolGUI::onSetSystemFromTime);
    
    tabWidget->addTab(timeTab, "Time Management");
}

void PTPToolGUI::setupOffsetTab() {
    QWidget *offsetTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(offsetTab);
    
    // Offset measurement
    QGroupBox *offsetGroup = new QGroupBox("Offset Measurement");
    QHBoxLayout *offsetLayout = new QHBoxLayout(offsetGroup);
    
    offsetLayout->addWidget(new QLabel("Samples (1-25):"));
    offsetSamplesSpinBox = new QSpinBox();
    offsetSamplesSpinBox->setRange(1, 25);
    offsetSamplesSpinBox->setValue(5);
    offsetLayout->addWidget(offsetSamplesSpinBox);
    
    measureOffsetButton = new QPushButton("Measure Offset");
    offsetLayout->addWidget(measureOffsetButton);
    
    layout->addWidget(offsetGroup);
    
    // Results
    offsetResults = new QTextEdit();
    offsetResults->setReadOnly(true);
    layout->addWidget(offsetResults);
    
    // Connect signals
    connect(measureOffsetButton, &QPushButton::clicked, this, &PTPToolGUI::onMeasureOffset);
    
    tabWidget->addTab(offsetTab, "Offset Measurement");
}

void PTPToolGUI::setupPinsTab() {
    QWidget *pinsTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(pinsTab);
    
    // Pin operations
    QGroupBox *pinsGroup = new QGroupBox("Pin Configuration");
    QVBoxLayout *pinsLayout = new QVBoxLayout(pinsGroup);
    
    listPinsButton = new QPushButton("List Pin Configuration");
    pinsLayout->addWidget(listPinsButton);
    
    layout->addWidget(pinsGroup);
    
    // Results
    pinsResults = new QTextEdit();
    pinsResults->setReadOnly(true);
    layout->addWidget(pinsResults);
    
    // Connect signals
    connect(listPinsButton, &QPushButton::clicked, this, &PTPToolGUI::onListPins);
    
    tabWidget->addTab(pinsTab, "Pin Management");
}

void PTPToolGUI::setupNetworkTab() {
    QWidget *networkTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(networkTab);
    
    // Network configuration
    QGroupBox *networkGroup = new QGroupBox("Network Configuration");
    QGridLayout *networkLayout = new QGridLayout(networkGroup);
    
    networkLayout->addWidget(new QLabel("Server Address:"), 0, 0);
    serverAddressEdit = new QLineEdit("127.0.0.1");
    networkLayout->addWidget(serverAddressEdit, 0, 1);
    
    networkLayout->addWidget(new QLabel("Port:"), 1, 0);
    serverPortSpinBox = new QSpinBox();
    serverPortSpinBox->setRange(1024, 65535);
    serverPortSpinBox->setValue(9001);
    networkLayout->addWidget(serverPortSpinBox, 1, 1);
    
    startServerButton = new QPushButton("Start Server");
    stopServerButton = new QPushButton("Stop Server");
    networkLayout->addWidget(startServerButton, 2, 0);
    networkLayout->addWidget(stopServerButton, 2, 1);
    
    // Connect server buttons
    connect(startServerButton, &QPushButton::clicked, this, &PTPToolGUI::onStartServer);
    connect(stopServerButton, &QPushButton::clicked, this, &PTPToolGUI::onStopServer);
    
    layout->addWidget(networkGroup);
    
    // Network log
    networkLog = new QTextEdit();
    networkLog->setReadOnly(true);
    layout->addWidget(networkLog);
    
    tabWidget->addTab(networkTab, "Network");
}

void PTPToolGUI::onGetTime() {
    QMetaObject::invokeMethod(worker, "getTime", Qt::QueuedConnection);
}

void PTPToolGUI::onGetCapabilities() {
    QMetaObject::invokeMethod(worker, "getCapabilities", Qt::QueuedConnection);
}

void PTPToolGUI::onMeasureOffset() {
    int samples = offsetSamplesSpinBox->value();
    QMetaObject::invokeMethod(worker, "measureOffset", Qt::QueuedConnection, Q_ARG(int, samples));
}

void PTPToolGUI::onAdjustFrequency() {
    int ppb = adjustFreqSpinBox->value();
    QMetaObject::invokeMethod(worker, "adjustFrequency", Qt::QueuedConnection, Q_ARG(int, ppb));
}

void PTPToolGUI::onAdjustTime() {
    int seconds = adjustTimeSpinBox->value();
    QMetaObject::invokeMethod(worker, "adjustTime", Qt::QueuedConnection, Q_ARG(int, seconds));
}

void PTPToolGUI::onSetTimeFromSystem() {
    QMetaObject::invokeMethod(worker, "setTimeFromSystem", Qt::QueuedConnection);
}

void PTPToolGUI::onSetSystemFromTime() {
    QMetaObject::invokeMethod(worker, "setSystemFromTime", Qt::QueuedConnection);
}

void PTPToolGUI::onListPins() {
    QMetaObject::invokeMethod(worker, "listPins", Qt::QueuedConnection);
}

void PTPToolGUI::onConnectDevice() {
    int deviceIndex = deviceComboBox->currentIndex();
    QMetaObject::invokeMethod(worker, "connectDevice", Qt::QueuedConnection, Q_ARG(int, deviceIndex));
}

void PTPToolGUI::onDisconnectDevice() {
    QMetaObject::invokeMethod(worker, "disconnectDevice", Qt::QueuedConnection);
}

void PTPToolGUI::onTimeUpdated(const QString &time) {
    currentTimeLabel->setText(time);
    logOutput->append(QString("[%1] %2").arg(QTime::currentTime().toString("hh:mm:ss")).arg(time));
}

void PTPToolGUI::onCapabilitiesUpdated(const QString &capabilities) {
    logOutput->append(QString("[%1] Capabilities:\n%2").arg(QTime::currentTime().toString("hh:mm:ss")).arg(capabilities));
}

void PTPToolGUI::onOffsetMeasured(const QString &offset) {
    offsetResults->setText(offset);
    logOutput->append(QString("[%1] Offset measurement completed").arg(QTime::currentTime().toString("hh:mm:ss")));
}

void PTPToolGUI::onErrorOccurred(const QString &error) {
            QMessageBox::warning(this, "ShiwaPTPTool Error", error);
    logOutput->append(QString("[%1] ERROR: %2").arg(QTime::currentTime().toString("hh:mm:ss")).arg(error));
}

void PTPToolGUI::onStatusUpdated(const QString &status) {
    statusBarWidget->showMessage(status);
    logOutput->append(QString("[%1] %2").arg(QTime::currentTime().toString("hh:mm:ss")).arg(status));
}

void PTPToolGUI::onAbout() {
            QMessageBox::about(this, "About ShiwaPTPTool GUI",
                "ShiwaPTPTool GUI - Precision Time Protocol Management Tool\n\n"
                       "Version 2.0\n"
                       "A graphical interface for managing PTP clocks and measuring time synchronization.\n\n"
                       "Features:\n"
                       "- Device connection and management\n"
                       "- Time synchronization operations\n"
                       "- Offset measurement\n"
                       "- Pin configuration\n"
                       "- Network time distribution");
}

void PTPToolGUI::loadSettings() {
    QSettings settings("PTPTool", "PTPToolGUI");
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
}

void PTPToolGUI::saveSettings() {
    QSettings settings("PTPTool", "PTPToolGUI");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
}

void PTPToolGUI::closeEvent(QCloseEvent *event) {
    saveSettings();
    QMainWindow::closeEvent(event);
}

void PTPToolGUI::onStartServer() {
    QString address = serverAddressEdit->text();
    int port = serverPortSpinBox->value();
    
    networkLog->append(QString("Starting server on %1:%2...").arg(address).arg(port));
    QMetaObject::invokeMethod(ptpServer, "startServer", Qt::QueuedConnection,
                             Q_ARG(QString, address), Q_ARG(int, port));
}

void PTPToolGUI::onStopServer() {
    networkLog->append("Stopping server...");
    QMetaObject::invokeMethod(ptpServer, "stopServer", Qt::QueuedConnection);
}

// Server event handlers
void PTPToolGUI::onServerStarted(const QString &message) {
    networkLog->append(message);
    startServerButton->setEnabled(false);
    stopServerButton->setEnabled(true);
    statusBarWidget->showMessage("PTP Server running");
}

void PTPToolGUI::onServerStopped(const QString &message) {
    networkLog->append(message);
    startServerButton->setEnabled(true);
    stopServerButton->setEnabled(false);
    statusBarWidget->showMessage("PTP Server stopped");
}

void PTPToolGUI::onClientConnected(const QString &clientInfo) {
    networkLog->append(clientInfo);
}

void PTPToolGUI::onClientDisconnected(const QString &clientInfo) {
    networkLog->append(clientInfo);
}

void PTPToolGUI::onServerDataReceived(const QString &data) {
    networkLog->append(data);
}

void PTPToolGUI::onServerError(const QString &error) {
    networkLog->append(QString("Server Error: %1").arg(error));
    QMessageBox::warning(this, "PTP Server Error", error);
    startServerButton->setEnabled(true);
    stopServerButton->setEnabled(false);
}

#include "ptptool_gui.moc"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    PTPToolGUI window;
    window.show();
    
    return app.exec();
}