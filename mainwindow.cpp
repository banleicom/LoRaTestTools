/*------------------------------------------------------------------------------
* Module Name      :
* Copyright        :
* Description      :
* Revision History :
* Date          Author        Version        Notes
 2019/06/10    banduoxing     V1.0           创建
------------------------------------------------------------------------------*/
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtWidgets>
#include <QMessageBox>
#include <sys/time.h>
#include <QSerialPortInfo>
#include<windows.h>
#define TARGET_NAME   "LoRaTestTools "
#define VER           "V1.0.0 "
#define LOG_ULR       "log.txt"
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_status(new QLabel),
    lora_serial(new QSerialPort(this))
{
    ui->setupUi(this);
    setWindowTitle(tr(TARGET_NAME)+tr(" "));
    /*界面控制*/
    ui->Button_open_com->setEnabled(false);
    ui->Button_clear_com->setEnabled(true);
    ui->groupBox_setting->setEnabled(false);
    ui->read_textEdit->setFocusPolicy(Qt::NoFocus);
    ui->progressBar_process->setRange(0,100);                 //进度条
    ui->progressBar_process->setVisible(false);               //false:隐藏进度条  true:显示进度条
    ui->statusBar->addWidget(m_status);                       //状态栏
    UiControl(false);

    /*缺省参数设置*/
    InitCombobox();                                           //填充参数
    ui->lineEdit_freq->setText("470000000");                  //470M
    ui->comboBox_power->setCurrentIndex(8);                   //22
    ui->comboBox_BW->setCurrentIndex(0);                      //125K
    ui->comboBox_SF->setCurrentIndex(5);                      //sf=12
    ui->comboBox_CR->setCurrentIndex(0);                      //cr1
    ui->radioButton_auto->setChecked(true);                   //默认自动测试
    ui->lineEdit_power_en->setText("22");                     //门限22dbm
    ui->lineEdit_power_en_2->setText("100");                  //门限100%
    ui->lineEdit_min_total->setText("5");                     //测试5包
    ui->lineEdit_total->setText("5");                         //共测试5个模块
    ui->lineEdit_timeout->setText("30000");                   //超时30S
    ui->lineEdit_min_cyc->setText("3000");                    //灵敏度测试周期3s
    ui->lineEdit_unit->setText(QString("%1").arg(TEST_LORA_NUM));
    set_test.aut=true;
    state_flg=STOP_STATE;

    /*日志文件*/
    file=new QFile(LOG_ULR);
    /*注册界面数据方便赋值*/
    RegisteredUI();

    /*串口*/
    QObject::connect(lora_serial, &QSerialPort::readyRead, this, &MainWindow::readData);

    /*定时器*/
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(onTimeout()));
    showStatusMessage(tr("欢迎使用本软件"));
}

