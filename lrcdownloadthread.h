#ifndef LRCDOWNLOADTHREAD_H
#define LRCDOWNLOADTHREAD_H

#include <QThread>
#include <QNetworkReply>

class QFile;
class QNetworkAccessManager;

class lrcDownloadThread : public QThread
{
    Q_OBJECT
public:
    explicit lrcDownloadThread(QString url, QString savePathName, QObject *parent = 0);

    void run(){}

signals:

public slots:
    void readyRead();       // 防止正在下载找到歌词，但是不能打开！！！
    void finished();
    void error(QNetworkReply::NetworkError);

private:
    QNetworkAccessManager* manager;
    QNetworkReply* reply;
    QFile* file;

};

#endif // LRCDOWNLOADTHREAD_H
