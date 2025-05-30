/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2009 Boudewijn Rempt <boud@valdyas.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */


#include <QAction>

#include <KoPointerEvent.h>
#include <KoCanvasBase.h>
#include <KoCanvasController.h>
#include <KoViewConverter.h>
#include <input/kis_input_manager.h>

#include "kis_tool_polyline_base.h"
#include "kis_canvas2.h"
#include <kis_canvas_resource_provider.h>
#include <KisViewManager.h>
#include <kis_action.h>
#include <kactioncollection.h>
#include <kis_icon.h>

#include "kis_action_registry.h"

#define SNAPPING_THRESHOLD 10
#define SNAPPING_HANDLE_RADIUS 8
#define PREVIEW_LINE_WIDTH 1

KisToolPolylineBase::KisToolPolylineBase(KoCanvasBase * canvas,  KisToolPolylineBase::ToolType type, const QCursor & cursor)
    : KisToolShape(canvas, cursor),
      m_dragging(false),
      m_type(type),
      m_closeSnappingActivated(false)
{
    KisCanvas2 *kritaCanvas = dynamic_cast<KisCanvas2*>(canvas);

    connect(kritaCanvas->viewManager()->canvasResourceProvider(), SIGNAL(sigEffectiveCompositeOpChanged()), SLOT(resetCursorStyle()));
}


void KisToolPolylineBase::activate(const QSet<KoShape *> &shapes)
{
    KisToolShape::activate(shapes);
    connect(action("undo_polygon_selection"), SIGNAL(triggered()), SLOT(undoSelectionOrCancel()), Qt::UniqueConnection);

    KisInputManager *inputManager = (static_cast<KisCanvas2*>(canvas()))->globalInputManager();
    if (inputManager) {
        inputManager->attachPriorityEventFilter(this);
    }
}

void KisToolPolylineBase::deactivate()
{
    disconnect(action("undo_polygon_selection"), 0, this, 0);
    cancelStroke();

    KisInputManager *inputManager = (static_cast<KisCanvas2*>(canvas()))->globalInputManager();
    if (inputManager) {
        inputManager->detachPriorityEventFilter(this);
    }

    KisToolShape::deactivate();
}

void KisToolPolylineBase::requestStrokeEnd()
{
    endStroke();
}

void KisToolPolylineBase::requestStrokeCancellation()
{
    cancelStroke();
}

KisPopupWidgetInterface* KisToolPolylineBase::popupWidget()
{
    return m_dragging || m_type == SELECT ? nullptr : KisToolShape::popupWidget();
}

// Install an event filter to catch right-click events.
// The simplest way to accommodate the popup palette binding.
bool KisToolPolylineBase::eventFilter(QObject *obj, QEvent *event)
{
    Q_UNUSED(obj);

    if (!m_dragging) {
        return false;
    }
    if (event->type() == QEvent::MouseButtonPress ||
        event->type() == QEvent::MouseButtonDblClick) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::RightButton) {
            undoSelectionOrCancel();
            return true;
        }
    } else if (event->type() == QEvent::TabletPress) {
        QTabletEvent *tabletEvent = static_cast<QTabletEvent*>(event);
        if (tabletEvent->button() == Qt::RightButton) {
            undoSelectionOrCancel();
            return true;
        }
    }
    return false;
}

void KisToolPolylineBase::beginPrimaryAction(KoPointerEvent *event)
{
    Q_UNUSED(event);
    NodePaintAbility paintability = nodePaintAbility();
    if ((m_type == PAINT && (!nodeEditable() || paintability == UNPAINTABLE || paintability  == KisToolPaint::CLONE || paintability == KisToolPaint::MYPAINTBRUSH_UNPAINTABLE)) ||
        (m_type == SELECT && !selectionEditable())) {

        if (paintability == KisToolPaint::CLONE){
            KisCanvas2 * kiscanvas = static_cast<KisCanvas2*>(canvas());
            QString message = i18n("This tool cannot paint on clone layers.  Please select a paint or vector layer or mask.");
            kiscanvas->viewManager()->showFloatingMessage(message, koIcon("object-locked"));
        }

        if (paintability == KisToolPaint::MYPAINTBRUSH_UNPAINTABLE) {
            KisCanvas2 * kiscanvas = static_cast<KisCanvas2*>(canvas());
            QString message = i18n("The MyPaint Brush Engine is not available for this colorspace");
            kiscanvas->viewManager()->showFloatingMessage(message, koIcon("object-locked"));
        }

        event->ignore();
        return;
    }

    setMode(KisTool::PAINT_MODE);

    if(m_dragging && m_closeSnappingActivated) {
        m_points.append(m_points.first());
        endStroke();
    } else {
        beginShape();
        m_dragging = true;
    }
}

void KisToolPolylineBase::endPrimaryAction(KoPointerEvent *event)
{
    CHECK_MODE_SANITY_OR_RETURN(KisTool::PAINT_MODE);
    setMode(KisTool::HOVER_MODE);

    if(m_dragging) {
        m_dragStart = convertToPixelCoordAndSnap(event);
        m_dragEnd = m_dragStart;
        m_points.append(m_dragStart);
    }
}

