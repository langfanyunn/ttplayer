#include "mylrc.h"
#include "mywidget.h"
#include "desLrcDialog.h"
#include <QDebug>
#include <QPainter>
#include <QCursor>
#include <QMenu>
#include <QRegion>
#include <QFileInfo>
#include <QFontMetrics>
#include <QStyleOption>

myLrc::myLrc(QMap<qint64, QString> *m, QMediaPlayer* media, QMenu *menu, QWidget *parent) :
    noFrameWidget(parent),//QLabel(parent)
    curRow(-1),
    xLeft(0),
    xMask(0),
    _lock(false),
    _trans(false),
    _inside(false),
    _douLine(true),
    player(media),
    _desLrcMap(m)
{
    setMouseTracking( true );   // 必须设置才能检测移动窗口大小之类的额！！！！！
    setWindowFlags( Qt::Tool | Qt::FramelessWindowHint );
    setAttribute(Qt::WA_TranslucentBackground); // 设置背景透明
    setMinimumSize( 200, 30 );
    setMaximumHeight(100);
    setGeometry( 200, 30, 450, 60 );

    for ( int i = 0; i < 3; i++ )
    {
        foreColor[i].setHsv(i*60,255,255);
        bkColor[i].setHsv( (i + 1) * 80, 255, 255);
    }

    linearGradient.setStart( 0, 0 );
    linearGradient.setFinalStop( 0, 100 );
    linearGradient.setColorAt(0.1, bkColor[0] );
    linearGradient.setColorAt(0.5, bkColor[1] );
    linearGradient.setColorAt(0.9, bkColor[2] );
   // linearGradient.setCoordinateMode(QGradient::ObjectBoundingMode);
  //  linearGradient.setSpread(QGradient::RepeatSpread );

    linearGradient2.setStart( 0, 0 );
    linearGradient2.setFinalStop( 0, 100 );
    linearGradient2.setColorAt(0.1, foreColor[0] );
    linearGradient2.setColorAt(0.5, foreColor[1] );
    linearGradient2.setColorAt(0.9, foreColor[2] );
  //  linearGradient2.setSpread(QGradient::RepeatSpread );

    font.setFamily( "幼圆" );
    font.setBold( true );
    font.setPointSize( 30 );

    /// 必须置顶父窗口，desTool 创建按钮时连接了主窗口槽
    desTool = new toolWindow( media, menu, this);
    desTool->setObjectName( "desTool" );
    desTool->resize(200, 25);

    /// 检测鼠标进入歌词的计时器
    timer = new QTimer(this);
    connect( timer, SIGNAL(timeout()), this, SLOT(mouseTracking()) );
  //  timer->setInterval( 100 );

    /// 检测当前行
    curTimer = new QTimer(this);
    connect( curTimer, SIGNAL(timeout()), this, SLOT(checkCurRow()) );
    //curTimer->setInterval(20);
    curTimer->start(20);

    /// 遮罩运动
    maskTimer = new QTimer(this);
    connect( maskTimer, SIGNAL(timeout()), this, SLOT(maskTimeEvent()) );

    xleftTimer = new QTimer(this);
    connect( xleftTimer, SIGNAL(timeout()), this, SLOT(xLeftEvent()) );

    connect( media, SIGNAL(stateChanged(QMediaPlayer::State)),
                this, SLOT(stateChanged(QMediaPlayer::State)) );
    //防止跟随隐藏activateWindow();       // 跟随父窗口显示到前台

    //   qDebug() << this;
}

