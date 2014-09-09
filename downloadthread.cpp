#include "downloadthread.h"
#include "lrcdownloadthread.h"
#include "downloadtablewidget.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>
#include <QSettings>
#include <QDebug>
#include <QTime>
#include <QTimer>
#include <QListWidget>
#include <QMessageBox>
#include <QTableWidgetItem>
#include <QTableWidget>
#include <QNetworkRequest>
#include <QProgressBar>

int downloadThread::threadCounter = 0;

downloadThread::downloadThread(bool isDownloadLrc, QString savePath, QString strId, QListWidgetItem* item,
                                    downloadTableWidget *table, QObject *parent):
    QThread(parent),
    pauseCommand(false),
    hasDownSize(0),
    totalSize(0),
    finshed(false),
    speed(0)
{
    QStringList list = item->whatsThis().split("。");
    if ( list.isEmpty() || list.size() != 9 )
        qDebug() << "downloadThread::downloadThread()构造函数错误";

    //// 获取需要下载的歌曲信息
    QString songname = list.at(0);
    QString songLink = list.at(3);
    QString lrcLink = list.at(4);
    qint64 size = list.at(8).toInt();
    QString sizeStr = QString("%1.%2Mb").arg(size/1000000).arg(size/10000%100);
    QString format = list.at(5);
    QString artist = list.at(1);

    //// 将信息写进 QTableWidgetItem
    songItem    = new QTableWidgetItem(artist + " - " + songname);// 不可乱改，需要根据这个判断东西Brower::downloadItem
    progressBar = new QProgressBar(table);
    speedItem   = new QTableWidgetItem("0.0 kb/s");
    sizeItem    = new QTableWidgetItem(sizeStr);
    statusItem  = new QTableWidgetItem("正在下载...");
    statusItem->setIcon( QIcon("./res/down.png") );

    QString newSongName = savePath + artist + " - " + songname + "." + format + ".tdl";
    songItem->setToolTip( newSongName );   // 储存新建的文件名
    songItem->setStatusTip( strId );
    songItem->setWhatsThis( item->whatsThis() ); // 干脆干所有信息都储存起来了！！！
    songItem->setTextAlignment( Qt::AlignCenter );
    speedItem->setWhatsThis( QVariant(isDownloadLrc).toString() );  // 记录是否下载歌词
    speedItem->setTextAlignment( Qt::AlignCenter );
    sizeItem->setTextAlignment( Qt::AlignCenter );
    statusItem->setTextAlignment( Qt::AlignCenter );

    progressBar->setAlignment( Qt::AlignCenter );   // 设置的是10% 文本进度显示位置
    progressBar->setFixedHeight( 12 );

    int ret = QMessageBox::Yes;
    if ( threadCounter < MAXNUM_THREAD )
    {
        /// 新建一个线程下载歌词，如果有歌词连接才新建线程
        if ( isDownloadLrc && lrcLink.contains("lrc"))
        {
            QString temp = newSongName;
            QString lrcPath = temp.remove( temp.right(7) ) + "lrc";
            lrcDownloadThread* lrcdownload = new lrcDownloadThread( lrcLink, lrcPath );
            lrcdownload->start();
        }
        else if ( !lrcLink.contains("lrc") )
            qDebug() << "歌曲信息内找不到歌词连接";

        /// 检测是否存在同名 歌曲
        QString exists = newSongName;
        exists.remove( exists.right(4) );       // 移除 .tdl 后缀

        if ( QFile::exists(exists) )
        {
            QString note = QString("文件 \"%1\" 已经存在\n是否要覆盖 ???????????????").arg(exists);
            ret = QMessageBox::question( table, "提示", note,
                                         QMessageBox::Yes, QMessageBox::No );
            if ( ret == QMessageBox::Yes )
                QFile::remove( exists );
        }

        /// 开始新建任务、下载
        if ( ret == QMessageBox::Yes )
        {
            file = new QFile( newSongName );
            if ( file->open(QIODevice::WriteOnly) )
            {
                threadCounter++;
                updateSpeedTimer  = new QTimer(this);
                connect( updateSpeedTimer, SIGNAL(timeout()), this, SLOT(updateSpeed()) );
                startDownloadTime = new QTime;

                updateSpeedTimer->start(400);
                startDownloadTime->start();

                manager = new QNetworkAccessManager(this);

                // reply->abort() 到底是切断什么。。。。会切断信号槽吗？！？！？
                // 不可，必须赋值才能连接信号槽！！！！ if ( threadCounter++ < MAXNUM_THREAD )
                reply = manager->get( QNetworkRequest(QUrl(songLink)) );

                connect( reply, SIGNAL(readyRead()), this, SLOT(readyRead()) );
                connect( reply, SIGNAL(downloadProgress(qint64,qint64)),
                            this, SLOT(updateDataProgress(qint64,qint64)) );
                connect( reply, SIGNAL(finished()), this, SLOT(finished()) );
                connect( reply, SIGNAL(error(QNetworkReply::NetworkError)),
                                this, SLOT(error(QNetworkReply::NetworkError)) );

                // 同一个信号两个槽，没有规定顺序调用的！！！
                connect( this, SIGNAL(hasTaskFinshed()), parent, SLOT(hasTaskFinshed()) );
            }
            else
            {
                finshed = true; // 下载失败，以后释放线程
                statusItem->setText( "下载错误" );
                statusItem->setIcon( QIcon("./res/delete.png") );
                statusItem->setTextColor( Qt::red );
                qDebug() << "创建文件失败，无法下载 downloadThread 构造函数内";
            }
        }
        else
            finshed = true;
    }
    else
    {
        finshed = true;
        statusItem->setText( "等待" );
        statusItem->setIcon( QIcon("./res/wait.png"));
    }

    if ( ret == QMessageBox::Yes )
    {
        //// 插入一行 table，将 5 个 item 设置进去
        int row = table->rowCount();
        table->insertRow(row);
        table->setRowHeight(row, 25);
        table->setItem(row, 0, songItem);
        table->setCellWidget( row, 1, progressBar );
        table->setItem(row, 2, speedItem );
        table->setItem(row, 3, sizeItem );
        table->setItem(row, 4, statusItem );
    }
}

