#include "brower.h"
#include "playlist.h"
#include "mypushbutton.h"
#include "downloadthread.h"
#include "downloaddialog.h"
#include "downloadtablewidget.h"
#include <QLayout>
#include <QIcon>
#include <QUrl>
#include <QMediaPlayer>
#include <QLabel>
#include <QMovie>
#include <QLineEdit>
#include <QHeaderView>
#include <QListWidgetItem>
#include <QJsonArray>
#include <QJsonObject>
#include <QMessageBox>
#include <QToolBar>
#include <QJsonValue>
#include <QJsonParseError>
#include <QNetworkRequest>
#include <QNetworkAccessManager>

int Brower::downloadId = 0;

Brower::Brower(playList *p, QWidget *parent) :
    noFrameWidget(parent),
    songIdIndex(0),
    playlist(p),
    reply(0),
    songIdReply(0)
{
    setMouseTracking(true);
    setObjectName( "brower" );
    setWindowFlags( Qt::Tool | Qt::FramelessWindowHint );
    setGeometry( 700, 100, 300, 500 );
    activateWindow();
    setMinimumSize(50, 50);

    manager = new QNetworkAccessManager(this);
    songIdManager = new QNetworkAccessManager(this);

    createStackWidget();
}

void Brower::play(QListWidgetItem *item)
{
    playlist->doubleClickBrowerItem( item );
}

void Brower::addToPlayList(QList<QListWidgetItem*> itemList)
{
    playlist->addToPlayList(itemList);
}

void Brower::searchXXXName(QString role)
{
    lineEdit->setText( role );
    startSearch();
}

//// 在搜索窗口内右击选中的项目
void Brower::downloadItem(QList<QListWidgetItem*> itemList)
{
    downloadDialog dlg;
    int result = dlg.exec();
    if ( result == QDialog::Rejected )
        return ;

    //stackWidget->setCurrentIndex(2);
    clickDownloadWnd(); // 代替上面进行切换，保证归一性

    foreach( QListWidgetItem* item, itemList )
    {
        QStringList list = item->whatsThis().split("。");
        QString downloadTaskName = list.at(1) + " - " + list.at(0); // 必须与新建任务时对应
        QList<QTableWidgetItem*> itemFindList
                                    = downloadWidget->findItems(
                                            downloadTaskName, Qt::MatchFixedString );
        if ( itemFindList.isEmpty() )
        {
            // 必须指定父窗口、线程类里面连接了本类的槽
            QString strId = QString("%1").arg(downloadId++);
            downloadThread* newDownload = new downloadThread( dlg.downloadLrc(), dlg.path(),
                                            strId, item, downloadWidget, this );
            newDownload->start();
            downloadHash.insert( strId, newDownload );
        }
        else
            QMessageBox::information( this, "提示", downloadTaskName
                                            + " 下载任务已经存在...", QMessageBox::Ok );
    }
}

/// 有任务完成，threadCounter--，此时不必判断 < 5
void Brower::hasTaskFinshed()
{
  //  downloadThread::threadCounter--;
    for ( int i = 0; i < downloadWidget->rowCount(); i++ )
    {
        if ( downloadWidget->item(i, 4)->text().contains("等待") )
        {
            // 更改状态，防止其他线程同时检测到同一个任务。导致同一个任务开启多个线程下载 bug
            downloadWidget->item(i, 4)->setText("正在下载");
            QList<QTableWidgetItem*> list;
            list << downloadWidget->item(i, 0) << downloadWidget->item(i, 2)
                    << downloadWidget->item(i, 3) << downloadWidget->item(i, 4);
            resumeDownload( list );
            break;
        }
    }
}

/// 直接供给下载窗口右击菜单调用
/// 双击任务时候也调用
void Brower::resumeDownload(QList<QTableWidgetItem *> itemList )
{
    for ( int i = 0; i < itemList.size(); i++ )
    {
        if ( i % 4 == 0 )   // 进度条算 item 但是不算 selectedItem !!!!!!!!!!
        {
            // 如果任务没有下载完成，才继续下载。不然会重复下载
            int row = downloadWidget->row( itemList.at(i) );
            if ( !downloadWidget->item(row, 4)->text().contains("完成") )
            {
                QString strId = QString::number(downloadId++);
                itemList.at(i)->setStatusTip( strId );

                // 必须置顶父窗口，线程类里面连接了本类的槽！！！
                downloadThread* newDownload = new downloadThread(
                                            itemList.at(i), downloadWidget, this );
                newDownload->start();

                if ( downloadHash.value(strId) )
                    qDebug() << "有暂停的任务线程没有被释放！！！！";

                // 如果存在会被覆盖
                downloadHash.insert( strId, newDownload );
            }
        }
    }
}

