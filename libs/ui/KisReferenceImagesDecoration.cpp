/*
 * SPDX-FileCopyrightText: 2016 Boudewijn Rempt <boud@valdyas.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KisReferenceImagesDecoration.h"

#include "KoShapeManager.h"

#include "kis_algebra_2d.h"
#include "KisDocument.h"
#include "KisReferenceImagesLayer.h"
#include "kis_layer_utils.h"

struct KisReferenceImagesDecoration::Private {
    struct Buffer
    {
        /// Top left corner of the buffer relative to the viewport
        QPointF position;
        QImage image;

        QRectF bounds() const
        {
            return QRectF(position, image.size() / image.devicePixelRatio());
        }
    };

    KisReferenceImagesDecoration *q;

    KisWeakSharedPtr<KisReferenceImagesLayer> layer;
    Buffer buffer;
    QTransform previousTransform;
    QSizeF previousViewSize;

    explicit Private(KisReferenceImagesDecoration *q)
        : q(q)
    {}

    void updateBufferByImageCoordinates(const QRectF &dirtyImageRect)
    {
        QRectF dirtyWidgetRect = q->view()->viewConverter()->imageToWidget(dirtyImageRect);
        updateBuffer(dirtyWidgetRect, dirtyImageRect);
    }

    void updateBufferByWidgetCoordinates(const QRectF &dirtyWidgetRect)
    {
        QRectF dirtyImageRect = q->view()->viewConverter()->widgetToImage(dirtyWidgetRect);
        updateBuffer(dirtyWidgetRect, dirtyImageRect);
    }

private:
    void updateBuffer(QRectF widgetRect, QRectF imageRect)
    {
        KisCoordinatesConverter *viewConverter = q->view()->viewConverter();
        QTransform transform = viewConverter->imageToWidgetTransform();

        qreal devicePixelRatioF = q->view()->devicePixelRatioF();
        if (buffer.image.isNull() || !buffer.bounds().contains(widgetRect)) {
            const QRectF boundingImageRect = layer->boundingImageRect();
            const QRectF boundingWidgetRect = q->view()->viewConverter()->imageToWidget(boundingImageRect);
            widgetRect = boundingWidgetRect.intersected(q->view()->rect());

            if (widgetRect.isNull()) return;

            buffer.position = widgetRect.topLeft();
            // to ensure that buffer is big enough for all the pixels on high dpi displays
            // BUG 411118
            buffer.image = QImage((widgetRect.size()*devicePixelRatioF).toSize(), QImage::Format_ARGB32);
            buffer.image.setDevicePixelRatio(devicePixelRatioF);

            imageRect = q->view()->viewConverter()->widgetToImage(widgetRect);

        }

        QPainter gc(&buffer.image);

        gc.translate(-buffer.position);
        gc.setTransform(transform, true);

        gc.save();
        gc.setCompositionMode(QPainter::CompositionMode_Source);
        gc.fillRect(imageRect, Qt::transparent);
        gc.restore();

        // to ensure that clipping rect is also big enough for all the pixels
        // BUG 411118
        gc.setClipRect(QRectF(imageRect.topLeft(), imageRect.size()*devicePixelRatioF));
        layer->paintReferences(gc);
    }
};

KisReferenceImagesDecoration::KisReferenceImagesDecoration(QPointer<KisView> parent, KisDocument *document, bool viewReady)
    : KisCanvasDecoration("referenceImagesDecoration", parent)
    , d(new Private(this))
{
    connect(document->image().data(), SIGNAL(sigNodeAddedAsync(KisNodeSP, KisNodeAdditionFlags)), this, SLOT(slotNodeAdded(KisNodeSP, KisNodeAdditionFlags)));
    connect(document->image().data(), SIGNAL(sigRemoveNodeAsync(KisNodeSP)), this, SLOT(slotNodeRemoved(KisNodeSP)));
    connect(document->image().data(), SIGNAL(sigLayersChangedAsync()), this, SLOT(slotLayersChanged()));
    connect(document, &KisDocument::sigReferenceImagesLayerChanged, this, qOverload<KisNodeSP>(&KisReferenceImagesDecoration::slotNodeAdded));

    auto referenceImageLayer = document->referenceImagesLayer();
    if (referenceImageLayer) {
        setReferenceImageLayer(referenceImageLayer, /* updateCanvas = */ viewReady);
    }
}

