#ifndef MYLRC_H
#define MYLRC_H

#include <QLabel>
#include <QTimer>
#include <QPoint>
#include <QMap>
#include <QMouseEvent>
#include "noframewidget.h"
#include <QMediaPlayer>
#include "mypushbutton.h"

#define DES_BUTTON_MAX      9
#define DB_MENU     0
#define DB_PAUSPLAY  1
#define DB_STOP     2
#define DB_PREV     3
#define DB_NEXT     4
#define DB_MUTE     5
#define DB_WND      6
#define DB_CLOSE    7
#define DB_ONTOP    8

class desLrcDialog;
class QMenu;
class toolWindow;

class myLrc : public noFrameWidget //QLabel
{
    Q_OBJECT
public:
    explicit myLrc(QMap<qint64, QString>*, QMediaPlayer*, QMenu*, QWidget *parent = 0);

    // 托盘菜单弹出前、工具窗口右击菜单弹出前检测
    bool isLock()
    {
        return _lock;
    }
    bool isTrans()
    {
        return _trans;
    }
    bool isDouLine()
    {
        return _douLine;
    }

    void seekProgress();

signals:

public slots:
    void mouseTracking();

    /// 连接托盘某个菜单，设置成员 _lock
    void setLock(bool);

    /// 给工具窗口内右键菜单调用
    void setTrans(bool);

    void setDouLine(bool);

    /// 受主窗口的 mediaChanged(..) 槽分配调用
    void mediaChange();

    /// 受 curTimer 计时器调用
    void checkCurRow();

    /// 更新遮罩位置
    void maskTimeEvent();

    /// 对应 xLeft、xleftTimer 计时器
    void xLeftEvent();

    /// 构造函数内连接
    void stateChanged(QMediaPlayer::State state);

    /// 点击工具窗的置顶按钮
    void clickOnTop(bool checked);

    /// 连接工具窗的字体选择菜单
    /// 在工具窗创建该菜单的时候连接
    void setDesFont(QAction*);

    /// 在desLrcDialog 类构造函数内连接
    void foreColor0Changed(int);
    void foreColor1Changed(int);
    void foreColor2Changed(int);
    void bkCOlor0Changed(int h);
    void bkCOlor1Changed(int h);
    void bkCOlor2Changed(int h);

    /// 在工具窗右击菜单调用
    /// 参数置顶要备份还是恢复
    void backUpColor(bool restore = false);     // 调用对话框设置歌词颜色前备份之前的颜色

protected:

    void paintEvent(QPaintEvent* );
    void wheelEvent(QWheelEvent*);
    void mousePressEvent3(QMouseEvent* event);
    void mouseMoveEvent3(QMouseEvent* event);

    void enterEvent(QEvent *);
    void leaveEvent(QEvent *);

    // desTool 跟随该窗口移动
    void resizeEvent(QResizeEvent *);
    void moveEvent(QMoveEvent *);

    void hideEvent(QHideEvent *);
    void showEvent(QShowEvent *);

private:
    QTimer* timer;      // 检测鼠标用

    QTimer* maskTimer;      // 歌词遮罩速度

    // 连接 checkCurRow() 槽
    QTimer* curTimer;   // 检测当前行的计时器

    QTimer* xleftTimer;     // 对应 xLeft 成员

    int curRow;

    // 歌词过长显示不完时用到
    int xLeft;      // 改变该值，控制歌词所在矩形移动
    int xMask;      // 当前遮罩到的位置
    QString curString;      // 储存当前歌词内容，在paintEvnet 内输出
    QString nextString;

    bool _lock, _trans; // 背景穿透
    bool _inside;       // 鼠标是否在桌面歌词窗口内
    bool _douLine;      // 双行显示歌词

    QMediaPlayer* player;

    /// 从主窗口构造函数内传递过来
    QMap<qint64, QString>* _desLrcMap;

    QFont font;
    QLinearGradient linearGradient;
    QLinearGradient linearGradient2;
    QColor foreColor[3], bkColor[3];      // 字体的前景、背景渐变色的范围

    QPoint offset;      // 鼠标单击歌词时，鼠标与框架左上角的相对位置

    toolWindow* desTool;
};

class toolWindow : public QWidget
{
    Q_OBJECT
public:
    toolWindow(QMediaPlayer*, QMenu*, QWidget* parent = 0);


protected:
    void paintEvent(QPaintEvent *);

    void mousePressEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);
    void wheelEvent(QWheelEvent*);
    void contextMenuEvent(QContextMenuEvent *);

public slots:
    void clickStop();
    void clickPausPlay(bool);
    void playStateChanged(QMediaPlayer::State);

    /// player 的静音状态被主窗口改变后
    /// 跟随改变该工具窗里的静音按钮
    void setMute(bool);

private:

    QMenu* _mainMenu;       // 指向主窗口的菜单
    QMediaPlayer* player;

    desLrcDialog* desDlg;

    bool drag;
    QPoint off_tool, off_lrc;

    myPushButton* desButton[DES_BUTTON_MAX];

    QString myStyleSheet();
    void createButton();

    /// 创建字体选择菜单，添加进右键菜单
    QMenu* fontMenu;
    void createFontMenu();
 //   void createContextMenu();       // 创建右击菜单给两个窗口用
};

#endif // MYLRC_H
