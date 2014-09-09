#include "mylrc.h"
#include "mywidget.h"
#include "myminimainwnd.h"
#include "mypushbutton.h"
#include "myminilrcwnd.h"

#include <QDesktopWidget>
#include <QApplication>
#include <QStyleOption>
#include <QPainter>
#include <QPushButton>
#include <QSettings>
#include <QState>
#include <QStateMachine>
#include <QTimer>
#include <QDebug>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>

myMiniMainWnd::myMiniMainWnd(QMap<qint64, QString>* m, myLrc* lrc, QMenu* menu, QMediaPlayer *pl, QWidget *parent) :
    QWidget(parent),
    offset(QPoint(0,0)),
    desLrc(lrc),
    drag(false),
    mainMenu(menu),
    player(pl)
{
    activateWindow();
    setObjectName( "miniMainWnd" );     // 用在脚本设置背景
    setWindowFlags( Qt::Tool | Qt::FramelessWindowHint );

    /// 必须置顶这个父窗口 ，右击菜单用到！！
    miniLrcWnd = new myMiniLrcWnd(m, pl, this);    // 确保在创建按钮之前创建歌词，因为按钮会连接里面的槽
    createMiniButton();
    //createHideShowState();    放到 myWidget 构造函数 setSkin() 后面
    move(400, 50 );
    setWindowFlags( windowFlags() | Qt::WindowStaysOnTopHint );

    // 根据播放状态改变按钮状态
    connect( player, SIGNAL(stateChanged(QMediaPlayer::State)),
                this, SLOT(playerStateChanged(QMediaPlayer::State)) );
}


/// 在主窗口类 myWidget::setSkin(..) 调用。索引为 0、1
void myMiniMainWnd::setMiniGeometry(const int& index)
{
    QRect rect;
    QSettings set( "config.ini", QSettings::IniFormat );
    set.beginGroup( QString("miniButtonGroup%1").arg(index) );
    for ( int i = 0; i < MINI_BUTTON_MAX; i++ )
    {
        rect = set.value( QString("%1").arg(i) ).toRect();
        miniButton[i]->setGeometry(rect);
    }
    set.endGroup();
}

/// 连接诶退出信号，启用计时器调用下下面的 quitTimer() 函数
void myMiniMainWnd::opacityQuit()
{
    QTimer* t = new QTimer(this);
    connect( t, SIGNAL(timeout()), this, SLOT(quitTimer()) );
    t->start( 19 );
}
void myMiniMainWnd::quitTimer()
{
    static qreal opacity = 1;
    opacity -= 0.03;
    if ( opacity <= 0 )
        qApp->quit();

    setWindowOpacity( opacity );
    miniLrcWnd->setWindowOpacity( opacity );

    player->setVolume( opacity * 70 ); // 制作声音渐小退出
}

void myMiniMainWnd::initNarrowHide()
{
    s1->assignProperty( this, "geometry", this->geometry() );
    s1->assignProperty( miniLrcWnd, "geometry", miniLrcWnd->geometry() );
    emit narrowHide();
}

void myMiniMainWnd::initEnlargeShow()
{
    emit enlargeShow();/// 信号貌似不能在其它类里面激活
}

void myMiniMainWnd::toMainWnd()
{
    if ( !parentWidget() || !parentWidget()->inherits("MyWidget") )
        qDebug() << "错误，必须设置父窗口。。。才能切换到主窗口";

    parentWidget()->show();
    //mainWidget->show();
    hide();
    myPushButton::W_mode = W_main;

    if ( myPushButton::L_mode == L_mini )
        myPushButton::L_mode = L_wnd;
}

void myMiniMainWnd::setLrcShow(bool show)
{
    myPushButton::isLrcShow = show;

    if ( myPushButton::L_mode == L_mini )
        miniLrcWnd->setVisible( show );
    else if ( myPushButton::L_mode == L_des )
        desLrc->setVisible( show );
}

void myMiniMainWnd::clickStop()
{
    miniButton[MB_STOP]->setDisabled( true );
    miniButton[MB_PAUSPLAY]->setChecked( true );
    //myPushButton::playState = S_stop;
    player->stop();
}


void myMiniMainWnd::clickPausPlay(bool check)
{
    if ( !check )    // 如果没有勾选=播放状态=暂停图标
    {
        player->play();
        if ( player->state() == QMediaPlayer::PlayingState ) // 如果播放成功
            miniButton[MB_STOP]->setDisabled(false); // 启用停止按钮
        else
            miniButton[MB_PAUSPLAY]->setChecked(true);   // 播放失败继续保持暂停
    }
    else
        player->pause();
}

