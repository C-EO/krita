/* This file is part of the KDE project

   SPDX-FileCopyrightText: 2006 Thorsten Zachmann <zachmann@kde.org>
   SPDX-FileCopyrightText: 2006-2007 Thomas Zander <zander@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef SELECTIONDECORATOR_H
#define SELECTIONDECORATOR_H

#include <KoViewConverter.h>
#include <KoFlake.h>

#include "KoShapeMeshGradientHandles.h"

#include <QPainter>

class KoSelection;
class KoCanvasResourceProvider;

static const struct DecoratorIconPositions {
    QPoint uiOffset = QPoint(0, 40);
} decoratorIconPositions;

/**
 * The SelectionDecorator is used to paint extra user-interface items on top of a selection.
 */
class SelectionDecorator
{
public:
    /**
     * Constructor.
     * @param arrows the direction that needs highlighting. (currently unused)
     * @param rotationHandles if true; the rotation handles will be drawn
     * @param shearHandles if true; the shearhandles will be drawn
     */
    SelectionDecorator(KoCanvasResourceProvider *resourceManager);
    ~SelectionDecorator() {}

    /**
     * paint the decorations.
     * @param painter the painter to paint to.
     * @param converter to convert between internal and view coordinates.
     */
    void paint(QPainter &painter, const KoViewConverter &converter);

    /**
     * set the selection that is to be painted.
     * @param selection the current selection.
     */
    void setSelection(KoSelection *selection);

    /**
     * set the radius of the selection handles
     * @param radius the new handle radius
     */
    void setHandleRadius(int radius);

    /**
     * set the thickness of decoration lines, used for HiDPI support.
     * @param thickness -- the new thickness
     */
    void setDecorationThickness(int thickness);

    /**
     * Set true if you want to render gradient handles on the canvas.
     * Default value: false
     */
    void setShowFillGradientHandles(bool value);

    /**
     * Set true if you want to render gradient handles on the canvas.
     * Default value: false
     */
    void setShowStrokeFillGradientHandles(bool value);

    void setShowFillMeshGradientHandles(bool value);
    void setCurrentMeshGradientHandles(const KoShapeMeshGradientHandles::Handle &selectedHandle,
                                       const KoShapeMeshGradientHandles::Handle &hoveredHandle);

    void setForceShapeOutlines(bool value);

private:
    void paintGradientHandles(KoShape *shape, KoFlake::FillVariant fillVariant, QPainter &painter, const KoViewConverter &converter);

    void paintMeshGradientHandles(KoShape *shape, KoFlake::FillVariant fillVariant, QPainter &painter, const KoViewConverter &converter);

private:
    KoFlake::AnchorPosition m_hotPosition;
    KoSelection *m_selection {nullptr};
    KoShapeMeshGradientHandles::Handle m_currentHoveredMeshHandle;
    KoShapeMeshGradientHandles::Handle m_selectedMeshHandle;
    int m_handleRadius {7};
    int m_decorationThickness {1};
    bool m_showFillGradientHandles {false};
    bool m_showStrokeFillGradientHandles {false};
    bool m_showFillMeshGradientHandles {false};
    bool m_forceShapeOutlines {false};
};

#endif
