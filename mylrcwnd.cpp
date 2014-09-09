#include "mylrcwnd.h"
#include "dialog.h"
#include "mywidget.h"
#include <QPainter>
#include <QLayout>
#include <QDebug>
#include <QPixmap>
#include <QTimer>
#include <QPen>
#include <QMenu>
#include <QAction>
#include <QColorDialog>
#include <math.h>
#include <QString>
#include <QFontDialog>
#include <QCoreApplication>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QLinearGradient>

myLrcWnd::myLrcWnd(QMap<qint64, QString> * map, QMediaPlayer* media, QWidget *parent) :
    noFrameWidget(parent),
    player(media)
{
    setMouseTracking( true );
    activateWindow();
   // setObjectName("lrcwnd");
    setWindowFlags( Qt::Tool | Qt::FramelessWindowHint );

    lrcLab = new myLabel( player, map, this );
    createButton();

    /// 歌词画布跟随窗口大小变化
    lrcLab->setObjectName( "lrcLab" );
    QHBoxLayout* lay = new QHBoxLayout;
    lay->addWidget( lrcLab );
    lay->setContentsMargins( 7, 24, 7, 8 );
    setLayout( lay );

    show();

    // 用来暂停、继续歌词的移动
    connect( player, SIGNAL(stateChanged(QMediaPlayer::State)),
                this, SLOT(playerStateChanged(QMediaPlayer::State)) );

    // 用来初始化歌词位置、属性之类的数据
  //  connect( player, SIGNAL(currentMediaChanged(QMediaContent)),
   //             this, SLOT(mediaChange(QMediaContent)) );

   // connect( player, SIGNAL(positionChanged(qint64)),
    //频率太低 1s/次！！！           lrcLab, SLOT(checkCurRow(qint64)) );
}

/// void MyWidget::dispatchSliderAction(int action) 里面调用
void myLrcWnd::seekProgress()
{
    lrcLab->seekLrc();
}

/// 在主窗口的 mediaChanged（）槽内调用
void myLrcWnd::mediaChanged()
{
    if (myPushButton::L_mode != L_wnd )
        return ;

    lrcLab->init();
    lrcLab->repaint();
}

/// 变成播放状态、获取媒体里面的歌曲名、传递出去读取歌词、填充 QMap
void myLrcWnd::playerStateChanged(QMediaPlayer::State state)
{
    if ( myPushButton::L_mode != L_wnd )
        return ;

    switch ( state )
    {
    case QMediaPlayer::StoppedState:
        lrcLab->init();
        lrcLab->pause();
        break;

    case QMediaPlayer::PlayingState:
        lrcLab->play();
        break;

    case QMediaPlayer::PausedState:
        lrcLab->pause();
        break;
    }
}

/*
/// 貌似只要 player.setMedia(..) 就会发射 currentMediaChanged() 信号
void myLrcWnd::mediaChange(const QMediaContent &media)
{
    if ( myPushButton::L_mode != L_wnd )
        return ;

    QString songPathName = media.canonicalUrl().toString();
    readLrc( songPathName );
    lrcLab->init();
    lrcLab->repaint();
}
*/
/// 创建 置顶按钮 的时候连接
void myLrcWnd::clickOnTop(bool checked )
{
    static Qt::WindowFlags flags = windowFlags();
    if ( checked )
    {
        setWindowFlags( flags | Qt::WindowStaysOnTopHint );
        show();     // 置顶后被隐藏，貌似还会设置父窗口！！！fuck
    }
    else        /// 无法取消置顶！？！？！？？！！？！？！？！？！？！？！？！？
    {
        setWindowFlags( Qt::Widget );
        hide();
    }
}


