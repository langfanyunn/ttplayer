#include "myminilrcwnd.h"
#include "mywidget.h"
#include "myminimainwnd.h"
#include "mypushbutton.h"
#include <QMenu>
#include <QEvent>
#include <QMediaPlayer>
#include <QMap>
#include <QPainter>
#include <QTimer>
#include <QFileInfo>
#include <QColorDialog>
#include <QFontMetrics>

myMiniLrcWnd::myMiniLrcWnd(QMap<qint64, QString> *m, QMediaPlayer* media, QWidget *parent) :
    noFrameWidget(parent),
    _x(0),
    //_xToCur(0),
    currow(-1),
    player(media),
    map(m)
{
    setMouseTracking( true );
    setObjectName( "miniLrcWnd" );
    setWindowFlags( Qt::Tool | Qt::FramelessWindowHint );

    setGeometry( 500, 110, 280, 44 );
    activateWindow();

    setFixedHeight( 26 );
    setMinimumWidth( 40 );

    curTimer = new QTimer(this);
    connect( curTimer, SIGNAL(timeout()), this, SLOT(checkCurRow()) );

    scrollTimer = new QTimer(this);
    connect( scrollTimer, SIGNAL(timeout()), this, SLOT(scrollSlot()) );

    connect( player, SIGNAL(stateChanged(QMediaPlayer::State)),
                this, SLOT(stateChanged(QMediaPlayer::State)) );

    lrcFont.setFamily( "Tahoma" );
    lrcFont.setPointSize(11);
    lrcColor.setHsv(77,255,255);
    curColor.setHsv(170,255,255);
}

void myMiniLrcWnd::init()
{
    if ( !map )
        return ;

    _x = width() / 2;
    currow = -1;

    if ( map->isEmpty() )
    {
        _x -= 30;
        curString = QFileInfo(player->media().canonicalUrl().toString()).fileName();
    }
    update();
}

void myMiniLrcWnd::checkCurRow()
{
    if ( !map || map->isEmpty() )
        return ;

    QFontMetrics metric(lrcFont);
    qint64 pos = player->position();
    QList<qint64> tList = map->keys();

    int _xToCur = 0;
    for ( int i = 0; i < tList.size(); i++ )
    {
        _xToCur += SPACE + metric.width(map->value(tList.at(i)));
        if ( pos > tList.at(i) - 70 && pos < tList.at(i) && currow != i )
        {
            currow = i;
            curString = map->value( tList.at(i) );
            update();

            if ( i + 1 < tList.size() )
            {
                int dt = tList.at(i + 1) - tList.at(i);
                int speed = dt / ( metric.width(curString) + SPACE + 30 );  // 修补30

                /// 加减速
                if ( _xToCur + _x - metric.width(curString) - SPACE > 5 * width() / 8 )
                    speed = 3 * speed / 4 ;
                if ( _xToCur + _x - metric.width(curString) - SPACE < width() / 4 )
                    speed += 50;
                scrollTimer->setInterval( speed );
            }
        }
    }
}

///
void myMiniLrcWnd::stateChanged(QMediaPlayer::State state)
{
    if ( !myPushButton::isLrcShow || myPushButton::L_mode != L_mini )
        return ;

    switch ( state )
    {
    case QMediaPlayer::StoppedState:
        init();
        curTimer->stop();
        scrollTimer->stop();
        break;

    case QMediaPlayer::PlayingState:
        if ( !map->isEmpty() )
        {
            curTimer->start(20);
            if ( scrollTimer->interval() <= 0 )
                scrollTimer->start(1000);
            else
                scrollTimer->start(666);
        }
        break;

    case QMediaPlayer::PausedState:
        curTimer->stop();
        scrollTimer->stop();
        break;
    }
}

void myMiniLrcWnd::scrollSlot()
{
    scroll( -1, 0 );
    _x--;
}

void myMiniLrcWnd::contextMenuEvent(QContextMenuEvent *e)
{
    QMenu menu;

    QAction* toDes = menu.addAction("切换到桌面歌词....");
    QAction* color1 = menu.addAction("字体颜色");
    QAction* color2 = menu.addAction("当前歌词字体颜色");

    QAction* result = menu.exec(e->globalPos() );

    myMiniMainWnd* miniMain = static_cast<myMiniMainWnd*>(parentWidget());
    if ( !miniMain->inherits("myMiniMainWnd") )
        qDebug() << "父部件错误   myMiniLrcWnd::contextMenuEvent";

    if ( result == toDes )
        miniMain->miniToDesLrc();
    else if ( result == color1 )
    {
        QColor color = QColorDialog::getColor( lrcColor, this,
                    QObject::tr("under color") );
        if ( color.isValid() )
            lrcColor = color;
    }
    else if ( result == color2 )
    {
        QColor color = QColorDialog::getColor( curColor, this,
                    QObject::tr("under color") );
        if ( color.isValid() )
            curColor = color;
    }
}

void myMiniLrcWnd::wheelEvent(QWheelEvent *e)
{
    int wheel = e->delta() / 40;
    MyWidget* mainWidget = static_cast<MyWidget*>( parentWidget()->parentWidget() );

    if ( !mainWidget || !mainWidget->inherits("MyWidget") || !parentWidget()
         || !parentWidget()->inherits("myMiniMainWnd") )
        qDebug() << "父部件错误  myMiniLrcWnd::wheelEvent";

    mainWidget->setVolume( wheel );
}


void myMiniLrcWnd::paintEvent(QPaintEvent *)
{
    QPainter paint(this);
    paint.setFont( lrcFont );
    paint.setPen( lrcColor );
    QFontMetrics metric(lrcFont);

    int i = 0, xwidth = 0;
    foreach(QString str, *map)
    {
        xwidth += (metric.width(str) + SPACE);

        if ( i == currow )
            paint.setPen(curColor);
        else
            paint.setPen( lrcColor );

        if ( _x + xwidth > 0 && xwidth - metric.width(str) - SPACE + _x < width() )
        {
            int x = _x + xwidth - metric.width(str) - SPACE /*+ width() / 2*/;
            paint.drawText( x, 0, metric.width(str) + SPACE, height(),
                                Qt::AlignLeft | Qt::AlignVCenter, str );
        }
        i++;
    }
}

void myMiniLrcWnd::resizeEvent(QResizeEvent *e)
{
    _x += (e->size().width() - e->oldSize().width()) / 2;
}

void myMiniLrcWnd::showEvent(QShowEvent *)
{
    if (  player->state() == QMediaPlayer::PlayingState )
    {
        if ( scrollTimer->interval() <=0 )
            scrollTimer->start(90);
        else
            scrollTimer->start();
        curTimer->start(20);

        if ( !map || map->isEmpty() )
            return ;

        QFontMetrics metric(lrcFont);
        qint64 pos = player->position();
        QList<qint64> tList = map->keys();

        int _xToCur = 0;
        for ( int i = 0; i < tList.size() - 1; i++ )
        {
            if ( pos > tList.at(i) && pos < tList.at(i + 1) )
            {
                currow = i;
                _x = width() / 2 - _xToCur;
                update();
                break;
            }
            _xToCur += (SPACE + metric.width( map->value(tList.at(i))) );
        }
    }
}

void myMiniLrcWnd::hideEvent(QHideEvent *)
{
    curTimer->stop();
    scrollTimer->stop();
}