/// 下载窗口右击菜单调用
void Brower::downloadWndAction(int command, QStringList idStrList)
{
    foreach( QString id, idStrList )
    {
        /// 歌曲下载完后，不能在内部删除线程，因为 hash 里面的指针是不能用了的
        /// 但是它也不是零！！！!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!11
        downloadThread* thread = downloadHash.value(id);
        if ( thread )
        {
            switch ( command )
            {
            case DOWNLOAD_PAUSE:    thread->pauseTask();    break;
           //暂停的任务被释放掉了找不到线程的1！！ case DOWNLOAD_RESUME:   thread->resumeTask();   break;
            case DOWNLOAD_DELETE:
                thread->delTask();
                downloadHash.remove(id);
                break;
            }
        }
        else
            qDebug() << "任务线程已经不存在。。。。";
    }
}

/// 在下载窗口的鼠标点击事件内调用
/// 在切换到下载窗口的时候也调用
void Brower::releaseThread()
{
    if ( downloadHash.isEmpty() ){  qDebug() << "                    没有线程需要释放。。。。";
        return ;}

    /// 保证操作执行前释放一些 线程
    foreach ( downloadThread* dl, downloadHash )
    {
        if ( dl->getFinshed() )
        {
            QString key = downloadHash.key(dl);
            dl->deleteLater();
            downloadHash.remove(key);qDebug() << "释放 一个 线程了";
        }
    }
}

void Brower::clickSongerWnd()
{
    stackWidget->setCurrentIndex(0);
}

void Brower::clickSearchWnd()
{
    stackWidget->setCurrentIndex(1);
}

//// 进入下载窗口前，释放一些线程
void Brower::clickDownloadWnd()
{
    releaseThread();
    stackWidget->setCurrentIndex(2);
}

/// 下载状态直接判断字符串了
/// 双击正在下载的任务、暂停
/// 暂停的任务、继续
/// 等待的任务、暂停
void Brower::doubleClickTask(QTableWidgetItem *item)
{
    int row = downloadWidget->row( item );
    QString strId = downloadWidget->item(row, 0)->statusTip();
    QStringList list(strId);

    QString status = downloadWidget->item(row, 4)->text();

    // 如果正在下载就暂停，此时 downloadHash 容器里面还储存有线程指针
    if ( status.contains("下载") )
        downloadWndAction( DOWNLOAD_PAUSE, list );

    // 此时 downloadHash 里面已经没有对应 strId 的线程指针了。！不可以在用了
    if ( status.contains("等待") )
    {
        downloadWidget->item(row, 4)->setText( "暂停" );
        downloadWidget->item(row, 4)->setIcon( QIcon("./res/pause.png") );
    }

    if ( status.contains("暂停") )
        resumeDownload( downloadWidget->selectedItems() );

}


/// 连接搜索按钮
/// 连接 lineEdit 的回车信号(自定义的)
void Brower::startSearch()
{
    /// 正在搜索的时候，防止回车搜索
    if ( !searchButton->isEnabled() ){qDebug() << "别回车了！！！不能搜索";
        return ;}
    if ( lineEdit->text().isEmpty() )   // 如果没有内容搜索貌似会卡比
        return ;

    stackWidget->setCurrentIndex(1);        // 切换部件s
    searchButton->setDisabled( true );

    QUrl url = "http://mp3.baidu.com/dev/api/?tn=getinfo&ct=o&ie=utf-8&word="
                    + lineEdit->text() + "&format=json";
    if ( reply )
    {
       // reply->close();         // 貌似可以终止网络数据的传输
        reply->deleteLater();
        reply = 0;
    }
    reply = manager->get( QNetworkRequest(url) );
    connect( reply, SIGNAL(finished()), this, SLOT(searchFinshed()) );
    connect( reply, SIGNAL(error(QNetworkReply::NetworkError)),
                this, SLOT(replyError(QNetworkReply::NetworkError)) );
}

