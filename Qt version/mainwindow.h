
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
    void Open_Map();

protected:
#ifndef QT_NO_CONTEXTMENU
    void contextMenuEvent(QContextMenuEvent *event) override;
#endif // QT_NO_CONTEXTMENU

    void closeEvent (QCloseEvent *event) override;
    void mousePressEvent (QMouseEvent *event) override;
    void mouseDoubleClickEvent (QMouseEvent *event) override;


private slots:
    void newFile_diag();
    void open_diag();
    void open_by_code_diag();
    void save_diag();
    void saveas_diag();
    void add_diag();
    void grid_diag();
    void tilewindow_diag();
    void unitwindow_diag();
    void map_resize_diag();
    void season_diag();
    void replace_diag();
    void buildable_units_diag();
    void setPath_diag();
    void setScale_diag();
    void statistics_diag();
    void warning_diag();
    void maptype_diag();


private:
    void createActions();
    void createMenus();
    void adjustScrollBar(QScrollBar *scrollBar, double factor);


    QMenu *fileMenu;
    QMenu *editMenu;
    QMenu *configMenu;
    QAction *newAct;
    QAction *openAct;
    QAction *openbyCodeAct;
    QAction *saveAct;
    QAction *saveasAct;
    QAction *addtogameAct;
    QAction *exitAct;
    QAction *setPathAct;
    QAction *setScaleFactorAct;
    QAction *showgridAct;
    QAction *showtilewindowAct;
    QAction *showunitwindowAct;
    QAction *mapresizeAct;
    QAction *changeseasonAct;
    QAction *replaceAct;
    QAction *buildableunitsAct;
    QAction *statisticsAct;
    QAction *warningAct;
    QAction *maptypeAct;
    QLabel  *infoLabel;


};

#endif
