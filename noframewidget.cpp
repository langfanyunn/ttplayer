#include "noframewidget.h"
#include <QDebug>

int noFrameWidget::skin_index = 0;
noFrameWidget::noFrameWidget(QWidget *parent) :
    QWidget(parent),
    drag(false)
{
}

void noFrameWidget::mousePressEvent(QMouseEvent *event)
{
    drag = true;
    switch(event->button())
    {
    case Qt::LeftButton:
       // isLeftPressDown = true;
        if( dir != NONE )
            this->grabMouse();       // 捕获鼠标所有信息，发送给当前部件
        else
            dragPosition = event->globalPos() - this->frameGeometry().topLeft();
        break;

    default:
        QWidget::mousePressEvent(event);
    }
}

void noFrameWidget::mouseMoveEvent(QMouseEvent *event)
{
    QPoint gloPoint = event->globalPos();   // 鼠标在屏幕的坐标
    QRect rect = this->rect();  // 0,0,400,300，客户区范围
    QPoint tl = mapToGlobal(rect.topLeft());    // 将左上角转换为屏幕坐标700,100
    QPoint rb = mapToGlobal(rect.bottomRight());    // 右下角在屏幕的坐标，1099，399

    if ( !(event->buttons() & Qt::LeftButton) )
        this->region(gloPoint);
    else
    {
        if(dir != NONE)
        {
            QRect rMove(tl, rb);

            switch(dir)
            {
                case LEFT:
                    if(rb.x() - gloPoint.x() <= this->minimumWidth())
                        rMove.setX(tl.x());
                    else
                        rMove.setX(gloPoint.x());
                    break;
                case RIGHT:
                    rMove.setWidth(gloPoint.x() - tl.x());
                    break;
                case UP:
                    if(rb.y() - gloPoint.y() <= this->minimumHeight())
                        rMove.setY(tl.y());
                    else if ( rb.y() - gloPoint.y() <= this->maximumHeight() )
                        rMove.setY(gloPoint.y());
                    break;
                case DOWN:
                    rMove.setHeight(gloPoint.y() - tl.y());
                    break;
                case LEFTTOP:
                    if(rb.x() - gloPoint.x() <= this->minimumWidth())
                        rMove.setX(tl.x());
                    else
                        rMove.setX(gloPoint.x());
                    if(rb.y() - gloPoint.y() <= this->minimumHeight())
                        rMove.setY(tl.y());
                    else if ( rb.y() - gloPoint.y() <= this->maximumHeight() )
                        rMove.setY(gloPoint.y());
                    break;
                case RIGHTTOP:
                    rMove.setWidth(gloPoint.x() - tl.x());
                    if ( rb.y() - gloPoint.y() <= this->minimumHeight() )
                        rMove.setY(tl.y());
                    else if ( rb.y() - gloPoint.y() <= this->maximumHeight() )
                        rMove.setY(gloPoint.y());
                    break;
                case LEFTBOTTOM:
                    if(rb.x() - gloPoint.x() <= this->minimumWidth())
                        rMove.setX(tl.x());
                    else
                        rMove.setX(gloPoint.x());
                    rMove.setHeight(gloPoint.y() - tl.y());
                    break;
                case RIGHTBOTTOM:
                    rMove.setWidth(gloPoint.x() - tl.x());
                    rMove.setHeight(gloPoint.y() - tl.y());
                    break;
                default:

                    break;
            }
            this->setGeometry(rMove);
        }
        else if ( drag )
        {
            move(event->globalPos() - dragPosition);
            event->accept();
        }
    }
    QWidget::mouseMoveEvent(event);

   // if ( (event->buttons() & Qt::LeftButton) && drag )
    //  move( event->globalPos() - off_point );
}

void noFrameWidget::mouseReleaseEvent(QMouseEvent *event)
{
    // 貌似即使不捕获，也可以在窗口外检测到鼠标信息！？？！
    if( event->button() == Qt::LeftButton )
    {
     //   isLeftPressDown = false;
        if( dir != NONE )
        {
            this->releaseMouse();       // 释放 mouseGrabber()
            this->setCursor(QCursor(Qt::ArrowCursor));
        }
    }
    drag = false;
}

void noFrameWidget::region(const QPoint &cursorGlobalPoint)
{
    //  QRect rect = this->rect();
      //QPoint tl = this->mapToGlobal(rect.topLeft());
      //QPoint rb = this->mapToGlobal(rect.bottomRight());
      int x = cursorGlobalPoint.x() - frameGeometry().x();  // 鼠标在屏幕的坐标
      int y = cursorGlobalPoint.y() - frameGeometry().y();

      if( x <= PADDING && y <= PADDING )
      {
          dir = LEFTTOP;
          this->setCursor(QCursor(Qt::SizeFDiagCursor));
      }
      else if( x >= width() - PADDING && y >= height() - PADDING )
      {
          dir = RIGHTBOTTOM;
          this->setCursor(QCursor(Qt::SizeFDiagCursor));
      }
      else if( x <= PADDING && y >= height() - PADDING )  // 左下
      {
          dir = LEFTBOTTOM;
          this->setCursor(QCursor(Qt::SizeBDiagCursor));
      }
      else if( x >= width() - PADDING && y <= PADDING )   // 右上
      {
          dir = RIGHTTOP;
          this->setCursor(QCursor(Qt::SizeBDiagCursor));
      }
      else if( x <= PADDING )        // 左边
      {
          dir = LEFT;
          this->setCursor(QCursor(Qt::SizeHorCursor));
      }
      else if( x >= width() - PADDING )         // 右边
      {
          dir = RIGHT;
          this->setCursor(QCursor(Qt::SizeHorCursor));
      }
      else if( y <= PADDING )        // 上边
      {
          dir = UP;
          this->setCursor(QCursor(Qt::SizeVerCursor));
      }
      else if( y >= height() - PADDING )// 下边
      {
          dir = DOWN;
          this->setCursor(QCursor(Qt::SizeVerCursor));
      }
      else
      {
          dir = NONE;
          this->setCursor(QCursor(Qt::ArrowCursor));
      }
}