/// 在 startSearch() 内连接 reply 的 finished（） 信号
void Brower::searchFinshed()
{
    QVariant status_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);

    if(reply->error() == QNetworkReply::NoError)
    {
        QByteArray bytes = reply->readAll();  //获取字节
        QJsonParseError json_error;
        QJsonDocument parse_doucment = QJsonDocument::fromJson(bytes, &json_error );
        if(json_error.error != QJsonParseError::NoError)
            return ;
        if( !parse_doucment.isArray() )
            return ;

        songIdIndex = 0;
        songIdList.clear();     // 清空上次搜索到的歌曲 ids
        searchList->clear();        // 清空原先搜索结果

        QJsonArray array = parse_doucment.array();
        int size = array.size();
        for(int i=0; i < size; i++ )//////?????????
        {
            QJsonValue value = array.at(i);
            if( !value.isObject())
                continue ;

            QJsonObject object = value.toObject();
            if ( !object.contains("song_id") )
                 continue ;

            QJsonValue songid_value = object.take("song_id");
            if ( !songid_value.isString() )
                 continue ;

            QString song_id = songid_value.toString();
            if ( !song_id.isEmpty() )
                songIdList << song_id;
        }

        /// 如果没有搜索到 song_id，则重复在搜索几次，
        /// 如果超过 5 次还是没有结果就放弃
        static int counter = 5;
        if ( !songIdList.isEmpty() )
        {
            counter = 5;
            startSearchSongId();
        }
        else if ( counter-- > 0 )
        {qDebug() << "             --- " << counter;
            searchButton->setDisabled( false );     // 为了可以进入下面那个函数的内内部
            startSearch();
        }
        else
        {qDebug() << "5 次都失败！！！";
            counter = 5;
            searchButton->setDisabled( false );
        }
    }
}

/// 出现错误貌似不用删除 reply  ！！？！！？
void Brower::replyError(QNetworkReply::NetworkError )
{qDebug() << "网络连接异常Brower::replyError()！！！";
    searchButton->setDisabled( false );
}

