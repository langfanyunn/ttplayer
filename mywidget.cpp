#include "mywidget.h"
#include "ui_mywidget.h"

#include "mylrc.h"
#include "brower.h"
#include "myslider.h"
#include "playlist.h"
#include "mylrcwnd.h"
#include "mypushbutton.h"
#include "myminimainwnd.h"

#include <conio.h>
#include <QPixmap>
#include <QBitmap>
#include <QDebug>
#include <QFileInfo>
#include <QPoint>
#include <QCursor>
#include <QMenu>
#include <QFile>
#include <QTimer>
#include <QRegExp>
#include <QString>
#include <QTextCodec>
#include <QSettings>
#include <QPushButton>
#include <QStateMachine>
#include <QDesktopWidget>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>


MyWidget::MyWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MyWidget),
   // isGetLrc(false),
    drag(false)             // 要放在 ui(new Ui::。。。后面？！？！
{
    ui->setupUi(this);

    /// 音量条部件
    volume = new mySlider( Qt::Horizontal, this );
    volume->setObjectName( "volume" );
    volume->setRange(0, 100);
    volume->setValue( 50 );
    volume->setFocusPolicy( Qt::NoFocus );

    /// 进度条部件
    progress = new mySlider( Qt::Horizontal, this );
    progress->setObjectName( "progress" );
    progress->setRange(0, 500);     // 后面可以用歌曲长度来设置？？？！！
    progress->setValue( 0 );
    progress->setFocusPolicy( Qt::NoFocus );

    /// 播放列表部件,必须在 miniMainWnd之前创建！！！
    /// 必须设置这个父窗口，里面的关闭按钮用到了
    /// brower 指针new 时用到该指针
    playlist = new playList(&player, this);        // 貌似指定 this 父窗口不能显示列表窗口
    //qDebug() << playlist;

    /// 歌词窗口，
    /// 必须指定这个父窗口，里面 createButton（）用到了！！！
    lrcWnd = new myLrcWnd(&lrcMap, &player, this );        // this 用于跟随主窗口前端显示
   // qDebug() << this << "    ----d";

WriteIni();

    createMainMenu();   // 貌似需要在迷你窗口创建之前创建，因为后面等着用 mainMenu

    /// 不可乱改位置，不然秒退
    /// createTrayMenu（） 里面使用了该指针连接槽
    /// 必须保证已经创建了。必须使用父窗口，连接desTool的按钮事件
    /// 传递该指针给了迷你窗口
    lrcDes = new myLrc(&lrcMap, &player, mainMenu, this);   // 指定这个后必须 activateWindow() 歌词窗口才能同步显示在最前面

    /// 用到了 lrcDes
    createTrayMenu();

    /// 用到了 createTrayMenu() 里面的 trayMenu
    createTrayIcon();
    createButton();

    /// 迷你主窗口，必须设置父窗口，后面使用了父窗口
    miniMainWnd = new myMiniMainWnd( &lrcMap, lrcDes, mainMenu, &player, this);

    /// 网络音乐窗口
    /// 必须创建按钮之前，按钮连接了该指针的槽
    brower = new Brower(playlist, this);

    // 去标题、去边框、保持菜单
    setWindowFlags( Qt::WindowMinimizeButtonHint |      // 启用最小化功能，但不显示按钮(因为有第FramelessWindowHint标志)
                    Qt::WindowSystemMenuHint |
                    Qt::FramelessWindowHint );
    setSkin(0);

    /// 确保设置皮肤、确定状态之后才创建状态
    createHideShowState();
    miniMainWnd->createHideShowState();

    /// 滤出滑轮事件来设置音量
    progress->installEventFilter( this );   // 必须安装才能用
    installEventFilter(this);       // 自监

    progress->setTracking( false ); // false=拖动释放后才更新 slider 值
    /// 连接按钮、音量之类的
   //整合进下面连接 connect( volume, SIGNAL(valueChanged(int)), &player, SLOT(setVolume(int)) );
    connect( volume, SIGNAL(valueChanged(int)),     // 交给playlist，顺便储存音量，下次播放用
                playlist, SLOT(setMyVolume(int)) );

    connect(&player, SIGNAL(durationChanged(qint64)),   // 重新确定进度条长度
                progress, SLOT(setmyRange(qint64)) );

    connect(&player, SIGNAL(positionChanged(qint64)),   // 歌曲进度控制滑块位置
                progress, SLOT(setmyPos(qint64)) );
 //   connect(&player, SIGNAL(positionChanged(qint64)),
   //             this, SLOT(checkLrcExsits(qint64)) );

    connect( progress, SIGNAL(actionTriggered(int)),    // 滑块控制歌曲进度
                this, SLOT(dispatchSliderAction(int)) );

    connect( progress, SIGNAL(valueChanged(int)),
                this, SLOT(isFinsh(int)) );        // 如果完毕播放下一首

    connect( &player, SIGNAL(stateChanged(QMediaPlayer::State)), // 歌曲状态设置按钮状态
                this, SLOT(playerStateChanged(QMediaPlayer::State)) );

    connect( &player, SIGNAL(currentMediaChanged(QMediaContent)),
                this, SLOT(mediaChanged(QMediaContent)) );

    /// 桌面歌词静音后，同时更改主窗口的静音按钮状态s
    connect( &player, SIGNAL(mutedChanged(bool)), this, SLOT(setMuteButton(bool)) );
}

