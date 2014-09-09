#ifndef DESLRCDIALOG_H
#define DESLRCDIALOG_H

#include <QDialog>
#include <QLabel>

class colorLabel;

namespace Ui {
class desLrcDialog;
}

class desLrcDialog : public QDialog
{
    Q_OBJECT

public:
    explicit desLrcDialog(QWidget *parent = 0);
    ~desLrcDialog();

private slots:

    void on_horizontalScrollBar_4_valueChanged(int value);

    void on_horizontalScrollBar_5_valueChanged(int value);

    void on_horizontalScrollBar_6_valueChanged(int value);

    void on_horizontalScrollBar_valueChanged(int value);

    void on_horizontalScrollBar_2_valueChanged(int value);

    void on_horizontalScrollBar_3_valueChanged(int value);

private:
    Ui::desLrcDialog *ui;

    colorLabel* lab0;
    colorLabel* lab1;
    colorLabel* lab2;
    colorLabel* lab3;
    colorLabel* lab4;
    colorLabel* lab5;
};


class colorLabel : public QLabel
{
    Q_OBJECT
public:
    explicit colorLabel( QWidget* parent = 0);

    void updateBackGround(int);

protected:
    void paintEvent(QPaintEvent *);

private:
    int hsv;        // 只等于 h 值
};

#endif // DESLRCDIALOG_H