/// 跳跃进度条时，立刻更新当前歌词所在行
/// void MyWidget::dispatchSliderAction(int action) 内调用
void myLrc::seekProgress()
{
    if ( !_desLrcMap || _desLrcMap->isEmpty() )
        return ;

    qint64 pos = player->position();
    QList<qint64> tList = _desLrcMap->keys();

    for ( int i = 0; i < tList.size() - 1; i++ )
    {
        if ( pos > tList.at(i) && pos < tList.at(i + 1) )
        {
            curRow = i;

            QFontMetrics metrics(font);
            curString = _desLrcMap->value( tList.at(i) );

            /// 反正就是计算当前遮罩到的位置
            if ( tList.at(i + 1) - tList.at(i) != 0 )
                xMask = metrics.width(curString) * (pos - tList.at(i))
                                            / (tList.at(i + 1) - tList.at(i))
                                            + width() / 2
                                            - metrics.width(curString)/2;
            /// 如果歌词长度不为零
            if ( metrics.width(curString) != 0 )
                maskTimer->setInterval( (tList.at(i + 1) - tList.at(i))
                                            / metrics.width(curString) );
            else
                maskTimer->setInterval( 100 );
            update();
            break;
        }
    }
}


/// 内外尽量互相制约，减少多余的重绘
void myLrc::mouseTracking()
{
    /// 此时也会收到 enterEvent() 事件！！！
    if ( !_lock && geometry().contains(QCursor::pos()) && !_inside )
    {//qDebug() << "            aass";
        _inside = true;
        update();       // 更新才能激活窗口
        desTool->show();
    }
    else if ( _inside && !geometry().contains(QCursor::pos())
             && !desTool->geometry().contains(QCursor::pos()) )
    {//qDebug() << "sad";
        if ( !isDrag() )        // 防止拖拽的过程，_inside=false，paintEvent() 就不会绘制背景了
        {
            desTool->hide();
            _inside = false;
        }
        repaint();
    }
}

///
void myLrc::setLock(bool checked)
{
    _lock = checked;

    if ( checked )
        timer->stop();
    else
        timer->start(100);
}

///
void myLrc::setTrans(bool trans)
{
    _trans = trans;
}

///
void myLrc::setDouLine(bool check)
{
    _douLine = check;

    if ( !_douLine )
        font.setPointSize(height() * 2 / 5);
    else
        font.setPointSize(height() * 15 / 40 );
}

///
void myLrc::mediaChange()
{
    if ( myPushButton::L_mode != L_des )
        return ;

    if ( !_desLrcMap )
        return ;

    /** 草泥马，empty 与 NULL 是不一样的！！！！
if ( _desLrcMap->isEmpty() )
    qDebug() << "asdas       F"    ;

qDebug() << _desLrcMap->first();*/

    xLeft = 0;
    curRow = -1;

    if ( !_desLrcMap->isEmpty() )
        curString = _desLrcMap->first();
    else
        curString = QFileInfo(player->currentMedia().canonicalUrl().toString()).fileName();

    nextString = "";
    update();

   /* foreach( QString str, _desLrcMap->values() )
        qDebug() << str;
    foreach( QString str, *_desLrcMap )
        qDebug() << str;        // 效果同上。但是不能获取 key 值！！！！！
*/
  //  foreach( qint64 time, _desLrcMap->keys() )
    //    qDebug() << time;
}

///
void myLrc::checkCurRow()
{
    if ( !_desLrcMap || _desLrcMap->isEmpty() )
        return ;

    qint64 pos = player->position();
    QList<qint64> tList = _desLrcMap->keys();

    for( int i = 0; i < tList.size(); i++ )
    {
         if ( pos > tList.at(i) - 70 && pos < tList.at(i) && curRow != i )
         {
             curRow = i;
             curString = _desLrcMap->value( tList.at(i) );
             if ( i < tList.size() - 1 )
                 nextString = _desLrcMap->value( tList.at(i + 1) );
             else
                 nextString = "谢谢惠顾 Qt 静听！！！";
             if ( nextString.isEmpty() )
                 nextString = "谢谢惠顾 Qt 静听！！！";
             if ( curString.isEmpty() )
                 curString = "谢谢惠顾 Qt 静听！！！";

             update();

             /// 确定 xMask 初始位置：歌词字的最左边
             QFontMetrics metrics(font);
             int dw = metrics.width(curString) - this->width();
             xMask = 0;

             if ( dw < 0 && !_douLine )
                 xMask = ( width() - metrics.width(curString) ) / 2;

             if ( _douLine )    // 如果是双行de下行，mask 起始位置不一样
                 if ( curRow % 2 == 1 ) // 下行
                     if ( dw < 0 )
                         xMask = width() - metrics.width(curString);

             /// 如果歌词过长，启用xleftEvent() 计时槽
             xLeft = 0;
             if ( dw > 0 )
             {
                 if ( i + 1 < tList.size() )
                 {
                     // 提快 1/4
                     int speed = 3*(tList.at(i + 1) - tList.at(i)) / dw / 4;
                     speed = ( speed > 0 ) ? speed : 10;
                     xleftTimer->start(speed);
                 }
             }

             /// 确定遮罩速度
             if ( i + 1 < tList.size() )
             {
                 int dt = tList.at(i + 1) - tList.at(i);

                 if ( metrics.width(curString) != 0 )
                 {
                     int speed = dt / metrics.width(curString);
                     speed = ( speed > 0 ) ? speed : 10;
                     maskTimer->setInterval( speed );
                 }
             }
             break;
         }
    }
}

