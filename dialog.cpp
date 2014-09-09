#include "dialog.h"
#include "ui_dialog.h"
#include "mylrcwnd.h"

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);

    connect( ui->spinBox, SIGNAL(valueChanged(int)), parent, SLOT(HSlot(int)) );
    connect( ui->spinBox_2, SIGNAL(valueChanged(int)), parent, SLOT(SSlot(int)) );
    connect( ui->spinBox_3, SIGNAL(valueChanged(int)), parent, SLOT(VSlot(int)) );
}

Dialog::~Dialog()
{
    delete ui;
}