MyWidget::~MyWidget()
{
    delete ui;
}

QMenu* MyWidget::getPlayModeMenu()
{
    return playlist->getPlayModeMenu();
}

///
void MyWidget::setVolume(int dv)
{
    volume->setValue( volume->value() + dv );
}

/// 在 mediaChanged() 内调用
void MyWidget::readLrc(const QString &songPathName)
{
    lrcMap.clear();   // 先清空以前的内容，如果有的话
    if(songPathName.isEmpty())    // 获取LRC歌词的文件名
        return ;

    QString fileName = songPathName;
    QString lrcFileName = fileName.remove(fileName.right(3)) + "lrc";

    // 打开歌词文件
    QFile file(lrcFileName);
    if (!file.open(QIODevice::ReadOnly)){qDebug() << "歌词不能打开！！MyWidget::readLrc";
        return ;}

    /////////////////////////////////////////////
    /// 读取文件头、判断编码、使用相应的编码读取才正确
    ///
    char c[3];
    file.read( c, 3 );

    QTextCodec *codec;
    if ( (int)c[0] == -1 && (int)c[1] == -2 )
        codec = QTextCodec::codecForName("UNICODE");
    else if ( (int)c[0] == -17 && (int)c[1] == -69 && (int)c[2] == -65 )
        codec = QTextCodec::codecForName("UTF-8");
    else
        codec = QTextCodec::codecForName("GBK");
  //  qDebug() << (int)c[0] << " - " << (int)c[1] << " - " << (int)c[2];

    file.seek(0);   // 必须
    QByteArray encodedString = file.readAll();
    QString allText = codec->toUnicode(encodedString);
    file.close();

    // 将歌词按行分解为歌词列表
    QStringList lines = allText.split("\n");

    // 使用正则表达式将时间标签和歌词内容分离
    QRegExp rx("\\[\\d{2}:\\d{2}\\.\\d{2}\\]");
    foreach (QString oneLine, lines)
    {
        // 先在当前行的歌词的备份中将时间内容清除，这样就获得了歌词文本
        QString temp = oneLine;
        temp.replace(rx, "");

        // 然后依次获取当前行中的所有时间标签，并分别与歌词文本存入QMap中
        int pos = rx.indexIn(oneLine, 0);
        while (pos != -1)
        {
            QString cap = rx.cap(0);
            // 将时间标签转换为时间数值，以毫秒为单位
            QRegExp regexp;
            regexp.setPattern("\\d{2}(?=:)");
            regexp.indexIn(cap);
            int minute = regexp.cap(0).toInt();
            regexp.setPattern("\\d{2}(?=\\.)");
            regexp.indexIn(cap);
            int second = regexp.cap(0).toInt();
            regexp.setPattern("\\d{2}(?=\\])");
            regexp.indexIn(cap);
            int millisecond = regexp.cap(0).toInt();
            qint64 totalTime = minute * 60000 + second * 1000 + millisecond * 10;
            // 插入到lrcMap中
            lrcMap.insert(totalTime, temp);
            pos += rx.matchedLength();
            pos = rx.indexIn(oneLine, pos);
        }
    }
}


//////////////////////////////////
/// 创建托盘菜单、连接槽
///
void MyWidget::createTrayMenu()
{    
    /// 创建顶级执行菜单
    trayRestore    = new QAction( "还原", this );
    trayHide       = new QAction( "隐藏", this );
    QAction* trayQuit   = new QAction( "退出", this );

    lockDesLrc = new QAction( "锁定桌面歌词", this );
    lockDesLrc->setCheckable( true );
    lockDesLrc->setChecked( lrcDes->isLock() );

    showLrcDes = new QAction("显示桌面歌词", this);
    showLrcDes->setCheckable( true );
    connect( showLrcDes, SIGNAL(triggered(bool)), this, SLOT(trayShowDesLrc(bool)) );

    connect( lockDesLrc, SIGNAL(toggled(bool)), lrcDes, SLOT(setLock(bool)) );
    connect( trayRestore,   SIGNAL(triggered()), this, SLOT(dispatchEnlargeShow()) );
    connect( trayHide,      SIGNAL(triggered()), this, SLOT(dispatchInitNarrowHide()) );
    connect( trayQuit,      SIGNAL(triggered()), this, SLOT(opacityQuit()) );// 透明退出

    /// 添加菜单图标
    trayHide->setIcon( style()->standardIcon(QStyle::SP_DialogResetButton) );
    trayQuit->setIcon( style()->standardIcon(QStyle::SP_DialogCloseButton) );
    trayRestore->setIcon( QIcon("./res/1.png") );
    lockDesLrc->setIcon( QIcon("./res/lock.png"));

    /// 将菜单设置给托盘
    trayMenu = new QMenu(this);

    trayMenu->addAction(showLrcDes);
    trayMenu->addAction(lockDesLrc);
    trayMenu->addSeparator();
    trayMenu->addAction(trayRestore);
    trayMenu->addAction(trayHide);
    trayMenu->addAction(trayQuit);
}

