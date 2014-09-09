#ifndef MYPUSHBUTTON_H
#define MYPUSHBUTTON_H

#include <QEvent>
#include <QPushButton>

enum WINDOWMODE { W_main = 0, W_mini }; // 是迷你窗口模式还是主窗口模式
enum LRCMODE { L_des = 0, L_wnd, L_mini };        // 歌词模式，三种之一

class myPushButton : public QPushButton
{
    Q_OBJECT
public:
    explicit myPushButton(QWidget *parent = 0);
    myPushButton( const QIcon& icon, const QString& text, QWidget *parent = 0);

  //  static PLAYSTATE playState;
    static WINDOWMODE W_mode;
    static LRCMODE L_mode;
    static bool isLrcShow;

signals:

public slots:
  //  void setMyDisabled(bool);   // 从暂停或停止状态到播放状态，启用停止按钮
  //  void disabled();            // 用于停止按钮，点击了停止按钮，立即禁用停止按钮
 //   void checked();     // 用于点击停止按钮后，将播放按钮设置为暂停状态

protected:
    void enterEvent( QEvent* );
    void leaveEvent(QEvent* );
};

#endif // MYPUSHBUTTON_H
