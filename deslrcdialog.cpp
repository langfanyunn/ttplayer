#include "deslrcdialog.h"
#include "ui_deslrcdialog.h"
#include "mylrc.h"
#include <QLayout>
#include <QPalette>
#include <QPainter>
#include <QGroupBox>
#include <QDebug>

desLrcDialog::desLrcDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::desLrcDialog)
{
    ui->setupUi(this);

    myLrc* desLrc = static_cast<myLrc*>( parent->parentWidget() );
    if ( !desLrc->inherits("myLrc") )
        qDebug() << "父部件造篡改错误！！desLrcDialog::desLrcDialog()";

    connect( ui->horizontalScrollBar, SIGNAL(valueChanged(int)),
                desLrc, SLOT(foreColor0Changed(int)) );
    connect( ui->horizontalScrollBar_2, SIGNAL(valueChanged(int)),
                desLrc, SLOT(foreColor1Changed(int)) );
    connect( ui->horizontalScrollBar_3, SIGNAL(valueChanged(int)),
                desLrc, SLOT(foreColor2Changed(int)) );
    connect( ui->horizontalScrollBar_4, SIGNAL(valueChanged(int)),
                desLrc, SLOT(bkCOlor0Changed(int)) );
    connect( ui->horizontalScrollBar_5, SIGNAL(valueChanged(int)),
                desLrc, SLOT(bkCOlor1Changed(int)) );
    connect( ui->horizontalScrollBar_6, SIGNAL(valueChanged(int)),
                desLrc, SLOT(bkCOlor2Changed(int)) );

    lab0 = new colorLabel ;
    lab1 = new colorLabel ;
    lab2 = new colorLabel ;

    QVBoxLayout* vlay1 = new QVBoxLayout;
    vlay1->addWidget( lab0 );
    vlay1->addWidget( lab1 );
    vlay1->addWidget( lab2 );

    QVBoxLayout* vlay2 = new QVBoxLayout;
    vlay2->addWidget(ui->label_7);
    vlay2->addWidget(ui->label_8);
    vlay2->addWidget(ui->label_9);

    QVBoxLayout* vlay3 = new QVBoxLayout;
    vlay3->addWidget(ui->horizontalScrollBar_4);
    vlay3->addWidget(ui->horizontalScrollBar_5);
    vlay3->addWidget(ui->horizontalScrollBar_6);

    QHBoxLayout* h1 = new QHBoxLayout;
    h1->addLayout( vlay1, 1 );
    h1->addLayout( vlay2, 1 );
    h1->addLayout( vlay3, 9 );

    ui->groupBox->setLayout( h1 );
/*
    */

    /////////

     lab3 = new colorLabel;
     lab4 = new colorLabel ;
     lab5 = new colorLabel ;

    QVBoxLayout* vlay4 = new QVBoxLayout;
    vlay4->addWidget( lab3 );
    vlay4->addWidget( lab4 );
    vlay4->addWidget( lab5 );

    QVBoxLayout* vlay5 = new QVBoxLayout;
    vlay5->addWidget(ui->label);
    vlay5->addWidget(ui->label_2);
    vlay5->addWidget(ui->label_3);

    QVBoxLayout* vlay6 = new QVBoxLayout;
    vlay6->addWidget(ui->horizontalScrollBar);
    vlay6->addWidget(ui->horizontalScrollBar_2);
    vlay6->addWidget(ui->horizontalScrollBar_3);

    QHBoxLayout* h2 = new QHBoxLayout;
    h2->addLayout( vlay4, 1 );
    h2->addLayout( vlay5, 1 );
    h2->addLayout( vlay6, 9 );

    ui->groupBox_2->setLayout( h2 );

    ////
    /// \brief mainLay
    ///

    QVBoxLayout* mainLay = new QVBoxLayout;
    mainLay->addWidget( ui->groupBox );
    mainLay->addWidget( ui->groupBox_2 );
    mainLay->addWidget( ui->buttonBox );
    setLayout( mainLay );

}

desLrcDialog::~desLrcDialog()
{
    delete ui;
}

void desLrcDialog::on_horizontalScrollBar_4_valueChanged(int value)
{
    lab0->updateBackGround( value );
}

void desLrcDialog::on_horizontalScrollBar_5_valueChanged(int value)
{
    lab1->updateBackGround( value );
}

void desLrcDialog::on_horizontalScrollBar_6_valueChanged(int value)
{
    lab2->updateBackGround( value );
}

void desLrcDialog::on_horizontalScrollBar_valueChanged(int value)
{
    lab3->updateBackGround( value );
}

void desLrcDialog::on_horizontalScrollBar_2_valueChanged(int value)
{
    lab4->updateBackGround( value );
}

void desLrcDialog::on_horizontalScrollBar_3_valueChanged(int value)
{
    lab5->updateBackGround( value );
}




//////////////////////



colorLabel::colorLabel(QWidget *parent):
    QLabel(parent),
    hsv(80)
{

}

void colorLabel::updateBackGround(int v)
{
    hsv = v;
    update();
}



void colorLabel::paintEvent(QPaintEvent *)
{
    QPainter paint(this);
    QColor color;
    color.setHsv( hsv,255,255);

    paint.fillRect( rect(), QBrush(color) );
}

////////////////


