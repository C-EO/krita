/*
 *  SPDX-FileCopyrightText: 2011 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef __KIS_TRANSFORM_PROCESSING_VISITOR_H
#define __KIS_TRANSFORM_PROCESSING_VISITOR_H

#include "kis_processing_visitor.h"
#include "KisSelectionBasedProcessingHelper.h"

#include <kis_types.h>

#include <QTransform>

class KisFilterStrategy;


class KRITAIMAGE_EXPORT KisTransformProcessingVisitor : public KisProcessingVisitor
{
public:
    KisTransformProcessingVisitor(qreal  xscale, qreal  yscale,
                                  qreal  xshear, qreal  yshear, qreal angle,
                                  qreal  tx, qreal ty,
                                  KisFilterStrategy *filter,
                                  const QTransform &shapesCorrection = QTransform());

    void setSelection(KisSelectionSP selection);
    KUndo2Command *createInitCommand() override;


    void visit(KisNode *node, KisUndoAdapter *undoAdapter) override;
    void visit(KisPaintLayer *layer, KisUndoAdapter *undoAdapter) override;
    void visit(KisGroupLayer *layer, KisUndoAdapter *undoAdapter) override;
    void visit(KisAdjustmentLayer *layer, KisUndoAdapter *undoAdapter) override;
    void visit(KisExternalLayer *layer, KisUndoAdapter *undoAdapter) override;
    void visit(KisGeneratorLayer *layer, KisUndoAdapter *undoAdapter) override;
    void visit(KisCloneLayer *layer, KisUndoAdapter *undoAdapter) override;
    void visit(KisFilterMask *mask, KisUndoAdapter *undoAdapter) override;
    void visit(KisTransformMask *mask, KisUndoAdapter *undoAdapter) override;
    void visit(KisTransparencyMask *mask, KisUndoAdapter *undoAdapter) override;
    void visit(KisSelectionMask *mask, KisUndoAdapter *undoAdapter) override;
    void visit(KisColorizeMask *mask, KisUndoAdapter *undoAdapter) override;

private:
    void transformClones(KisLayer *layer, KisUndoAdapter *undoAdapter);
    void transformPaintDevice(KisPaintDeviceSP device, KisUndoAdapter *adapter, const ProgressHelper &helper);
    void transformSelection(KisSelectionSP selection, KisUndoAdapter *adapter, const ProgressHelper &helper);

    void transformOneDevice(KisPaintDeviceSP device, KoUpdater *updater);

private:
    qreal m_sx, m_sy;
    qreal m_tx, m_ty;
    qreal m_shearx, m_sheary;
    KisFilterStrategy *m_filter;
    qreal m_angle;
    QTransform m_shapesCorrection;
    KisSelectionBasedProcessingHelper m_selectionHelper;
};

#endif /* __KIS_TRANSFORM_PROCESSING_VISITOR_H */
