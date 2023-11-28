#ifndef BUILDABLE_H
#define BUILDABLE_H

#include "qwidget.h"


QT_BEGIN_NAMESPACE
class QScrollArea;
class QScrollBar;
QT_END_NAMESPACE


class buildablewindow : public QWidget
{
    Q_OBJECT
private:

public:

protected:
    void mousePressEvent(QMouseEvent *event) override;
   // void closeEvent (QCloseEvent *event) override;

};

#endif // BUILDABLE_H
