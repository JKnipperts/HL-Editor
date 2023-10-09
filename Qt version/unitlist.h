#ifndef UNITLIST_H
#define UNITLIST_H

#include "qwidget.h"


QT_BEGIN_NAMESPACE
class QScrollArea;
class QScrollBar;
QT_END_NAMESPACE


class unitlistwindow : public QWidget
{
    Q_OBJECT
private:

public:

protected:
    void mousePressEvent(QMouseEvent *event);

};
#endif // UNITLIST_H
