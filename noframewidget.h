#ifndef NOFRAMEWIDGET_H
#define NOFRAMEWIDGET_H

#include <QWidget>
#include <QMouseEvent>

#define PADDING 4

//class playList;

enum Direction{
    UP = 0,
    DOWN,
    LEFT,
    RIGHT,
    LEFTTOP,
    LEFTBOTTOM,
    RIGHTBOTTOM,
    RIGHTTOP,
    NONE
};

////////////////////////////////////////////////////
/// 该类用来生成一个无边框可以变化大小的窗口，共播放列表、歌词窗口继承
///
class noFrameWidget : public QWidget
{
    Q_OBJECT
  //  friend class playList;
    //friend class myLrcWnd;
public:
    explicit noFrameWidget(QWidget *parent = 0);
    static int skin_index;      // 播放列表、歌词窗口类共用数据

    /// 用在桌面歌词，伸缩窗口大小时脱离窗口不设置 _inset = false
    bool isDrag()
    {
        return drag;
    }

signals:

public slots:

protected:
    void mousePressEvent( QMouseEvent* );
    void mouseMoveEvent( QMouseEvent* );
    void mouseReleaseEvent(QMouseEvent*);

private:
    QPoint dragPosition;
    Direction dir;

    bool drag;

    void region( const QPoint& cursorGlobalPoint );

};

#endif // NOFRAMEWIDGET_H
