#ifndef MYSLIDER_H
#define MYSLIDER_H

#include <QSlider>

class mySlider : public QSlider
{
    Q_OBJECT
public:
    explicit mySlider(QWidget *parent = 0);
    mySlider(Qt::Orientation orientation, QWidget *parent);

signals:

public slots:
    void setmyRange(qint64 duration ); // 连接player歌曲时长改变信号
    void setmyPos(qint64 pos);      // 用歌曲进度设置进度条

protected:
    void enterEvent( QEvent* );
    void leaveEvent( QEvent* );

};

#endif // MYSLIDER_H