MainWindow::~MainWindow()
{
    delete ui;
}
/*!
    brief 串口打开按钮响应函数
*/
void MainWindow::on_Button_open_com_clicked(bool checked)
{
    /*打开串口*/
    if (!lora_serial->isOpen())
    {
        /* 判断是否有可用串口 */
        if(ui->comboBox_com->count() != 0)
        {
            /* 串口已经关闭，现在来打开串口 */
            /* 设置串口名称 */
            lora_serial->setPortName(ui->comboBox_com->currentText());
            /* 设置波特率 */
            lora_serial->setBaudRate(QSerialPort::Baud9600);
            /* 设置数据位数 */
            lora_serial->setDataBits(QSerialPort::Data8);
            /* 设置奇偶校验 */
            lora_serial->setParity(QSerialPort::NoParity);
            /* 设置停止位 */
            lora_serial->setStopBits(QSerialPort::OneStop);
            /* 设置流控制 */
            lora_serial->setFlowControl(QSerialPort::NoFlowControl);
            /* 打开串口 */
            lora_serial->open(QIODevice::ReadWrite);
            /* 注册回调函数 */

            /*UI*/
            lora_serial->readAll();
            ui->Button_open_com->setText(tr("关闭串口"));
            ui->Button_open_com->setStyleSheet(QString("QPushButton{color:red}"));
            ui->Button_clear_com->setEnabled(false);
            UiControl(true);
            //激活开始按钮
            ui->pushButton_write->setEnabled(true);
            UiStopStata();

            showStatusMessage(tr("Connected to %1").arg(ui->comboBox_com->currentText()));

        }else{
            qDebug()<<"没有可用串口，请重新常识扫描串口";
            // 警告对话框
            QMessageBox::warning(this,tr("警告"),tr("没有可用串口，请重新尝试扫描串口！"),QMessageBox::Ok);
        }

    }else{
        /* 关闭串口 */
        lora_serial->close();
        ui->Button_open_com->setText(tr("打开串口"));
        ui->Button_open_com->setStyleSheet(QString("QPushButton{color:black}"));
        ui->Button_clear_com->setEnabled(true);
        UiControl(false);
        /*关闭定时器*/
        if(timer->isActive())
        {
            timer->stop();
        }
        /*关闭文件*/
        if(file->isOpen())
        {
            file->close();
        }
        /*失活开始按钮*/
        ui->pushButton_write->setEnabled(false);
        UiStopStata();

        showStatusMessage(tr("Unconnected"));
        qDebug()<<"串口在打开状态，串口关闭";
    }
}
/*!
    brief 串口刷新按钮响应函数
    param  checked:
*/
void MainWindow::on_Button_clear_com_clicked(bool checked)
{
    /* 查找可用串口 */
    foreach(const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        QSerialPort serial;
        serial.setPort(info);
        /* 判断端口是否能打开 */
        if(serial.open(QIODevice::ReadWrite))
        {
            int isHaveItemInList = 0;
            /* 判断是不是已经在列表中了 */
            for(int i=0; i<ui->comboBox_com->count(); i++)
            {
                /* 如果，已经在列表中了，那么不添加这一项了就 */
                if(ui->comboBox_com->itemText(i) == serial.portName())
                {
                    isHaveItemInList++;
                }
            }

            if(isHaveItemInList == 0)
            {
                ui->comboBox_com->addItem(serial.portName());
            }
            serial.close();
        }
    }
    ui->Button_open_com->setEnabled(true);
}
/*!
    brief  串口接收数据，处理
*/
void MainWindow::readData()
{
    uart_buffer.append(lora_serial->readAll());
    if(uart_buffer.size()<HOST_PACK_SIZE){
        return;
    }
    if(uart_buffer[0]==TEST_FLG_1 && uart_buffer[1]==TEST_FLG_2)
    {
        //pos=Find_EaEb(uart_buffer);
        memcpy(&host_pack,uart_buffer.data(),HOST_PACK_SIZE);
        uart_buffer.clear();
        /*关闭超时中断*/
        timer->stop();
        /*状态机*/
        switch (timeout_flg) {
        case CMD_A:
            if(host_pack.sig == CMD_LOST)                //未发现LoRa模块
            {
                /*更新数据*/
                UpdataResult();
                /*更新UI界面*/
                PutResult(test_result.index);
                AddText(tr("lora %1/%2 lost!\r\n已完成：%3 正常: %4,异常: %5\r\n")
                        .arg(test_result.total)                                                         \
                        .arg(test_result.index)                                                         \
                        .arg(test_result.total)                                                         \
                        .arg(test_result.idealvalue)                                                    \
                        .arg(test_result.disvalue));
                /*测试下一个模块*/
                TestNextLora();
                return;
            }
            /*链接成功*/
            AddText(tr("lora %1 connect ok!\r\n").arg(test_result.index));
            SendTest(CMD_B,test_result.index);                         //发送数据包
            timer->start(set_test.timeout);                                       //开启定时器
            timeout_flg=CMD_B;
            break;
        case CMD_B:
            if(host_pack.sig != CMD_B)
            {
                uart_buffer.clear();
                qDebug()<<uart_buffer;
                /*交互窗口显示*/
                AddText(tr("main:readData command get CMD_B error!\r\n"));
                return;
            }
            /*更新数据*/
            UpdataResult();
            /*更新UI界面*/
            PutResult(test_result.index);
            /*交互窗口显示*/
            AddText(tr("lora %1/%2 Test done!\r\n已完成：%3 正常: %4,异常: %5\r\n")
                    .arg(test_result.total)                                                      \
                    .arg(test_result.index)                                                      \
                    .arg(test_result.total)                                                      \
                    .arg(test_result.idealvalue)                                                 \
                    .arg(test_result.disvalue));
            /*测试下一个模块*/
            TestNextLora();
            break;
        default:
            break;
        }

    }else{
        qDebug()<<"main:readData ERROR!";
        /*交互窗口显示*/
        AddText(tr("main:readData ERROR!\r\n"));
        return;
    }
}
/*!
    brief 串口发送数据
    param 二进制数据
*/
void MainWindow::writeData(const QByteArray &data)
{
    lora_serial->write(data);
}
/*!
    brief 输出状态栏信息
    param message: 信息数据
*/
void MainWindow::showStatusMessage(const QString &message)
{
    QString time = QDateTime::currentDateTime().toString("hh:mm:ss");
    m_status->setText(time + QString("---") + message);
}
/*!
    brief 初始化界面控件参数
*/
void MainWindow::InitCombobox()
{
    //QStringList freq_list;
    QStringList BW_list;
    QStringList power_list;
    QStringList SF_list;
    QStringList CR_list;
    //QStringList mode_list;
    //freq_list<<"EU868"<<"CN470"<<"EU433"<<"AU915";
    power_list<<"14"<<"15"<<"16"<<"17"<<"18"<<"19"<<"20"<<"21"<<"22";
    BW_list<<"125K"<<"250K"<<"500K"<<"20K";
    SF_list<<"SF7"<<"SF8"<<"SF9"<<"SF10"<<"SF11"<<"SF12";
    CR_list<<"4/5"<<"4/6"<<"4/7"<<"4/8";
    //mode_list<<tr("只接收")<<tr("只发送")<<tr("连续发送")<<tr("休眠")<<tr("设置模式")<<"FS"<<tr("输出载频");
    //ui->comboBox_freq->addItems(freq_list);
    ui->comboBox_power->addItems(power_list);
    ui->comboBox_BW->addItems(BW_list);
    ui->comboBox_SF->addItems(SF_list);
    ui->comboBox_CR->addItems(CR_list);

}