///
void myLrc::maskTimeEvent()
{
    xMask++;
    update();
}

/// 歌词过长移动左旗标
/// checkCurRow() 内设置计时间隔
/// maskTimerEvent() 内如果歌词走得过快，加快该函数的调用
void myLrc::xLeftEvent()
{
    xLeft--;

    // 如果最后一个字可以显示完，停止计时
    QFontMetrics metrics(font);
    if ( metrics.width(curString) + xLeft <= width() )
        xleftTimer->stop();
}

void myLrc::stateChanged(QMediaPlayer::State state)
{
    if ( myPushButton::L_mode != L_des )
        return ;

    switch ( state )
    {
    case QMediaPlayer::StoppedState:
        maskTimer->stop();
        break;

    case QMediaPlayer::PlayingState:
        if ( !_desLrcMap->isEmpty() )
        {
            if ( maskTimer->interval() <= 0 )
                maskTimer->start(20);
            else
                maskTimer->start();
            curTimer->start(20);
        }
        break;

    case QMediaPlayer::PausedState:
        maskTimer->stop();
        break;
    }
}

/// 连接工具窗口里面的按钮
void myLrc::clickOnTop(bool checked)
{
    if ( checked )
    {
        setWindowFlags( windowFlags() | Qt::WindowStaysOnTopHint );
        show();
    }
    else
        setWindowFlags( windowFlags() & ~Qt::WindowStaysOnTopHint );
}

///
void myLrc::setDesFont(QAction *action)
{
    font.setFamily( action->text() );
}

void myLrc::foreColor0Changed(int h)
{
    foreColor[0].setHsv( h, 255, 255 );
    linearGradient2.setColorAt(0.1, foreColor[0] );
}

void myLrc::foreColor1Changed(int h)
{
    foreColor[1].setHsv( h, 255, 255 );
    linearGradient2.setColorAt(0.5, foreColor[1] );
}

void myLrc::foreColor2Changed(int h)
{
    foreColor[2].setHsv( h, 255, 255 );
    linearGradient2.setColorAt(0.9, foreColor[2] );
}

void myLrc::bkCOlor0Changed(int h)
{
    bkColor[0].setHsv(h, 255, 255 );
    linearGradient.setColorAt(0.1, bkColor[0] );
}

void myLrc::bkCOlor1Changed(int h)
{
    bkColor[1].setHsv(h, 255, 255 );
    linearGradient.setColorAt(0.5, bkColor[1] );
}

void myLrc::bkCOlor2Changed(int h)
{
    bkColor[2].setHsv(h, 255, 255 );
    linearGradient.setColorAt(0.9, bkColor[2] );
}

