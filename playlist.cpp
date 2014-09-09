#include "playlist.h"
#include "lrcdownloadthread.h"
#include <QPainter>
#include <QStyleOption>
#include <QDebug>
#include <QLayout>
#include <QFileDialog>
#include <QDir>
#include <QUrl>
#include <QTime>
#include <QLabel>
#include <QHeaderView>
#include <QMediaPlayer>
#include <QMessageBox>

playList::playList(QMediaPlayer *p, QWidget *parent) :
    noFrameWidget(parent),
    volume(50),
    play_mode(RAND),   // 顺序播放
    player(p)
{
    setMouseTracking( true );
    setObjectName( "play_list" );
    setWindowFlags( Qt::Tool | Qt::FramelessWindowHint );

    createMenuBar();
    createDefaultList();

    // 设置随机数种子
    qsrand( QTime(0,0,0).secsTo(QTime::currentTime()) );

    closeButton = new myPushButton(this);
    closeButton->setObjectName( "closeButton" );
    closeButton->setGeometry( 310 - 20, 2, 15, 15 );
    connect( closeButton, SIGNAL(clicked()),
                parent, SLOT(clickPlayListCloseButton()));
    show();
    activateWindow();   // 可以跟随父窗口显示在最前面！！！必须指定父窗口

 //   connect( player, SIGNAL(durationChanged(qint64)), this, SLOT(getDuration(qint64)) );
}

/// 主窗口 setSheet() 同时必须设置该值
void playList::setSkin(int index)
{
    noFrameWidget::skin_index = index;
    for( int i = 0; i < 7 && menu[i]; i++ )
    {
       if ( noFrameWidget::skin_index == 0 )     // 貌似设置了图标就不会显示文字了！！！
          menu[i]->setIcon(QIcon(QString("./res/skin1/%1.png").arg(i)) );
       else // 去掉图标
          menu[i]->setIcon(QIcon());
    }
}

void playList::playPrev()
{
    myTableWidget* currentList = static_cast<myTableWidget*>   // 当前显示的歌曲列表
                                    (stackWidget->currentWidget());
    if ( !currentList )
        return ;    // stackWidget 里面的歌曲列表被删除完了
    if ( currentList->rowCount() <= 0 )
        return ;        // 防止随机播放模式时，qrand() % 0 !！！！秒退

    int cur_row = currentList->getPlayingRow();
    cur_row = (cur_row - 1 >= 0 ) ? cur_row - 1 : currentList->rowCount() - 1;

    switch ( play_mode )
    {
    case SINGLE:
    case SINGLECIRCLE:
    case ORDER:
    case OPPO:
    case ORDERCIRCLE:
    case OPPOCIRCLE:
        playSong( cur_row ); // 貌似超出 item 返回 0 ！？？
        break;
    case RAND:
        playSong( qrand() % currentList->rowCount() );
        break;
    }
}

/// 处理点击下一首按钮，最后一首过后播放第一首
void playList::playNext()
{
    myTableWidget* currentList = static_cast<myTableWidget*>   // 当前显示的歌曲列表
                                    (stackWidget->currentWidget());
    if ( !currentList )
        return ;    // stackWidget 里面的歌曲列表被删除完了
    if ( currentList->rowCount() <= 0 )
        return ;        // 防止随机播放模式时，qrand() % 0 !！！！秒退

    int cur_row = currentList->getPlayingRow();
    cur_row = (cur_row + 1 < currentList->rowCount() ) ? cur_row + 1 : 0;

    switch ( play_mode )
    {
    case SINGLE:
    case SINGLECIRCLE:
    case ORDER:
    case OPPO:
    case ORDERCIRCLE:
    case OPPOCIRCLE:
        playSong( cur_row ); // 貌似超出 item 返回 0 ！？？
        break;
    case RAND:
        playSong( qrand() % currentList->rowCount() );
        break;
    }
}

void playList::autoNext()
{
    myTableWidget* currentList = static_cast<myTableWidget*>   // 当前显示的歌曲列表
                                    (stackWidget->currentWidget());
    if ( !currentList )
        return ;    // stackWidget 里面的歌曲列表被删除完了
    if ( currentList->rowCount() <= 0 )
        return ;        // 防止随机播放模式时，qrand() % 0 !！！！秒退

    // 获取之前播放的歌曲所在行
    int row = currentList->getPlayingRow();

    switch ( play_mode )
    {
    case SINGLE:
        break;
    case SINGLECIRCLE:
        playSong( row );
         break;
    case ORDER:
        playSong( row + 1 ); // 貌似超出 item 返回 0 ！？？
        break;
    case OPPO:
        playSong( row - 1 );    /// 因为不是循环，所以不判断越界
        break;
    case ORDERCIRCLE:
        playSong( (row + 1 < currentList->rowCount()) ? row + 1 : 0 );
        break;
    case OPPOCIRCLE:            // 逆序循环
        playSong( (row - 1 >= 0) ? row - 1 : currentList->rowCount() - 1 );
        break;
    case RAND:
        playSong( qrand() % currentList->rowCount() );
        break;
    }
}

/// 移除某个播放列表项目与其对应的 stack 子部件
void playList::deletePlayListItem(QListWidgetItem *item)
{
    if ( !item )
    {
        qDebug() << "当前没有列表，别删了！";
        return ;
    }
    int index = pList->row( item );     // 获取 item 所在行，从 0 计算
    QWidget* stack = stackWidget->widget( index );  // 获取 stackWidget 里面相同索引的部件

    pList->takeItem( index );
    stackWidget->removeWidget( stack );
}

