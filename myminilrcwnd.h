#ifndef MYMINILRCWND_H
#define MYMINILRCWND_H
#include "noframewidget.h"
#include <QMediaPlayer>
#include <QFont>

#define SPACE   30  // 每行歌词之间距离

class QTimer;

class myMiniLrcWnd : public noFrameWidget
{
    Q_OBJECT
public:
    explicit myMiniLrcWnd(QMap<qint64, QString>*, QMediaPlayer*, QWidget *parent = 0);

    void init();
signals:

public slots:
    void checkCurRow();

    void stateChanged(QMediaPlayer::State state);

    void scrollSlot();

protected:
    void contextMenuEvent(QContextMenuEvent *);

    void wheelEvent(QWheelEvent *);
    void paintEvent(QPaintEvent *);
    void resizeEvent(QResizeEvent *);
    void showEvent(QShowEvent*);
    void hideEvent(QHideEvent *);
private:

    /// 检测当前行
    QTimer* curTimer;

    QTimer* scrollTimer;

    int _x;     // 第一行歌词实时位置
  //  int _xToCur;        // _x 都当前行后面的距离，包含当前行的 SPACE
    int currow;
    QString curString;

    QFont lrcFont;
    QColor lrcColor, curColor;

    QMediaPlayer* player;

    QMap<qint64, QString>* map;
};

#endif // MYMINILRCWND_H