/// 工具窗右击菜单调用
void myLrc::backUpColor( bool restore )
{
    static QColor fore[3] = {foreColor[0], foreColor[1], foreColor[2]},
                bk[3] = {bkColor[0], bkColor[1], bkColor[2] };
    if ( restore )
    {
        for ( int i = 0; i < 3; i++ )
        {
             foreColor[i] = fore[i];
             bkColor[i] = bk[i];
        }
        linearGradient.setColorAt(0.1, bkColor[0] );
        linearGradient.setColorAt(0.2, bkColor[1] );
        linearGradient.setColorAt(0.9, bkColor[2] );

        linearGradient2.setColorAt(0.1, foreColor[0] );
        linearGradient2.setColorAt(0.5, foreColor[1] );
        linearGradient2.setColorAt(0.9, foreColor[2] );
    }
    else        // 备份
    {
        for ( int i = 0; i < 3; i++ )
        {
            fore[i] = foreColor[i];
            bk[i] = bkColor[i];
        }
    }
}


///
void myLrc::paintEvent(QPaintEvent *)
{
    QPainter paint(this);
    paint.setFont(font);
    QFontMetrics metrics(font);

    if ( _inside && !_lock && !_trans )
        paint.fillRect( rect(), QBrush(QColor(200,200,200, 70)) );

    if ( !_douLine )    // 单行
    {
        /// 歌词过长左对齐、其余中间显示
        Qt::Alignment align = (metrics.width(curString) > this->width())
                                    ? Qt::AlignLeft|Qt::AlignVCenter : Qt::AlignCenter;

        /* 错开一个像素写文本，制作立体感 */
        paint.setPen( QColor(10, 0, 0));
        paint.drawText( xLeft + 1, 1, rect().right() - xLeft, rect().bottom(),
                                    align, curString );
        /* 背景字体 */
        paint.setPen( QPen(QBrush(linearGradient), 4) );
        paint.drawText( xLeft, 0, rect().right() - xLeft, rect().bottom(),
                                    align, curString );

        QRegion reg( 0, 0, xMask + xLeft, rect().bottom() );
        paint.setClipRegion( reg );

        /* 前景字体 */
        paint.setPen( QPen(QBrush(linearGradient2), 4) );
        paint.drawText( xLeft, 0, rect().right() - xLeft, rect().bottom(),
                                    align, curString );
     }
    else    // 双行
    {
        if ( curRow % 2 == 0 )  // 上行
        {
            /// 后一行歌词，必须先输出后一行，因为当前行有剪辑区遮罩，设置了剪辑区
            /// 不然不取消剪辑区后一行无法显示
            Qt::Alignment align = (metrics.width(nextString) > this->width())
                                    ? Qt::AlignLeft|Qt::AlignVCenter
                                        : Qt::AlignRight | Qt::AlignVCenter;
            paint.setPen( QColor(10, 0, 0));
            paint.drawText( 1, rect().bottom() / 2 + 1,
                                rect().right(), rect().bottom() / 2,
                                    align, nextString );
            paint.setPen( QPen(QBrush(linearGradient), 4) );
            paint.drawText( 0, rect().bottom() / 2,
                                rect().right(), rect().bottom() / 2,
                                    align, nextString );
            /// 正在唱的
            paint.setPen( QColor(10, 0, 0));
            paint.drawText( xLeft + 1, 1, rect().right() - xLeft, rect().bottom() / 2,
                                Qt::AlignLeft|Qt::AlignVCenter, curString );
            paint.setPen( QPen(QBrush(linearGradient), 4) );
            paint.drawText( xLeft, 0, rect().right() - xLeft, rect().bottom() / 2,
                                Qt::AlignLeft|Qt::AlignVCenter, curString );

            QRegion reg( 0, 0, xMask + xLeft, rect().bottom() / 2 );
            paint.setClipRegion( reg );
            paint.setPen( QPen(QBrush(linearGradient2), 4) );
            paint.drawText( xLeft, 0, rect().right() - xLeft, rect().bottom() / 2,
                                Qt::AlignLeft|Qt::AlignVCenter, curString );
        }
        else                    // 下行
        {
            /// 在上行的下一行歌词
            paint.setPen( QColor(10, 0, 0));
            paint.drawText( 1, 1, rect().right(), rect().bottom() / 2,
                                Qt::AlignLeft|Qt::AlignVCenter, nextString );
            paint.setPen( QPen(QBrush(linearGradient), 4) );
            paint.drawText( 0, 0, rect().right(), rect().bottom() / 2,
                                Qt::AlignLeft|Qt::AlignVCenter, nextString );

            /// 正在唱的行
            Qt::Alignment align = (metrics.width(curString) > this->width())
                                    ? Qt::AlignLeft|Qt::AlignVCenter
                                        : Qt::AlignRight | Qt::AlignVCenter;
            paint.setPen( QColor(10, 0, 0));
            paint.drawText( xLeft + 1, rect().bottom() / 2 + 1,
                                rect().right() - xLeft, rect().bottom() / 2,
                                    align, curString );

            paint.setPen( QPen(QBrush(linearGradient), 4) );
            paint.drawText( xLeft,  rect().bottom() / 2,
                                rect().right() - xLeft, rect().bottom() / 2,
                                    align, curString );

            QRegion reg( 0,  rect().bottom() / 2, xMask + xLeft, rect().bottom() / 2 );
            paint.setClipRegion( reg );
            paint.setPen( QPen(QBrush(linearGradient2), 4) );
            paint.drawText( xLeft, rect().bottom() / 2,
                                rect().right() - xLeft, rect().bottom() / 2,
                                    align, curString );


        }
    }
    //QLabel::paintEvent(e);      // 必须调用基类的绘制函数才能是 setText() 函数生效
}