void MainWindow::Par2Com(host_pack_t &host_data)
{

}
/*!
    brief 提取UI界面LoRa配置参数
    param host_data：上位机发送数据帧
    sa Com2Level()\Com2Setting()\
*/
bool MainWindow::Com2Par(test_pack_t &host_data)
{
    FQV_t fqv;
    QString frq;
    if(CheckPar() == false);
      //return false;
    host_data.cfg[1]= ui->comboBox_power->currentIndex()+14;
    host_data.cfg[2]= ui->comboBox_BW->currentIndex();
    host_data.cfg[3]= ui->comboBox_SF->currentIndex()+7;
    host_data.cfg[4]= ui->comboBox_CR->currentIndex()+1;
    //host_data.cfg[5]= ui->comboBox_mode->currentIndex();
    host_data.cfg[5]= 6;   //输出

    /*频率参数*/
    frq=ui->lineEdit_freq->text();
    fqv.Fqv_32 = (uint32_t)frq.toInt();
    host_data.cfg[6]=fqv.Fqv_8[3];
    host_data.cfg[7]=fqv.Fqv_8[2];
    host_data.cfg[8]=fqv.Fqv_8[1];
    host_data.cfg[9]=fqv.Fqv_8[0];

    /*频段*/
    if(fqv.Fqv_32 == 433000000)
    {
        host_data.cfg[0]=0;
    }else if(fqv.Fqv_32 == 470000000){
        host_data.cfg[0]=1;
    }else if(fqv.Fqv_32 == 868000000){
        host_data.cfg[0]=2;
    }else if(fqv.Fqv_32 == 915000000){
        host_data.cfg[0]=3;
    }else{
        qDebug()<<"mainwindow : com2par error!";
    }

    /*灵敏度测试包数*/
    host_data.lin_total=(uint8_t)ui->lineEdit_min_total->text().toInt();
    /*灵敏度测试周期*/
    host_data.min_cyc=(uint32_t)ui->lineEdit_min_cyc->text().toInt();
    return true;
}
/*!
    brief UI界面提取门限参数
    param lel: 门限结构体
    sa Com2Setting()\Com2Par()
*/
void MainWindow::Com2Level(level_t &lel)
{
    lel.lin=(uint8_t)ui->lineEdit_power_en_2->text().toInt();
    lel.outPower=ui->lineEdit_power_en->text().toFloat();
}
/*!
    brief 在UI界面提取设置参数
    param set: 参数设置结构体
    sa Com2Level\Com2Par
*/
void MainWindow::Com2Setting(setting_t &set)
{
   set.aut=1;
   set.total_lora=(uint32_t)ui->lineEdit_total->text().toInt();
   set.total_unit=(uint8_t)ui->lineEdit_unit->text().toInt();
   set.total_test=(uint32_t)ui->lineEdit_min_total->text().toInt();
   set.timeout=(uint32_t)ui->lineEdit_timeout->text().toInt();
}