///
void myLrcWnd::paintEvent(QPaintEvent *)
{
    int w = width();
    int h = height();
    QPixmap pix;
    QPainter paint(this);

    switch ( noFrameWidget::skin_index )
    {
    case 0:
        pix.load("./res/skin1/lyric_skin.bmp" );
        paint.drawPixmap( 10, 30, w-20, h-40, pix, 10, 30, 290, 146 );

        paint.drawPixmap( 0, 0, pix, 0, 0, 60, 30 );            // 苗四个角
        paint.drawPixmap( 0, h-10, pix, 0, 176, 10, 10 );
        paint.drawPixmap( w-10, h-10, pix, 300, 176, 10, 10 );
        paint.drawPixmap( w-10, 0, pix, 300, 0, 10, 30 );

        paint.drawPixmap( 60, 0, w-70, 30, pix, 60, 0, 240, 30 );  // 上下左右
        paint.drawPixmap( 10, h-10, w-20, 10, pix, 10, 176, 290, 10 );
        paint.drawPixmap( 0, 30, 10, h-40, pix, 0, 30, 10, 146 );
        paint.drawPixmap( w-10, 30, 10, h-40, pix, 300, 30, 10, 146 );
        break;

    case 1:
        pix.load("./res/skin2/lyric_skin.bmp" );
        //paint.drawPixmap( 10, 30, w-20, h-40, pix, 10, 30, 267, 42 );
        paint.fillRect(10,30,w-20,h-40, QBrush(QColor(192,234,234)) );

        paint.drawPixmap( 0, 0, pix, 0, 0, 50, 30 );            // 苗四个角
        paint.drawPixmap( 0, h-10, pix, 0, 72, 10, 10 );
        paint.drawPixmap( w-10, h-10, pix, 277, 72, 10, 10 );
        paint.drawPixmap( w-30, 0, pix, 257, 0, 30, 30 );

        paint.drawPixmap( 50, 0, w-80, 30, pix, 50, 0, 207, 30 );  // 上下左右
        paint.drawPixmap( 10, h-10, w-20, 10, pix, 10, 72, 267, 10 );
        paint.drawPixmap( 0, 30, 10, h-40, pix, 0, 30, 10, 42 );
        paint.drawPixmap( w-10, 30, 10, h-40, pix, 277, 30, 10, 42 );
        break;
    }
}

void myLrcWnd::resizeEvent(QResizeEvent *)
{
    int w = this->width();
    for ( int i = 0; i < 3; i++ )
        if ( lrcButton[i] ) // 有可能还没有创建按钮
            lrcButton[i]->setGeometry( w - (3-i) * 20, 2, 15, 15 );
}


void myLrcWnd::createButton()
{
    MyWidget* mainWidget = static_cast<MyWidget*>(parentWidget());
    if ( !mainWidget->inherits("MyWidget") )
        qDebug() << "父部件错误 myLrcWnd::createButton()";

    for ( int i = 0; i < 3; i++ )
    {
        lrcButton[i] = new myPushButton(this);
        lrcButton[i]->setFocusPolicy( Qt::NoFocus );
        lrcButton[i]->setObjectName( QString("lrcButton%1").arg(i) );
    }
    lrcButton[LWB_ONTOP]->setCheckable( true ); // checked = 置顶
    // 在 resizeEvent() 内实时修改三个按钮位置

    connect( lrcButton[LWB_CLOSE], SIGNAL(clicked()),
                mainWidget, SLOT(lrcClose()) );

    connect( lrcButton[LWB_ONTOP], SIGNAL(clicked(bool)),
                this, SLOT(clickOnTop(bool)) );

    connect( lrcButton[LWB_TODES], SIGNAL(clicked()),
                mainWidget, SLOT(clickDesLrcButton()) );
}

/*
///
void myLrcWnd::readLrc(const QString &songPathName)
{

   // foreach ( qint64 time, lrcMap.keys() )
   //     qDebug() << time;
}*/




///
myLabel::myLabel(QMediaPlayer *media, QMap<qint64, QString> *m, QWidget *parent):
    QLabel(parent),
    player(media),
    map(m),
    _lineHeight(22),
    _H(176),
    _S(255),
    _V(255)
{
    //  this->setWindowFlags();
    timer = new QTimer(this);
    connect( timer, SIGNAL(timeout()), this, SLOT(timeOut()) );

    /// 用于检测并设置当前行
    checkCurRowTimer = new QTimer(this);
    connect( checkCurRowTimer, SIGNAL(timeout()), this, SLOT(checkCurRow()) );

    widdlyTimer = new QTimer(this);
    connect( widdlyTimer, SIGNAL(timeout()), this, SLOT(widdlyTimeOut()) );

    _lrcFont.setPointSize(_lineHeight - 13);
    _lrcFontColor.setRgb( 131, 132, 184 );

    _lrcWidColorDlg = new Dialog(this);
    _lrcWidColorDlg->setFixedSize(370, 100);

   // setContextMenuPolicy( Qt::CustomContextMenu );
}

