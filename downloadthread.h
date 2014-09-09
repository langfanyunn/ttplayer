#ifndef DOWNLOADTHREAD_H
#define DOWNLOADTHREAD_H

#include <QFile>
#include <QThread>
#include <QNetworkReply>

#define MAXNUM_THREAD   3      // 最多同时下载任务狐数

class QTime;
class QTimer;
class downloadTableWidget;
class QListWidgetItem;
class QTableWidget;
class QNetworkAccessManager;
class QProgressBar;
class QTableWidgetItem;

class downloadThread : public QThread
{
    Q_OBJECT
public:
    explicit downloadThread(bool isDownloadLrc, QString savePath, QString strId, QListWidgetItem* item, downloadTableWidget* table,
                                QObject* parent);

    downloadThread(QTableWidgetItem*, downloadTableWidget *table, QObject* parent);   // 用于继续下载任务

    void run()        // 新建线程就是运行这里的代码了，还有就是各种槽、信号。可以为空。用不上
    {
        //while(1){}
    }

    /// 在下载部件右击菜单事件函数里面调用
    /// id 就是歌名所在 item 储存起来的标识符
   // static void downloadAction(int action, QStringList id);

    /// 右键菜单调用
    void pauseTask();
    void resumeTask();
    void delTask();

    /// 给 brower 类调用
    bool getFinshed()
    {
        return finshed;
    }

    static int threadCounter;   // 已经新建的下载线程数量，限制同时下载任务数量

signals:

    /// 这个信号在构造函数内连接了 brower 的槽 hasTaskFinshed()
    /// 最好在 threadCounter-- 之后才发射，不然没法自动下载等待的任务
    void hasTaskFinshed();   // 有任务完成、暂停、错误。threadCounter-- 发射这个信号

public slots:
    void updateDataProgress(qint64,qint64);

    void error(QNetworkReply::NetworkError);
    void readyRead();
    void finished();

    /// 连接 updateSpeedTimer
    void updateSpeed();

private:

    /// 右键暂停后(如果内找到该线程)，设置对应线程的该值为 true
    /// 该值又在 updateDateProgress() 槽内实时检测是否为 true
    /// 如果为 true 就 abort() reply。这样做的目的是为了防止线程互相干扰。
    /// 由 pauseTasj() 调用
    bool pauseCommand;      // 该值用来个右键任务暂停设置的

    /// 在 updateDataProgress() 内赋值
    /// 用来在恢复任务的时候断点续传
    qint64 hasDownSize;     // 储存已经下载了的字节数

    /// 记录第一次暂停前，整个网路数据的大小
    qint64 totalSize;       // 记录，用来更新进度条，如果继续下载，进
                            // 度条信号的后面那个参数是这次发送数据的大小。而不是整个文件的大小了

    /// 构造函数内创建文件失败也设置 = true；
    /// 任务被暂停也设置该值为 true，表示需要释放线程，继续下载的时候重新创建线程了
    /// 用来在遍历在 brower 的成员 downloadHash<...>
    bool finshed;       // 在 finshed() 槽内设置

    // 连接 updateSpeed() 槽
    QTimer* updateSpeedTimer;   // 每隔400ms更新一次速度 item
    QTime* startDownloadTime;   // 下载开始的时候计时 start()
    double speed;       // 已经下载的 bytes / 经历过的时间得到速度

    QFile* file;
    QNetworkAccessManager *manager;
    QNetworkReply * reply;

    QProgressBar *progressBar;
    QTableWidgetItem *songItem;
    QTableWidgetItem *speedItem;
    QTableWidgetItem *sizeItem;
    QTableWidgetItem *statusItem;
};

#endif // DOWNLOADTHREAD_H