/////////////////////////
/// 创建托盘图标、添加菜单
///
void MyWidget::createTrayIcon()
{
    if ( trayMenu == NULL )     qDebug() << "菜单未初始化！！";

    trayicon = new QSystemTrayIcon(this);
    QIcon ico( "./res/TTPlayer.png" );

    trayicon->setContextMenu( trayMenu );
    trayicon->setIcon( ico );
    trayicon->setToolTip( " asdfasdfdsf" );
    trayicon->show();

    // 鼠标点击托盘图标响应的信号
    connect( trayicon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
                this, SLOT(trayMouseHit(QSystemTrayIcon::ActivationReason)) );
    setWindowIcon( ico );
}

//////////////////////////////
/// 创建主窗口 13 个按钮,有的皮肤12个按钮
///
void MyWidget::createButton()
{
/*    QRect rc[] = {	{10,136,35,35},     {52,136,35,35},	{94,128,50,50},	{151,136,35,35},
                    {193,136,35,35},    {238,133,64,21}, {267,100,31,13},
                    {228,100,31,13},    {190,100,31,13}, {187,75,17,14},
                    {244,5,17,15},      {265,5,17,15},   {286,5,17,15}       };*/

    for ( int i = 0; i < MAIN_BUTTON_MAX; i++ )
    {
        button[i] = new myPushButton( this );
        button[i]->setObjectName( QString("button%1").arg(i) ); //
        button[i]->setFocusPolicy( Qt::NoFocus );   // 去掉焦点虚框，美观
    }
/*    connect(button[B_STOP], SIGNAL(clicked()),
                button[B_STOP], SLOT(disabled()) );// 停止按钮
    connect(button[B_STOP], SIGNAL(clicked()),
                button[B_PAUSPLAY], SLOT(checked()) );*/
    connect( button[B_STOP], SIGNAL(clicked()), this, SLOT(clickStop()) );
 //播放歌曲的时候自动设置了   connect( button[B_PREV], SIGNAL(clicked()), this, SLOT(clickNextPrev()) );
  //貌似首歌曲状态调节了  connect( button[B_NEXT], SIGNAL(clicked()), this, SLOT(clickNextPrev()) );
    connect( button[B_PREV], SIGNAL(clicked()), this, SLOT(playPrev()) );
    connect( button[B_NEXT], SIGNAL(clicked()), this, SLOT(playNext()) );

  /*  connect(button[B_PAUSPLAY], SIGNAL(clicked(bool)),
                button[B_STOP], SLOT(setMyDisabled(bool)) );// 暂停按钮*/
    connect( button[B_PAUSPLAY], SIGNAL(clicked(bool)),
                this, SLOT(clickPausPlay(bool)) );

  //  connect(button[B_LRC], SIGNAL(clicked(bool)),
  //融进下面连接内！！！              lrcWnd, SLOT(setVisible(bool)) );
    connect(button[B_LRC], SIGNAL(clicked(bool)),   // 更改 isLrcShow 变量
                this, SLOT(setLrcShow(bool)) );

    connect(button[B_PL], SIGNAL(clicked(bool)),
                playlist, SLOT(setVisible(bool)) );

    connect( button[B_MUTE], SIGNAL(clicked(bool)),
                &player, SLOT(setMuted(bool)) );

    connect(button[B_MINI], SIGNAL(clicked()),
                this, SLOT(toMiniWnd()) );

    connect(button[B_MIN], SIGNAL(clicked()),
                this, SLOT(dispatchInitNarrowHide()) );

    connect(button[B_CLOSE], SIGNAL(clicked()),
                this, SLOT(opacityQuit()) ); // 透明退出

    connect(button[B_BROWER], SIGNAL(clicked()), this, SLOT(clickBrower()) );

    button[B_MINI]->setToolTip( "切换到迷你窗口模式" );
    button[B_MIN]->setToolTip( "最小化到托盘" );
    button[B_CLOSE]->setToolTip( "退出程序" );

    button[B_STOP]->setDisabled(true);       // 初始为停止状态

    // checked 对应 play 三角图标
    button[B_PAUSPLAY]->setCheckable(true);      // 非checked || 状态，启用停止按钮
    button[B_PAUSPLAY]->setChecked(true );
    button[B_LRC]->setCheckable( true );    // 歌词
    button[B_LRC]->setChecked( true );
    button[B_PL]->setCheckable( true );    // 播放列表
    button[B_PL]->setChecked( true );
    button[B_MUTE]->setCheckable( true );   // check = 静音
   // button[B_BROWER]->setCheckable(true);
}

