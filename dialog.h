#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>


/** 该类用来设置当前歌词跳动的颜色

spinBox、确定、取消按钮。都连接到 myLabel 类里面响应*/

namespace Ui {
class Dialog;
}

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog( QWidget *parent = 0);
    ~Dialog();

private:
    Ui::Dialog *ui;

};

#endif // DIALOG_H