void playList::addToPlayList(QList<QListWidgetItem*> itemList)
{
    myTableWidget* currentList = static_cast<myTableWidget*>   // 当前显示的歌曲列表
                                    (stackWidget->currentWidget());
    if ( !currentList || itemList.isEmpty() )
        return ;

    int id = currentList->rowCount();
    foreach( QListWidgetItem* netItem, itemList )
    {
        /// 以据号为切割点，切割成字符串列表
        QStringList list     = netItem->whatsThis().split("。");
        if ( list.isEmpty() || list.size() != 9 )
            qDebug() << "playList::addToPlayList() 错误";

        QString songName     = list.at(0);    /// 获取歌名
        QString artistName   = list.at(1);    /// 获取艺术家
        QString albumName    = list.at(2);    /// 获取专辑
        QString songLink     = list.at(3);    /// 获取歌曲连接
        QString format       = list.at(5);        /// 获取格式

        int t = list.at(6).toInt();             /// 获取时间
        QTime time(0, t / 60, t % 60 );
        QString strTime = time.toString("mm:ss");

        QString rate = list.at(7);           /// 获取比特率
        qint64 size = list.at(8).toInt();    /// 获取大小

        /// toolTip
        QString tip = "歌名：" + songName + "\n歌手：" + artistName + "\n专辑："
                        + albumName + "\n格式：" + format + "\n时间："
                        + QString("%1分%2秒").arg(t / 60).arg(t % 60)
                        + "\n比特率：" + rate + "kb/s\n大小："
                        + QString("%1.%2 MB").arg(size/1000000).arg(size/10000%100);

        QTableWidgetItem* table = new QTableWidgetItem(artistName + " - " + songName);
        table->setData( Qt::StatusTipRole, songLink );
        table->setToolTip( tip );
        table->setIcon( QIcon("./res/song.png") );
        currentList->insertRow( id );
        currentList->setRowHeight(id, 18);
        currentList->setItem( id, 0, table );
        currentList->setItem( id++, 1, new QTableWidgetItem(strTime) );
    }
}

/// 没有列表可用，禁用俩个菜单
void playList::showAddMenu()
{
    bool disable = false;
    if ( pList->count() == 0 )
        disable = true;

    for ( int i = 0; i < 2; i++ )
        addingAction[i]->setDisabled( disable );
}

///
void playList::showDelMenu()
{
    myTableWidget* currentList = static_cast<myTableWidget*>(
                                        stackWidget->currentWidget());

    // 没有列表、或有列表但为空
    if ( pList->count() == 0 || (currentList != 0 && currentList->rowCount() == 0) )
    {
        for ( int i = 0; i < 4; i++ )
            delAction[i]->setDisabled(true);
    }
    else    // 有歌曲在里面
    {
        for ( int i = 0; i < 4; i++ )
            delAction[i]->setDisabled( false );
        if ( currentList->selectedItems().empty() )
            delAction[0]->setDisabled( true );  // 如果没有当前歌曲
    }
}

void playList::showListMenu()
{
    // 有时候不存在当前列表
    if ( !pList->currentItem() )
        listAction[1]->setDisabled( true );
    else
        listAction[1]->setDisabled( false );
}

void playList::showSortMenu()
{
    bool disable = false;
    myTableWidget* currentList = static_cast<myTableWidget*>(
                                        stackWidget->currentWidget());
    if ( currentList == 0 || (currentList != 0 && currentList->rowCount() == 0) )
        disable = true;

    for ( int i = 0; i < 4; i++ )
        sortAction[i]->setDisabled( disable );
}

void playList::showFindMenu()
{
    bool disable = false;
    myTableWidget* currentList = static_cast<myTableWidget*>(
                                        stackWidget->currentWidget());
    if ( currentList == 0 || (currentList != 0 && currentList->rowCount() == 0) )
        disable = true;
    findAction->setDisabled( disable );
}

void playList::showEditMenu()
{
    myTableWidget* currentList = static_cast<myTableWidget*>(
                                        stackWidget->currentWidget());

    // 如果没有列表、或有列表但是没有歌曲
    if ( currentList == 0 || (currentList != 0 && currentList->rowCount() == 0) )
    {
        for ( int i = 0; i < 6; i++ )
            editAction[i]->setDisabled( true );

        // 有列表、没有歌曲、剪切板有内容：启用粘贴菜单
        if ( currentList != 0 && myTableWidget::plate.empty() == false )
            editAction[2]->setDisabled( false );
    }
    else    // 如果有歌曲
    {
        // 是否有歌曲被选中
        editAction[0]->setDisabled( currentList->selectedItems().empty() );
        editAction[1]->setDisabled( currentList->selectedItems().empty() );
        editAction[2]->setDisabled( myTableWidget::plate.empty() ); // 剪切版有没有内容
        editAction[3]->setDisabled( currentList->selectedItems().empty() );
        editAction[4]->setDisabled( false );    // 反选
        editAction[5]->setDisabled( false );        //全选
    }
}

void playList::showModeMenu()
{
    bool disable = false;
    myTableWidget* currentList = static_cast<myTableWidget*>(
                                        stackWidget->currentWidget());
    if ( currentList == 0 || (currentList != 0 && currentList->rowCount() == 0) )
        disable = true;

    for ( int i = 0; i < 7; i++ )
        modeAction[i]->setDisabled( disable );
}