void MyWidget::createHideShowState()
{
    static QStateMachine machine;
    s1 = new QState(&machine);
    s2 = new QState(&machine);

    /// 静态多次设置同属性的值，貌似会覆盖旧的
    /// 一个状态可以储存多个部件的多种属性
    s1->assignProperty( this, "geometry", this->geometry() );
    s1->assignProperty( playlist, "geometry", playlist->geometry() );
    s1->assignProperty( lrcWnd, "geometry", lrcWnd->geometry() );

    /// 缩小到右下角
    QPoint br = QApplication::desktop()->geometry().bottomRight() - QPoint(120,30);
    s2->assignProperty( this, "geometry", QRect(br, QSize(0,0)) );
    s2->assignProperty( playlist, "geometry", QRect(br, QSize(0,0)) );
    s2->assignProperty( lrcWnd, "geometry", QRect(br, QSize(0,0)) );

    /// 点击最小化按钮后开始缩小，缩小完之后隐藏
    s1->addTransition( this, SIGNAL(narrowHide()), s2 );
    s2->addTransition( this, SIGNAL(enlargeShow()), s1 );

    connect( s1, SIGNAL(entered()), this, SLOT(showNormal()) );
    connect( s2, SIGNAL(propertiesAssigned()), this, SLOT(hide()) ); // 完全转换为 s2 后

    QPropertyAnimation *animation1 = new QPropertyAnimation( this, "geometry" );
    QPropertyAnimation *animation2 = new QPropertyAnimation( playlist, "geometry" );
    QPropertyAnimation *animation3 = new QPropertyAnimation( lrcWnd, "geometry" );

    static QParallelAnimationGroup group;       // 必须保证数据在整个程序时间都有效
    group.addAnimation( animation1 );
    group.addAnimation( animation2 );
    group.addAnimation( animation3 );
    machine.addDefaultAnimation( &group );
    machine.setInitialState( s1 );
    machine.start();        // 不会立即播放转换，只会自动调节到 s1 状态
}

void MyWidget::createMainMenu()
{
    mainMenu = new QMenu(this);

    QMenu*   skinMenu       = new QMenu( "换肤" );    /// 创建皮肤弹出菜单
    QActionGroup* group_skin = new QActionGroup(this);  // 为了互斥菜单
    QAction* skin_kind[2];
    QString str[2] = { "眩紫诱惑", "绿色汪洋" };
    for ( int i = 0; i < 2; i++ )
    {
        skin_kind[i] = group_skin->addAction( str[i] );
        skin_kind[i]->setCheckable(true);
        skin_kind[i]->setData(i);                // 记录索引，用来在槽内区别用户点击的是哪个皮肤
        skinMenu->addAction( skin_kind[i] );
    }
    connect( skinMenu, SIGNAL(triggered(QAction*)), this, SLOT(dispatchSkin(QAction*)) );
    skinMenu->setIcon( QIcon("./res/skin.png") );
    skin_kind[0]->setChecked(true);
    skin_kind[0]->setIcon( QIcon("./res/skin0.png") );
    skin_kind[1]->setIcon( QIcon("./res/skin1.png") );
    skinMenu->setIcon( QIcon("./res/skin.png") );

    mainMenu->addMenu( skinMenu );
    myPushButton* menuButton = new myPushButton(QIcon("./res/TTPlayer.png"), "", this);
    menuButton->setFocusPolicy( Qt::NoFocus );
  //  menuButton->setStyleSheet( "QPushButton::menu-indicator{image: none;}");
    menuButton->setGeometry( 3, 3, 16, 16 );
    menuButton->setMenu(mainMenu);
}

void MyWidget::WriteIni()
{
   /* QRect rc[9] = { {150, 2, 24, 24}, {175, 2, 24, 24}, {200, 2, 24, 24},
                  {225, 2, 24, 24}, {-999, -999, 0, 0}, {255, 1, 17, 15},
                  {274, 1, 17, 15}, {292, 1, 17, 15}, {270, 15, 31, 11} };*/


    QSettings set( "config.ini", QSettings::IniFormat );
  /*  set.beginGroup( "buttonGroup1" );
    for (int i = 0; i < MAIN_BUTTON_MAX; i++ )
    {
        set.setValue( QString("%1").arg(i), rc[i] );
    }
    set.endGroup();*/

  /*  QRect rc[] = { {207, 77, 89, 9}, {197, 138, 82, 6} };
    QSettings setvol( "config.ini", QSettings::IniFormat );
    setvol.beginGroup( "volumePos" );
    for (int i = 0; i < 2; i++ )
        setvol.setValue( QString("%1").arg(i), rc[i] );
    setvol.endGroup();*/

  /*  QRect rc[] = { {10, 118, 292, 9 }, {8, 114, 181, 6} };
    QSettings setpro( "config.ini", QSettings::IniFormat );
    setpro.beginGroup( "progressPos" );
    for ( int i = 0; i < 2; i ++ )
        setpro.setValue( QString("%1").arg(i), rc[i] );
    setpro.endGroup();*/

   /* set.beginGroup( "miniButtonGroup1" );
    for ( int i = 0; i < 9; i++ )
        set.setValue(QString("%1").arg(i), rc[i] );
    set.endGroup();*/
}

