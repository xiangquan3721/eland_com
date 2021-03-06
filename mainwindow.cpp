#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent),
                                          ui(new Ui::MainWindow)
{
    serialOpened = false;
    ReadDataTimes = 0;
    hide();
    setWindowFlags(Qt::WindowStaysOnTopHint);
    show();
    ui->setupUi(this);
}
MainWindow::~MainWindow()
{
    delete ui;
}

typedef enum
{
    KEY_FUN_NONE = 0x00, /* 空命令*/
    SEND_ELAND_ERR_01,   /* SEND ELAND ERROR CODE*/
    KEY_READ_02,         /* READ MCU KEY STATE*/
    TIME_SET_03,         /* SEND ELAND TIME*/
    TIME_READ_04,        /* READ MCU TIME*/
    ELAND_STATES_05,     /* SEND ELAND STATES*/
    SEND_FIRM_WARE_06,   /* SEND ELAND FIRMWARE VERSION*/
    REND_FIRM_WARE_07,   /* READ MUC FIRMWARE VERSION*/
    SEND_LINK_STATE_08,  /* SEND WIFI LINK STATE*/
    MCU_FIRM_WARE_09,    /* START MCU FIRM WARE UPDATE*/
    ALARM_READ_0A,       /* READ MCU ALARM*/
    ALARM_SEND_0B,       /* SEND NEXT ALARM STATE*/
    ELAND_DATA_0C,       /* SEND ELAND DATA TO MCU*/
    ELAND_RESET_0D,      /* RESET SYSTEM */
    ELAND_DELETE_0E,     /* DEVICE DATA DELETE */
} __msg_function_t;

typedef enum
{
    EL_ERROR_NONE = 0x00, /*error none */
    EL_HTTP_TIMEOUT,      /*http time out*/
    EL_HTTP_204,          /*http 204*/
    EL_HTTP_400,          /*http 400*/
    EL_HTTP_OTHER,        /*http other error*/
    EL_FLASH_READ,        /*flash read error*/
    EL_AUDIO_PLAY,        /*audio play error*/
    EL_MAIN_THREAD,       /*file download  error*/
} __eland_error_t;

typedef enum
{
    EL_ERROR_NONE = 0x00, /*error none */
    EL_HTTP_TIMEOUT,      /*http time out*/
    EL_HTTP_204,          /*http 204*/
    EL_HTTP_400,          /*http 400*/
} __eland_error_t;

typedef enum
{
    REFRESH_NONE = 0,
    REFRESH_TIME,
    REFRESH_ALARM,
    REFRESH_MAX,
} MCU_Refresh_type_t;

typedef enum
{
    ElandNone = 0,
    ElandBegin,
    APStatusStart,
    APStatusClosed,
    APServerStart,
    HttpServerStop,
    ELAPPConnected,
    WifyConnected,
    WifyDisConnected,
    WifyConnectedFailed,
    HTTP_Get_HOST_INFO,
    TCP_CN00,
    CONNECTED_NET,
    TCP_DV00,
    TCP_AL00,
    TCP_HD00,
    TCP_HC00,
} Eland_Status_type_t;

