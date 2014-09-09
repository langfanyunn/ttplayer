#include "mypushbutton.h"
#include <QDebug>

//PLAYSTATE myPushButton::playState = S_stop;
WINDOWMODE myPushButton::W_mode = W_main;
LRCMODE myPushButton::L_mode = L_wnd;
bool myPushButton::isLrcShow = true;

myPushButton::myPushButton(QWidget *parent) :
    QPushButton(parent)
{
}

myPushButton::myPushButton(const QIcon &icon, const QString &text, QWidget *parent):
    QPushButton(icon, text, parent)
{
}
/*
/// 从暂停变成播放状态，启用停止按钮，checked是暂停、非checked是播放状态
void myPushButton::setMyDisabled(bool able)
{
    if ( !able )    // 如果没有勾选=播放状态=暂停图标
        setDisabled(false); // 启用停止按钮
}*/
/*
/// 该槽貌似只限于停止按钮使用
void myPushButton::disabled()
{
    setDisabled(true);
    playState = S_stop;
}

/// 该槽只限于播放暂停按钮，将播放状态变成暂停状态，即按钮被 checked
void myPushButton::checked()
{
    setChecked(true);
    playState = S_stop;     // 注意是 stop！！！！
}*/


void myPushButton::enterEvent(QEvent *)
{
    setCursor( Qt::PointingHandCursor );
}

void myPushButton::leaveEvent(QEvent *)
{
    setCursor( Qt::ArrowCursor );
}
