/**
 * Copyright (c) 2015      Los Alamos National Security, LLC
 *                         All rights reserved.
 *
 * This file is part of the Gladius project. See the LICENSE.txt file at the
 * top-level directory of this distribution.
 */

#ifndef TIMELINE_WIDGET_H_INCLUDED
#define TIMELINE_WIDGET_H_INCLUDED

#include "proc-widget.h"
#include "info-types.h"

#include <QBrush>
#include <QPen>
#include <QWidget>
#include <QVBoxLayout>

#include <deque>

class TimelineWidget : public QWidget {
    Q_OBJECT

public:
    TimelineWidget(
        const LegionProfData &profData,
        QWidget *parent = nullptr
    );
    //
    QSize minimumSizeHint() const Q_DECL_OVERRIDE;
    //
    QSize sizeHint() const Q_DECL_OVERRIDE;

public slots:
    void setPen(const QPen &pen);
    //
    void setBrush(const QBrush &brush);
    //
    void setAntialiased(bool antialiased);
    //
    void setTransformed(bool transformed);

protected:
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;

private:
    QPen pen;
    //
    QBrush brush;
    //
    bool antialiased;
    //
    bool transformed;
    //
    QPixmap pixmap;
    //
    std::deque<ProcWidget *> mProcWidgets;
    //
    QVBoxLayout *mProcLayout = nullptr;
    //
    LegionProfData mProfData;
};

#endif // TIMELINE_WIDGET_H_INCLUDED
