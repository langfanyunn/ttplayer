#ifndef MYLRCWND_H
#define MYLRCWND_H

//#include <QWidget>
#include <QListWidget>
#include "mypushbutton.h"
#include "noframewidget.h"
//#include <QMap>
#include <QLabel>
#include <QMouseEvent>
#include <QTimer>
#include <QMediaPlayer>
#include <QResizeEvent>

#define LRCSTEP       17 // 当前行歌词跳动的振幅
#define LINEHEIGHT  30      // 每行歌词占据高度

#define LWB_ONTOP   0   // 置顶
#define LWB_TODES   1   // 切换到桌面歌词
#define LWB_CLOSE   2

class Dialog;       // 设置跳动歌词的颜色对话框
class myLabel;
class QString;
class QMediaPlayer;

class myLrcWnd : public noFrameWidget
{
    Q_OBJECT
public:
    explicit myLrcWnd( QMap<qint64, QString> *, QMediaPlayer*, QWidget *parent = 0 );

    // 使用该函数从 myWidget 类里面 update 歌词的 myLabel
    void seekProgress();        // 如果用户改变了歌曲进度

    /// 主窗口的 mediaChanged(...) 决定调用三种歌词里面的哪个该函数
    void mediaChanged();

signals:

public slots:

    // 连接歌曲播放状态
    // 暂停移动歌词、据需移动歌词
    void playerStateChanged( QMediaPlayer::State state );

    // 连接 player 的 currentMediaChanged() 信号
    // 用来初始化歌词的一些数据
    //void mediaChange(const QMediaContent & media);

    // 点击置顶按钮时
    void clickOnTop(bool);

protected:
    void paintEvent( QPaintEvent * );
    void resizeEvent(QResizeEvent *);

private:
    QMediaPlayer* player;

    myLabel* lrcLab;
  //  QMap<qint64, QString> lrcMap;

    myPushButton* lrcButton[3];
    void createButton();

   // void readLrc(const QString& songPathName);
};

class myLabel : public QLabel
{
    Q_OBJECT
public:
    myLabel( QMediaPlayer*, QMap<qint64, QString> *, QWidget* parent = 0);

    void seekLrc();

public slots:

    // 设置初始 y 值
    void init();
    void play();       // 媒体变成播放状态时调用
    void pause();           //
   // void stop();

    void timeOut();

    // 连接个计时器，检测当前行唱到哪里了
    void checkCurRow();

    void widdlyTimeOut();

    /// 设置 curColor 的颜色
    /// 连接了对话框的 spinBox
    /// 在对话框构造函数内连接的
    void HSlot(int);
    void SSlot(int);
    void VSlot(int);

    /// 歌词窗口关闭后停止所有计时器
    /// 重新显示后启用计时器、并重新设定当前行
 //   void setVisible(bool);增加该函数有bug，切换时 cpu 爆

protected:
    void enterEvent(QEvent*);
    void paintEvent(QPaintEvent *);
    void resizeEvent(QResizeEvent *);
    void contextMenuEvent(QContextMenuEvent*);

    /// 用来在隐藏窗口的时候停止歌词计时器
    void hideEvent(QHideEvent *);
    void showEvent(QShowEvent *);

private:
    QMediaPlayer* player;

    /// 构造函数传递过来
    QMap<qint64, QString> *map; // 指向 主窗口 成员数据 lrcMap

    int y;      // 第一行歌词所在左上角坐标
    int curRow;//, oldCurRow;     // 当前正在唱的行,从 0 开始

    /// 当前行上下跳过中使用
    int step;           // 跳动的第一针
    QTimer* widdlyTimer;    // 重绘跳动区域歌词
    QMap<int, int> wid;     // 前面 int 表示每行歌词每个字的索引，后面表示已经跳动了的次数

    QTimer* timer;
    QTimer* checkCurRowTimer;

    QColor _lrcFontColor;   // 非当前行歌词的颜色
    QFont _lrcFont;
    int   _lineHeight;   // 每行高度，根据该高度设置字体大小

    //
    int _H, _S, _V; // 当前行歌词跳动色调、饱和度、灰度
    Dialog* _lrcWidColorDlg;    // 用来修改上面那个颜色
};


#endif // MYLRCWND_H
