#ifndef BROWER_H
#define BROWER_H
#include "noframewidget.h"
#include <QTreeWidget>
#include <QLineEdit>
#include <QHash>
#include <QTableWidget>
#include <QListWidget>
#include <QNetworkReply>
#include <QStackedWidget>
#include <QTreeWidgetItem>

class playList;
class downloadThread;
class downloadTableWidget;
class searchListWidget;
class myLineEdit;
class myTreeWidget;
class myPushButton;
class myStackedWidget;
class QNetworkAccessManager;

class Brower : public noFrameWidget
{
    Q_OBJECT
public:
    explicit Brower(playList*, QWidget *parent = 0);

    /// 给搜索窗口的右击菜单调用的
    void play(QListWidgetItem*);

    /// 从搜索窗口的右键菜单调用
    void addToPlayList(QList<QListWidgetItem*> );

    /// 这个函数也是给搜索窗口的右击菜单调用的
    /// 参数指定要搜索的歌名、歌手、还是专辑
    void searchXXXName(QString);

    /// 也是给搜索窗口的右键菜单调用的
    void downloadItem( QList<QListWidgetItem*> );

    /// 继续下载暂停的任务
    void resumeDownload( QList<QTableWidgetItem*>);

    /// 给下载窗口的右击菜单调用
    void downloadWndAction(int action, QStringList idStrList);

    /// 在下载窗口里有鼠标点击事件就检测是否有线程可以释放
    /// clickDownloadWnd() 内也可以检测，只要不是在另外以个线程内都可以！！！！！
    void releaseThread();

signals:

public slots:
    /// 分别连接底部三个按钮
    void clickSongerWnd();
    void clickSearchWnd();
    void clickDownloadWnd();

    /// 连接下载窗口 tableWidget 的双击信号
    /// 决定下载任务还是暂停任务
    void doubleClickTask(QTableWidgetItem*);

    /// 在 downloadThread 类构造函数内连接
    void hasTaskFinshed();

    /// 点击搜索按钮、按下回车后，开始搜索
    void startSearch();
    void searchFinshed();
    void replyError(QNetworkReply::NetworkError);      // 出现错误记得启用搜索按钮

    /// 在 searchFinshed() 结束后开始搜索得到的每一个 song_id
    void startSearchSongId();
    void songIdSearchFinshed();     // 每一次的搜索结束后，再回调上面那个函数进行下一次搜索
    void songIdReplyError(QNetworkReply::NetworkError);        // 某个 song_id 错误，跳过继续搜索其它

private:
    static int downloadId;  // 新建一次下载就 + 1

    /// 用来接受用户的下载路径、是否下载歌词....槽内设置void isDownloadLrc(bool);void downloadPath(QString);
  //  bool downloadLrc;
    //什么鸟！？增加这两行直接闪退！！！！？？QString downloadPath;

    /// 从 0 开始
    /// string 为下载任务每一行第一个 item 的 statusTipRole 储存的标识字符"0","1"....
    /// 在 downloadItem() 内记录
    QHash<QString, downloadThread*> downloadHash;   // 储存下载的任务

    int songIdIndex;        // 用来一次搜索下面列表里面的每一个 id

    /// 在 searchFinshed() 里面赋值
    QStringList songIdList;     // 储存第一次搜索得到的所以 song_id

    playList* playlist;     // 通过主窗口构造函数传递进来
    QNetworkAccessManager *manager, *songIdManager;
    QNetworkReply *reply, *songIdReply;

    myLineEdit* lineEdit;
    myPushButton* searchButton, *closeButton, *searchWnd,
                *songerWnd, *downloadWnd;

    myStackedWidget* stackWidget;
    myTreeWidget* treeWidget;
    searchListWidget* searchList;
    downloadTableWidget*  downloadWidget;

    /// 创建 stack 部件，和里面的两个大部件
    void createStackWidget();

    /// 将搜索到的歌曲信息做成一个 QString
    QString makeString(QString,QString,QString,QString,QString,QString,double,double,int);
};

/////////////////////////////////////////////
/// \brief The myTreeWidget class
///
class myTreeWidget : public QTreeWidget
{
    Q_OBJECT
public:
    explicit myTreeWidget( QWidget* parent = 0 );
};

////////////
/// \brief The myStackedWidget class
///
class myStackedWidget : public QStackedWidget
{
    Q_OBJECT
public:
    explicit myStackedWidget(QWidget* parent = 0 );

protected:
    void enterEvent(QEvent *);
};

/////////////////////////
/// \brief The myLineEdit class
///
class myLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    myLineEdit(QWidget* parent = 0);

signals:

    /// 在创建该类对象的时候连接
    void startSearch(); // 连接最上面那个类的 startSearch() 槽

protected:
    void keyPressEvent(QKeyEvent *);
};


//////////////////////////
/// \brief The searchListWidget class
///
class searchListWidget : public QListWidget
{
     Q_OBJECT
public:
    explicit searchListWidget(QWidget* parent = 0);

protected:
    void contextMenuEvent(QContextMenuEvent *);
};



#endif // BROWER_H