/// 在 searchFinshed() 搜索完后调用该函数
void Brower::startSearchSongId()
{
    if ( songIdIndex < songIdList.size() )
    {
        QUrl url = "http://ting.baidu.com/data/music/links?songIds="
                        + songIdList.at(songIdIndex++);
        if ( songIdReply )
        {
            //songIdReply->abort();
            songIdReply->deleteLater();
            songIdReply = 0;
        }
        songIdReply = songIdManager->get( QNetworkRequest(url) );
        connect( songIdReply, SIGNAL(finished()), this, SLOT(songIdSearchFinshed()) );
        connect( songIdReply, SIGNAL(error(QNetworkReply::NetworkError)),
                    this, SLOT(songIdReplyError(QNetworkReply::NetworkError)) );
    }
    else
        searchButton->setDisabled( false );
}
/// 搜索完一个 song_id 后继续调用上面那个函数
void Brower::songIdSearchFinshed()
{
    QVariant status_code = songIdReply->attribute(QNetworkRequest::HttpStatusCodeAttribute);

    if( songIdReply->error() == QNetworkReply::NoError )
    {
        QByteArray bytes = songIdReply->readAll();  //获取字节
        QJsonParseError json_error; //qDebug() << bytes;
        QJsonDocument parse_doucment = QJsonDocument::fromJson(bytes, &json_error );
        if( json_error.error == QJsonParseError::NoError )
        {
            if ( parse_doucment.isObject() )
            {
                QJsonObject object = parse_doucment.object();   // 也可以用 toObject() 获取？？
                if ( object.contains("data") )
                {
                    QJsonValue data_value = object.take("data");
                    if ( data_value.isObject() )
                    {
                        QJsonObject data_object = data_value.toObject();
                        if ( data_object.contains("songList") )
                        {
                            QJsonValue songList_value = data_object.take("songList");
                            if ( songList_value.isArray() )
                            {
/* songList:[{....}] 表示一种数组 */
QJsonArray array = songList_value.toArray();
for ( int i = 0; i < array.size(); i++ )
{
    QJsonValue value = array.at(i);
    if ( !value.isObject() ){qDebug() << "解析数据错误5";
        continue ;}
    QJsonObject songList_object = value.toObject();

    /// 获取歌名
    QString songname = "";
    if ( songList_object.contains("songName") )
    {
        QJsonValue songname_value = songList_object.take("songName");
        if ( songname_value.isString() )
            songname = songname_value.toString();
    }
    /// 获取艺术家名
    QString artistname = "";
    if ( songList_object.contains("artistName") )
    {
        QJsonValue artistname_value = songList_object.take("artistName");
        if ( artistname_value.isString() )
            artistname = artistname_value.toString();
    }
    /// 获取专辑名
    QString albumname = "";
    if ( songList_object.contains("albumName") )
    {
        QJsonValue albumname_value = songList_object.take("albumName");
        if ( albumname_value.isString() )
            albumname = albumname_value.toString();
    }
    /// 获取歌曲时长
    double time = 0;
    if ( songList_object.contains("time") )
    {
        QJsonValue time_value = songList_object.take("time");
        if ( time_value.isDouble() )
            time = time_value.toDouble();
    }
    /// 歌曲格式
    QString format = "";
    if ( songList_object.contains("format") )
    {
        QJsonValue format_value = songList_object.take("format");
        if ( format_value.isString() )
            format = format_value.toString();
    }
    /// 获取比特率
    double rate = 0;
    if ( songList_object.contains("rate") )
    {
        QJsonValue rate_value = songList_object.take("rate");
        if ( rate_value.isDouble() )
            rate = rate_value.toDouble();
    }
    /// 获取歌曲大小
    double size = 0;
    if ( songList_object.contains("size") )
    {
        QJsonValue size_value = songList_object.take("size");
        if ( size_value.isDouble() )
            size = size_value.toDouble();
    }
    /// 获取歌曲连接
    QString songlink = "";
    if ( songList_object.contains("songLink") )
    {
        QJsonValue songlink_value = songList_object.take("songLink");
        if ( songlink_value.isString() )
            songlink = songlink_value.toString();
    }
    /// 获取歌词连接，歌词连接要加上一段地址才可以
    QString lrclink = "";
    if ( songList_object.contains("lrcLink"))
    {
        QJsonValue lrclink_value = songList_object.take("lrcLink");
        if ( lrclink_value.isString() )
            lrclink = "http://ting.baidu.com" + lrclink_value.toString();
    }

    QString itemstr = artistname + " - " + songname
                        + "\t\t《" + albumname + "》";
    QListWidgetItem* item = new QListWidgetItem(itemstr);
    item->setIcon( QIcon("./res/song.png"));
    searchList->addItem( item );

    item->setWhatsThis( makeString(songname, artistname, albumname, songlink,
                                        lrclink, format, time, rate, (int)size) );
  //  item->setToolTip(songlink);
}
                            }else   qDebug() << "解析数据错误4";
                        }else   qDebug() << "解析数据错误3";
                    }else   qDebug() << "解析数据错误2";
                }else   qDebug() << "解析数据错误1";
            }
        }
    }
//    qDebug() << "找到歌曲了！！";  // 回调函数会不会没完没了???!
    startSearchSongId();
    //为毛会一直输出这个？！？！难道这就是回调规则  qDebug() << "回调完了！！！";
}

void Brower::songIdReplyError(QNetworkReply::NetworkError)
{qDebug() << "找到一个错误的 song_id ！！";
    startSearchSongId();
}

