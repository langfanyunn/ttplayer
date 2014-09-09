#ifndef MYWIDGET_H
#define MYWIDGET_H

#include <QWidget>
#include <QState>
#include <QMap>
#include <QPainter>
#include <QMediaPlayer>
#include <QPushButton>
#include <QSystemTrayIcon>

#define B_STOP  0
#define B_PREV  1
#define B_PAUSPLAY  2
#define B_NEXT   3
#define B_OPEN  4
#define B_BROWER  5
#define B_LRC   6
#define B_EQ    7
#define B_PL    8
#define B_MUTE  9
#define B_MINI  10
#define B_MIN   11
#define B_CLOSE 12
#define MAIN_BUTTON_MAX 13



class myLrc;
class mySlider;
class playList;
class myLrcWnd;
class Brower;
class myPushButton;
class myMiniMainWnd;

namespace Ui {
class MyWidget;
}

class MyWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MyWidget(QWidget *parent = 0);
    ~MyWidget();

    /// 调用 playList 类的同名函数
    /// 在桌面工具窗右键菜单内调用
    QMenu *getPlayModeMenu();

    /// 共桌面两个窗口添加音量
    void setVolume(int dv);

private:
    Ui::MyWidget *ui;

    /// 在本类的 mediaChanged() 内设置
    QTimer* findLrcTimer;   // 播放网络歌曲时用于检测是否已经下载了歌词
  //  bool isGetLrc;  // 用在 checkLrcExsits()

    myLrc* lrcDes;
    playList* playlist;
    myLrcWnd* lrcWnd;
    myMiniMainWnd* miniMainWnd;
    Brower* brower;     // 网络音乐窗口

    mySlider* volume, *progress;
    QPoint offset_point;
    QSystemTrayIcon* trayicon;
    QPushButton* button[13];
    bool drag;      // 防止按住按钮移动到主窗口 bug
    QState* s1, *s2;
    QMediaPlayer player;

    /// 托盘图标菜单
    QMenu* trayMenu = NULL;     // 这样也行！？！？fuck
    QMenu* mainMenu;
    QAction* trayHide, *trayRestore;
    QAction* lockDesLrc, *showLrcDes;

    /// 储存歌词所有的时间、歌词
    /// 分别传递该成员的地址给窗口歌词、桌面歌词、迷你歌词
    QMap<qint64, QString> lrcMap;

    /// 读取歌词信息写进上面 那个 lrcMap 中
    void readLrc(const QString& songPathName);

    void createTrayMenu();
    void createTrayIcon();  // 创建托盘图标
    void createButton();
    void createHideShowState();/// 显示窗口、隐藏窗口时播放的动作
    void createMainMenu();      // 创建的是左上角图标的菜单

    void WriteIni();
    void setMainGeomety(const int &index);

protected:
    void closeEvent( QCloseEvent* );
    void paintEvent( QPaintEvent* );
    void mousePressEvent(QMouseEvent*);
    void mouseReleaseEvent(QMouseEvent*);
    void mouseMoveEvent(QMouseEvent*);

    void setVisible(bool);
    bool eventFilter( QObject*, QEvent* );

signals:

    // 两个信号只在 dispatch...() 里面被激发
    // 只在 createHideShowState() 里面关联状态
    void narrowHide();
    void enlargeShow();

public slots:

    /// 连接计时器 findLrcTimer
    /// 在 mediaChanged() nei
    void lrcIsDownloaded();

    /// 主要为了兼容网络歌曲的歌词显示
    /// 使用一个成员变量 isGetLrc 控制该函数内部的调用
  //  void checkLrcExsits(qint64);  // 连接 player 的positionChanged(信号，检测歌词是否存在。

    void dispatchSkin(QAction*);        // 判断用户点击的是哪个菜单，在调用 setSkin() 函数
    void setSkin(const int& index);
    void trayMouseHit( QSystemTrayIcon::ActivationReason );
    //void showLrc(bool);

    /// 点击关闭按钮、退出托盘的时候、附带检测是退出迷你窗口吗
    void opacityQuit();     // 启用下面事件计时器
    void quitTimer();     // 透明后关闭程序

    /// 所以最小化、还原操作多调用该槽(不管需不需要判断)，由槽内分配信号
    void dispatchInitNarrowHide(); // 点击托盘时，决定隐藏迷你、还是主窗口
    void dispatchEnlargeShow();     //

    void toMiniWnd();       // 点击迷你按钮，转换到迷你模式

    // 点击了桌面歌词的 切换到窗口 模式按钮，连接到该槽
    void clickToWnd();

    // 点击了窗口歌词的 切换桌面歌词按钮
    void clickDesLrcButton();
    void clickPlayListCloseButton();
    void lrcClose(); // 点击了窗口歌词的关闭按钮
    void setLrcShow(bool isShow);     // 用于连接 歌词 按钮，设定当前是否显示歌词
    void clickStop();       // 处理点击停止按钮后的事项
    void clickPausPlay(bool);       // 暂停/播放按钮

    void dispatchSliderAction(int);   // 单击/拖动进度条连接 progress 的 actionTriggred() 信号，跳跃进度
    void playerStateChanged( QMediaPlayer::State );// 播放完一首歌曲后

    /// player.setMedia() 后激发 currentMediaChanged() 信号，连接该槽
    void mediaChanged(const QMediaContent &media);

    void playPrev();
    void playNext();    // 处理点击下一首、自动进行下一首
    void isFinsh(int);  // 连接进度条 valueChanged() 信号，歌曲播放结束后调用上面那个playNext函数

    /// 连接 player 的 muteChanged() 信号改变按钮状态
    void setMuteButton(bool);

    /// 显示桌面歌词
    /// 窗口歌词、迷你歌词各种切换
    void trayShowDesLrc(bool checked);

    void clickBrower();

    void test(const QMediaContent &);
};

#endif // MYWIDGET_H
