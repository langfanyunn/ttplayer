#ifndef DOWNLOADDIALOG_H
#define DOWNLOADDIALOG_H

#include <QDialog>

namespace Ui {
class downloadDialog;
}

class downloadDialog : public QDialog
{
    Q_OBJECT

public:
    explicit downloadDialog(QWidget *parent = 0);
    ~downloadDialog();

    QString path();
    bool downloadLrc();

    static QString dirPath;

signals:

private slots:
    void on_pushButton_2_clicked();

    void on_pushButton_3_clicked();

    void on_pushButton_clicked();

private:
    Ui::downloadDialog *ui;
};

#endif // DOWNLOADDIALOG_H