/*!
    brief 激活/失活ui界面控件
    param on:true激活 false:失活
*/
void MainWindow::UiControl(bool on)
{
    ui->groupBox_setting->setEnabled(on);
    ui->groupBox_lin->setEnabled(on);
    ui->groupBox_mode->setEnabled(on);
    //ui->groupBox_resut->setEnabled(on);
    ui->pushButton_MCU->setEnabled(on);
}
/*!
    brief  UI 界面进入停止状态
    sa UiBeginStat()
*/
void MainWindow::UiStopStata()
{
    ui->pushButton_write->setText(tr("开始"));
    ui->pushButton_pause->setText(tr("暂停"));
    ui->pushButton_pause->setEnabled(false);
    ui->progressBar_process->setVisible(false);
    ui->progressBar_process->setValue(0);
    PutResult(0);   //清空结果
    state_flg=STOP_STATE;

}
/*!
    brief  UI 界面进入开始状态
    sa UiStopStata()
*/
void MainWindow::UiBeginStat()
{
    ui->pushButton_write->setText(tr("停止"));
    ui->pushButton_pause->setEnabled(true);
    ui->pushButton_pause->setText(tr("暂停"));
    ui->progressBar_process->setVisible(true);
    ui->progressBar_process->setValue(0);
    state_flg=BEGIN_STATE;

}

/*!
    brief  记录结果
    n    0清除结果
*/
void MainWindow::PutResult(int n)
{
    if(n == 0)//clear
    {
        //清除ui界面
        for(int i=0;i<TEST_LORA_NUM;i++)
        {
            SetLed(UNTEST,label_lora[i]);
            power_lora[i]->clear();
            lin_lora[i]->clear();
        }
    }else{//put result
        int index=n-1;
        SetLed(test_result.result[index].stat,label_lora[index]);
        power_lora[index]->setText(QString("%1").arg(test_result.result[index].outPower));
        lin_lora[index]->setText(QString("%1").arg(test_result.result[index].lin));
    }
    ui->progressBar_process->setValue(test_result.total*100/set_test.total_lora);        //更新进度条
}

/*!
    brief  注册界面控件，方便循环调用
*/
void MainWindow::RegisteredUI()
{
    label_lora[0] = ui->label_lora1;
    label_lora[1] = ui->label_lora2;
    label_lora[2] = ui->label_lora3;
    label_lora[3] = ui->label_lora4;
    label_lora[4] = ui->label_lora5;
    power_lora[0] = ui->lineEdit_power1;
    power_lora[1] = ui->lineEdit_power2;
    power_lora[2] = ui->lineEdit_power3;
    power_lora[3] = ui->lineEdit_power4;
    power_lora[4] = ui->lineEdit_power5;
    lin_lora[0] = ui->lineEdit_lin1;
    lin_lora[1] = ui->lineEdit_lin2;
    lin_lora[2] = ui->lineEdit_lin3;
    lin_lora[3] = ui->lineEdit_lin4;
    lin_lora[4] = ui->lineEdit_lin5;
}

/*!
    brief 向MCU发送命令包
    param cmd:CMD_A 或 CMD_B
    param index:要检测的LORA 模块索引1-5
*/
void MainWindow::SendTest(cmd_state_t cmd,uint8_t index)
{
    QByteArray pak;
    test_pack.sig=cmd;
    test_pack.lora_num=index;
    test_pack.flg[0]=HOST_FLG_1;
    test_pack.flg[1]=HOST_FLG_2;
    pak.resize(TEST_PACK_SIZE);
    memcpy(pak.data(), &test_pack, TEST_PACK_SIZE);

    //writeData(QString("AT+TEST=").toUtf8());
    writeData(pak);
    //writeData(QString("/r/n").toUtf8());
}