/// 改变歌曲进度时，更新歌词位置
void myLabel::seekLrc()
{
    if ( !map || map->isEmpty() )
        return ;

    qint64 pos = player->position();
    QList<qint64> tList = map->keys();

    for ( int i = 0; i < tList.size() - 1; i++ )
    {
        if ( pos > tList.at(i) && pos < tList.at(i + 1) )
        {
            curRow = i;
            y = height() / 2 - curRow * _lineHeight + 0;
            update();
            break;
        }
    }
    timer->setInterval( 600 );  // 防止残余上次的速度！！歌词走得过快
}

void myLabel::init()
{
    if ( !map || map->isEmpty() )
        return ;

    y = this->height() / 2 + 0;    // 50 留给动画
    curRow = -1;

    step = 0;   // 歌词未开始跳动
}

/// 计时器必须指定时间开始，最好不要调用没有参数的 start() 函数！！
/// 因为它的溢出时间是不确定的！！！！！！！！
void myLabel::play()
{
    checkCurRowTimer->start(20);
    timer->start(1000);
    widdlyTimer->start(40);
}

/// 计时器没有工作也 stop 会怎么样?!???!!?!?
void myLabel::pause()
{
    if ( timer->isActive() )
        timer->stop();

    if ( widdlyTimer->isActive() )
        widdlyTimer->stop();

    if ( checkCurRowTimer->isActive() )
        checkCurRowTimer->stop();
}

///
void myLabel::timeOut()
{
    scroll(0, -1);
    y--;
}

///
void myLabel::checkCurRow()
{
    if ( !map || map->isEmpty() )
        return ;

    qint64 pos = player->position();
    QList<qint64> tList = map->keys();

    for ( int i = 0; i < tList.size(); i++ )
    {
       // qDebug() << tList.at(i) - 100;
        if ( pos > tList.at(i) - 70 && pos < tList.at(i) && curRow != i )
        {//qDebug() << "asd";
            curRow = i;

            if ( i + 1 < tList.size() )
            {
                int dt = tList.at(i + 1) - tList.at(i);
                if ( dt <= 0 )
                    dt = 1000;  // 防止二笔歌词制作者

                /// 落后、过快修补
                int ddt = dt / _lineHeight;
                if ( y + curRow * _lineHeight > height() / 2 + 30 )
                    ddt = ( ddt - 40 > 0 ) ? ddt - 40 : 30;
                else if ( y + curRow * _lineHeight < height() / 2 - 20 )
                    ddt += 50;
                timer->setInterval( ddt );

                if ( dt <= 0 || ddt <= 0 )
                    qDebug() << "dt 计时器间隔错误小于零：     " << dt;

                //////////////////////////////////////
                /// 归零该行歌词 容器 内每个字的跳动次数、启动计时器
                widdlyTimer->stop();
                wid.clear();
                step = 0;
                QFontMetrics metrics(font());

                for ( int index = 0; index < map->value(tList.at(curRow)).size(); index++ )
                    wid.insert( index, 0 );

                // 保证唱完这一行，刚好滚动完
                if ( !map->value(tList.at(curRow)).isEmpty() )
                {
                    int speed = dt / metrics.width(map->value(tList.at(curRow)));
                    speed = ( speed < 50 && speed > 0 ) ? speed : 40;
                    widdlyTimer->start(speed);

                    if ( speed <= 0 )
                        qDebug() << "计时器小雨 0 了！！！--->   " << speed;
                   // widdlyTimer->start( 40 );
                }
            }
            break;
        }
    }
}

/// map<index, sineTable>
/// <0,1> ... <0,16>
/// <1,0> ... <1,15>
/// <2,0> ... <2,14>
/// <3,0> ... <3,13>
///  ...  ...  ...
void myLabel::widdlyTimeOut()
{
    if ( wid.size() == 0 || map->isEmpty() || !map )
        return ;

    step++;
    int i = 0;

    foreach( QString str, *map )
    {
        if ( i++ == curRow )
        {
            for( int s = step, n = 0; s > 0 && n < str.size(); s--, n++ )
            {
                if ( wid.value(n) <= LRCSTEP - 2 )
                    wid.insert( n, s );
            }
        }
    }
}