typedef enum
{
    LEVEL0 = (uint8_t)0x00,
    LEVEL1 = (uint8_t)0x08,
    LEVEL2 = (uint8_t)0x0C,
    LEVEL3 = (uint8_t)0x0E,
    LEVEL4 = (uint8_t)0x0F,
    LEVELNUM = (uint8_t)0xFF,
} LCD_Wifi_Rssi_t;
typedef enum _eland_mode
{
    ELAND_MODE_NONE = (uint8_t)0x00,
    ELAND_CLOCK_MON,
    ELAND_CLOCK_ALARM,
    ELAND_NC,
    ELAND_NA,
    ELAND_MODE_MAX,
} _ELAND_MODE_t;
void MainWindow::Read_Data()
{
    static QByteArray buf;
    int32_t memCache = 0;
    buf += serial->readAll();
    if (!buf.isEmpty())
    {
        ReadDataTimes++;
        if (ui->comboBox->currentIndex() == 0) //字符
        {
            ui->textEdit->moveCursor(QTextCursor::End);
            QString str = ui->textEdit->toPlainText();
            str += tr(buf);
            ui->textEdit->clear();
            ui->textEdit->append(str);
            buf.clear();
        }
        else if (ui->comboBox->currentIndex() == 1) //協議
        {
            while (!buf.isEmpty())
            {
                if (buf.startsWith((char)0x55))
                {
                    if (buf.length() < 3)
                        return;
                    else if (buf.length() < (buf.at(2) + 3))
                        return;
                    else if ((uint8_t)buf.at(3 + buf.at(2)) != 0xaa)
                    {
                        buf.remove(0, 1);
                        continue;
                    }
                    QString str;
                    str.clear();
                    switch (buf.at(1))
                    {
                    case SEND_ELAND_ERR_01:
                    {
                        qDebug() << "SEND_ELAND_ERR_01";
                        switch (buf.at(3))
                        {
                        case EL_ERROR_NONE: /*error none */
                            str += tr("ERROR_NONE ");
                            break;
                        case EL_HTTP_TIMEOUT: /*http time out*/
                            str += tr("EL_HTTP_TIMEOUT ");
                            break;
                        case EL_HTTP_204: /*http 204*/
                            str += tr("EL_HTTP_204 ");
                            break;
                        case EL_HTTP_400: /*http 400*/
                            str += tr("EL_HTTP_400 ");
                            break;
                        case EL_HTTP_OTHER: /*http other error*/
                            str += tr("EL_HTTP_OTHER ");
                            break;
                        case EL_FLASH_READ: /*flash read error*/
                            str += tr("FLASH_READ ERROR");
                            break;
                        case EL_AUDIO_PLAY: /*audio play error*/
                            str += tr("AUDIO_PLAY ERROR");
                            break;
                        case EL_MAIN_THREAD: /*audio play error*/
                            str += tr("EL_MAIN_THREAD ERROR");
                            break;
                        default:
                            str += tr("ERROR_NONE");
                            break;
                        }
                    }
                        ui->textEdit->moveCursor(QTextCursor::End);
                        ui->textEdit->append(str);
                        break;
                    case KEY_READ_02:
                        if (buf.at(2) > 5)
                        {
                            str += tr("READ_KEY####");
                            memcpy(&memCache, buf.data() + 3, 4);
                            str += tr(" num_of_chunks:") + QString::number(memCache, 10);
                            memcpy(&memCache, buf.data() + 7, 4);
                            str += tr(" free_memory:") + QString::number(memCache, 10);
                            qDebug() << "num_of_chunks = " << QString::number(*(int *)(buf.data() + 3), 10) << "free_memory = " << QString::number(*(int *)(buf.data() + 7), 10);
                            ui->textEdit->moveCursor(QTextCursor::End);
                            ui->textEdit->append(str);
                        }
                        break;
                    case TIME_SET_03:
                        str += tr("time set:");
                        str += QString::number(*(unsigned char *)(buf.data() + 9), 10);
                        str += tr("-");
                        str += QString::number(*(unsigned char *)(buf.data() + 8), 10);
                        str += tr("-");
                        str += QString::number(*(unsigned char *)(buf.data() + 7), 10);
                        str += tr(" ");
                        str += QString::number(*(unsigned char *)(buf.data() + 5), 10);
                        str += tr(":");
                        str += QString::number(*(unsigned char *)(buf.data() + 4), 10);
                        str += tr(":");
                        str += QString::number(*(unsigned char *)(buf.data() + 3), 10);
                        str += tr(" ");
                        str += QString::number(*(unsigned char *)(buf.data() + 6), 10);
                        ui->textEdit->moveCursor(QTextCursor::End);
                        ui->textEdit->append(str);
                        break;
                    case TIME_READ_04:
                        str += tr("time read ");
                        ui->textEdit->moveCursor(QTextCursor::End);
                        ui->textEdit->append(str);
                        break;
                    case ELAND_STATES_05:
                        str += tr("STATE:");
                        if (buf.at(3) == ElandBegin)
                            str += tr("ElandBegin");
                        else if (buf.at(3) == APStatusStart)
                            str += tr("StartAPStatus ");
                        else if (buf.at(3) == APStatusClosed)
                            str += tr("APStatusClosed");
                        else if (buf.at(3) == APServerStart)
                            str += tr("HttpServer Start");
                        else if (buf.at(3) == HttpServerStop)
                            str += tr("HttpServerStop");
                        else if (buf.at(3) == ELAPPConnected)
                            str += tr("ELAPPConnected");
                        else if (buf.at(3) == WifyConnected)
                            str += tr("WifyConnected ");
                        else if (buf.at(3) == WifyDisConnected)
                            str += tr("WifyDisConnected ");
                        else if (buf.at(3) == WifyConnectedFailed)
                            str += tr("WifyFailed ");
                        else if (buf.at(3) == HTTP_Get_HOST_INFO)
                            str += tr("HTTP_Get_HOST_INFO ");
                        else if (buf.at(3) == TCP_CN00)
                            str += tr("TCP_CN00 ");
                        else if (buf.at(3) == TCP_DV00)
                            str += tr("TCP_DV00 ");
                        else if (buf.at(3) == TCP_AL00)
                            str += tr("TCP_AL00 ");
                        else if (buf.at(3) == TCP_HD00)
                            str += tr("TCP_HD00 ");
                        else if (buf.at(3) == TCP_HC00)
                            str += tr("TCP_HC00 ");
                        ui->textEdit->moveCursor(QTextCursor::End);
                        ui->textEdit->append(str);
                        break;
                    case SEND_FIRM_WARE_06:
                        str += tr("firmware :") + tr((buf.data() + 3));
                        ui->textEdit->moveCursor(QTextCursor::End);
                        ui->textEdit->append(str);
                        break;
                    case REND_FIRM_WARE_07:
                        str += tr("firmware :");
                        if (buf.at(2) > 0)
                            str += tr((buf.data() + 3));
                        ui->textEdit->moveCursor(QTextCursor::End);
                        ui->textEdit->append(str);
                        break;
                    case SEND_LINK_STATE_08:
                        switch ((LCD_Wifi_Rssi_t)(buf.at(3)))
                        {
                        case LEVEL0:
                            str += tr("LEVEL0");
                            break;
                        case LEVEL1:
                            str += tr("LEVEL1");
                            break;
                        case LEVEL2:
                            str += tr("LEVEL2");
                            break;
                        case LEVEL3:
                            str += tr("LEVEL3");
                            break;
                        case LEVEL4:
                            str += tr("LEVEL4");
                            break;
                        case LEVELNUM:
                            str += tr("LEVELNUM");
                            break;
                        default:
                            str += tr("default");
                            break;
                        }
                        switch ((_ELAND_MODE_t)buf.at(4))
                        {
                        case ELAND_MODE_NONE:
                            str += tr(" NONE");
                            break;
                        case ELAND_CLOCK_MON:
                            str += tr(" CLOCK_MON");
                            break;
                        case ELAND_CLOCK_ALARM:
                            str += tr(" CLOCK_ALARM");
                            break;
                        case ELAND_NC:
                            str += tr(" NC");
                            break;
                        case ELAND_NA:
                            str += tr(" NA");
                            break;
                        case ELAND_MODE_MAX:
                            str += tr(" MODE_MAX");
                            break;
                        default:
                            str += tr(" default");
                            break;
                        }
                        str += tr(" E_MODE:") + QString::number(buf.at(4), 10);
                        str += tr(" state:") + QString::number(buf.at(5), 10);
                        qDebug() << "data" << QString::number(buf.at(3), 10) << QString::number(buf.at(4), 10) << QString::number(buf.at(5), 10);
                        ui->textEdit->moveCursor(QTextCursor::End);
                        ui->textEdit->append(str);
                        break;
                    case ALARM_READ_0A:
                        str += tr("ALARM_READ_0A");
                        ui->textEdit->moveCursor(QTextCursor::End);
                        ui->textEdit->append(str);
                        break;
                    case ALARM_SEND_0B:
                        str += tr("ALARM_SEND_0B");
                        ui->textEdit->moveCursor(QTextCursor::End);
                        ui->textEdit->append(str);
                        break;
                    case ELAND_DATA_0C:
                        str += tr("ELAND_DATA_0C");
                        ui->textEdit->moveCursor(QTextCursor::End);
                        ui->textEdit->append(str);
                        break;
                    case ELAND_RESET_0D:
                        str += tr("ELAND_RESET_0D");
                        ui->textEdit->moveCursor(QTextCursor::End);
                        ui->textEdit->append(str);
                        break;
                    case ELAND_DELETE_0E:
                        str += tr("ELAND_DELETE_0E");
                        ui->textEdit->moveCursor(QTextCursor::End);
                        ui->textEdit->append(str);
                        break;
                    default:
                        break;
                    }
                    buf.remove(0, 4 + buf.at(2));
                    continue;
                }
                else
                {
                    buf.remove(0, 1);
                }
            }
            //qDebug() << "buf.size:  " << QString::number(buf.size(),10);
        }
        else if (ui->comboBox->currentIndex() == 2) //hex輸出
        {
            QString str;
            for (int i = 0; i < buf.size(); i++)
            {
                if ((buf.at(i) < 0x10) && (buf.at(i) >= 0))
                    str += tr("0");

                str += QString::number(buf.at(i) & 0xFF, 16) + tr(" ");
            }
            ui->textEdit->moveCursor(QTextCursor::End);
            ui->textEdit->insertPlainText(str);
            buf.clear();
        }
    }
}
void MainWindow::handleError(QSerialPort::SerialPortError error)
{
    qDebug() << "SerialPortError " << QString::number(error, 10);
    if (error == QSerialPort::ResourceError)
    {
        QMessageBox::critical(this, tr("Critical Error"), serial->errorString());
        closeserial();
    }
    else if (error == QSerialPort::PermissionError)
    {
        QMessageBox::critical(this, tr("Critical Error"), tr("\r\nThe serial has been already opened\r\nPlease Check"));
        //closeserial();
    }
}