void MainWindow::SendBGCFG()
{
}
/*!
    brief 更新检测结果数据
*/
void MainWindow::UpdataResult()
{
    /*更新结果*/
    test_result.total++;                                        //总数量+1
    test_result.result[test_result.index-1].outPower=host_pack.outPower;
    test_result.result[test_result.index-1].lin=uint8_t((float)host_pack.packNum/set_test.total_test*100);
    /*判断异常*/
    if(host_pack.sig == CMD_LOST)
    {
        test_result.disvalue++;
        test_result.result[test_result.index-1].stat=TEST_UNCONNECT;
    }else if(host_pack.outPower >= level.outPower && test_result.result[test_result.index-1].lin >= level.lin)  //正常
    {
        test_result.idealvalue++;
        test_result.result[test_result.index-1].stat=TEST_OK;

    }else{                                                                                                //异常
        test_result.disvalue++;
        test_result.result[test_result.index-1].stat=TEST_ERROR;
    }
    Body2File();
}
/*!
    brief 检查UI界面参数格式
    return true :正确 false :错误
*/
bool MainWindow::CheckPar()
{
    QString text;
    /*检查频率参数*/
    text=ui->lineEdit_freq->text();
    if(text.isEmpty() || !isDigitString(text) || text.length()!=9)// 为空 或 不是数字 或 位数不为9
    {
        QMessageBox::warning(this,tr("错误"),tr("频率参数错误"),
                             QMessageBox::Ok);
        return false;
    }
    /*输出功率参数检查*/
    text=ui->lineEdit_power_en->text();
    if(text.isEmpty() || !isDigitString(text))// 为空 或 不是数字 或 位数大于3
    {
        QMessageBox::warning(this,tr("错误"),tr("输出功率参数错误"),
                             QMessageBox::Ok);
        return false;
    }
    /*灵敏度门限参数检查*/
    text=ui->lineEdit_power_en_2->text();
    if(text.isEmpty() || !isDigitString(text) || text.toInt()>100)// 为空 或 不是数字 或 参数大于100
    {
        QMessageBox::warning(this,tr("错误"),tr("灵敏度错误"),
                             QMessageBox::Ok);
        return false;
    }
    /*方式设置参数检查*/
    text=ui->lineEdit_total->text();
    if(text.isEmpty() || !isDigitString(text) || text.toInt()<1)// 为空 或 不是数字 或 小于1
    {
        QMessageBox::warning(this,tr("错误"),tr("测试总数量参数错误"),
                             QMessageBox::Ok);
        return false;
    }
    /*灵敏度测试周期参数检查*/
    text=ui->lineEdit_min_cyc->text();
    if(text.isEmpty() || !isDigitString(text))// 为空 或 不是数字
    {
        QMessageBox::warning(this,tr("错误"),tr("灵敏度测试周期参数错误"),
                             QMessageBox::Ok);
        return false;
    }
    /*超时时间参数检查*/
    text=ui->lineEdit_min_cyc->text();
    if(text.isEmpty() || !isDigitString(text))// 为空 或 不是数字
    {
        QMessageBox::warning(this,tr("错误"),tr("灵敏度测试周期参数错误"),
                             QMessageBox::Ok);
        return false;
    }
    /*单位数量参数检查*/
    text=ui->lineEdit_unit->text();
    if(text.isEmpty() || !isDigitString(text) || text.toInt()<1)// 为空 或 不是数字 或 小于1
    {
        QMessageBox::warning(this,tr("错误"),tr("单位数量参数错误"),
                             QMessageBox::Ok);
        return false;
    }
    /*灵敏度测试包数参数检查*/
    text=ui->lineEdit_min_total->text();
    if(text.isEmpty() || !isDigitString(text) || text.toInt()<1)// 为空 或 不是数字 或 小于1
    {
        QMessageBox::warning(this,tr("错误"),tr("单位数量参数错误"),
                             QMessageBox::Ok);
        return false;
    }
    return true;

}
/*!
    brief 测试下一个LoRa模块
    param
    return
    sa
*/
void MainWindow::TestNextLora()
{
    /*测试是否测试下一个模块*/
    if(test_result.total == set_test.total_lora)
    {
        //UiStopStata();
        /*关闭定时器*/
        if(timer->isActive())
        {
            timer->stop();
        }
        timeout_flg=CMD_A;
        return;
    }
    test_result.index++;
    if(test_result.index >set_test.total_unit)                 //重新装填LoRa模块
    {
        test_result.index=1;
        //ui->progressBar_process->setValue(0);
        /*更新UI界面*/
        PutResult(0);
    }
    SendTest(CMD_A,(test_result.index));                     //发送数据包
    timer->start(2000);                                        //开启定时器
    timeout_flg=CMD_A;
}
/*!
    brief UI 交互界面打印信息
    param str:显示信息
*/
void MainWindow::AddText(QString str)
{
    QString time = QDateTime::currentDateTime().toString("hh:mm:ss : ");
    QString curText=ui->read_textEdit->toPlainText();
    curText.append(time+str);
    ui->read_textEdit->setPlainText(curText);
    ui->read_textEdit->moveCursor(QTextCursor::End);

}
/*!
    brief 初始化测试结果数据
*/
void MainWindow::InitResult()
{
    test_result.total=0;
    test_result.index=1;
    test_result.idealvalue=0;
    test_result.disvalue=0;
}
/*!
    brief 输入日志文件表头
*/
void MainWindow::Head2File()
{
   QTextStream in_file(file);
   QString str=tr("  ID           门限功率          状态          发射功率           灵敏度           测试时间");
   in_file<<str<<"\n";
}
/*!
    brief 测量结果输入文件
*/
void MainWindow::Body2File()
{
    QTextStream in_file(file);
    QString str=tr(" %1           %2          %3          %4          %5          %6")        \
            .arg(test_result.total)                                                           \
            .arg(level.outPower)                                                              \
            .arg(stat_str[test_result.result[test_result.index-1].stat])                      \
            .arg(test_result.result[test_result.index-1].outPower)                            \
            .arg(test_result.result[test_result.index-1].lin)
            .arg(QDateTime::currentDateTime().toString("hh:mm:ss"));
            ;
    in_file<<str<<"\n";
}
/*!
    brief 状态指示灯指示
    param stat：状态指示结果体 lab:指示灯控件
    return
    sa
*/
void MainWindow::SetLed(lora_stat_t stat, QLabel *lab)
{
    if(stat == TEST_UNCONNECT)
    {
        lab->setStyleSheet("background-color: rgb(220, 220, 220);"); //灰
    }else if(stat == TEST_OK){
        lab->setStyleSheet("background-color: rgb(0, 238, 0);");  //绿
    }else if(stat == TEST_ERROR){
        lab->setStyleSheet("background-color: rgb(238, 121 , 66);");  //红
    }else{
        lab->setStyleSheet("background-color: rgb(245, 245, 245);");
    }
}
/*!
    brief 交互界面清空按钮响应函数
*/
void MainWindow::on_clear_Button_clicked()
{
    ui->read_textEdit->clear();
}

