/*
 *  SPDX-FileCopyrightText: 2016 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef IMAGESHAPE_H
#define IMAGESHAPE_H

#include <QSharedDataPointer>

#include "KoTosContainer.h"
#include <SvgShape.h>

#define ImageShapeId "ImageShape"


class ImageShape : public KoTosContainer, public SvgShape
{
public:
    ImageShape();
    ~ImageShape() override;

    KoShape *cloneShape() const override;

    void paint(QPainter &painter) const override;

    void setSize(const QSizeF &size) override;

    bool saveSvg(SvgSavingContext &context) override;
    bool loadSvg(const QDomElement &element, SvgLoadingContext &context) override;

    void setImage(const QImage &img);
    QImage image() const;

    void setViewBoxTransform(const QTransform &tf);
    QTransform viewBoxTransform() const;

private:
    ImageShape(const ImageShape &rhs);

private:
    struct Private;
    QSharedDataPointer<Private> m_d;
};

#endif // IMAGESHAPE_H