void KisToolPolylineBase::beginPrimaryDoubleClickAction(KoPointerEvent *event)
{
    endStroke();

    // this action will have no continuation
    event->ignore();
}

void KisToolPolylineBase::beginAlternateAction(KoPointerEvent *event, AlternateAction action)
{
    if ((action != ChangeSize && action != ChangeSizeSnap) || !m_dragging) {
        KisToolPaint::beginAlternateAction(event, action);
    }

    if (m_closeSnappingActivated) {
        m_points.append(m_points.first());
    }
    endStroke();
}

void KisToolPolylineBase::mouseMoveEvent(KoPointerEvent *event)
{
    if (m_dragging && !m_points.empty()) {
        // erase old lines on canvas
        QRectF updateRect = dragBoundingRect();
        // get current mouse position
        m_dragEnd = convertToPixelCoordAndSnap(event);
        // draw new lines on canvas
        updateRect |= dragBoundingRect();
        updateCanvasViewRect(updateRect);


        QPointF basePoint = pixelToView(m_points.first());
        m_closeSnappingActivated =
            m_points.size() > 1 &&
            (basePoint - pixelToView(m_dragEnd)).manhattanLength() < SNAPPING_THRESHOLD;

        updateCanvasViewRect(QRectF(basePoint, 2 * QSize(SNAPPING_HANDLE_RADIUS + PREVIEW_LINE_WIDTH, SNAPPING_HANDLE_RADIUS + PREVIEW_LINE_WIDTH)).translated(-SNAPPING_HANDLE_RADIUS + PREVIEW_LINE_WIDTH,-SNAPPING_HANDLE_RADIUS + PREVIEW_LINE_WIDTH));
        KisToolPaint::requestUpdateOutline(event->point, event);
    } else {
        KisToolPaint::mouseMoveEvent(event);
    }
}

void KisToolPolylineBase::undoSelection()
{
    if (m_dragging) {
        // Initialize with the dragging segment's rect
        QRectF updateRect = dragBoundingRect();

        if (m_points.size() > 1) {
            // Add the rect for the last segment
            const QRectF lastSegmentRect =
                pixelToView(QRectF(m_points.last(), m_points.at(m_points.size() - 2)).normalized())
                .adjusted(-PREVIEW_LINE_WIDTH, -PREVIEW_LINE_WIDTH, PREVIEW_LINE_WIDTH, PREVIEW_LINE_WIDTH);
            updateRect = updateRect.united(lastSegmentRect);

            m_points.pop_back();
        }
        m_dragStart = m_points.last();

        // Add the new dragging segment's rect
        updateRect = updateRect.united(dragBoundingRect());
        updateCanvasViewRect(updateRect);
    }
}

void KisToolPolylineBase::undoSelectionOrCancel()
{
    if (m_points.size() > 1) {
        undoSelection();
    } else {
        cancelStroke();
    }
}

void KisToolPolylineBase::paint(QPainter& gc, const KoViewConverter &converter)
{
    Q_UNUSED(converter);

    if (!canvas() || !currentImage())
        return;

    QPointF start, end;
    QPointF startPos;
    QPointF endPos;

    QPainterPath path;
    if (m_dragging && !m_points.empty()) {
        startPos = pixelToView(m_dragStart);
        endPos = pixelToView(m_dragEnd);
        path.moveTo(startPos);
        path.lineTo(endPos);
    }

    for (vQPointF::iterator it = m_points.begin(); it != m_points.end(); ++it) {

        if (it == m_points.begin()) {
            start = (*it);
        } else {
            end = (*it);

            startPos = pixelToView(start);
            endPos = pixelToView(end);
            path.moveTo(startPos);
            path.lineTo(endPos);
            start = end;
        }
    }

    if (m_closeSnappingActivated) {
        QPointF basePoint = pixelToView(m_points.first());
        path.addEllipse(basePoint, SNAPPING_HANDLE_RADIUS, SNAPPING_HANDLE_RADIUS);
    }

    paintToolOutline(&gc, path);
    KisToolPaint::paint(gc,converter);
}

void KisToolPolylineBase::updateArea()
{
    updateCanvasPixelRect(image()->bounds());
}

void KisToolPolylineBase::endStroke()
{
    if (!m_dragging) return;

    m_dragging = false;
    if(m_points.count() > 1) {
        finishPolyline(m_points);
    }
    m_points.clear();
    m_closeSnappingActivated = false;
    updateArea();
    endShape();
}

void KisToolPolylineBase::cancelStroke()
{
    if (!m_dragging) return;

    m_dragging = false;
    m_points.clear();
    m_closeSnappingActivated = false;
    updateArea();
    endShape();
}

QRectF KisToolPolylineBase::dragBoundingRect()
{
    QRectF rect = pixelToView(QRectF(m_dragStart, m_dragEnd).normalized());
    rect.adjust(-PREVIEW_LINE_WIDTH, -PREVIEW_LINE_WIDTH, PREVIEW_LINE_WIDTH, PREVIEW_LINE_WIDTH);
    return rect;
}