void MainWindow::String2Hex(QString str, QByteArray &senddata)
{
    int hexdata,lowhexdata;
    int hexdatalen = 0;
    int len = str.length();
    senddata.resize(len/2);
    char lstr,hstr;
    for(int i=0; i<len; )
    {
        //char lstr,
        hstr=str[i].toLatin1();
        if(hstr == ' ')
        {
            i++;
            continue;
        }
        i++;
        if(i >= len)
            break;
        lstr = str[i].toLatin1();
        hexdata = ConvertHexChar(hstr);
        lowhexdata = ConvertHexChar(lstr);
        if((hexdata == 16) || (lowhexdata == 16))
            break;
        else
            hexdata = hexdata*16+lowhexdata;
        i++;
        senddata[hexdatalen] = (char)hexdata;
        hexdatalen++;
    }
    senddata.resize(hexdatalen);
}

char MainWindow::ConvertHexChar(char ch)
{
    if((ch >= '0') && (ch <= '9'))
        return ch-0x30;
    else if((ch >= 'A') && (ch <= 'F'))
        return ch-'A'+10;
    else if((ch >= 'a') && (ch <= 'f'))
        return ch-'a'+10;
    else return (-1);
}

int MainWindow::Find_EaEb(QByteArray src)
{
    int pos=0;
    for(int i=0;i<src.size()-2;i++)
    {
        if(src[i]==0xEA && src[i+1]==0xEB)
        {
            return pos;
        }
        pos++;
    }
    return -1;
}
/*!
    brief 检查字符串是否为数字字符串，不含小数点
    param true:是 false：否
*/
bool MainWindow::isDigitString(const QString &src)
{
    const char *s = src.toUtf8().data();
    while(*s && *s>='0' && *s<='9')s++;
    return !bool(*s);
}

