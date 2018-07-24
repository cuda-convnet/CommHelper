#include "serialport.h"
#include <QSerialPortInfo>
#include <QMessageBox>

SerialPort::SerialPort(QWidget *tab, QObject *parent) :
    QObject(parent),
    m_pTab(tab),
    m_pSerial(new QSerialPort(parent))
{
    m_pParent = qobject_cast<QWidget *>(parent);
    initComboBox();
    initParameters();

    connect(m_pSerial, SIGNAL(bytesWritten(qint64)), this, SIGNAL(bytesSended(qint64)));
    connect(m_pSerial, SIGNAL(errorOccurred(QSerialPort::SerialPortError)), this, SLOT(handleError(QSerialPort::SerialPortError)));
    connect(m_pSerial, &QSerialPort::readyRead, this, &SerialPort::recvData);
}

SerialPort::~SerialPort() {
    delete m_pSerial;
}

void SerialPort::initComboBox() {
     m_pCboxPort = m_pTab->findChild<QComboBox*>(QStringLiteral("cboxPort"));
     m_pCboxBaud = m_pTab->findChild<QComboBox*>(QStringLiteral("cboxBaud"));
     m_pCboxBit = m_pTab->findChild<QComboBox*>(QStringLiteral("cboxBit"));
     m_pCboxParity = m_pTab->findChild<QComboBox*>(QStringLiteral("cboxParity"));
     m_pCboxStop = m_pTab->findChild<QComboBox*>(QStringLiteral("cboxStop"));
     m_pCboxFlow = m_pTab->findChild<QComboBox*>(QStringLiteral("cboxFlow"));
}

void SerialPort::initParameters()
{
    const QList<QSerialPortInfo> infos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : infos) {
        QString text = QString("%1, %2").arg(info.portName()).arg(info.description());
        m_pCboxPort->addItem(text, info.portName());
    }

    m_pCboxBaud->setValidator(new QIntValidator(0, 4000000, this));
    m_pCboxBaud->addItem(QStringLiteral("9600"), QSerialPort::Baud9600);
    m_pCboxBaud->addItem(QStringLiteral("19200"), QSerialPort::Baud19200);
    m_pCboxBaud->addItem(QStringLiteral("38400"), QSerialPort::Baud38400);
    m_pCboxBaud->addItem(QStringLiteral("57600"), QSerialPort::Baud57600);
    m_pCboxBaud->addItem(QStringLiteral("115200"), QSerialPort::Baud115200);

    m_pCboxBit->addItem(QStringLiteral("5"), QSerialPort::Data5);
    m_pCboxBit->addItem(QStringLiteral("6"), QSerialPort::Data6);
    m_pCboxBit->addItem(QStringLiteral("7"), QSerialPort::Data7);
    m_pCboxBit->addItem(QStringLiteral("8"), QSerialPort::Data8);
    m_pCboxBit->setCurrentIndex(3);

    m_pCboxParity->addItem(QStringLiteral("None"), QSerialPort::NoParity);
    m_pCboxParity->addItem(QStringLiteral("Even"), QSerialPort::EvenParity);
    m_pCboxParity->addItem(QStringLiteral("Odd"), QSerialPort::OddParity);
    m_pCboxParity->addItem(QStringLiteral("Space"), QSerialPort::SpaceParity);
    m_pCboxParity->addItem(QStringLiteral("Mark"), QSerialPort::MarkParity);

    m_pCboxStop->addItem(QStringLiteral("1"), QSerialPort::OneStop);
#ifdef Q_OS_WIN
    m_pCboxStop->addItem(QStringLiteral("1.5"), QSerialPort::OneAndHalfStop);
#endif
    m_pCboxStop->addItem(QStringLiteral("2"), QSerialPort::TwoStop);

    m_pCboxFlow->addItem(QStringLiteral("None"), QSerialPort::NoFlowControl);
    m_pCboxFlow->addItem(QStringLiteral("RTS/CTS"), QSerialPort::HardwareControl);
    m_pCboxFlow->addItem(QStringLiteral("XON/XOFF"), QSerialPort::SoftwareControl);
}

QString SerialPort::openSerialPort() {
    QString message = "";
    m_pSerial->setPortName(m_pCboxPort->itemData(m_pCboxPort->currentIndex()).toString());
    m_pSerial->setBaudRate(m_pCboxBaud->currentText().toInt());
    m_pSerial->setDataBits(static_cast<QSerialPort::DataBits>(m_pCboxBit->itemData(m_pCboxBit->currentIndex()).toInt()));
    m_pSerial->setParity(static_cast<QSerialPort::Parity>(m_pCboxParity->itemData(m_pCboxParity->currentIndex()).toInt()));
    m_pSerial->setStopBits(static_cast<QSerialPort::StopBits>(m_pCboxStop->itemData(m_pCboxStop->currentIndex()).toInt()));
    m_pSerial->setFlowControl(static_cast<QSerialPort::FlowControl>(m_pCboxFlow->itemData(m_pCboxFlow->currentIndex()).toInt()));
    if (m_pSerial->open(QIODevice::ReadWrite)) {
        message = "成功打开串口: " + m_pSerial->portName();
    } else {
        message = "[Error]无法打开串口: " + m_pSerial->portName() + ", " + m_pSerial->errorString();
    }
    return message;
}

QString SerialPort::closeSerialPort() {
    QString message = "";
    if (m_pSerial->isOpen())
        m_pSerial->close();
    message = "已关闭串口";
    return message;
}

QString SerialPort::sendData(const QByteArray &data) {
    QString message;
    if (m_pSerial->isOpen()) {
        if (m_pSerial->write(data)) {
            message = "串口: " + m_pSerial->portName() + "发送数据成功";
        } else {
            message = "[Error]串口: " + m_pSerial->portName() + "发送数据出错, " + m_pSerial->errorString();
        }
    } else {
        message = "[Error]串口未打开";
    }
    return message;
}

QString SerialPort::getPort() {
    return m_pSerial->portName();
}

void SerialPort::handleError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ResourceError) {
        QMessageBox::critical(m_pParent, QString("SerialPort Error"), QString("Error code: %1\nDescription: %2").arg(error).arg(m_pSerial->errorString()));
        this->closeSerialPort();
        emit serialPortClosed();
    }
}

void SerialPort::updatePort() {
    m_pCboxPort->clear();
    const QList<QSerialPortInfo> infos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : infos) {
        QString text = QString("%1, %2").arg(info.portName()).arg(info.description());
        m_pCboxPort->addItem(text, info.portName());
    }
    m_pCboxPort->setCurrentIndex(m_pCboxPort->findData(m_pSerial->portName()));
}

QString SerialPort::recvData() {
    const QByteArray data = m_pSerial->readAll();
    emit hasRecved(data);
    return QString("串口: %1 已接收: %2字节").arg(m_pSerial->portName()).arg(data.size());
}
