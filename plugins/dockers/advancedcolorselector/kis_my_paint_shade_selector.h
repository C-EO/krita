/*
 *  SPDX-FileCopyrightText: 2010 Adam Celarek <kdedev at xibo dot at>
 *  SPDX-FileCopyrightText: 2008 Martin Renold <martinxyz@gmx.ch>
 *  SPDX-FileCopyrightText: 2009 Ilya Portnov <nomail>
 *
 *  This class is based on "lib/colorchanger.hpp" from MyPaint (mypaint.intilinux.com)
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KIS_MY_PAINT_SHADE_SELECTOR_H
#define KIS_MY_PAINT_SHADE_SELECTOR_H

#include "kis_color_selector_base.h"
#include <QColor>
#include <KoColor.h>

class KoColorSpace;

class QTimer;

class KisMyPaintShadeSelector : public KisColorSelectorBase
{
Q_OBJECT
public:
    KisMyPaintShadeSelector(QWidget *parent = 0);

    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;

public:
    void setColor(const KoColor &color) override;

protected Q_SLOTS:
    void canvasResourceChanged(int key, const QVariant& v) override;

protected:
    void paintEvent(QPaintEvent *) override;
    KisColorSelectorBase* createPopup() const override;

private:
    qreal m_colorH {0.0};
    qreal m_colorS {0.0};
    qreal m_colorV {0.0};
    qreal R {0.0};
    qreal G {0.0};
    qreal B {0.0};
    QTimer* m_updateTimer {nullptr};
    KoColor m_lastRealColor;
    KisPaintDeviceSP m_realPixelCache;
    KisPaintDeviceSP m_realCircleBorder;
    const KoColorSpace *m_cachedColorSpace {nullptr};
};

#endif // KIS_MY_PAINT_SHADE_SELECTOR_H
