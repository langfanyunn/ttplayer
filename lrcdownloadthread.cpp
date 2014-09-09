#include "lrcdownloadthread.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QUrl>
#include <QFile>
#include <QDebug>
#include <QTextCodec>
#include <QString>

lrcDownloadThread::lrcDownloadThread(QString url, QString savePath, QObject *parent) :
    QThread(parent)
{
    // 传递过来的歌曲路径名包含  .mp3.tdl  的
   //改成/已经转换过的了！！！！ QString lrcPathName = musicPath.remove( musicPath.right(7) ) + "lrc";

    QFile f(savePath);
    if ( !f.exists() || f.size() < 4 )
    {
        file = new QFile(savePath);
        if ( file->open(QIODevice::WriteOnly |QIODevice::Append) )
        {
            manager = new QNetworkAccessManager;
            reply = manager->get( QNetworkRequest(QUrl(url)) );

          //  connect(reply, SIGNAL(readyRead()), this, SLOT(readyRead()) );
            connect(reply, SIGNAL(finished()), this, SLOT(finished()));
            connect(reply, SIGNAL(error(QNetworkReply::NetworkError)),
                        this, SLOT(error(QNetworkReply::NetworkError)) );
        }
        else
        {
            this->deleteLater();
            qDebug() << "歌词文件创建失败，无法下载！";
        }
    }
}

void lrcDownloadThread::readyRead()
{
    if ( file )
        file->write( reply->readAll() );
}

void lrcDownloadThread::finished()
{
    QString s(reply->readAll()) ;
    char c[3] = {-17, -69, -65};        // 貌似要自己写文件头！！！
    file->write(c);
    file->write( s.toUtf8() );
    file->flush();
    file->close();
    delete file;
    file = 0;

    // 貌似不用了，manager 设置了父对象，会自动释放？？！？
    if ( manager )
        manager->deleteLater();
    manager = 0;

    if ( reply )
        reply->deleteLater();
    reply = 0;

    this->deleteLater();
}

void lrcDownloadThread::error(QNetworkReply::NetworkError)
{
    this->deleteLater();
}