/// 选择文件，得到全路径文件名储存进 item 的 tooltip 属性里
void playList::getOpenFile()
{
   // myListWidget* currentList = static_cast<myListWidget*> (stackWidget->currentWidget());
   // qDebug() << currentList;    // stackWidget 里面的列表被删除完之后，返回 0

    myTableWidget* currentList = static_cast<myTableWidget*>
                                    (stackWidget->currentWidget());
    if ( !currentList )
    {
        QMessageBox::information( this, tr("error"), tr("当前没有可用列表..."),
                                            QMessageBox::Ok );
        return ;
    }

    static QString def = "e:";
    QStringList fileList = QFileDialog::getOpenFileNames( this, QObject::tr("文件对话框"),
                       def, tr("音频文件(*.mp3 *.wma *.wav)") );
   // foreach( QString str, fileList )    // 每个文件名有 逗号+空格 隔开
     //   qDebug() << str;      // 包含路径的文件名.扩展名

    // id 接着末尾序列号添加
    for ( int i = 0, id = currentList->rowCount(); i < fileList.size(); i++ )
    {
        QString temp = fileList.at(i);
        //QString s = QString("[xx:xx]%1. ").arg(++id) + QFileInfo(temp).fileName();
        //QListWidgetItem* item = new QListWidgetItem(s);
        QTableWidgetItem* item = new QTableWidgetItem(QFileInfo(temp).baseName() );

        item->setIcon( QIcon("./res/song.png") );
        item->setData( Qt::StatusTipRole, temp ); // 储存全路径文件名，用于播放
        currentList->insertRow( id );       // 在最后一行后面插入一行

        currentList->setRowHeight( id, 18 );
        currentList->setItem( id, 0, item );
        currentList->setItem( id++, 1, new QTableWidgetItem("xx:xx") );

        // 设置一首歌曲为当前歌曲
        if ( 1 == id && player->mediaStatus() == QMediaPlayer::NoMedia )
            player->setMedia( QUrl(temp) );
    }
}

/// 选择文件夹
void playList::getOpenDir()
{
    myTableWidget* currentList = static_cast<myTableWidget*>   // 当前显示的歌曲列表
                                    (stackWidget->currentWidget());
    if ( !currentList )
    {
        QMessageBox::information( this, tr("error"), tr("当前没有可用列表..."), QMessageBox::Ok );
        return ;
    }
    static QString defDirectory = "e:/music";   // 用来储存用户最近选择的那个目录

    // 参数：父窗口、标题、默认路径、只显示目录
    defDirectory = QFileDialog::getExistingDirectory( this, tr("选择音乐文件所在目录"),
                       defDirectory, QFileDialog::ShowDirsOnly );

    if ( defDirectory.isEmpty() )
        return ;

    /// 过滤指定歌曲文件
    QDir dir(defDirectory);
    QStringList fitler;
    fitler << "*.mp3" << "*.wma" << "*.wav";
    QList<QFileInfo> list = dir.entryInfoList(fitler);

    ///
    for ( int i = 0, id = currentList->rowCount(); i < list.size(); i++ )
    {
        QString temp = list.at(i).absoluteFilePath();   // 将绝对路径名转成 string
        if (list.at(i).isFile() )
        {
          //  QString s = QString("%1. [4:23]. ").arg(++id) + list.at(i).fileName();
          //  QListWidgetItem* item = new QListWidgetItem(s);
            QTableWidgetItem* item = new QTableWidgetItem(list.at(i).baseName());

            // 将全路径文件名储存进 tooltip 中，播放时用到
            item->setData( Qt::StatusTipRole, temp );
            item->setIcon( QIcon("./res/song.png") );
            currentList->insertRow( id );

            currentList->setRowHeight(id, 18);
            currentList->setItem( id, 0, item );
            currentList->setItem( id++, 1, new QTableWidgetItem("xx:xx") );

            // 设置一首歌曲为当前歌曲
            if ( 1 == id && player->mediaStatus() == QMediaPlayer::NoMedia )
                player->setMedia( QUrl(temp) );
        }
    }
}

///
void playList::deleteCurSong()
{
    myTableWidget* currentList = static_cast<myTableWidget*>   // 当前显示的歌曲列表
                                    (stackWidget->currentWidget());
    if ( !currentList )     // 照理不会，菜单显示前会被禁用
        return ;

    currentList->deleteAction();
}

void playList::deleteAll()
{
    myTableWidget* currentList = static_cast<myTableWidget*>   // 当前显示的歌曲列表
                                    (stackWidget->currentWidget());
    if ( !currentList )     // 照理不会，菜单显示前会被禁用
        return ;
    currentList->clearList();
}

void playList::cut()
{
    myTableWidget* currentList = static_cast<myTableWidget*>   // 当前显示的歌曲列表
                                    (stackWidget->currentWidget());
    if ( !currentList )     // 照理不会，菜单显示前会被禁用
        return ;
    currentList->copyAction();
    currentList->deleteAction();
}

void playList::copy()
{
    myTableWidget* currentList = static_cast<myTableWidget*>   // 当前显示的歌曲列表
                                    (stackWidget->currentWidget());
    if ( !currentList )     // 照理不会，菜单显示前会被禁用
        return ;
    currentList->copyAction();
}

void playList::paste()
{
    myTableWidget* currentList = static_cast<myTableWidget*>   // 当前显示的歌曲列表
                                    (stackWidget->currentWidget());
    if ( !currentList )     // 照理不会，菜单显示前会被禁用
        return ;
    currentList->pasteAction();
}