/// 类似主窗口函数
void myMiniMainWnd::playerStateChanged(QMediaPlayer::State state)
{
    switch ( state )
    {
    case QMediaPlayer::StoppedState:
        miniButton[MB_STOP]->setDisabled( true );
        miniButton[MB_PAUSPLAY]->setChecked(true);
    //    playNext();   双击另外一首歌曲播放时，会stop之前的。然后状态改变，调用该函数。然后又要播放双击的那首，又要停止该函数播放的再playing双击的那首
        break;

    case QMediaPlayer::PlayingState:
        miniButton[MB_STOP]->setDisabled( false );
        miniButton[MB_PAUSPLAY]->setChecked( false );
        break;

    case QMediaPlayer::PausedState:
        miniButton[MB_STOP]->setDisabled( false );
        miniButton[MB_PAUSPLAY]->setChecked( true );
        break;
    default:
        break;
    }
}

///
void myMiniMainWnd::paintEvent(QPaintEvent *)
{
    QStyleOption opt;   /* 必须才能给窗口QWidget 设置背景图片！？！？？*/
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

///
void myMiniMainWnd::mousePressEvent(QMouseEvent *event)
{
    drag = true;
    if (event->button() == Qt::LeftButton )
        offset = event->globalPos() - geometry().topLeft();
}

///
void myMiniMainWnd::mouseMoveEvent(QMouseEvent *e)
{
    if ( (e->globalPos() - offset).y() < QApplication::desktop()->height() - 54
         && (e->globalPos() - offset).y() >= 0 )    // 防止超出上下屏幕
        if ( e->buttons() & Qt::LeftButton && drag )
            move(e->globalPos() - offset);
}

void myMiniMainWnd::mouseReleaseEvent(QMouseEvent *)
{
    drag = false;
}

void myMiniMainWnd::wheelEvent(QWheelEvent *e)
{
    int wheel = e->delta() / 40;
    MyWidget* mainWidget = static_cast<MyWidget*>(parentWidget());

    if ( !mainWidget || !mainWidget->inherits("MyWidget") )
        qDebug() << "父部件错误  myMiniMainWnd::wheelEvent ";

    mainWidget->setVolume( wheel );
}

void myMiniMainWnd::setVisible(bool visible)
{
    if ( visible )
    {
        // 根据播放状态设定按钮状态
        bool b_stop = false, b_paus = false;
        if ( player->state() == QMediaPlayer::StoppedState )    // 禁用停止按钮
            b_stop = b_paus = true;
        else if ( player->state() == QMediaPlayer::PausedState )   // 暂停状态、checked、播放图标
            b_paus = true;
        miniButton[MB_STOP]->setDisabled( b_stop );
        miniButton[MB_PAUSPLAY]->setChecked( b_paus );

        // 根据显示歌词与否设置歌词按钮状态
        miniButton[MB_LRC]->setChecked( myPushButton::isLrcShow );

        if ( myPushButton::L_mode != L_des && myPushButton::isLrcShow )
            miniLrcWnd->setVisible( true );
    }
    else
        miniLrcWnd->setVisible( false );

   /* // 决定是否显示迷你窗口歌词
    if ( visible )
         miniButton[MB_LRC]->isChecked() );
    else
        miniLrcWnd->setVisible(false);*/

    QWidget::setVisible( visible ); // 必须！！！否则不能显示
}


///
void myMiniMainWnd::createMiniButton()
{
    if ( !parentWidget() || !parentWidget()->inherits("MyWidget") )
        qDebug() << "父部件错误 myMiniMainWnd::createMiniButton()";

    for ( int i = 0; i < MINI_BUTTON_MAX; i++ )
    {
        miniButton[i] = new myPushButton(this);
        miniButton[i]->setFocusPolicy( Qt::NoFocus );
        miniButton[i]->setObjectName( QString("miniButton%1").arg(i) );
    }

  /*  connect( miniButton[MB_STOP], SIGNAL(clicked()),    // 点击停止按钮后立即禁用停止按钮
                miniButton[MB_STOP], SLOT(disabled()) );
    connect( miniButton[MB_STOP], SIGNAL(clicked()),
                miniButton[MB_PAUSPLAY], SLOT(checked()) );*/
    connect( miniButton[MB_STOP], SIGNAL(clicked()), this, SLOT(clickStop()) );
  //  connect( miniButton[MB_PREV], SIGNAL(clicked()), this, SLOT(clickNextPrev()) );
   // connect( miniButton[MB_NEXT], SIGNAL(clicked()), this, SLOT(clickNextPrev()) );
    connect( miniButton[MB_PREV], SIGNAL(clicked()),
                parentWidget(), SLOT(playPrev()) );
    connect( miniButton[MB_NEXT], SIGNAL(clicked()),
                parentWidget(), SLOT(playNext()) );

   // connect( miniButton[MB_PAUSPLAY], SIGNAL(clicked(bool)),
     //           miniButton[MB_STOP], SLOT(setMyDisabled(bool)) );
    connect( miniButton[MB_PAUSPLAY], SIGNAL(clicked(bool)),
                this, SLOT(clickPausPlay(bool)) );

    connect( miniButton[MB_MINI], SIGNAL(clicked()),
                this, SLOT(toMainWnd()) );

    connect( miniButton[MB_MIN], SIGNAL(clicked()),
                this, SLOT(initNarrowHide()) );

    connect( miniButton[MB_CLOSE], SIGNAL(clicked()),
                this, SLOT(opacityQuit()) );

  //  connect( miniButton[MB_LRC], SIGNAL(clicked(bool)),
    //            miniLrcWnd, SLOT(setVisible(bool)) );
    connect( miniButton[MB_LRC], SIGNAL(clicked(bool)),
                this, SLOT(setLrcShow(bool)) );

    miniButton[MB_STOP]->setDisabled(true);     // 初始禁用停止按钮
    miniButton[MB_PAUSPLAY]->setCheckable(true);
    miniButton[MB_PAUSPLAY]->setChecked(true);      // 勾选=暂停状态=播放图标
    miniButton[MB_LRC]->setCheckable(true);
    miniButton[MB_LRC]->setChecked(true);

    miniButton[MB_MENU]->setMenu( mainMenu );
    miniButton[MB_MENU]->setIcon( QIcon("./res/TTPlayer.png") );
  //  miniButton[MB_MENU]->setStyleSheet( "");
}

// 创建缩小放大之间的两个状态
void myMiniMainWnd::createHideShowState()
{
    static QStateMachine machine;
    s1 = new QState(&machine);
    s2 = new QState(&machine);

    QPoint br = QApplication::desktop()->geometry().bottomRight() - QPoint(130, 30);
    s1->assignProperty( this, "geometry", this->geometry() );
    s1->assignProperty( miniLrcWnd, "geometry", miniLrcWnd->geometry() );

    s2->assignProperty( this, "geometry", QRect(br, QSize(0,0)) );
    s2->assignProperty( miniLrcWnd, "geometry", QRect(br, QSize(0,0)) );

    s1->addTransition( this, SIGNAL(narrowHide()), s2 );
    s2->addTransition( this, SIGNAL(enlargeShow()), s1 );

    connect( s1, SIGNAL(entered()), this, SLOT(showNormal()) );
    connect( s2, SIGNAL(propertiesAssigned()), this, SLOT(hide()) );

    QPropertyAnimation* animation1 = new QPropertyAnimation(this, "geometry" );
    QPropertyAnimation* animation2 = new QPropertyAnimation(miniLrcWnd, "geometry" );
    static QParallelAnimationGroup group;
    group.addAnimation( animation1 );
    group.addAnimation( animation2 );

    machine.addDefaultAnimation( &group );
    machine.setInitialState( s1 );   // 会将窗口显示！！！！！
    machine.start();        // 会将窗口显示！！！！！
    connect( &machine, SIGNAL(started()), this, SLOT(hide()) );// 防止同时显示两个窗口
}

/// 迷你窗口模式未显示桌面歌词
/// 右击托盘显示的时候，可能会调用
void myMiniMainWnd::lrcClose(bool checked)
{
    miniButton[MB_LRC]->setChecked(checked);
}

/// 从桌面歌词切换到迷你歌词
void myMiniMainWnd::desToMini()
{
    myPushButton::L_mode = L_mini;
    miniLrcWnd->setVisible( isVisible() );
}

///
void myMiniMainWnd::miniToDesLrc()
{
   miniLrcWnd->hide();
   desLrc->show();
   myPushButton::L_mode = L_des;
}

///
void myMiniMainWnd::mediaChanged()
{
    if ( myPushButton::L_mode != L_mini )
        qDebug() << "不应该的调用";
    miniLrcWnd->init();
}