void myLabel::HSlot(int h )
{
    _H = h;
}

void myLabel::SSlot(int s )
{
    _S = s;
}

void myLabel::VSlot(int v )
{
    _V = v;
}

///
void myLabel::enterEvent(QEvent *)
{
    setCursor( Qt::ArrowCursor );
}

void myLabel::paintEvent(QPaintEvent *)
{
    if ( map->isEmpty() || !map )
        return ;

    static const int sineTable[LRCSTEP] = {0,2,4,6,8,10,12,14,16,
                                            14,12,10,8,6,4,2,0 };
    QPainter paint(this);    
    paint.setFont( _lrcFont );

    int i = 0, a = 255;
    foreach( QString str, *map )
    {
        // 只输出窗口内部的歌词
        if ( y + _lineHeight * i >= -20 && y + _lineHeight * i < height()
                && curRow != i/* || i == 0)*/ )
        {
            // 上下渐隐
            a = 255 - abs(curRow - i) * 250 * 2 * _lineHeight / height();
            a = ( a > 0 ) ? a : 30;
            a = ( a < 255 ) ? a : 255;

            // 更改成员数据颜色的透明度
            _lrcFontColor.setAlpha( a );
            paint.setPen( _lrcFontColor );
            paint.drawText( 0, y + _lineHeight * i, width(),
                             _lineHeight, Qt::AlignCenter, str );
        }
        else if ( curRow == i )
        {
            QColor color;
            QFontMetrics metrics(_lrcFont);
            int x = ( width() - metrics.width(str) ) / 2;
            int top = y + _lineHeight * i + 16; // 不知喂猫，跳动会错位？！

            for( int n = 0; n < str.size(); n++ )
            {
                color.setHsv( wid.value(n) * _H/16, _S, _V );
                paint.setPen( color );

                // 波动 1/4 的字体高度
                paint.drawText( x, top - sineTable[wid.value(n)] * metrics.height() / 64,
                                    QString(str[n]) );
                x += metrics.width(str[n]);
            }
        }
        i++;
    }
}


/// 计算把当前歌词放在窗口中间的 y 值
void myLabel::resizeEvent(QResizeEvent *)
{
    y = height() / 2 - curRow * _lineHeight + 0;
    //  e->accept();
}

void myLabel::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu;

    QAction* font = menu.addAction("选择字体..");
    QAction* fontColor = menu.addAction("更改字体颜色...");
    QAction* curColor = menu.addAction("当前行颜色...");

    QAction* result = menu.exec( event->globalPos() );

    if ( result == font )
    {
        bool ok;
        QFont font = QFontDialog::getFont( &ok, this );
        if ( ok )
        {
            _lrcFont = font;
            _lineHeight = _lrcFont.pointSize() + 13;

            // 即刻调节当前行到中间
            y = height() / 2 - curRow * _lineHeight + 0;
            update();
        }
    }
    else if ( result == fontColor )
    {
        QColor color = QColorDialog::getColor( _lrcFontColor, this,
                    QObject::tr("under color") );
        if ( color.isValid() )
            _lrcFontColor = color;
    }
    else if ( result == curColor )
    {
        int h = _H, s = _S, v = _V;
        if ( QDialog::Rejected == _lrcWidColorDlg->exec() )
        {
            _H = h;
            _S = s;
            _V = v;
        }

    }
}

void myLabel::hideEvent(QHideEvent *)
{
    pause();
}


/// 必须保证有歌曲播放
void myLabel::showEvent(QShowEvent *)
{
    if ( player->state() == QMediaPlayer::PlayingState )
    {
        play();
        seekLrc();
    }
}
/*bug
void myLabel::setVisible(bool visible)
{
    if ( !visible )
    {
        pause();
    }
    else
    {
        play();
        seekLrc();
    }
    QWidget::setVisible(visible);

    qDebug() << "asd";
}
*/