void playList::oppoSel()
{
    myTableWidget* currentList = static_cast<myTableWidget*>   // 当前显示的歌曲列表
                                    (stackWidget->currentWidget());
    if ( !currentList )     // 照理不会，菜单显示前会被禁用
        return ;
    currentList->oppoSelected();
}

void playList::selAll()
{
    myTableWidget* currentList = static_cast<myTableWidget*>   // 当前显示的歌曲列表
                                    (stackWidget->currentWidget());
    if ( !currentList )     // 照理不会，菜单显示前会被禁用
        return ;
    currentList->selectAll();
}

/// 点击菜单里面的 "新建菜单" 时，连接该槽，
/// 右击新建菜单也调用该槽
/// 创建一个 播放的List 和一个 歌曲的List
void playList::createPlayListItem()
{
    // 新建一个播放子项
    QListWidgetItem* newItem = new QListWidgetItem("新建列表");
    newItem->setFlags( newItem->flags() | Qt::ItemIsEditable | Qt::ItemIsSelectable );
    pList->addItem( newItem );
  //貌似切换不正常了  pList->setCurrentItem( newItem );   // 防止新建列表没有当前项目的情况

    // 新建一个歌曲属性的列表,必须指定这个父部件，后面用着呢！！！！
  /*  myListWidget* tempList = new myListWidget(L_song, stackWidget);
    tempList->setSpacing(2);
    tempList->setFocusPolicy( Qt::NoFocus );
    tempList->setSelectionMode( QAbstractItemView::ExtendedSelection );
    tempList->setFlow( QListView::TopToBottom );
    tempList->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );

    // 将歌曲属性的列表加入到 stack
    stackWidget->addWidget( tempList );
    connect( tempList, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
             this, SLOT(doubleClickSong(QListWidgetItem*)) );*/

    myTableWidget* songlist = new myTableWidget(0, 2, stackWidget);
    songlist->setSelectionBehavior(QAbstractItemView::SelectRows);  // 一行一行的选择
    songlist->setShowGrid( false );     // 隐藏网格
    songlist->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    songlist->verticalHeader()->setDefaultAlignment(Qt::AlignCenter);    // 受 boxModel 影响
    songlist->setEditTriggers(QAbstractItemView::NoEditTriggers);    // 禁用编辑
    songlist->setFocusPolicy( Qt::NoFocus );     // 去掉焦点虚框
    songlist->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);// 只延伸第一列
    songlist->horizontalHeader()->resizeSection(1, 40); // 第二列宽度，没设置延伸的话，保持不变
    songlist->horizontalHeader()->hide();      // 隐藏水平表头
    songlist->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );// 禁用水平滚动
    stackWidget->addWidget( songlist );
    connect( songlist, SIGNAL(cellDoubleClicked(int,int)),  // 双击个项目播放该行的歌曲
                this, SLOT(doubleClickRow(int,int)) );
}

/// 点击删除当前菜单时调用槽
void playList::delCurPlayListItem()
{
    /// 注意，有时候不存在当前列表
    /// 当弹出菜单时连接了一个槽，检测时候有当前列表，没有则禁用删除当前列表菜单
    QListWidgetItem* curItem = pList->currentItem();
    this->deletePlayListItem(curItem);
}

/// 播放被双击的项目歌曲、记录双击的歌曲项目(函数里面会恢复歌曲原来的颜色)、滚动显示该歌曲、高亮歌曲
void playList::playSong(const int &row)
{
    myTableWidget* currentList = static_cast<myTableWidget*>   // 当前显示的歌曲列表
                                    (stackWidget->currentWidget());
    QTableWidgetItem* item = currentList->item(row, 0);

    if ( !item || !currentList )    // 非循环时，会越界，直接停止播放歌曲了
        return ;

    curPlayingSongName = item->text();

    currentList->setPlayingRow(row);    // 记录正在播放的项目
    currentList->scrollToItem( item );  // 将正在播放的项目滚到视野内
    item->setTextColor( QColor(250, 113, 80) );

    // 直接进行下一首播放会自动停止之前的歌曲，状态stop->playing->stop->playing！？！？！
  //  if ( player->state() != QMediaPlayer::StoppedState )
    //会引起多余的信号    player->stop();

    /// 获取储存在项目里面的歌曲全路径名
    QString pathName = item->data(Qt::StatusTipRole).toString();//qDebug() << pathName;
    player->setMedia( QUrl(pathName) );
    player->setVolume( volume );
    player->play();
}

void playList::doubleClickRow(int row, int)
{
    playSong( row );
}

