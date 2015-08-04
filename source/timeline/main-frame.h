/**
 * Copyright (c) 2015      Los Alamos National Security, LLC
 *                         All rights reserved.
 *
 * This file is part of the Gladius project. See the LICENSE.txt file at the
 * top-level directory of this distribution.
 */

#ifndef TIMELINE_VIEW_H_INCLUDED
#define TIMELINE_VIEW_H_INCLUDED

#include <QFrame>
#include <QGraphicsView>

QT_BEGIN_NAMESPACE
class QLabel;
class QSlider;
class QToolButton;
QT_END_NAMESPACE

class View;

/**
 * @brief The GraphicsView class
 */
class GraphicsView : public QGraphicsView {
    Q_OBJECT

public:
    GraphicsView(
        View *v
    ) : QGraphicsView()
      , view(v) { }

protected:
#ifndef QT_NO_WHEELEVENT
    void wheelEvent(QWheelEvent *) Q_DECL_OVERRIDE;
#endif

private:
    View *view = nullptr;
};

/**
 * @brief The View class
 */
class View : public QFrame {
    Q_OBJECT
public:
    explicit View(QWidget *parent = nullptr);
    //
    QGraphicsView *view(void) const;

public slots:
    void zoomIn(int level = 1);
    //
    void zoomOut(int level = 1);

private slots:
    void resetView(void);
    //
    void setResetButtonEnabled(void);
    //
    void setupMatrix(void);
    //
    void togglePointerMode(void);
    //
    void toggleAntialiasing(void);
    //
    void print(void);

private:
    GraphicsView *mGraphicsView = nullptr;
    //
    QToolButton *resetButton = nullptr;
    //
    QSlider *zoomSlider = nullptr;
};

#endif // TIMELINE_VIEW_H_INCLUDED