KisReferenceImagesDecoration::~KisReferenceImagesDecoration()
{}

void KisReferenceImagesDecoration::addReferenceImage(KisReferenceImage *referenceImage)
{
    KUndo2Command *cmd = KisReferenceImagesLayer::addReferenceImages(view()->document(), {referenceImage});
    view()->canvasBase()->addCommand(cmd);
}

bool KisReferenceImagesDecoration::documentHasReferenceImages() const
{
    return view()->document()->referenceImagesLayer() != nullptr;
}

void KisReferenceImagesDecoration::drawDecoration(QPainter &gc, const QRectF &/*updateRect*/, const KisCoordinatesConverter *converter, KisCanvas2 */*canvas*/)
{
    // TODO: can we use partial updates here?

    KisSharedPtr<KisReferenceImagesLayer> layer = d->layer.toStrongRef();

    if (!layer.isNull()) {
        QSizeF viewSize = view()->size();

        QTransform transform = converter->imageToWidgetTransform();
        if (d->previousViewSize != viewSize || !KisAlgebra2D::fuzzyMatrixCompare(transform, d->previousTransform, 1e-4)) {
            d->previousViewSize = viewSize;
            d->previousTransform = transform;
            d->buffer.image = QImage();
            d->updateBufferByWidgetCoordinates(QRectF(QPointF(0,0), viewSize));
        }

        if (!d->buffer.image.isNull()) {
            gc.drawImage(d->buffer.position, d->buffer.image);
        }
    }
}

void KisReferenceImagesDecoration::slotNodeAdded(KisNodeSP node)
{
    slotNodeAdded(node, KisNodeAdditionFlag::None);
}

void KisReferenceImagesDecoration::slotNodeAdded(KisNodeSP node, KisNodeAdditionFlags flags)
{
    Q_UNUSED(flags)

    KisReferenceImagesLayer *referenceImagesLayer =
        dynamic_cast<KisReferenceImagesLayer*>(node.data());

    if (referenceImagesLayer) {
        setReferenceImageLayer(referenceImagesLayer, /* updateCanvas = */ true);
    }
}

void KisReferenceImagesDecoration::slotNodeRemoved(KisNodeSP node)
{
    KisReferenceImagesLayer *referenceImagesLayer =
        dynamic_cast<KisReferenceImagesLayer*>(node.data());

    if (referenceImagesLayer && referenceImagesLayer == d->layer) {
        setReferenceImageLayer(0, true);
    }
}

void KisReferenceImagesDecoration::slotLayersChanged()
{
    KisImageSP image = view()->image();

    KisReferenceImagesLayer *referenceImagesLayer =
        KisLayerUtils::findNodeByType<KisReferenceImagesLayer>(image->root());

    setReferenceImageLayer(referenceImagesLayer, true);
}

void KisReferenceImagesDecoration::slotReferenceImagesChanged(const QRectF &dirtyRect)
{
    d->updateBufferByImageCoordinates(dirtyRect);

    QRectF documentRect = view()->viewConverter()->imageToDocument(dirtyRect);
    view()->canvasBase()->updateCanvasDecorations(documentRect);
}

void KisReferenceImagesDecoration::setReferenceImageLayer(KisSharedPtr<KisReferenceImagesLayer> layer, bool updateCanvas)
{
    if (d->layer != layer.data()) {
        KisSharedPtr<KisReferenceImagesLayer> oldLayer = d->layer.toStrongRef();
        if (oldLayer) {
            oldLayer->disconnect(this);
        }

        d->layer = layer;

        if (layer) {
            connect(layer.data(), SIGNAL(sigUpdateCanvas(QRectF)),
                    this, SLOT(slotReferenceImagesChanged(QRectF)));

            const QRectF dirtyRect = layer->boundingImageRect();

            // If the view is not ready yet (because this is being constructed
            // from view.d's ctor and thus view.d is not available now),
            // do not update canvas because it will lead to a crash.
            if (updateCanvas && !dirtyRect.isEmpty()) { // in case the reference layer is just being loaded from the .kra file
                slotReferenceImagesChanged(dirtyRect);
            }
        }
    }
}