/// 连接音乐搜索窗口内双击某个歌曲事件
/// 将本类的指针 在主窗口构造函数传递给 音乐窗口类
void playList::doubleClickBrowerItem(QListWidgetItem * netItem )
{
    myTableWidget* currentList = static_cast<myTableWidget*>   // 当前显示的歌曲列表
                                    (stackWidget->currentWidget());
    if ( !currentList || !netItem )
        return ;

    /// 以逗号为切割点，切割成字符串列表
    QStringList list = netItem->whatsThis().split("。");
    if ( list.isEmpty() || list.size() != 9 )
        qDebug() << "playList::doubleClickBrowerItem() 错误！";
//    foreach(QString str, list)
 //       qDebug() << str;

    QString songName = list.at(0);    /// 获取歌名
    QString artistName = list.at(1);    /// 获取艺术家
    QString albumName = list.at(2);    /// 获取专辑
    QString songLink = list.at(3);    /// 获取歌曲连接
    QString lrcLink = list.at(4);
    QString format = list.at(5);    /// 获取格式
  //  qDebug() << songLink;

    int t = list.at(6).toInt();    /// 获取时间
    QTime time(0, t / 60, t % 60 );
    QString strTime = time.toString("mm:ss");

    QString rate = list.at(7);    /// 获取比特率
    qint64 size = list.at(8).toInt();    /// 获取大小

    QString tip = "歌名：" + songName + "\n歌手：" + artistName + "\n专辑："
                    + albumName + "\n格式：" + format + "\n时间："
                    + QString("%1分%2秒").arg(t / 60).arg(t % 60)
                    + "\n比特率：" + rate + "kb/s\n大小："
                    + QString("%1.%2 MB").arg(size/1000000).arg(size/10000%100);

    ////////////////
    /// 在最后面插入一个

    int id = currentList->rowCount();
    QTableWidgetItem* item = new QTableWidgetItem(artistName + " - " + songName);
    item->setToolTip( tip );
    item->setData( Qt::StatusTipRole, songLink ); // 储存全路径文件名，用于播放
    item->setIcon( QIcon("./res/song.png") );
    currentList->insertRow( id );       // 在最后一行后面插入一行

    currentList->setRowHeight( id, 18 );
    currentList->setItem( id, 0, item );
    currentList->setItem( id, 1, new QTableWidgetItem(strTime) );

    playSong( id );

    if ( !QDir().exists( LRCDIR ))
        QDir().mkdir( LRCDIR );
    lrcDownloadThread* download = new lrcDownloadThread(lrcLink,
                                          LRCDIR + artistName + " - " + songName + ".lrc" );
    download->start();
}
/*
/// playSong() 参数不对不能连接 QListWidgetItem 的双击信号，故此转换
void playList::doubleClickSong(QListWidgetItem *item)
{
    myListWidget* currentList = static_cast<myListWidget*>   // 当前显示的歌曲列表
                                    (stackWidget->currentWidget());
    playSong( currentList->row(item) );
}*/

///
void playList::setMyVolume(int v)
{
    volume = v;

    /// 不知停止的时候能设置音量吗！？！
    if ( player->state() != QMediaPlayer::StoppedState )
        player->setVolume( v );
}

/// 响应播放模式菜单
void playList::setPlayMode(QAction *a)
{
    play_mode = a->data().toInt();  // 制作菜单时，储存进来的值刚好与 play_mode 宏对应
}

/*
void playList::getDuration(qint64 vol)
{
    qDebug() << vol << " - " << player->duration();
}*/


void playList::paintEvent(QPaintEvent *)
{
    int w = width();
    int h = height();
    QPixmap pix;
    QPainter paint(this);

    switch ( noFrameWidget::skin_index )
    {
    case 0:
        pix.load("./res/skin1/playlist_skin.bmp" );
       // paint.drawPixmap( 0, 0, w, h, pix, 80, 60, 200, 96 ); 被列表部件代替了  // 拉伸中间部分铺满窗口

        paint.drawPixmap( 0, 0, pix, 0, 0, 80, 60 );            // 苗四个角
        paint.drawPixmap( 0, h-30, pix, 0, 156, 30, 30 );
        paint.drawPixmap( w-30, h-30, pix, 280, 156, 30, 30 );
        paint.drawPixmap( w-30, 0, pix, 280, 0, 30, 60 );

        paint.drawPixmap( 80, 0, w-110, 60, pix, 80, 0, 200, 60 );  // 上下左右
        paint.drawPixmap( 30, h-30, w-60, 30, pix, 30, 156, 250, 30 );
        paint.drawPixmap( 0, 60, 30, h-90, pix, 0, 60, 30, 96 );
        paint.drawPixmap( w-30, 60, 30, h-90, pix, 280, 60, 30, 96 );
        break;

    case 1:
        pix.load("./res/skin2/playlist_skin.bmp" );
       // paint.drawPixmap( 0, 0, w, h, pix, 80, 60, 177, 24 );   // 拉伸中间部分铺满窗口

        paint.drawPixmap( 0, 0, pix, 0, 0, 80, 60 );            // 苗四个角
        paint.drawPixmap( 0, h-30, pix, 0, 84, 30, 30 );
        paint.drawPixmap( w-30, h-30, pix, 257, 84, 30, 30 );
        paint.drawPixmap( w-30, 0, pix, 257, 0, 30, 60 );

        paint.drawPixmap( 80, 0, w-110, 60, pix, 80, 0, 177, 60 );  // 上下左右
        paint.drawPixmap( 30, h-30, w-60, 30, pix, 30, 84, 227, 30 );
        paint.drawPixmap( 0, 60, 30, h-90, pix, 0, 60, 30, 24 );
        paint.drawPixmap( w-30, 60, 30, h-90, pix, 257, 60, 30, 24 );
        break;
    }

  /*  QStyleOption opt;   /// 必须才能给窗口QWidget 设置背景图片！？！？？*
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);*/
}

void playList::keyPressEvent2(QKeyEvent *e)
{
    static int r = 0, g = 0, b = 0;
    static bool flag = false;

    switch ( e->key() )
    {
        case Qt::Key_Q:  r+=5;       break;
        case Qt::Key_W:  g+=5;       break;
        case Qt::Key_E:  b+=5;       break;
        case Qt::Key_A:  r-=5;       break;
        case Qt::Key_S:  g-=5;       break;
        case Qt::Key_D:  b-=5;       break;

    case Qt::Key_K: flag = !flag;   break;
    }
    qDebug() << r << " - "<< g << " - " << b;
    QString str;

  //  if (flag)
        str  = QString("QMenuBar#menuBar QMenu:selected{background-color: rgb(%1,%2,%3);}")
                .arg(r).arg(g).arg(b);
  //  else
//        str = QString("QMenuBar#menuBar QMenu::item{color: rgb(%1,%2,%3);}").arg(r).arg(g).arg(b);

        setStyleSheet( str );
}

