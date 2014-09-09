#ifndef MYMINIMAINWND_H
#define MYMINIMAINWND_H

#include <QWidget>
#include <QMediaPlayer>

#define MB_STOP     0
#define MB_PREV 1
#define MB_PAUSPLAY 2
#define MB_NEXT 3
#define MB_OPEN 4
#define MB_MINI 5
#define MB_MIN  6
#define MB_CLOSE    7
#define MB_LRC  8
#define MB_MENU 9       // 图标主菜单
#define MINI_BUTTON_MAX 10

class myLrc;
class QState;
class QMenu;
class myPushButton;
class myMiniLrcWnd;
class playList;

class myMiniMainWnd : public QWidget
{
    Q_OBJECT
public:
    explicit myMiniMainWnd( QMap<qint64, QString>*, myLrc*, QMenu* mainMenu, QMediaPlayer*, QWidget *parent = 0);
  //  myMiniMainWnd(QWidget* pmain, QWidget *parent = 0);

    void setMiniGeometry(const int &index);     // 布局迷你窗口按钮的位置
    void createHideShowState();     // 放到 myWidget 构造函数调用

    /// 主窗口的 lrcClose() 函数内调用
    void lrcClose(bool);

    /// 在主窗口 clickToWnd() 内调用
    void desToMini();

    /// 迷你歌词窗口右键菜单切换到桌面歌词调用
    void miniToDesLrc();

    /// 主窗口的该函数内调用
    void mediaChanged();

signals:

    /// 只在 init...() 槽内激发
    void narrowHide();
    void enlargeShow();

public slots:
    void opacityQuit();
    void quitTimer();

    // 用在点击托盘显示、右击托盘还原是调用
    void initNarrowHide();  // 如果需要隐藏窗口就调用该函数，该函数内会激发 narrowHide() 信号，从而缩小窗口
    void initEnlargeShow();        // 用来在 myWidget 类里面激活 enlargeShow() 信号

    void toMainWnd();
    void setLrcShow(bool);      // 改变 myPushButton::sLrcShow 变量
    void clickStop();       // 处理点击停止按钮后的事项
    void clickPausPlay(bool);

    void playerStateChanged(QMediaPlayer::State state );

protected:
    void paintEvent(QPaintEvent*);
    void mousePressEvent(QMouseEvent*);
    void mouseMoveEvent(QMouseEvent*);
    void mouseReleaseEvent(QMouseEvent*);
    void wheelEvent(QWheelEvent*);

    void setVisible(bool visible);

private:
    QPoint offset;      // 记录鼠标相对窗口的位置

    myLrc* desLrc;      // 构造函数内传递过来
    myPushButton* miniButton[MINI_BUTTON_MAX];    // 不包图标
    myMiniLrcWnd* miniLrcWnd;
    bool drag;      // 防止按着按钮移动鼠标 bug
    QMenu* mainMenu;        // 左边图标菜单，有构造函数传递进来
    QMediaPlayer* player;       // 接受主窗口传递过来的地址

    QState* s1, *s2;
//    QWidget* mainWidget;    // 指向主窗口

    void createMiniButton();
};

#endif // MYMINIMAINWND_H