/// 用于右击继续下载任务时创建线程下载
/// 歌曲歌词下载信息在 WhatsThis 内
/// 歌曲储存路径名在 tooltip 内
/// 如果日记文件存在，读取、继续上次的下载位置开始下载
/// 如果有一个不存在，重新下载
downloadThread::downloadThread(QTableWidgetItem* item, downloadTableWidget *table,
                                    QObject *parent):
    QThread(parent),
    pauseCommand(false),
    hasDownSize(0),
    totalSize(0),
    finshed(false),
    speed(0)
{
    int row = table->row(item);
    songItem = table->item( row, 0 );
    progressBar = static_cast<QProgressBar*>( table->cellWidget(row, 1) );
    speedItem = table->item( row, 2 );
    sizeItem = table->item( row, 3 );
    statusItem = table->item( row, 4 );

    if ( threadCounter < MAXNUM_THREAD )
    {
        QStringList infoList = item->whatsThis().split("。");
        QString songLink = infoList.at(3);
        QString lrcLink = infoList.at(4);
        QString pathName = item->toolTip(); // 储存的是带 .mp3.tdl 的！！
        QString temp = pathName;
        QString cfgPathName = temp.remove( temp.right(7) ) + "cfg";

        /// 是否下载歌词的bool储存在 speed里面，
        /// 如果有歌词连接才新建线程
        /// 如果歌词已经下载过就不下载 了
        if ( speedItem->whatsThis().contains("true") && lrcLink.contains("lrc"))
        {
            QString temp2 = pathName;
            QString lrcPathName = temp2.remove( temp2.right(7) ) + "lrc";
            if ( !QFile::exists(lrcPathName) )  // 防止重复下载歌词
            {qDebug() << "sda";
                lrcDownloadThread* lrcdownload = new lrcDownloadThread(lrcLink, lrcPathName );
                lrcdownload->start();
            }
        }
        else if ( !lrcLink.contains("lrc") )
            qDebug() << "歌曲信息内找不到歌词连接";

        /// 检测是否已经下载过了
        QString exists = pathName;
        exists.remove( exists.right(4) );
        int ret = QMessageBox::Yes;
        if ( QFile::exists(exists) )
        {
            QString note = QString("文件 \"%1\" 已经存在\n是否要覆盖 ?").arg(exists);
            ret = QMessageBox::question( table, "提示", note,
                                         QMessageBox::Yes, QMessageBox::No );
            if ( ret == QMessageBox::Yes )
                QFile::remove( exists );
        }

        if ( ret == QMessageBox::Yes )
        {
            QNetworkRequest qheader;
            qheader.setUrl( songLink );

            if ( QFile::exists(pathName) && QFile::exists(cfgPathName) )
            {
                // 获取日记记录文件
                QSettings set( cfgPathName, QSettings::IniFormat );
                set.beginGroup( "bytesRead" );
                QString bytesRead = set.value( "size" ).toString();
                hasDownSize = bytesRead.toLongLong();
                totalSize = set.value( "totalSize" ).toLongLong();
                set.endGroup();

                /// 设置服务器从哪里开始传递数据
                QString Range = "bytes=" + bytesRead + "-";
                qheader.setRawHeader( "RANGE", Range.toLatin1() );  // 貌似RANGE 不一定要大写
            }
            else{ qDebug() << "下载日记文件找不到，重新下载..."; }

            file = new QFile(pathName);
            if ( file->open(QIODevice::Append) )
            {
                threadCounter++;

                updateSpeedTimer  = new QTimer(this);
                connect( updateSpeedTimer, SIGNAL(timeout()), this, SLOT(updateSpeed()) );
                startDownloadTime = new QTime;

                updateSpeedTimer->start(400);
                startDownloadTime->start();

                statusItem->setText( "正在下载..." );
                statusItem->setIcon( QIcon("./res/down.png") );

                manager = new QNetworkAccessManager(this);
                reply = manager->get( qheader );
                connect( reply, SIGNAL(readyRead()), this, SLOT(readyRead()) );
                connect( reply, SIGNAL(downloadProgress(qint64,qint64)),
                            this, SLOT(updateDataProgress(qint64,qint64)) );
                connect( reply, SIGNAL(finished()), this, SLOT(finished()) );
                connect( reply, SIGNAL(error(QNetworkReply::NetworkError)),
                                    this, SLOT(error(QNetworkReply::NetworkError)) );
                // 同一个信号两个槽，没有规定顺序调用的！！！
                connect( this, SIGNAL(hasTaskFinshed()), parent, SLOT(hasTaskFinshed()) );
            }
            else
            {
                finshed = true;
                statusItem->setText( "下载错误..." );
                statusItem->setIcon( QIcon("./res/delete.png") );
                statusItem->setBackgroundColor( Qt::red );
                qDebug() << "文件创建失败，无法继续下载";
            }
        }
        else
        {
            finshed = true;
            statusItem->setText("暂停");
            statusItem->setIcon( QIcon("./res/pause.png") );
        }
    }
    else
    {
        finshed = true;
        statusItem->setText( "等待" );
        statusItem->setIcon( QIcon("./res/wait.png"));
    }
}