void myLrc::wheelEvent(QWheelEvent *e)
{
    int wheel = e->delta() / 40;
    MyWidget* mainWidget = static_cast<MyWidget*>(parentWidget());

    if (!mainWidget->inherits("MyWidget") || !mainWidget )
        qDebug() << "父部件bug myLrc::wheelEvent";

    mainWidget->setVolume( wheel );
}

// 貌似只有在字上面单击才响应该事件
void myLrc::mousePressEvent3( QMouseEvent *event )
{
    if (event->button() == Qt::LeftButton)
        offset = event->globalPos() - frameGeometry().topLeft();
}

// 貌似只要有一次按下拖动，后面即使没有按键，也可以检测到鼠标移动！？！？！？
// 部件内没有设置鼠标跟踪，所以只有按着鼠标移动才能激活该事件，
// QWidget 类的成员函数：void	setMouseTracking(bool enable)
void myLrc::mouseMoveEvent3( QMouseEvent *event )
{
    if ( event->buttons() & Qt::LeftButton )
    {
        setCursor( Qt::PointingHandCursor );
        move( event->globalPos() - offset );
        return ;
    }
}

void myLrc::enterEvent(QEvent *)
{
   // qDebug() << "asda";
}

void myLrc::leaveEvent(QEvent *)
{
}

/// 同时移动工具窗
void myLrc::resizeEvent(QResizeEvent *e)
{
    desTool->move( width() / 2 - 100 + geometry().x(),
                   height() + geometry().y() );

    if ( !_douLine )
        font.setPointSize(e->size().height() * 2 / 5);
    else
        font.setPointSize(e->size().height() * 15 / 40 );
}

void myLrc::moveEvent(QMoveEvent *e)
{
    desTool->move( width() / 2 - 100 + e->pos().x(),
                   height() + e->pos().y() );
}

void myLrc::hideEvent(QHideEvent *)
{
    timer->stop();
    maskTimer->stop();
    curTimer->stop();
    xleftTimer->stop();
    desTool->hide();        // 只能跟随隐藏，不可跟随显示！！！
}

void myLrc::showEvent(QShowEvent *)
{
    timer->start(100);

    if ( player->state() == QMediaPlayer::PlayingState )
    {
        maskTimer->start(22);
        curTimer->start(20);
    }
}


