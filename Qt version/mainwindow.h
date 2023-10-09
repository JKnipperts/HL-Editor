
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>



QT_BEGIN_NAMESPACE
class QScrollArea;
class QScrollBar;
class QLabel;
QT_END_NAMESPACE



class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();

protected:
#ifndef QT_NO_CONTEXTMENU
    void contextMenuEvent(QContextMenuEvent *event) override;
#endif // QT_NO_CONTEXTMENU

  //  void paintEvent(QPaintEvent *event) override;
    void closeEvent (QCloseEvent *event) override;
    void mousePressEvent (QMouseEvent *event) override;


private slots:
    void newFile();
    void open();
    void save();
    void griddiag();
    void tilewindowdiag();
    void unitwindowdiag();
    void mapresizediag();
    void setPath();
    void setScale();

private:
    void createActions();
    void createMenus();
    void adjustScrollBar(QScrollBar *scrollBar, double factor);

    QScrollArea *scrollArea;


    QMenu *fileMenu;
    QMenu *editMenu;
    QMenu *configMenu;
    QAction *newAct;
    QAction *openAct;
    QAction *saveAct;
    QAction *exitAct;
    QAction *setPathAct;
    QAction *setScaleFactorAct;
    QAction *showgridAct;
    QAction *showtilewindowAct;
    QAction *showunitwindowAct;
    QAction *mapresizeAct;
    QLabel  *infoLabel;


};

#endif