/// 如果构造函数在 show() 之后才创建 closeButton，貌似会秒退
void playList::resizeEvent(QResizeEvent *)
{
    if ( closeButton )
        closeButton->setGeometry( this->width() - 20, 2, 15, 15 );
}


/// 菜单不可乱增加！！！有成员数组绑定着！！！！！！
void playList::createMenuBar()
{
    menuBar = new QMenuBar(this);
    menuBar->setObjectName("menuBar");
    QList<QString> list;

    list << "添加" << "删除" << "列表" << "排序" << "查找" << "编辑" <<"模式";

    for ( int i = 0; i < 7; i++ )
    {
        menu[i] = menuBar->addMenu(list.at(i));     /// 如果设置了 ico 就不会显示文本了
        menu[i]->setObjectName( QString("menu%1").arg(i) );
       // menu[i]->setIcon( QIcon(QString("./res/skin%1/%2.png").arg(skin+1).arg(i)) );
    }
    menuBar->move( 5, 23 );

    addingAction[0] = menu[0]->addAction( style()->standardIcon(QStyle::SP_FileIcon), "文件(&F)" );
    addingAction[1] = menu[0]->addAction( QIcon("./res/file.png"), "文件夹(&D)");
    connect( addingAction[0], SIGNAL(triggered()), this, SLOT(getOpenFile()) );
    connect( addingAction[1], SIGNAL(triggered()), this, SLOT(getOpenDir()) );
    connect(menu[0], SIGNAL(aboutToShow()), this, SLOT(showAddMenu()) );

    delAction[0] = menu[1]->addAction( "删除选中的文件" );
    delAction[1] = menu[1]->addAction( "重复的文件..." );
    delAction[2] = menu[1]->addAction( "错误的文件..." );
    menu[1]->addSeparator();
    delAction[3] = menu[1]->addAction( "全部删除..." );
    connect( delAction[0], SIGNAL(triggered()), this, SLOT(deleteCurSong()) );
    connect( delAction[3], SIGNAL(triggered()), this, SLOT(deleteAll()) );
    connect( menu[1], SIGNAL(aboutToShow()), this, SLOT(showDelMenu()) );
  //  connect( delAction[3], SIGNAL(triggered()), this, SLOT(clearList()) );

    listAction[0] = menu[2]->addAction( "新建列表..." );
    connect( listAction[0], SIGNAL(triggered()), this, SLOT(createPlayListItem()) );

    // 储存，在菜单弹出时检测是否禁用、、比如列表被删完了
    listAction[1] = menu[2]->addAction( "删除当前列表..." );
    connect( listAction[1], SIGNAL(triggered()), this, SLOT(delCurPlayListItem()) );
    connect( menu[2], SIGNAL(aboutToShow()), this, SLOT(showListMenu()) );

    sortAction[0] = menu[3]->addAction( "歌曲名称." );
    sortAction[1] = menu[3]->addAction( "歌曲时长..." );
    sortAction[2] = menu[3]->addAction( "文件大小...." );
    menu[3]->addSeparator();
    sortAction[3] = menu[3]->addAction( "随机打乱" );
    connect( menu[3], SIGNAL(aboutToShow()), this, SLOT(showSortMenu()) );

    findAction = menu[4]->addAction( "列表上直接输入" );
    menu[4]->setToolTip( "在播放列表内直接敲打搜索内容即可.." );
    connect(menu[4], SIGNAL(aboutToShow()), this, SLOT(showFindMenu()) );

    editAction[0] = menu[5]->addAction( "剪切(&T)" );
    editAction[1] = menu[5]->addAction( "复制(&C)" );
    editAction[2] = menu[5]->addAction( "粘贴(&P)" );
    editAction[3] = menu[5]->addAction("删除(&D)");
    menu[5]->addSeparator();
    editAction[4] = menu[5]->addAction( "反选(&O)" );
    editAction[5] = menu[5]->addAction( "全部选中(&A)" );

    editAction[0]->setIcon( QIcon("./res/cut.png") );
    editAction[1]->setIcon( QIcon("./res/copy.png") );
    editAction[3]->setIcon( QIcon("./res/delete.png") );

    connect( editAction[0], SIGNAL(triggered()), this, SLOT(cut()) );
    connect( editAction[1], SIGNAL(triggered()), this, SLOT(copy()) );
    connect( editAction[2], SIGNAL(triggered()), this, SLOT(paste()) );
    connect( editAction[3], SIGNAL(triggered()), this, SLOT(deleteCurSong()) );
    connect( editAction[4], SIGNAL(triggered()), this, SLOT(oppoSel()) );
    connect( editAction[5], SIGNAL(triggered()), this, SLOT(selAll()) );
    connect( menu[5], SIGNAL(aboutToShow()), this, SLOT(showEditMenu()) );

    QActionGroup* group = new QActionGroup(this);
    QString menuName[7] = {"单曲播放(&S)",  "单曲循环(&I)",   "顺序播放(&N)", "逆序播放(&M)" ,
                        "顺序循环(&C).", "逆序循环(&O)..", "随机播放(&R)..." };
    //QAction* tempAction;
    for ( int i = 0; i < 7; i++ )
    {
        modeAction[i] = group->addAction( menuName[i] );
        modeAction[i]->setCheckable( true );
        modeAction[i]->setData( i );     // 用于在信号 triggered() 区分用户点击的是哪个
        menu[6]->addAction( modeAction[i] );

        if ( i == 3 )
            menu[6]->addSeparator();
    }
    modeAction[6]->setChecked(true);
    connect(menu[6], SIGNAL(triggered(QAction*)), this, SLOT(setPlayMode(QAction*)) );
    connect(menu[6], SIGNAL(aboutToShow()), this, SLOT(showModeMenu()) );
}