void MainWindow::on_pushButton_MCU_clicked()
{

}

void MainWindow::on_pushButton_INF_clicked()
{

}

void MainWindow::on_pushButton_help_clicked()
{

}

void MainWindow::on_pushButton_About_clicked()
{
    QMessageBox::about(this, tr("关于"),
                       tr(TARGET_NAME)+tr(VER)+tr("Copyright © 2019 SYSJOINT"));

}
/*!
    brief 暂停按钮响应函数
*/
void MainWindow::on_pushButton_pause_clicked(bool checked)
{
    if(state_flg == BEGIN_STATE)
    {
        state_flg = PAUSE_STATE;
        ui->pushButton_pause->setText(tr("继续"));
        if(timer->isActive())
        {
            timer->stop();
        }
    }else{
        state_flg = BEGIN_STATE;
        ui->pushButton_pause->setText(tr("暂停"));
    }
}

/*!
    brief 自动/手动切换按钮响应函数
    param
*/
void MainWindow::on_radioButton_auto_clicked(bool checked)
{
    set_test.aut = checked;
}
/*!
    brief 超时处理函数
*/
void MainWindow::onTimeout()
{
    switch (timeout_flg) {
    case CMD_A:
        if(test_result.index == 1){                                             //测试第一个模块循环检测用于自动开始
            /*交互窗口显示*/
            AddText(tr("Waiting for install LoRa...\r\n"));
            /*尝试再次链接*/
            SendTest(CMD_A,test_result.index);                                  //发送数据包
        }else{                                                                  //非第一个模块超时处理
            test_result.total++;                                                //总数量+1
            test_result.disvalue++;
            test_result.result[test_result.index-1].stat=TEST_UNCONNECT;
            test_result.result[test_result.index-1].outPower=0;
            test_result.result[test_result.index-1].lin=0;
            /*更新UI界面*/
            PutResult(test_result.index);
            /*交互窗口显示*/
            AddText(QString("MCU lost...\r\n"));
            /*测试下一个模块*/
            TestNextLora();
        }

        break;
    case CMD_B:
        test_result.total++;                                                    //总数量+1
        test_result.disvalue++;
        test_result.result[test_result.index-1].stat=TEST_UNCONNECT;
        test_result.result[test_result.index-1].outPower=0;
        /*更新UI界面*/
        PutResult(test_result.index);
        /*交互窗口显示*/
        AddText(QString("MCU lost...\r\n"));
        /*测试下一个模块*/
        TestNextLora();
        break;

    default:
        break;
    }

}

/*!
    brief 开始测试按钮响应函数
*/
void MainWindow::on_pushButton_write_clicked()
{
    if(state_flg == STOP_STATE)              //开始被按下
    {
        /*参数检查*/
        if(!Com2Par(test_pack))
        {
            return;
        }
        state_flg=BEGIN_STATE;
        UiBeginStat();
        Com2Level(level);              //门限初始化
        Com2Setting(set_test);         //测试数量初始化
        UiControl(false);
        /*打开文件*/
        if(!file->open(QIODevice::WriteOnly|QIODevice::Text|QIODevice::Append))
        {
         QMessageBox::warning(this,"file write","can't open",QMessageBox::Yes);
        }
        Head2File();
        /*结果参数初始化*/
        InitResult();
        /*发送指令A*/
        SendTest(CMD_A,test_result.index);

        timeout_flg=CMD_A;
        timer->start(2000);            //2000毫秒超时等待
    }else{                             //停止被按下
        SendTest(CMD_STOP,test_result.index);
        state_flg=STOP_STATE;
        file->close();
        if(timer->isActive())
        {
            timer->stop();
        }
        UiControl(true);
        UiStopStata();
        PutResult(0);                  //清除结果
    }

}

void MainWindow::on_pushButton_log_clicked()
{

    QDesktopServices::openUrl(QUrl(LOG_ULR));
}