///
void Brower::createStackWidget()
{
    ////////////////////////
    /// 创建搜索按钮、搜索编辑框、loading图标、关闭按钮。添加进一个布局中
    ///
    closeButton = new myPushButton;
    closeButton->setFocusPolicy( Qt::NoFocus );
    closeButton->setFixedSize(15,15);//>setGeometry( width() - 20, 5, 15, 15 );
    closeButton->setObjectName( "browerCloseButton" );
    connect( closeButton, SIGNAL(clicked()), this, SLOT(hide()) );

    lineEdit = new myLineEdit;
    lineEdit->setObjectName( "browerLineEdit" );
    connect( lineEdit, SIGNAL(startSearch()), this, SLOT(startSearch()) );

    searchButton = new myPushButton(QIcon("./res/find.png"), "搜索", this);
    searchButton->setFixedSize( 60, 20 );
    searchButton->setFocusPolicy(Qt::NoFocus);
    connect( searchButton, SIGNAL(clicked()), this, SLOT(startSearch()) );

    QHBoxLayout* hlay = new QHBoxLayout;
    hlay->addWidget( searchButton, 1 );
    hlay->addWidget( lineEdit, 6 );
    hlay->addStretch(3);
    hlay->addWidget(closeButton);

    ////////////////////
    /// 创建 第一页,歌手列表
    treeWidget = new myTreeWidget(stackWidget);
    treeWidget->setColumnCount( 3 );        // 必须设置！！！
    treeWidget->setFocusPolicy( Qt::NoFocus );  // 去掉焦点虚框
    treeWidget->header()->setSectionResizeMode(1, QHeaderView::Stretch);    // 设置延伸哪列
    treeWidget->header()->setStretchLastSection(false);
    treeWidget->header()->resizeSection(2, 40); /// 放在前面一行的后面才有效！？！！？

    // 表头
    QStringList headers;
    headers << "歌手" << "专辑" << "时间";
    treeWidget->setHeaderLabels(headers);
    treeWidget->header()->setDefaultAlignment( Qt::AlignCenter );

    // 添加一些歌手
    QStringList songerList;
    songerList << "张韶涵" << "张靓颖" << "水木年华" << "刘德华";
    for ( int i = 0; i < songerList.size(); i++ )
    {
        QTreeWidgetItem* temp = new QTreeWidgetItem( treeWidget );
        temp->setText( 0, songerList.at(i) );
        temp->setTextColor(0, Qt::green);
        temp->setFont(0, QFont("仿宋", 10));
        temp->setIcon( 0, QIcon("./res/singer.png"));

        QTreeWidgetItem* m = new QTreeWidgetItem(temp);
        m->setText(0, "我的最爱-与穆里奇.mp3" );
        m->setIcon( 0, QIcon("./res/song.png"));

        QTreeWidgetItem* n = new QTreeWidgetItem(temp);
        n->setText(0, "asdasdasd        --  asd" );
        n->setIcon( 0, QIcon("./res/song.png"));
    }

    /////////////////////////
    /// 创建第二页，搜索窗口
    searchList = new searchListWidget(stackWidget);
    searchList->setFocusPolicy( Qt::NoFocus );
    searchList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    searchList->setSpacing(3);
    searchList->setSelectionRectVisible(true);

    //////////////////////////
    /// 创建第三页，下载管理
    downloadWidget = new downloadTableWidget(0, 5, stackWidget);
    downloadWidget->setFocusPolicy( Qt::NoFocus );
    downloadWidget->horizontalHeader()->setFixedHeight(20);
    downloadWidget->horizontalHeader()->setStretchLastSection(true);
    downloadWidget->setShowGrid( false );
    downloadWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
   // downloadWidget->verticalHeader()->hide();
    downloadWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    downloadWidget->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    downloadWidget->setObjectName( "downloadWidget" );
    connect( downloadWidget, SIGNAL(itemDoubleClicked(QTableWidgetItem*)),
                this, SLOT(doubleClickTask(QTableWidgetItem*)) );

    QStringList headList;
    headList << "文件名" << "进度" << "速度" << "大小" << "下载状态";
    downloadWidget->setHorizontalHeaderLabels(headList);

    ///////////////////////////////////////////////////////////
    /// 创建工具栏、与 downloadWidget 部件一起布局进一个部件
    QToolBar* downloadBar = new QToolBar;
    downloadBar->setFixedHeight(20);
    downloadBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    // 添加工具
    QAction* all = new QAction("全部", this);
    QAction* downing = new QAction("正在下载", this);
    QAction* done = new QAction("已完成", this);
    QAction* recycle = new QAction("垃圾箱", this);
    QAction* pause = new QAction("暂停下载", this);
    QAction* resume = new QAction("继续下载", this );
    QAction* del = new QAction("删除", this);

    QMenu* menu1 = new QMenu;
    menu1->addAction("暂停所有任务");
    pause->setMenu( menu1 );

    QMenu* menu3 = new QMenu;
    menu3->addAction("开始所有任务");
    resume->setMenu( menu3 );

    QMenu* menu2 = new QMenu;
    menu2->addAction("清空当前任务列表");
    menu2->addAction("清空所有任务...");
    del->setMenu( menu2 );

    all->setIcon( QIcon("./res/all.png"));
    downing->setIcon( QIcon("./res/down.png") );
    done->setIcon( QIcon("./res/done.png"));
    recycle->setIcon( QIcon("./res/recycle.png") );
    pause->setIcon( QIcon("./res/pause.png"));
    resume->setIcon( QIcon("./res/goon.png"));
    del->setIcon( QIcon("./res/delete.png"));

    downloadBar->addAction(all);
    downloadBar->addAction(downing);
    downloadBar->addAction(done);
    downloadBar->addAction(recycle);
    downloadBar->addSeparator();
    downloadBar->addAction(pause);
    downloadBar->addAction(resume);
    downloadBar->addAction(del);

    //////////////////////////////////////////
    /// downloadWidget 的父部件关系不可乱更改，右击菜单中用到！！！！
    //////////////////////////////////////////
    QVBoxLayout* vlay3 = new QVBoxLayout;
    vlay3->addWidget( downloadBar, 1);
    vlay3->addWidget( downloadWidget, 17);
    vlay3->setContentsMargins(0,0,0,0);
    vlay3->setSpacing(3);

    QWidget* toolTableWidget = new QWidget;
    toolTableWidget->setLayout( vlay3 );

    /////////////////////////////////////////////////////////
    /// 添加进 stack
    /// 创建容器部件
    stackWidget = new myStackedWidget(this);
    stackWidget->setObjectName("browerStackWidget");
    stackWidget->addWidget( treeWidget );
    stackWidget->addWidget( searchList );
    stackWidget->addWidget( toolTableWidget );//

    ///////////////////////////////
    /// 创建左边工具按钮，添加进一个竖直布局，再与 stackWidget 水平布局
    songerWnd = new myPushButton( QIcon("./res/web"), "", this );
    searchWnd = new myPushButton( QIcon("./res/searchWnd.png"), "", this);
    downloadWnd = new myPushButton( QIcon("./res/download.png"), "", this );

    songerWnd->setFocusPolicy( Qt::NoFocus );
    searchWnd->setFocusPolicy( Qt::NoFocus );
    downloadWnd->setFocusPolicy( Qt::NoFocus );

    songerWnd->setFixedWidth(25);
    searchWnd->setFixedWidth(25);
    downloadWnd->setFixedWidth(25);;

    songerWnd->setToolTip( "切换到网络歌手列表" );
    searchWnd->setToolTip( "打开搜索窗口" );
    downloadWnd->setToolTip( "打开下载管理" );

    connect( songerWnd, SIGNAL(clicked()), this, SLOT(clickSongerWnd()) );
    connect( searchWnd, SIGNAL(clicked()), this, SLOT(clickSearchWnd()) );
    connect( downloadWnd, SIGNAL(clicked()), this, SLOT(clickDownloadWnd()) );
    connect( searchList, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
                playlist, SLOT(doubleClickBrowerItem(QListWidgetItem*)) );

    QVBoxLayout* vlay = new QVBoxLayout;
    vlay->addWidget(songerWnd);// 三个按钮设置了固定宽度
    vlay->addWidget(searchWnd);
    vlay->addWidget(downloadWnd);
    vlay->addStretch(4);
    QHBoxLayout* hlay2 = new QHBoxLayout;
    hlay2->addLayout(vlay, 1);
    hlay2->addWidget( stackWidget, 8 );
    hlay2->setSpacing(3);

    //////////////////////////
    /// \brief mainlay
    ///
    QVBoxLayout* mainlay = new QVBoxLayout;
    mainlay->addLayout( hlay );
    mainlay->addLayout( hlay2 );
    mainlay->setContentsMargins(4, 1, 4, 4);
    setLayout( mainlay );
}

