/*
 *  SPDX-FileCopyrightText: 2021 Sachin Jindal <jindalsachin01@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <QPainter>

#include "KisReferenceImageCropDecorator.h"
#include <KoShape.h>
#include <KoViewConverter.h>
#include "kis_canvas2.h"
#include "KisHandlePainterHelper.h"

KisReferenceImageCropDecorator::KisReferenceImageCropDecorator()
{
}

void KisReferenceImageCropDecorator::setReferenceImage(KisReferenceImage *referenceImage)
{
    m_referenceImage = referenceImage;
    m_cropBorderRect = referenceImage->cropRect();
}

void KisReferenceImageCropDecorator::paint(QPainter &gc, const KoViewConverter &converter)
{
    if (!m_referenceImage) {
        return;
    }

    gc.save();

    QTransform transform = m_referenceImage->absoluteTransformation();
    QPolygonF shapeBounds = transform.map(QPolygonF(m_referenceImage->outlineRect()));
    QPolygonF m_cropBorder = transform.map(QPolygonF(m_cropBorderRect));

    QPainterPath path;

    path.addPolygon(shapeBounds);
    path.addPolygon(m_cropBorder);
    gc.setPen(Qt::NoPen);
    gc.setBrush(QColor(0, 0, 0, 200));
    gc.drawPath(path);

    KisHandlePainterHelper helper =
            KoShape::createHandlePainterHelperDocument(&gc, m_referenceImage, 5);
    helper.setHandleStyle(KisHandleStyle::primarySelection());

    QPolygonF outline = m_referenceImage->cropRect();

    {
        helper.drawHandleRect(outline.value(0));
        helper.drawHandleRect(outline.value(1));
        helper.drawHandleRect(outline.value(2));
        helper.drawHandleRect(outline.value(3));
        helper.drawHandleRect(0.5 * (outline.value(0) + outline.value(1)));
        helper.drawHandleRect(0.5 * (outline.value(1) + outline.value(2)));
        helper.drawHandleRect(0.5 * (outline.value(2) + outline.value(3)));
        helper.drawHandleRect(0.5 * (outline.value(3) + outline.value(0)));
    }
    gc.restore();

}
