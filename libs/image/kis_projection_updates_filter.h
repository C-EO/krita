/*
 *  SPDX-FileCopyrightText: 2014 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef __KIS_PROJECTION_UPDATES_FILTER_H
#define __KIS_PROJECTION_UPDATES_FILTER_H

#include "kritaimage_export.h"
#include "kis_image_interfaces.h"

class KisImage;
class KisNode;
class QRect;

class KRITAIMAGE_EXPORT KisProjectionUpdatesFilter
{
public:
    virtual ~KisProjectionUpdatesFilter();

    /**
     * \return true if an update should be dropped by the image
     */
    virtual bool filter(KisImage *image, KisNode *node, const QVector<QRect> &rects, KisProjectionUpdateFlags flags) = 0;
    virtual bool filterRefreshGraph(KisImage *image, KisNode *node, const QVector<QRect> &rect, const QRect &cropRect, KisProjectionUpdateFlags flags) = 0;
};



/**
 * A dummy filter implementation that eats all the updates
 */
class KRITAIMAGE_EXPORT KisDropAllProjectionUpdatesFilter : public KisProjectionUpdatesFilter
{
public:
    bool filter(KisImage *image, KisNode *node, const QVector<QRect> &rects, KisProjectionUpdateFlags flags) override;
    bool filterRefreshGraph(KisImage *image, KisNode *node, const QVector<QRect> &rects, const QRect &cropRect, KisProjectionUpdateFlags flags) override;
};

#endif /* __KIS_PROJECTION_UPDATES_FILTER_H */