QString Brower::makeString(QString songName, QString artist, QString album,
                           QString songLink, QString lrcLink, QString format,
                           double time, double rate, int size)
{
    if ( format.isEmpty() )
        qDebug() << "获得的歌曲格式信息为空，消息出 bug Brower::makeString()";

    /// 不能用逗号作为分割点，有的歌曲的歌手会有多个，会有逗号出现！！
    QString str = songName + "。" + artist + "。" + album + "。" + songLink + "。"
                    + lrcLink + "。" + format + "。" + QString("%1。").arg(time)
                    + QString("%1。").arg(rate) + QString("%1").arg(size);

    if ( str.split("。").size() > 9 )
        qDebug() << "Brower::makeString() 错误";
    return str;
}



////////////////////////////////////////////////////////////
/// \brief myTreeWidget::myTreeWidget
/// \param parent
///

myTreeWidget::myTreeWidget(QWidget *parent):
    QTreeWidget(parent)
{}


//////////////////////////////////////////////////////////
/// \brief myStackedWidget::myStackedWidget
/// \param parent
///
myStackedWidget::myStackedWidget(QWidget *parent):
    QStackedWidget(parent)
{}

void myStackedWidget::enterEvent(QEvent *)
{
    setCursor( Qt::ArrowCursor );
}