/// 见 pauseCommand 成员解释
void downloadThread::pauseTask()
{
    pauseCommand = true;
   // 最好不！！！statusItem->setText("暂停");
   // statusItem->setIcon( QIcon("./res/pause.png"));
}

/// 只为了重置 为 false，继续下载的操作放到 Brower::resumeDownload（） 内执行
void downloadThread::resumeTask()
{
    pauseCommand = false;
}

/// 删除任务。兼容删除正在下载、已经下载完成的任务。前提是下载完成的任务之前没有释放线程！！！！！
/// 否则已经完成的任务，在 finshed() 内释放了线程了。不能在进入该线程非法操作！！！！！
void downloadThread::delTask()
{
    // 已经下载完了的，file=0，再删除就崩溃了！！1
    if ( file )
    {
        if ( file->isOpen() )
            file->close();
        delete file;
        file = 0;
    }

    // 已经下载完的任务已经删除了！！不可再删除！！！
    if ( manager )
        manager->deleteLater();
    if ( reply )
        reply->deleteLater();

    this->deleteLater();    // 貌似只要这一行就够了，上面的可以去掉！？！？s

   //貌似没必要，线程没了，里面的计时器焉能存在!?! updateSpeedTimer->stop();
}

/// 如果是继续下载，totalBytes 是断点续传后面那部分数据的大小。 bytesRead 是还是从 0 开始的
/// 标记 finshed ，以后释放线程
/// 记录已经读取的数据大小
/// 设置进度条
/// 计算已经下载的字节所经历过的时间，求出网速
/// 如果检测到用户的暂停命令，切断网络、释放网络、关闭文件、储存已经下载的数据大小到文件、设置标签文本
void downloadThread::updateDataProgress(qint64 bytesRead, qint64 totalBytes)
{
    if ( totalSize != 0 )
        progressBar->setMaximum( totalSize );
    else
        progressBar->setMaximum( totalBytes );
    progressBar->setValue( bytesRead + hasDownSize );

    // 距上次 start() 的时间差
    int currentTime = startDownloadTime->elapsed();
    speed = bytesRead / currentTime * 1000 / 1024;  // 单位 kb/s

    if ( bytesRead + hasDownSize >=
            ((totalSize != 0) ? totalSize : totalBytes) )   // 必须多加一对大括号！！！
    {
        updateSpeedTimer->stop();
        speedItem->setText( "0.0kb/s" );
        statusItem->setText( "完成" );
        statusItem->setIcon( QIcon("./res/done.png") );
       //多余 在 finshed() 内有了 threadCounter--;
    }

    /// 暂停、释放网络
    if ( pauseCommand )
    {
        finshed = true;
        threadCounter--;
        emit hasTaskFinshed();

        // 只有没有暂停前的数据总大小才是个歌曲文件的大小
        if ( hasDownSize == 0 )
            totalSize = totalBytes;
        hasDownSize += bytesRead;   // 每次记录已经下载的数据大小，必须在上面两行后面！！

        if ( reply )
        {
            disconnect( reply, 0, 0, 0 );   // 断开所有所有连接 reply 的信号槽
            reply->abort();
            reply->deleteLater();
            reply = 0;
        }
        if ( file )
        {
            /// 将已经下载的数据大小储存进文件
            QString pathName = file->fileName();
            QString dataFile = pathName.remove( pathName.right(7)) + "cfg" ;
            QSettings set( dataFile, QSettings::IniFormat );
            set.beginGroup( "bytesRead" );
            set.setValue( "size", QString::number(hasDownSize) );
            set.setValue( "totalSize", QString::number(totalSize) );
            set.endGroup();

            if ( file->isOpen() )
            {
                file->flush();
                file->close();
            }
            delete file;
            file = 0;
        }
        if ( manager )
        {
            manager->deleteLater();
            manager = 0;
        }
        updateSpeedTimer->stop();
        speedItem->setText("0.0kb/s");
        statusItem->setText("暂停");
        statusItem->setIcon( QIcon("./res/pause.png"));

        //QSettings set()
    }
}

