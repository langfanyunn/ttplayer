#ifndef PLAYLIST_H
#define PLAYLIST_H

//#include <QMouseEvent>
//#include <QWidget>
#include <QMenuBar>
#include <QListWidget>  // 如果是申明的话，貌似检测不到类里面的函数！！？
#include <QTableWidget>
#include <QStackedWidget>
#include <QSplitter>
#include <QMultiMap>
#include "noframewidget.h"
#include "mypushbutton.h"

#define LRCDIR      "./lyric/"      // 避免用 lrc。万一 remove(right(3)) 的时候如果扩展名也是 lrc，那么前面的lrc将被移除

// 播放顺序
#define SINGLE  0
#define SINGLECIRCLE    1
#define ORDER   2
#define OPPO    3   // 逆序
#define ORDERCIRCLE 4
#define OPPOCIRCLE  5
#define RAND    6

//enum List_Kind {L_play = 0, L_song };   // 创建列表的时候记录是播放列表还是歌曲列表

class mySplitter;
class myListWidget;
class myTableWidget;
class QMediaPlayer;
class QListWidgetItem;

class playList : public noFrameWidget
{
    Q_OBJECT
public:
    explicit playList(QMediaPlayer*, QWidget *parent = 0);
    void setSkin(int index);    // 与 myLrcWnd 类共用一个静态数据

    void playPrev();    // 供主窗口上一首按钮的槽里面调用
    void playNext();    // 供主窗口下一首按钮连接的槽调用
    void autoNext();        // 用于自动播放下一首

    // 右击删除列表调用、单击菜单栏删除列表菜单也调用(通过槽)
    void deletePlayListItem(QListWidgetItem*);      // 删除某个播放列表项目

    /// 在桌面歌词工具窗右键菜单内调用
    /// 将该菜单添加进右键菜单
    QMenu* getPlayModeMenu()
    {
        return menu[6];
    }

    QString getCurPlayingSongName()
    {
        return curPlayingSongName;
    }

    /// 搜索结果右击菜单调用 brower 的函数
    /// brower 同名函数调用又调用这个函数
    void addToPlayList(QList<QListWidgetItem*>);

signals:

public slots:
    void showAddMenu();
    void showDelMenu();
    void showListMenu();       // 弹出菜单栏的"列表"菜单时，禁用一些不能用的菜单
    void showSortMenu();
    void showFindMenu();
    void showEditMenu();
    void showModeMenu();

    void getOpenFile();
    void getOpenDir();      // 打开文件夹选择对话框

    void deleteCurSong();   // 删除当前歌曲菜单
    void deleteAll();      // 全部删除菜单

    void cut();
    void copy();
    void paste();
    void oppoSel();
    void selAll();

    // 点击菜单里面的 "新建菜单" 时，连接该槽，
    // 右击新建菜单也调用该槽
    // 创建一个 播放的List的 item 和一个 歌曲的myListWidget
    void createPlayListItem();

    void delCurPlayListItem();      // 连接菜单“删除当前列表”

   // void mapToList();        // 将

    // 判断 row 有歌曲则播放，没歌曲返回不播放
    void playSong(const int& row);

    // 双击歌曲列表内某个项目时，连接该槽，里面调用playSong()
  //  void doubleClickSong(QListWidgetItem*);//改

    void doubleClickRow(int, int);

    /// 连接音乐搜索窗口内的双击事件：添加、播放操作
    void doubleClickBrowerItem(QListWidgetItem * item);

    // 不能在主窗口设置该槽，因为本类要储存当前的音量到成员 volume，
    // 根据当前音量设置下一首歌曲的音量
    void setMyVolume(int);  // 音量变化时连接该槽

    // 点击模式菜单某个菜单时，连接调用该槽
    void setPlayMode(QAction*); // 响应播放模式菜单设置播放顺序

    // 只能通过槽，储存进成员内了
 //   void getDuration(qint64);   // 貌似直接调用 duration() 函数获取时长无效之类的。

protected:
    void paintEvent( QPaintEvent* );
    void keyPressEvent2(QKeyEvent*);
    void resizeEvent(QResizeEvent *);

    //void eventFilter(QObject *, QEvent *);  // 过滤出播放列表、歌曲列表右键菜单

private:

    /// 在 playSong（） 内设置
    QString curPlayingSongName; // 记录当前正在播放的歌曲名。必须与路径对应

    // 构造函数内创建了
    myPushButton* closeButton;
    QMenuBar* menuBar;
    QMenu* menu[7];
    QAction *addingAction[2], *delAction[4], *listAction[2], *sortAction[4],
                *findAction, *editAction[6], *modeAction[7];
   // QAction* delCurListAction;  // 指向 Menubar 里面的 "删除当前列表"菜单

    int volume;     // 储存主窗口音量条的值，播放歌曲的时候后用到,连接音量信号valueChanged(int)

    // 在槽 setPlayMode() 修改
    // 槽 setPlayMode() 连接菜单栏上的模式菜单
    // playNext()\playPrev()\autoPlay()三个函数都根据该模式判断下一首播放的歌曲
    int play_mode;  // 播放顺序模式

    // 在 createList() 里面赋值 new
    myListWidget* pList;       // 是播放列表，非歌曲列表
    QStackedWidget* stackWidget;      // 用来装载所有的歌曲列表

    void createMenuBar();

    // 给成员 pList、stackWidget 赋值 new
    // 同时创建一个默认列表
    void createDefaultList();      // 创建内部两种列表

    // 打开文件\文件夹的时候将获取到的歌曲储存
  /// 去掉，歌曲重名时，添加进来会导致出现多个 int
   /// QMultiMap<QString, int> mapList;    // 麻痹，不支持 QFileInfo 作为 T?

    // 由主窗口构造函数内创建该类 new 内容的时候传递过来
    QMediaPlayer* player;   // 指向主窗口的成员数据 player
};

/// 播放列表、歌曲列表都由该类生成
/// 属性 playingRow 只用于歌曲列表，记录正在播放的歌曲所在行
class myListWidget : public QListWidget
{
    Q_OBJECT
  //  friend class playList;      // 这个友元类对该类的所有成员都可以直接操作????!!!
public:
    explicit myListWidget( QWidget* parent = 0);

private:

protected:
  void contextMenuEvent(QContextMenuEvent*);
};

///////////////////////////////////
/// \brief The myTableWidget class
///
class myTableWidget : public QTableWidget
{
    Q_OBJECT
public:
    myTableWidget(int rows, int columns, QWidget* parent = 0);

    int getPlayingRow() const;
    void setPlayingRow(int value);

    void deleteAction();
    void copyAction();
    void pasteAction();
   //直接有现成函数 void selectedAll();
    void oppoSelected();
    void oppoDelete();
    void clearList();

    static QList<QTableWidgetItem> plate;   // 前面是歌曲名，后面是时间

private:
    //换成行数，安全点，不然删除后指针指向的内存不存在了 QListWidgetItem* curPlayingItem;    // 记录当前播放的歌曲
    int playingRow;     // 当前播放的歌曲所在行

protected:
  void contextMenuEvent(QContextMenuEvent*);
};

////////////////////////////////
/// 用于鼠标进入修改鼠标形状
///
class mySplitter : public QSplitter
{
    Q_OBJECT
  //  friend class playList;
public:
    explicit mySplitter(QWidget* parent = 0);

protected:
    void enterEvent( QEvent* );
};


#endif // PLAYLIST_H
