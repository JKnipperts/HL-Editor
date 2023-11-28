#ifndef TILELIST_H
#define TILELIST_H

#include "qwidget.h"


QT_BEGIN_NAMESPACE
class QScrollArea;
class QScrollBar;
QT_END_NAMESPACE


class tilelistwindow : public QWidget
{
    Q_OBJECT
private:

public:

protected:
    void mousePressEvent(QMouseEvent *event) override;
  //  void closeEvent (QCloseEvent *event) override;

};

#endif // TILELIST_H
