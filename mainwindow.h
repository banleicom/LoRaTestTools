#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QLineEdit>
#include <QFile>
#include <QTextStream>
//#include "serialthread.h"
class QLabel;

#define SREG_USE        10    // 已使用的S寄存器数量
#define SREG_MAX_NUM    15    // 总的S寄存器数量
#define TEST_LORA_NUM   5     // 测试工装Lora数量
#define TEST_PACK_SIZE  30    // 发送数据包大小
#define HOST_PACK_SIZE  15    // 接收数据包大小
#define HOST_FLG_1      0XEA  //上位机发送
#define HOST_FLG_2      0XEC
#define TEST_FLG_1      0XEA  //mcu 发送
#define TEST_FLG_2      0XED
typedef enum
{
  UNTEST=0,
  TEST_UNCONNECT,
  TEST_OK,
  TEST_ERROR,
}lora_stat_t;    //lora测试状态
const QString stat_str[4]={"UNTEST","TEST_UNCONNECT","TEST_OK","TEST_ERROR"};

typedef enum
{
    CMD_A=0,
    CMD_B,
    CMD_ERR,
    CMD_LOST,
    CMD_STOP,
}cmd_state_t;

#pragma pack (push, 1)
typedef struct
{
    uint8_t flg[2];                    //0xEA 0xED
    cmd_state_t sig;                   //命令标志
    uint8_t lora_num;                  //测试第num模块
    uint32_t lin_total;                //灵敏度测试包数
    uint32_t min_cyc;                  //灵敏度测试周期
    uint8_t cfg[SREG_MAX_NUM];         //参数信息
} test_pack_t;                         //上位机发送数据帧

typedef struct
{
    uint8_t flg[2];                    //0xEA 0xEC
    cmd_state_t sig;                   //命令标志
    uint8_t index;                     //当前测试模块索引
    float outPower;                    //输出功率
    uint32_t packNum;                  //灵敏度接收包数
} host_pack_t;                         //上位机接收数据帧

#pragma pack (pop)
typedef struct
{
   float outPower;                      //输出功率门限
   uint8_t lin;                         //灵敏度门限
}level_t;                               //门限设置

typedef struct
{
   bool aut;                             //自动测量
   uint32_t total_lora;                  //测试总数量
   uint8_t total_unit;                   //单位数量
   uint32_t total_test;                  //灵敏度测试包数
   uint32_t lin_cyc;                     //灵敏度测试周期
   uint32_t timeout;                     //超时时间
}setting_t;                              //设置

typedef struct
{
    lora_stat_t stat;                    //状态指示
    float outPower;                      //测试功率
    uint8_t lin;                         //灵敏度%比
}result_lora_t;                          //lora测试结果

typedef struct
{
  result_lora_t result[TEST_LORA_NUM];        //lora测试结果
  uint8_t index;                              //当前测试索引 1-5
  uint32_t total;                             //测试完成总数量
  uint32_t idealvalue;                        //正常数量
  uint32_t disvalue;                          //有问题数量
}result_t;                                    //测试结果

typedef union
{
  uint32_t Fqv_32;
  uint8_t Fqv_8[4];
}FQV_t;                                       //频率分段存储

typedef enum
{
    STOP_STATE=0,
    BEGIN_STATE,
    PAUSE_STATE
}ui_state;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);

    ~MainWindow();
private:
    void writeData(const QByteArray &data);

private slots:
    void on_Button_open_com_clicked(bool checked);
    void on_Button_clear_com_clicked(bool checked);
    void readData();
    void on_clear_Button_clicked();
    void on_pushButton_MCU_clicked();
    void on_pushButton_INF_clicked();
    void on_pushButton_help_clicked();
    void on_pushButton_About_clicked();
    void on_pushButton_pause_clicked(bool checked);
    void on_radioButton_auto_clicked(bool checked);
    void onTimeout();

    void on_pushButton_write_clicked();

    void on_pushButton_log_clicked();

private:
    void showStatusMessage(const QString &message);
    void InitCombobox();
    void Par2Com(host_pack_t &);
    bool Com2Par(test_pack_t &);
    void Com2Level(level_t &);
    void Com2Setting(setting_t &);

    void UiControl(bool);
    void UiStopStata();
    void UiBeginStat();
    void PutResult(int n);
    void RegisteredUI();
    void SetLed(lora_stat_t stat,QLabel *lab);
    /*指令发送相关函数*/
    void SendTest(cmd_state_t,uint8_t);
    void SendBGCFG();
    void UpdataResult();
    bool CheckPar();
    void TestNextLora();
    void AddText(QString );
    void InitResult();
    /*附加功能*/
    void Head2File();
    void Body2File();
    Ui::MainWindow *ui;
    QLabel *m_status = nullptr;
    QSerialPort *lora_serial = nullptr;
    QTimer *timer = nullptr;
    cmd_state_t timeout_flg;
    QFile *file;

    QByteArray uart_buffer;
    level_t level;
    setting_t set_test;
    result_t test_result;
    test_pack_t test_pack;
    host_pack_t host_pack;
    /*界面暂停继续标志位*/
    ui_state state_flg;
    /*存储UI界面控件指针，方便调用*/
    QLabel *label_lora[TEST_LORA_NUM];        //指向状态指示标签
    QLineEdit *power_lora[TEST_LORA_NUM];
    QLineEdit *lin_lora[TEST_LORA_NUM];

protected:
    void String2Hex(QString str, QByteArray &senddata);
    char ConvertHexChar(char ch);
    int Find_EaEb(QByteArray src);
    bool isDigitString(const QString& src);

};

#endif // MAINWINDOW_H
