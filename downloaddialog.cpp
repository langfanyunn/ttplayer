#include "downloaddialog.h"
#include "ui_downloaddialog.h"
#include <QString>
#include <QToolTip>
#include <QFileDialog>
#include <QDir>
#include <QDebug>

QString downloadDialog::dirPath;

downloadDialog::downloadDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::downloadDialog)
{
    ui->setupUi(this);
    ui->lineEdit->setText( dirPath );
}

downloadDialog::~downloadDialog()
{
    delete ui;
}

QString downloadDialog::path()
{
    return ui->lineEdit->text();
}

bool downloadDialog::downloadLrc()
{
    return ui->checkBox->isChecked();
}

/// 确定按钮
void downloadDialog::on_pushButton_2_clicked()
{
    QString text = ui->lineEdit->text();
    if ( text.isEmpty() )
        QToolTip::showText( QCursor::pos(), "路径不能为空..",
                                this, QRect(0,0,100,50), 1500);
    else
    {
        QDir dir(text);
        if ( !dir.exists() )
            QToolTip::showText( QCursor::pos(), "路径不存在，别乱搞。",
                                        this, QRect(0,0,100,50), 1500 );
        else
        {
            if ( !ui->lineEdit->text().endsWith('/') )
                ui->lineEdit->setText( ui->lineEdit->text() + "/" );

            dirPath = ui->lineEdit->text();
            this->accept();
        }
    }
}

void downloadDialog::on_pushButton_3_clicked()
{
    this->reject();
}

void downloadDialog::on_pushButton_clicked()
{
   dirPath = QFileDialog::getExistingDirectory( this, tr("选择文件储存路径"),
                       dirPath, QFileDialog::ShowDirsOnly );
   if ( !dirPath.isEmpty() )
       ui->lineEdit->setText( dirPath + "/");
}