/// 创建歌曲列表和播放列表
void playList::createDefaultList()
{
    /// 创建播放列表,必须指定父部件，
    /// 父部件最终将是 splitter，即使这里设置了 this
    pList = new myListWidget;   // 参数表示该列表用作播放列表
    pList->setFocusPolicy( Qt::NoFocus );       // 去掉焦点虚框
    pList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    pList->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );  // 禁用水平滚动
    pList->setEditTriggers( QAbstractItemView::SelectedClicked |    // 双击、多击都可以编辑
                           QAbstractItemView::DoubleClicked );

    QListWidgetItem* play_1 = new QListWidgetItem("默认列表");
    play_1->setFlags( play_1->flags() |     // 后面要设置每个项目自身可编辑才能进行编辑！！！！
                     Qt::ItemIsEditable | Qt::ItemIsSelectable );
    pList->addItem(play_1);      // 在末尾添加项目
    pList->setCurrentItem(play_1);

    /// 创建 stack 容乃歌曲列表
    stackWidget = new QStackedWidget;
    stackWidget->setObjectName( "playListStackWidget" );

    /// 使用 QTableWidget 制作歌曲列表部件 ①
    myTableWidget* songlist = new myTableWidget( 0, 2, stackWidget);
    songlist->setSelectionBehavior(QAbstractItemView::SelectRows);  // 一行一行的选择
    songlist->setShowGrid( false );     // 隐藏网格

    // 设置竖直表头的项目固定不可伸缩
    songlist->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    songlist->verticalHeader()->setDefaultAlignment(Qt::AlignCenter);    // 受 boxModel 影响
    songlist->setEditTriggers(QAbstractItemView::NoEditTriggers);    // 禁用编辑
    songlist->setFocusPolicy( Qt::NoFocus );     // 去掉焦点虚框
    songlist->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);// 只延伸第一列

    // 设置的列或行必须存在，否在内存不能为 write !!!
    songlist->horizontalHeader()->resizeSection(1, 40); // 第二列宽度，没设置延伸的话，保持不变
    songlist->horizontalHeader()->hide();      // 隐藏水平表头
    songlist->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );// 禁用水平滚动

    stackWidget->addWidget( songlist );
    connect( songlist, SIGNAL(cellDoubleClicked(int,int)),  // 双击个项目播放该行的歌曲
                this, SLOT(doubleClickRow(int,int)) );

    connect( pList, SIGNAL(currentRowChanged(int)), stackWidget, SLOT(setCurrentIndex(int)) );

    mySplitter* sp = new mySplitter;      // 默认是水平分裂的
    QHBoxLayout* mainLay = new QHBoxLayout;
    sp->setObjectName( "spliter" );
    sp->addWidget(pList);
    sp->addWidget(stackWidget);
    sp->setHandleWidth(4);          // 在脚本里面怎么写！？？！width？？
    sp->setStretchFactor(0, 1);     // 索引0、1处部件宽度比例为 1:3
    sp->setStretchFactor(1, 3);

    mainLay->addWidget( sp );
    mainLay->setContentsMargins( 7, 42, 7, 7 );
    setLayout(mainLay);
}











//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////



myListWidget::myListWidget(QWidget *parent):
    QListWidget(parent)
{
}
/*
void myListWidget::setPlayingRow(const int& row)
{
    if ( item(playingRow) ) // 恢复正常颜色，貌似要适应所有皮肤
        item(playingRow)->setTextColor(QColor(131, 132, 184));
    playingRow = row;
}

int myListWidget::getPlayingRow()
{
    return playingRow;
}*/

/// 右击时，判断成员 Listkind 是播放列表还是歌曲列表，弹出响应菜单
void myListWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu;     // 局部变量，貌似菜单上面的 QAction 也会跟着释放内存
    QAction* newList = menu.addAction("新建列表(&N)");
    QAction* delList = menu.addAction("删除列表(&D)");
    QAction* saveList = menu.addAction("保存列表(&S)");
    saveList->setIcon( QIcon("./res/save.png") );

    // 鼠标下面有项目启用删除菜单
    // 没有项目就禁用删除菜单
    QListWidgetItem* item = this->itemAt( event->pos() );
    delList->setDisabled( item == 0 );
    saveList->setDisabled( item == 0 );

    // 弹出菜单，它不会阻碍其它事件，也不会继续该函数下面的代码？！！直到菜单消失
    QAction* result = menu.exec(event->globalPos());

    if ( !parentWidget() || !parentWidget()->inherits("mySplitter") )
        qDebug() << "父部件bug  112  myListWidget::contextMenuEvent";

    playList* mainList = static_cast<playList*>(parentWidget()->parentWidget());

    if( !mainList || !mainList->inherits("playList"))
        qDebug() << "父部件bug  112  myListWidget::contextMenuEvent";

    if ( result == newList )
    {
        // playList::pList 被加到 QSpitter 里面，所以它父部件是 splitter
        // QSplitter 又被布局进 myWidget::playlist 。splitter的父部件又是 playlist

        mainList->createPlayListItem();
    }
    else if ( result == delList )
        mainList->deletePlayListItem( item );
}
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