///////////////////////////////
/// \brief myLineEdit::myLineEdit
/// \param parent
///
myLineEdit::myLineEdit(QWidget *parent):
    QLineEdit(parent)
{
}

void myLineEdit::keyPressEvent(QKeyEvent *e)
{
    if ( e->key() == Qt::Key_Return )
        emit startSearch();
    QLineEdit::keyPressEvent( e );
}


////////////////////////////////////
/// \brief searchListWidget::searchListWidget
/// \param parent
///
searchListWidget::searchListWidget(QWidget *parent):
    QListWidget(parent)
{
}

void searchListWidget::contextMenuEvent(QContextMenuEvent *e)
{
    QMenu menu;
    QAction* play = menu.addAction("播放(&P)");
    QAction* add = menu.addAction("添加到播放列表(&A)");
    QAction* save = menu.addAction("保存当前搜索到网络刘表(&S)");

    menu.addSeparator();
    QAction* searchSong = menu.addAction("搜索： \"\"");
    QAction* searchArtist = menu.addAction("搜索： \"\"");
    QAction* searchAlbum = menu.addAction("搜索： \"\"");

    menu.addSeparator();
    QAction* download = menu.addAction("下载选中歌曲.....(&D)");

    /// 禁用某些菜单
    QListWidgetItem* item = this->itemAt( e->pos() );
    play->setDisabled( item == 0 );
    add->setDisabled( item == 0 );
    download->setDisabled( item == 0 );

    QString songName;
    QString artistName;
    QString albumName;
    if ( item )
    {
        QStringList list     = item->whatsThis().split("。");
        if ( list.size() > 9 )
            qDebug() << "歌曲自身信息内包含相同的分割点，分割时错误！！";

        songName     = list.at(0);    /// 获取歌名
        artistName   = list.at(1);    /// 获取艺术家
        albumName    = list.at(2);    /// 获取专辑

        searchSong->setDisabled( songName.isEmpty() );
        searchArtist->setDisabled( artistName.isEmpty() );
        searchAlbum->setDisabled( albumName.isEmpty() );

        searchSong->setText( "搜索歌曲:  \"" + songName + "\"");
        searchArtist->setText( "搜索歌手:  \"" + artistName + "\"" );
        searchAlbum->setText( "搜索专辑:  《" + albumName + "》" );
    }else
    {
        searchSong->setDisabled( true );
        searchArtist->setDisabled( true );
        searchAlbum->setDisabled( true );
    }

    /// 打开菜单
    QAction* result = menu.exec(e->globalPos());
    Brower* brow = static_cast<Brower*>(parentWidget()->parentWidget());

    if ( !brow->inherits("Brower") )
        qDebug() << "父部件造篡改错误！！searchListWidget::contextMenuEvent()";

    /// 处理菜单
    if ( result == play )
        brow->play(item);
    else if ( result == add )
        brow->addToPlayList(this->selectedItems());
    else if ( result == searchSong )
        brow->searchXXXName(songName);
    else if ( result == searchArtist )
        brow->searchXXXName( artistName );
    else if ( result == searchAlbum )
        brow->searchXXXName( albumName );
    else if ( result == download )
        brow->downloadItem(this->selectedItems());
}


/////////////////////////////////
/// \brief myTableWidget::myTableWidget
///