//////////////////////////////////////
/// 读取主窗口各种部件坐标位置并设置
///
void MyWidget::setMainGeomety( const int& index )
{
    /// 读取按钮位置
    QRect rect;
    QSettings set( "config.ini", QSettings::IniFormat );
    set.beginGroup( QString("buttonGroup%1").arg(index) );
    for ( int i = 0; i < MAIN_BUTTON_MAX; i++ )
    {
        rect = set.value( QString("%1").arg(i) ).toRect();
        button[i]->setGeometry(rect);
    }
    set.endGroup();

    /// 音量条位置
    set.beginGroup( "volumePos" );
    volume->setGeometry( set.value(QString("%1").arg(index)).toRect() );
    set.endGroup();

    /// 进度条
    set.beginGroup( "progressPos" );
    progress->setGeometry( set.value(QString("%1").arg(index)).toRect() );
    set.endGroup();
}


///////////////////////////////////////
/// 读取皮肤、调用按钮位置设置函数
///
void MyWidget::setSkin(const int &index)
{
    setMainGeomety(index);          // 布局主窗口子部件位置
    miniMainWnd->setMiniGeometry(index);    // 布局迷你窗口按钮位置
    playlist->setSkin( index ); // 根据参数选择背景图片

    QString bk_skin   = QString("./res/skin%1/player_skin.png") .arg(index+1);
    QString sheet     = QString("../MusicWidget/skin%1.qss")    .arg(index+1);
    QString mini_skin = QString("./res/skin%1/mini_player.png") .arg(index+1);
    switch ( index )
    {
    case 0:
        playlist->resize(310, 186);
        lrcWnd->setGeometry( 200, 400, 310, 186 );
        break;

    case 1:
        playlist->resize(287, 114);
        lrcWnd->setGeometry( 200, 400, 310, 186 );
        break;
    default:
        qDebug() << "索引越界";
        break;
    }
    playlist->setMinimumSize(22, 22);   // 尽量最小，防止隐藏不会缩小
    lrcWnd->setMinimumSize(22, 22);

    /// 根据背景图片设置窗口形状
    QPixmap pix(bk_skin);
    setGeometry( 300, 200, pix.size().width(), pix.size().height() );
    resize(pix.size() );
    setMask( pix.mask() );
  //  setMinimumSize( pix.size() );   // 必须，不然隐藏状态下从绿色换成紫色皮肤，主窗口畸形
   // setMaximumSize( pix.size() );bug
 //   setFixedSize( pix.size() );bug  动作转换不会缩小！！

    QPixmap pix2(mini_skin);
    miniMainWnd->resize(pix2.size());// setGeometry( 300, 50, pix2.size().width(), pix2.size().height() );
    miniMainWnd->setMask( pix2.mask() );
   // miniMainWnd->setFixedSize( pix2.size() );
  //  miniMainWnd->setMinimumSize( pix2.size() );
  //  miniMainWnd->setMaximumSize( pix2.size() );

  //  playlist->setMask( pix2.mask() );     // mask 之后无法直接调节大小！！！！

    /// 读取样式脚本
    QFile file( sheet );
    if ( file.open(QFile::ReadOnly) )
    {
        QString styleSheet = file.readAll();
        qApp->setStyleSheet(styleSheet);
        file.close();
    }
}

///////////////////////////////
/// 使用主窗口菜单关闭程序时，不退出
///
void MyWidget::closeEvent(QCloseEvent *e)
{
    if ( trayicon->isVisible() )
    {
        trayicon->showMessage( "TTPlayer", "click to show MusicWindow",
                               QSystemTrayIcon::Information, 2000 );
        dispatchInitNarrowHide();     // 预备缩小隐藏
        e->ignore();
    }
}