QList<QTableWidgetItem> myTableWidget::plate;

myTableWidget::myTableWidget(int rows, int columns, QWidget* parent ):
    QTableWidget(rows, columns, parent),
    playingRow(0)
{}

///
int myTableWidget::getPlayingRow() const
{
    return playingRow;
}

///
void myTableWidget::setPlayingRow(int value)
{
    if ( item(playingRow, 0) ) // 只恢复歌曲名颜色
        item(playingRow, 0)->setTextColor(QColor(131, 132, 184));
    playingRow = value;
}

///
void myTableWidget::deleteAction()
{
    // 时长跟在歌曲名后面，必须两次循环删除一次
    QList<QTableWidgetItem*> list = this->selectedItems();
    for ( int i = 0; i < list.size(); i++ )
        if ( i % 2 == 0 )
           this->removeRow( this->row(list.at(i)) );
}

/// 复制操作：清空剪切板原来内容
void myTableWidget::copyAction()
{
    plate.clear();      // 或不会自动释放内存！？！？

    // 即使拷贝了指针列表，但是它们依然使用同一内存
    QList<QTableWidgetItem*> select = this->selectedItems();
    QTableWidgetItem copy;
    foreach( QTableWidgetItem* item, select )
    {
        copy = *item;
        plate.append(copy);
    }
}

/// 重新确定序列号
void myTableWidget::pasteAction()
{
    int id = this->rowCount();
    QTableWidgetItem temp;

   // foreach( QTableWidgetItem* item, plate )
    for(int i = 0; i < plate.size(); i++ )
    {
        if ( i % 2 == 0 )
        {
            this->insertRow(id);
            this->setRowHeight( id, 18 );
        }
        temp = plate.at(i);

        switch(i % 2)
        {
        case 0: // 歌曲名
            {
                QTableWidgetItem* newItem = new QTableWidgetItem( temp.text() );
                newItem->setData( Qt::StatusTipRole, temp.toolTip() );
                this->setItem( id, 0, newItem );
                break;
            }

        case 1: // 歌曲时长
            {
                QTableWidgetItem* newItem = new QTableWidgetItem( temp.text() );
                this->setItem( id++, 1, newItem );
                break;
            }
        }
    }
}

/*现成函数 selectAll
/// 先选中第一项(这样就相当于取消了其它被选的歌曲)
/// 切换模式，从第二项开始选完。因为该模式貌似会把选择的取消掉！！！
void myTableWidget::selectedAll()
{
    for ( int i = 0; i < this->rowCount(); i++ )
    {
        if ( i == 1 )
            this->setSelectionMode(QAbstractItemView::MultiSelection);
        this->selectRow( i );
    }
    this->setSelectionMode( QAbstractItemView::ExtendedSelection );
}*/

///
void myTableWidget::oppoSelected()
{
    this->setSelectionMode(QAbstractItemView::ContiguousSelection);
    for ( int i = 0; i < this->rowCount(); i++ )
        this->selectRow(i);
    this->setSelectionMode( QAbstractItemView::ExtendedSelection );
}

/// 反选后删除即可
void myTableWidget::oppoDelete()
{
    oppoSelected();
    deleteAction();
}

/// 清空方式不是一般方式
void myTableWidget::clearList()
{
    int rows = this->rowCount();
    for ( int i = 0; i < rows; i++ )
        this->removeRow( 0 );       // 因为移除一行后，序列号就会重排了，所以。。
}

///
void myTableWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu;
    QAction* del = menu.addAction("删除(&D)");
    QAction* copy = menu.addAction("复制(&C)");
    QAction* cut = menu.addAction("剪切(&T)");
    QAction* paste = menu.addAction("粘贴（&P)");
    menu.addSeparator();
    QAction* selectAll = menu.addAction("全选(&A)");
    QAction* oppoSelect = menu.addAction("反选(&O)");
    QAction* oppoDel = menu.addAction("反选删除(&R)");
    menu.addSeparator();
    QAction* clear = menu.addAction("清空歌曲列表...(&E)");

    /// 是否点击在歌曲上
    QTableWidgetItem* item = this->itemAt( event->pos() );
    del->setDisabled( item == 0 );
    copy->setDisabled( item == 0 );
    cut->setDisabled( item == 0 );

    /// 是否是空列表
    int songNum = this->rowCount();
    selectAll->setDisabled( songNum == 0 );
    oppoSelect->setDisabled( songNum == 0 );
    oppoDel->setDisabled( songNum == 0 );
    clear->setDisabled( songNum == 0 );

    paste->setDisabled( plate.empty() );

    QAction* result = menu.exec(event->globalPos());

    /// 剪切镶嵌进下面两个动作
    if (result == copy || result == cut)     copyAction();
    if (result == del  || result == cut)      deleteAction();
    if (result == paste)        pasteAction();
    if (result == selectAll )   this->selectAll();//selectedAll();
    if (result == oppoSelect)   oppoSelected();
    if (result == oppoDel)      oppoDelete();
    if (result == clear)        clearList();
}




//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

mySplitter::mySplitter(QWidget *parent):
    QSplitter(parent)
{
}

/// 防止拉伸窗口后进入该部件未能恢复鼠标形状
void mySplitter::enterEvent(QEvent *)
{
    setCursor( Qt::ArrowCursor );
}