//////////////////////////////////////////////////////////////////
/// \brief toolWindow::toolWindow
/// \param parent
///
/// ////////////////////////////////////////////////////////////////
toolWindow::toolWindow(QMediaPlayer* media, QMenu* menu, QWidget *parent):
    QWidget(parent),
    _mainMenu(menu),
    player(media),
    drag(false)
{
    setWindowFlags( Qt::Tool | Qt::FramelessWindowHint );

    desDlg = new desLrcDialog(this);

    createButton();
    createFontMenu();
   // activateWindow();

    setStyleSheet( myStyleSheet() );

    connect( player, SIGNAL(stateChanged(QMediaPlayer::State)),
                this, SLOT(playStateChanged(QMediaPlayer::State)) );

    connect( player, SIGNAL(mutedChanged(bool)), this, SLOT(setMute(bool)) );
}

///
void toolWindow::paintEvent(QPaintEvent *)
{/*
    QPainter paint(this);
    QPixmap pix("./res/desTool.bmp" );
    paint.drawPixmap( 0, 0, pix );
   */
    QStyleOption opt;   /* 必须才能给窗口QWidget 设置背景图片！？！？？*/
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

///
void toolWindow::mousePressEvent(QMouseEvent *e)
{
    drag = true;
    myLrc* desLrc = static_cast<myLrc*>(parentWidget());

    if ( !desLrc || !desLrc->inherits("myLrc") )
        qDebug() << "父窗口错误toolWindow::mousePressEvent";

    off_tool = e->globalPos() - geometry().topLeft();
    off_lrc = e->globalPos() - desLrc->geometry().topLeft();
}

/// 同时移动工具创、歌词
void toolWindow::mouseMoveEvent(QMouseEvent *e)
{
    myLrc* desLrc = static_cast<myLrc*>(parentWidget());

    if ( !desLrc || !desLrc->inherits("myLrc") )
        qDebug() << "父窗口错误toolWindow::mouseMoveEvent";

    if ( e->buttons() & Qt::LeftButton && drag )
    {
        move( e->globalPos() - off_tool );
        desLrc->move( e->globalPos() - off_lrc );
    }
}

void toolWindow::mouseReleaseEvent(QMouseEvent *)
{
    drag = false;
}

/// 滑轮事件控制主窗口的音量
void toolWindow::wheelEvent(QWheelEvent *e)
{
    int wheel = e->delta() / 40;
    MyWidget* mainWidget = static_cast<MyWidget*>(parentWidget()->parentWidget());

    if ( !mainWidget->inherits("MyWidget"))
        qDebug() << "父部件错误 toolWindow::wheelEvent  ";

    mainWidget->setVolume( wheel );
}

void toolWindow::contextMenuEvent(QContextMenuEvent *e)
{
    QMenu menu;
    myLrc* desWidget = static_cast<myLrc*>(parentWidget());
    MyWidget* mainWidget = static_cast<MyWidget*>(desWidget->parentWidget());

    QAction* bkTrans = menu.addAction("歌词背景穿透");
    QAction* lock = menu.addAction("锁定歌词...");

    menu.addSeparator();
    QAction* doubleLine = menu.addAction("双行显示歌词...");

    QAction* color = menu.addAction("设置歌词颜色");
    color->setIcon( QIcon("./res/color.png") );

    /// 菜单响应在创建的时候连接了
    menu.addMenu( mainWidget->getPlayModeMenu() );
    menu.addMenu( fontMenu );

    bkTrans->setIcon( QIcon("./res/trans.png"));
    lock->setIcon( QIcon("./res/lock.png") );

    bkTrans->setCheckable(true);
    bkTrans->setChecked( desWidget->isTrans() );
    lock->setCheckable(true);
    lock->setChecked( desWidget->isLock() );
    doubleLine->setCheckable( true );
    doubleLine->setChecked( desWidget->isDouLine() );

    QAction* result = menu.exec(e->globalPos() );
    if ( result == bkTrans )
        desWidget->setTrans( bkTrans->isChecked() );
    else if ( result == lock )
        desWidget->setLock( lock->isChecked() );
    else if ( result == doubleLine )
        desWidget->setDouLine( doubleLine->isChecked() );
    else if ( result == color )
    {
        desWidget->backUpColor();
        if ( QDialog::Rejected == desDlg->exec() )
        {
            desWidget->backUpColor(true);       // 恢复没有设置之前的颜色
        }
    }
}

void toolWindow::clickStop()
{
    desButton[DB_STOP]->setDisabled( true );
    desButton[DB_PAUSPLAY]->setChecked( true );
    //myPushButton::playState = S_stop;
    player->stop();
}

void toolWindow::clickPausPlay(bool check)
{
    if ( !check )    // 如果没有勾选=播放状态=暂停图标
    {
        player->play();
        if ( player->state() == QMediaPlayer::PlayingState ) // 如果播放成功
            desButton[DB_STOP]->setDisabled(false); // 启用停止按钮
        else
            desButton[DB_PAUSPLAY]->setChecked(true);   // 播放失败继续保持暂停
    }
    else
        player->pause();
}

void toolWindow::playStateChanged(QMediaPlayer::State state)
{
    if ( myPushButton::L_mode != L_des )
        return ;

    switch ( state )
    {
    case QMediaPlayer::StoppedState:
        desButton[DB_STOP]->setDisabled( true );
        desButton[DB_PAUSPLAY]->setChecked(true);
    //    playNext();   双击另外一首歌曲播放时，会stop之前的。然后状态改变，调用该函数。然后又要播放双击的那首，又要停止该函数播放的再playing双击的那首
        break;

    case QMediaPlayer::PlayingState:
        desButton[DB_STOP]->setDisabled( false );
        desButton[DB_PAUSPLAY]->setChecked( false );
        break;

    case QMediaPlayer::PausedState:
        desButton[DB_STOP]->setDisabled( false );
        desButton[DB_PAUSPLAY]->setChecked( true );
        break;
    default:
        break;
    }
}

/// 主窗口静音按钮同步
void toolWindow::setMute(bool checked)
{
    desButton[DB_MUTE]->setChecked( checked );
}


void toolWindow::createButton()
{
    QList<QRect> list;
    list << QRect(3,5,16,16)   << QRect(26,3,19,20) << QRect(49,5,16,16)
         << QRect(70,5,16,16)  << QRect(89,5,16,16) << QRect(111,5,16,16)
         << QRect(160,5,15,15) << QRect(180,5,15,15)<< QRect(140,5,15,15);

    for ( int i = 0; i < DES_BUTTON_MAX ; i++ )
    {
        desButton[i] = new myPushButton(this);
        desButton[i]->setObjectName( QString("desButton%1").arg(i) );
        desButton[i]->setFocusPolicy( Qt::NoFocus );
        desButton[i]->setGeometry( list.at(i) );
    }
    desButton[DB_PAUSPLAY]->setCheckable( true );       // 勾选=暂停=播放图标
    desButton[DB_PAUSPLAY]->setChecked(true);

    desButton[DB_STOP]->setDisabled( true );

    desButton[DB_MUTE]->setCheckable(true); // checked = 静音

    desButton[DB_ONTOP]->setCheckable( true );

    desButton[DB_MENU]->setIcon( QIcon("./res/TTPlayer.png") );
    desButton[DB_MENU]->setMenu( _mainMenu );   // 本来有箭头的，qss脚本里面去掉了

    myLrc* lrcDes = static_cast<myLrc*>(parentWidget());
    QWidget* mainWidget = parentWidget()->parentWidget();
    connect( desButton[DB_PREV], SIGNAL(clicked()), mainWidget, SLOT(playPrev()) );
    connect( desButton[DB_NEXT], SIGNAL(clicked()), mainWidget, SLOT(playNext()) );
    connect( desButton[DB_PAUSPLAY], SIGNAL(clicked(bool)), this, SLOT(clickPausPlay(bool)) );
    connect( desButton[DB_STOP], SIGNAL(clicked()), this, SLOT(clickStop()) );
    connect( desButton[DB_MUTE], SIGNAL(clicked(bool)), player, SLOT(setMuted(bool)) );
    connect( desButton[DB_WND], SIGNAL(clicked()), mainWidget, SLOT(clickToWnd()) );
    connect( desButton[DB_CLOSE], SIGNAL(clicked()), mainWidget, SLOT(lrcClose()) );
    connect( desButton[DB_ONTOP], SIGNAL(clicked(bool)), lrcDes, SLOT(clickOnTop(bool)) );

    /// toolTip
    desButton[DB_WND]->setToolTip( "返回窗口模式" );
    desButton[DB_CLOSE]->setToolTip( "关闭桌面歌词" );
    desButton[DB_MUTE]->setToolTip( "静音" );
    desButton[DB_ONTOP]->setToolTip( "置顶歌词" );
}

void toolWindow::createFontMenu()
{
    fontMenu = new QMenu( "选择字体", this);

    QStringList menuList;
    menuList << "华文隶书" << "华文行楷" << "华文中宋" << "华文仿宋"
             << "华文宋体" << "华文彩云" << "华文新魏" << "华文楷体"
             << "华文琥珀" << "华文细黑" << "楷体_GB2312" << "宋体"
             << "幼圆" << "Tahoma" << "方正姚体" << "方正舒体"
             << "黑体" << "隶书";

    QActionGroup* group = new QActionGroup(this);

    QAction* temp;
    for ( int i = 0; i < menuList.size(); i++ )
    {
        temp = group->addAction( menuList.at(i) );
        temp->setCheckable(true);
        temp->setData( i );
        fontMenu->addAction( temp );
    }

    myLrc* desLrc = static_cast<myLrc*>(parentWidget());
    connect(fontMenu, SIGNAL(triggered(QAction*)), desLrc, SLOT(setDesFont(QAction*)) );
}



QString toolWindow::myStyleSheet()
{
    return QString("QWidget#desTool{"

       "border-image:   url(./res/desTool.bmp);}"

       "QPushButton#desButton1:checked{border-image: url(./res/play0.png);}"
       "QPushButton#desButton1:hover:checked{border-image: url(./res/play1.png);}"
       "QPushButton#desButton1:pressed:checked{border-image: url(./res/play2.png);}"
       "QPushButton#desButton1:!checked{border-image: url(./res/paus0.png);}"
       "QPushButton#desButton1:hover:!checked{border-image: url(./res/paus1.png);}"
       "QPushButton#desButton1:pressed:!checked{border-image: url(./res/paus2.png);}"

       "QPushButton#desButton2{border-image: url(./res/stop0.png);}"
       "QPushButton#desButton2:hover{border-image: url(./res/stop1.png);}"
       "QPushButton#desButton2:pressed{border-image: url(./res/stop2.png);}"
       "QPushButton#desButton2:disabled{border-image: url(./res/stop3.png);}"

       "QPushButton#desButton3{border-image: url(./res/prev0.png);}"
       "QPushButton#desButton3:hover{border-image: url(./res/prev1.png);}"
       "QPushButton#desButton3:pressed{border-image: url(./res/prev2.png);}"

       "QPushButton#desButton4{border-image: url(./res/next0.png);}"
       "QPushButton#desButton4:hover{border-image: url(./res/next1.png);}"
       "QPushButton#desButton4:pressed{border-image: url(./res/next2.png);}"

       "QPushButton#desButton5{border-image: url(./res/mute0.png);}"
       "QPushButton#desButton5:hover{border-image: url(./res/mute1.png);}"
       "QPushButton#desButton5:pressed{border-image: url(./res/mute2.png);}"
       "QPushButton#desButton5:checked{border-image: url(./res/mute3.png);}"

       "QPushButton#desButton6{border-image: url(./res/window0.png);}"
       "QPushButton#desButton6:hover{border-image: url(./res/window1.png);}"
       "QPushButton#desButton6:pressed{border-image: url(./res/window2.png);}"

       "QPushButton#desButton7{border-image: url(./res/close0.png);}"
       "QPushButton#desButton7:hover{border-image: url(./res/close1.png);}"
       "QPushButton#desButton7:pressed{border-image: url(./res/close0.png);}"
       );
}