void downloadThread::error(QNetworkReply::NetworkError)
{
  /*  manager->deleteLater();       下载错误保留网络，删除任务的时候再释放
    reply->deleteLater();
    this->deleteLater();        // 释放线程*/
    file->close();
    delete file;
    file = 0;
}

///
void downloadThread::readyRead()
{
    if ( file )     // 会调用很多次这个函数！！！！
        file->write( reply->readAll() );
}

///
void downloadThread::finished()
{
    if ( file )
    {
        // 去掉mp3 后面先前加的后后缀
        // 删除 cfg 文件，如果有的话
        QString filename = file->fileName();
        QString newName = filename.remove(filename.right(4));
        file->rename( newName );    // 如果有同名文件存在命名会失败！！！！

        QString cfgFile = filename.remove(filename.right(3)) + "cfg";
        if ( QFile::exists(cfgFile) )
            QFile::remove(cfgFile);

        file->flush();
        file->close();
        delete file;
        file = 0;
    }

    if ( reply )    // 以防万一未知的 bug
        reply->deleteLater();
    reply = 0;

    if (manager )
        manager->deleteLater();
    manager = 0;
    finshed = true;
    // 放到 信号finished() 连接的另一个槽内：Brower::hasTaskFished()....threadCounter--;
threadCounter--;
emit hasTaskFinshed();
    //下载完不释放线程，因为 brower 的 hash<...> 里面储存着线程指针，而且还会用到
    // 改为每次切换到下载窗口的时候检测 hash 时候有可以释放的线程。以任务是否下载完毕为标准
    //this->deleteLater();
    // 任务留着没有删除........
}

/// 400毫秒间隔计时器
void downloadThread::updateSpeed()
{
    QString str;
    if ( speed < 1024 )
        str = QString::number(speed, 'f', 1) + "kb/s";
    else
        str = QString::number(speed/1024, 'f', 1) + "Mb/s";
    speedItem->setText(str);
}
