#ifndef DOWNLOADTABLEWIDGET_H
#define DOWNLOADTABLEWIDGET_H

#include <QTableWidget>

/// 右击菜单操作
#define DOWNLOAD_PAUSE      1
#define DOWNLOAD_RESUME     2
#define DOWNLOAD_DELETE     3


class downloadTableWidget : public QTableWidget
{
    Q_OBJECT
public:
    downloadTableWidget(int rows, int columns, QWidget* parent = 0);

signals:

public slots:

    /// 检测是否有等待下载的任务。。。
   // void hasTaskFinshed();      // 连接 downloadThread 构造函数内的 reply finished() 信号

protected:
    void contextMenuEvent(QContextMenuEvent *);
    void mousePressEvent(QMouseEvent *event);

};

#endif // DOWNLOADTABLEWIDGET_H