/////////////////////////////////
/// MyWidget::paintEvent
///
void MyWidget::paintEvent(QPaintEvent *)
{
    QStyleOption opt;   /* 必须才能给窗口QWidget 设置背景图片！？！？？*/
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

/////////////////////////////////
/// 抓取窗口
///
void MyWidget::mousePressEvent(QMouseEvent *)
{
    drag = true;
  //  setCursor( Qt::ClosedHandCursor );
    offset_point = QCursor::pos() - geometry().topLeft();
    //  QWidget::mousePressEvent(e);
}

//////////////////////////////////
/// 释放拖拽
///
void MyWidget::mouseReleaseEvent(QMouseEvent *)
{
    drag = false;
  //  setCursor( Qt::ArrowCursor );
}

//////////////////////////////////
/// 移动窗口
///
void MyWidget::mouseMoveEvent(QMouseEvent *e)
{
    if ( (e->buttons() & Qt::LeftButton) && drag )
        move( e->globalPos() - offset_point );
}

/////////////////////////////////
/// 窗口被显示或隐藏时自动调用该函数
/// 保持其它窗口的同步显示
void MyWidget::setVisible(bool visible )
{
    /////////// 必须先设置按钮状态，才能根据按钮状态设置窗口状态
    if ( visible )
    {
        bool b_stop = false, b_paus = false;
        if ( player.state() == QMediaPlayer::StoppedState )    // 禁用停止按钮
            b_stop = b_paus = true;
        else if ( player.state() == QMediaPlayer::PausedState )   // 暂停状态、checked、播放图标
            b_paus = true;
        button[B_STOP]->setDisabled( b_stop );
        button[B_PAUSPLAY]->setChecked( b_paus );

        button[B_LRC]->setChecked( myPushButton::isLrcShow );

        if ( myPushButton::L_mode != L_des && myPushButton::isLrcShow )
            lrcWnd->setVisible( true );
    }
    else
        lrcWnd->setVisible( false );

    ////////// 根据按钮状态检测 ////////////

    bool listbool = false;//, lrcbool = false;
    if ( visible && button[B_PL]->isChecked() )
        listbool = true;
  //  if ( visible && button[B_LRC]->isChecked() )
  //      lrcbool = true;

//    lrcWnd->setVisible( lrcbool );
    playlist->setVisible( listbool );

    QWidget::setVisible( visible );     // 必须！！！
}

/// 过滤出主窗口和进度条的滑轮事件。用来设置音量
bool MyWidget::eventFilter( QObject *object, QEvent *event )
{
    if ( object == this || object == progress )
    {
        if ( event->type() == QEvent::Wheel )
        {
            QWheelEvent* wEvent = static_cast<QWheelEvent*>(event);;
            int dv = wEvent->delta() / 40;       // 120 或 -120
            volume->setValue( volume->value() + dv );       // 越界了也会自动处理！！
            return true;        // 表示人为处理了消息
        }
    }

    return QWidget::eventFilter(object, event);
}

void MyWidget::lrcIsDownloaded()
{
    QString curSong = playlist->getCurPlayingSongName();
    QString findPath = LRCDIR + curSong + ".lrc";
qDebug() << findPath;
    if ( QFile::exists(findPath) )
    {
        findLrcTimer->stop();
        readLrc( findPath );

        if ( myPushButton::L_mode == L_wnd )
            lrcWnd->mediaChanged();
        else if ( myPushButton::L_mode == L_des )
            lrcDes->mediaChange();
        else
            miniMainWnd->mediaChanged();
    }
}

/*在 构造函数内 连接
void MyWidget::checkLrcExsits(qint64)
{
    if ( !isGetLrc )
    {
        QString curSong = playlist->getCurPlayingSongName();// 显示在列表上的 "歌手 - 歌名"

        QString mediaStr = player.currentMedia().canonicalUrl().toString();
        QString temp;
        if ( QFile::exists(mediaStr) )  // 如果是本地音乐
        {
            temp = mediaStr;
            QString lrcName = temp.remove( temp.right(3) ) + "lrc";
            if ( QFile::exists(lrcName) )
            {
                isGetLrc = true;
                readLrc( mediaStr );    // 将找到的歌词文件读取进成员数据 lrcMap 中
            }
        }
    }
}
*/
///////////////////////////////////
/// 传递用户选择的皮肤
///
void MyWidget::dispatchSkin(QAction *action)
{
    setSkin( action->data().toInt() );
}

/////////////////////////////////
/// \brief MyWidget::trayMouseHit
///
void MyWidget::trayMouseHit(QSystemTrayIcon::ActivationReason reason )
{
    switch ( reason )
    {

    // 判断是显示还是隐藏，再调用函数决定显示隐藏哪个窗口
    case QSystemTrayIcon::Trigger:
        if ( (W_main == myPushButton::W_mode && isVisible()) ||
             (W_mini == myPushButton::W_mode && miniMainWnd->isVisible()) )
            dispatchInitNarrowHide();
        else
            dispatchEnlargeShow();
        break;

    // 弹出菜单前必须禁用不能点击的菜单，如果在隐藏状态下再隐藏，那么 s1 状态的 geometry 就变成0,0,0,0了
        // 导致无法还原显示窗口
    case QSystemTrayIcon::Context:
        if ( (W_main == myPushButton::W_mode && isVisible()) ||
            (W_mini == myPushButton::W_mode && miniMainWnd->isVisible()) )
        {
            trayRestore->setDisabled( true );
            trayHide->setDisabled( false );
           // skinMenu->setDisabled( false );
        }
        else
        {
            trayRestore->setDisabled(false);
            trayHide->setDisabled(true);
         //   skinMenu->setDisabled( true );
        }

        lockDesLrc->setChecked( lrcDes->isLock() );

        // 根据当前桌面歌词显示状态设置菜单 checked 与否
        showLrcDes->setChecked( myPushButton::isLrcShow
                                    && (myPushButton::L_mode == L_des) );
        break;
    default:
        break;
    }
}

/// 一律检测是哪个窗口的退出
void MyWidget::opacityQuit()
{
    if ( myPushButton::W_mode == W_main )
    {
        QTimer* t = new QTimer(this);
        connect( t, SIGNAL(timeout()), this, SLOT(quitTimer()) );
        t->start( 19 );
    }
    else
        miniMainWnd->opacityQuit();
}

void MyWidget::quitTimer()
{
    static qreal opacity = 1;
    opacity -= 0.03;
    if ( opacity <= 0 )
        qApp->quit();

    setWindowOpacity( opacity );
    playlist->setWindowOpacity( opacity );
    lrcWnd->setWindowOpacity( opacity );
    brower->setWindowOpacity( opacity );

    player.setVolume( opacity * volume->value() );  // 制作声音渐小退出
}

/// 有些隐藏操作不用判断，也一并判断了！！
/// 根据窗口模式决定激发主窗口信号、还是调用迷你槽函数 - 激活迷你窗口信号
void MyWidget::dispatchInitNarrowHide()
{
    if ( W_main == myPushButton::W_mode )   // 如果是主窗口模式
    {
        if ( !isVisible() )     qDebug() << "主窗口被多次隐藏，s1 被篡改";
        s1->assignProperty( this, "geometry", this->geometry() );
        s1->assignProperty( playlist, "geometry", playlist->geometry() );
        s1->assignProperty( lrcWnd, "geometry", lrcWnd->geometry() );
        emit narrowHide();
    }
    else
    {
        if ( !miniMainWnd->isVisible() )
            qDebug() << "迷你窗口被多次隐藏，s1被篡改";
        miniMainWnd->initNarrowHide();
    }
}

/// 还原窗口的时候调用，决定显示主窗口还是迷你窗口
void MyWidget::dispatchEnlargeShow()
{
    if ( W_main == myPushButton::W_mode )
        emit enlargeShow();
    else
        miniMainWnd->initEnlargeShow(); // 调用槽函数激活信号
}

/// 切换到迷你窗口
void MyWidget::toMiniWnd()
{
    hide();             // 系统会调用 setVisible() 函数，里面设置了播放列表、歌词窗口的隐藏
    miniMainWnd->show();
    myPushButton::W_mode = W_mini;

    if ( myPushButton::L_mode == L_wnd )
        myPushButton::L_mode = L_mini;
}

/// 解析桌面歌词的 DB_WND 按钮
void MyWidget::clickToWnd()
{
    lrcDes->hide();
    if (myPushButton::W_mode == W_main)
    {
        myPushButton::L_mode = L_wnd;
        lrcWnd->setVisible( isVisible() );
    }
    else
        miniMainWnd->desToMini();
}

void MyWidget::clickDesLrcButton()
{
    myPushButton::L_mode = L_des;
    lrcDes->show();
    lrcWnd->hide();
}

void MyWidget::clickPlayListCloseButton()
{
    playlist->setVisible( false );
    button[B_PL]->setChecked( false );
}

/// 点击窗口歌词的关闭按钮后
/// 在窗口歌词创建按钮的时候连接
/// 点击桌面歌词工具窗关闭按钮 连接 这个函数
void MyWidget::lrcClose()
{
    setLrcShow( false );

    if ( myPushButton::W_mode == W_main )
        button[B_LRC]->setChecked( false );
    else
        miniMainWnd->lrcClose(false);
}

/// 用于连接 歌词 按钮，设定当前是否显示歌词
void MyWidget::setLrcShow(bool isShow)
{
    myPushButton::isLrcShow = isShow;

    if ( myPushButton::L_mode == L_wnd )
        lrcWnd->setVisible( isShow );
    else if ( myPushButton::L_mode == L_des )
        lrcDes->setVisible( isShow );
}

/// 点击停止按钮：禁用停止、切换播放图标=暂停状态=checked
void MyWidget::clickStop()
{
    button[B_STOP]->setDisabled(true);
    button[B_PAUSPLAY]->setChecked(true);
    player.stop();
}

///
void MyWidget::clickPausPlay( bool check )
{
    if ( !check )    // 如果没有勾选=播放状态=暂停图标
    {
        player.play();
       // playlist->playSong();
        if ( player.state() == QMediaPlayer::PlayingState ) // 如果播放成功
            button[B_STOP]->setDisabled(false); // 启用停止按钮
        else
            button[B_PAUSPLAY]->setChecked(true);   // 播放失败继续保持暂停
    }
    else
        player.pause();
}

/*
void MyWidget::showLrc( bool visible )
{
    lrcWnd->setVisible( visible );
}*/

//////////////////////////////
/// 点击滑条直接跳到鼠标位置播放 + 拖动释放后才跳动
void MyWidget::dispatchSliderAction(int action)
{
    if ( player.state() == QMediaPlayer::StoppedState )
        return ;

    if ( QAbstractSlider::SliderPageStepSub == action ||
         action == QAbstractSlider::SliderPageStepAdd )
    {
        QPoint p = progress->mapFromGlobal( QCursor::pos() );
        int new_pos = p.x() * progress->maximum() / progress->geometry().width() + 5;//修正5像素
        player.setPosition( new_pos * 1000 );
    }
    else if ( action == QAbstractSlider::SliderMove )
        player.setPosition( progress->sliderPosition() * 1000 );

    if ( myPushButton::L_mode == L_wnd )
        lrcWnd->seekProgress();
    else if ( myPushButton::L_mode == L_des )
        lrcDes->seekProgress();
   // qDebug() << player.position();
}

/// 根据歌曲状态切换按钮状态
/// 只用于更改按钮状态，歌曲播放完毕使用进度条检测！！！！
void MyWidget::playerStateChanged(QMediaPlayer::State state )
{
    switch ( state )
    {
    case QMediaPlayer::StoppedState:
        button[B_STOP]->setDisabled( true );
        button[B_PAUSPLAY]->setChecked(true);
    //    playNext();   双击另外一首歌曲播放时，会stop之前的。然后状态改变，调用该函数。然后又要播放双击的那首，又要停止该函数播放的再playing双击的那首
        break;

    case QMediaPlayer::PlayingState:
        button[B_STOP]->setDisabled( false );
        button[B_PAUSPLAY]->setChecked( false );
        break;

    case QMediaPlayer::PausedState:
        button[B_STOP]->setDisabled( false );
        button[B_PAUSPLAY]->setChecked( true );
        break;
    default:
        break;
    }
}

/// 接受 player 的 currentMediaChanged（）信号，
/// 根据当前歌词模式发送 当前歌曲
void MyWidget::mediaChanged(const QMediaContent &media)
{
    QString songPathName = media.canonicalUrl().toString();
    readLrc( songPathName );

    if ( !QFile::exists( songPathName )) // 检测是否是本地文件还是网络媒体
    {
        findLrcTimer = new QTimer(this);
        connect( findLrcTimer, SIGNAL(timeout()), this, SLOT(lrcIsDownloaded()) );
        findLrcTimer->start(1000);
    }

    if ( myPushButton::L_mode == L_wnd )
        lrcWnd->mediaChanged();
    else if ( myPushButton::L_mode == L_des )
        lrcDes->mediaChange();
    else
        miniMainWnd->mediaChanged();
}

///
void MyWidget::playPrev()
{
    playlist->playPrev();
}

/// 为了给迷你窗口下一首按钮连接该槽调用里面的函数
void MyWidget::playNext()
{
    playlist->playNext();
}

/// 不能根据player 的stateChanged() 信号来播放歌曲，因为认为播放其它歌曲会产生该信号。
/// 导致不能播放想要的歌曲
void MyWidget::isFinsh(int value)
{//qDebug() << value << " - " << progress->maximum() << " ==  " << player.duration();
    if ( value >= progress->maximum() ) // 必须与最大值比较，不然会产生多次条件=true，调用多次playNext()
        playlist->autoNext();
}

void MyWidget::setMuteButton(bool checked)
{
    button[B_MUTE]->setChecked( checked );
}

void MyWidget::trayShowDesLrc( bool checked )
{
    if ( checked )
    {
        if ( myPushButton::L_mode == L_wnd )
            clickDesLrcButton();
        else if ( myPushButton::L_mode == L_mini )
            miniMainWnd->miniToDesLrc();
        else
        {
            lrcDes->show();
            myPushButton::isLrcShow = true;
            if ( myPushButton::W_mode == W_main )
                button[B_LRC]->setChecked(true);
            else
                miniMainWnd->lrcClose( true );  // 为了勾选歌词按钮
        }
    }
    else
    {
        lrcDes->hide();
        myPushButton::isLrcShow = false;
        button[B_LRC]->setChecked(false);
        miniMainWnd->lrcClose( false );
    }
}

void MyWidget::clickBrower()
{
    brower->setVisible( !brower->isVisible() );
}

void MyWidget::test(const QMediaContent &)
{
    qDebug() << "asd";
}


