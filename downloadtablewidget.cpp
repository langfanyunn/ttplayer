#include "downloadtablewidget.h"
#include "brower.h"
#include <QMenu>
#include <QContextMenuEvent>

downloadTableWidget::downloadTableWidget(int rows, int columns, QWidget *parent):
    QTableWidget( rows, columns,  parent = 0)
{}

/*
void downloadTableWidget::hasTaskFinshed()
{

}*/


void downloadTableWidget::contextMenuEvent(QContextMenuEvent *e)
{
    QMenu menu;
    QAction* pause = menu.addAction("暂停(&P)");
    QAction* resume = menu.addAction("继续(&R)");
    QAction* del = menu.addAction( "删除任务(&D)" );
    menu.addSeparator();
    QAction* open = menu.addAction("打开文件所在目录..");

    QTableWidgetItem* item = itemAt( e->pos() );
    pause->setDisabled( item == 0 );
    resume->setDisabled( item == 0 );
    del->setDisabled( item == 0 );

    /// 获取被选择的项目
    QStringList strList;
    QList<QTableWidgetItem*> itemList = this->selectedItems();  //

    if ( item != 0 )
    {
        bool hasPause = false, hasDownloading = false;

        // 获取任务标识 strId、禁用一些情况下的菜单
        for ( int i = 0; i < itemList.size(); i++ )
        {
            if ( i % 4 == 0 ) ///莫4，进度条不算选择！！         // 如果是第一列的项目才获取
                strList << itemList.at(i)->statusTip();

            if ( (i + 1) % 4 == 0 ) // 状态栏
            {
                if ( itemList.at(i)->text().contains("暂停"))
                    hasPause = true;
                if ( itemList.at(i)->text().contains("正在下载")
                     || itemList.at(i)->text().contains("等待")  )
                    hasDownloading = true;
            }
        }
        pause->setDisabled( !hasDownloading );
        resume->setDisabled( !hasPause );
    }

    QAction* result = menu.exec(e->globalPos());
    Brower* brow = static_cast<Brower*>( parentWidget()->parentWidget()->parentWidget() );
    if ( !brow->inherits("Brower") )
        qDebug() << "父部件错误！！downloadTableWidget::contextMenuEvent";

    /// 调用 brower 的函数，里面又调用 downloadThread 的函数，里面释放线程、网络之类的
    /// 在本类操作 tableItem，暂停、删除、之类的
    if ( result == pause )
    {
        brow->downloadWndAction( DOWNLOAD_PAUSE, strList );

        // 只改变状态为 等待 的标签
        // 对于正在下载的放到线程内自己改变
        for ( int i = 0; i < itemList.size(); i++ )
        {
            if ( (i + 1) % 4 == 0 && itemList.at(i)->text().contains("等待") )
            {
                itemList.at(i)->setText("暂停");
                itemList.at(i)->setIcon( QIcon("./res/pause.png") );
            }
        }
    }
    else if ( result == resume )
        brow->resumeDownload(itemList);
    else if ( result == del )
    {
        // 移除选中的任务
        brow->downloadWndAction( DOWNLOAD_DELETE, strList );

        for ( int n = 0; n < itemList.size(); n++ )
            if ( n % 4 == 0 )
                this->removeRow( this->row(itemList.at(n)) );
    }
}

/// 目的为了释放一些下载线程。菜单内的鼠标按键不会进入该函数
void downloadTableWidget::mousePressEvent(QMouseEvent *event)
{
    Brower* brow = static_cast<Brower*>( parentWidget()->parentWidget()->parentWidget() );
    if ( !brow->inherits("Brower") )
        qDebug() << "父部件错误！！downloadTableWidget::mousePressEvent";

    brow->releaseThread();

    QTableWidget::mousePressEvent( event );
}