void MainWindow::on_open_close_clicked()
{
    //setWindowFlags(Qt::WindowStaysOnTopHint);
    if (serialOpened == false)
    {
        openserial();
    }
    else
        closeserial();
}

void MainWindow::openserial()
{
    foreach (const QSerialPortInfo &com_info, QSerialPortInfo::availablePorts())
    {
        if (com_info.description() == "USB Serial Port")
        {
            serial = new QSerialPort;
            connect(serial, static_cast<void (QSerialPort::*)(QSerialPort::SerialPortError)>(&QSerialPort::error), this, &MainWindow::handleError); //连接槽，串口出现问题连接到错误处理函数

            serial->setPortName(com_info.portName());
            if (serial->open(QIODevice::ReadWrite) == true)
            {
                serial->setBaudRate(115200);
                serial->setDataBits(QSerialPort::Data8);
                serial->setParity(QSerialPort::NoParity);
                serial->setStopBits(QSerialPort::OneStop);
                serialOpened = true;
                QString str = serial->portName() + " Opened";
                ui->textEdit->append(str);
                QObject::connect(serial, &QSerialPort::readyRead, this, &MainWindow::Read_Data);
                qDebug() << "find the serial: " << com_info.description();
                ui->open_close->setText(tr("Close Serial ") + serial->portName());
                ui->open_close->setShortcut(tr("S"));
            }
        }
    }
}

void MainWindow::closeserial()
{
    QString str = serial->portName() + " Closed";
    serial->clear();
    serial->close();
    serial->deleteLater();
    serialOpened = false;

    ui->textEdit->append(str);
    ui->open_close->setText(tr("Open Serial"));
    ui->open_close->setShortcut(tr("S"));
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape)
    {
        on_open_close_clicked();
        this->close();
    }
}

void MainWindow::on_comboBox_currentIndexChanged(int index)
{
    QString str = QString::number(index, 10);
    qDebug() << "currentIndex:%d" << str;
}

void MainWindow::on_Connect_clicked()
{
    tcp_ssl *tcp_dialog = new tcp_ssl;
    hide();
    setWindowFlag(Qt::WindowStaysOnTopHint, false);
    show();
    tcp_dialog->setAttribute(Qt::WA_QuitOnClose, false);
    tcp_dialog->show();
}
