#include "myslider.h"
#include <QDebug>

mySlider::mySlider(QWidget *parent) :
    QSlider(parent)
{
}

mySlider::mySlider(Qt::Orientation orientation, QWidget *parent):
    QSlider(orientation, parent)
{
}

/// 连接 player 信号 durationChange()
void mySlider::setmyRange(qint64 duration)
{
    // -2 为了保证能走到歌曲末尾
    setRange( 0, duration /1000 - 2 );   // 滑条的值是 int 型的。最大 65535，而歌曲时长是毫秒单位，可以越界
}

/// 连接 player 信号 positionChange()
void mySlider::setmyPos(qint64 pos)
{
    setValue( pos/1000 ); // 会引发 valueChanged() 信号，改信号又连接了歌曲进度跳跃
   // qDebug() << pos << " ---------- ";
}


void mySlider::enterEvent(QEvent *)
{
    setCursor( Qt::PointingHandCursor );
}

void mySlider::leaveEvent(QEvent *)
{
    setCursor( Qt::ArrowCursor );
}
