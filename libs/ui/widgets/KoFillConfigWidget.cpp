/* This file is part of the KDE project
 * Made by Tomislav Lukman (tomislav.lukman@ck.tel.hr)
 * SPDX-FileCopyrightText: 2012 Jean-Nicolas Artaud <jeannicolasartaud@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KoFillConfigWidget.h"

#include <QToolButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QButtonGroup>
#include <QSizePolicy>
#include <QBitmap>
#include <QAction>
#include <QSharedPointer>
#include <QMessageBox>

#include <klocalizedstring.h>

#include <KoIcon.h>
#include <KoColor.h>
#include <KoColorPopupAction.h>
#include "KoResourceServerProvider.h"
#include <KoSelection.h>
#include <KoCanvasBase.h>
#include <KoCanvasResourceProvider.h>
#include <KoDocumentResourceManager.h>
#include <KoShape.h>
#include <KoShapeController.h>
#include <KoShapeBackground.h>
#include <KoShapeBackgroundCommand.h>
#include <KoShapeStrokeCommand.h>
#include <KoShapeStroke.h>
#include <KoSelectedShapesProxy.h>
#include <KoColorBackground.h>
#include <KoGradientBackground.h>
#include <KoPatternBackground.h>
#include <KoResourcePopupAction.h>
#include "KoZoomHandler.h"
#include "KoColorPopupButton.h"
#include "ui_KoFillConfigWidget.h"
#include <kis_signals_blocker.h>
#include <kis_signal_compressor_with_param.h>
#include <kis_acyclic_signal_connector.h>
#include <kis_assert.h>
#include "kis_canvas_resource_provider.h"
#include <KoStopGradient.h>
#include <QInputDialog>
#include <KoShapeFillWrapper.h>
#include <functional>

#include <KisResourceUserOperations.h>

#include "kis_global.h"
#include "kis_debug.h"

static const char* const buttonnone[]={
    "16 16 3 1",
    "# c #000000",
    "e c #ff0000",
    "- c #ffffff",
    "################",
    "#--------------#",
    "#-e----------e-#",
    "#--e--------e--#",
    "#---e------e---#",
    "#----e----e----#",
    "#-----e--e-----#",
    "#------ee------#",
    "#------ee------#",
    "#-----e--e-----#",
    "#----e----e----#",
    "#---e------e---#",
    "#--e--------e--#",
    "#-e----------e-#",
    "#--------------#",
    "################"};

static const char* const buttonsolid[]={
    "16 16 2 1",
    "# c #000000",
    ". c #969696",
    "################",
    "#..............#",
    "#..............#",
    "#..............#",
    "#..............#",
    "#..............#",
    "#..............#",
    "#..............#",
    "#..............#",
    "#..............#",
    "#..............#",
    "#..............#",
    "#..............#",
    "#..............#",
    "#..............#",
    "################"};


// FIXME: Smoother gradient button.

static const char* const buttongradient[]={
    "16 16 15 1",
    "# c #000000",
    "n c #101010",
    "m c #202020",
    "l c #303030",
    "k c #404040",
    "j c #505050",
    "i c #606060",
    "h c #707070",
    "g c #808080",
    "f c #909090",
    "e c #a0a0a0",
    "d c #b0b0b0",
    "c c #c0c0c0",
    "b c #d0d0d0",
    "a c #e0e0e0",
    "################",
    "#abcdefghijklmn#",
    "#abcdefghijklmn#",
    "#abcdefghijklmn#",
    "#abcdefghijklmn#",
    "#abcdefghijklmn#",
    "#abcdefghijklmn#",
    "#abcdefghijklmn#",
    "#abcdefghijklmn#",
    "#abcdefghijklmn#",
    "#abcdefghijklmn#",
    "#abcdefghijklmn#",
    "#abcdefghijklmn#",
    "#abcdefghijklmn#",
    "#abcdefghijklmn#",
    "################"};

static const char* const buttonpattern[]={
    "16 16 4 1",
    ". c #0a0a0a",
    "# c #333333",
    "a c #a0a0a0",
    "b c #ffffffff",
    "################",
    "#aaaaabbbbaaaaa#",
    "#aaaaabbbbaaaaa#",
    "#aaaaabbbbaaaaa#",
    "#aaaaabbbbaaaaa#",
    "#aaaaabbbbaaaaa#",
    "#bbbbbaaaabbbbb#",
    "#bbbbbaaaabbbbb#",
    "#bbbbbaaaabbbbb#",
    "#bbbbbaaaabbbbb#",
    "#aaaaabbbbaaaaa#",
    "#aaaaabbbbaaaaa#",
    "#aaaaabbbbaaaaa#",
    "#aaaaabbbbaaaaa#",
    "#aaaaabbbbaaaaa#",
    "################"};

using namespace std::placeholders;


class Q_DECL_HIDDEN KoFillConfigWidget::Private
{
public:
    Private(KoFlake::FillVariant _fillVariant, KoFillConfigWidget* q)
    : canvas(0),
      colorChangedCompressor(100, std::bind(&KoFillConfigWidget::colorChanged, q, _1)),
      gradientChangedCompressor(100, KisSignalCompressor::FIRST_ACTIVE),
      shapeChangedCompressor(200,KisSignalCompressor::FIRST_ACTIVE),
      fillVariant(_fillVariant),
      noSelectionTrackingMode(false)
    {
    }

    KoColorPopupAction *colorAction {nullptr};
    KoResourcePopupAction *gradientAction {nullptr};
    KoResourcePopupAction *patternAction {nullptr};
    QButtonGroup *group {nullptr};

    KoCanvasBase *canvas {nullptr};

    KisSignalCompressorWithParam<std::pair<QColor, KoFlake::FillVariant>> colorChangedCompressor;
    KisAcyclicSignalConnector resourceManagerAcyclicConnector;
    KoFillConfigWidget::StyleButton selectedFillIndex {KoFillConfigWidget::None};

    KoStopGradientSP activeGradient;
    KisSignalCompressor gradientChangedCompressor;
    KisSignalCompressor shapeChangedCompressor;
    KoFlake::FillVariant fillVariant;

    bool noSelectionTrackingMode {false};

    SvgMeshPosition meshposition;
    QScopedPointer<SvgMeshGradient> activeMeshGradient;

    QScopedPointer<Ui_KoFillConfigWidget> ui;

    std::vector<KisAcyclicSignalConnector::Blocker> deactivationLocks;

    std::array<boost::optional<KoColor>, 2> overriddenColorFromProvider;
};

KoFillConfigWidget::KoFillConfigWidget(KoCanvasBase *canvas, KoFlake::FillVariant fillVariant, bool trackShapeSelection, QWidget *parent)
    :  QWidget(parent)
    , d(new Private(fillVariant, this))
{
    d->canvas = canvas;

    if (trackShapeSelection) {
        connect(d->canvas->selectedShapesProxy(), SIGNAL(selectionChanged()), &d->shapeChangedCompressor,
                SLOT(start()));
        connect(&d->shapeChangedCompressor, SIGNAL(timeout()), this, SLOT(shapeChanged()));
    }

    d->resourceManagerAcyclicConnector.connectBackwardResourcePair(
            d->canvas->resourceManager(), SIGNAL(canvasResourceChanged(int,QVariant)),
            this, SLOT(slotCanvasResourceChanged(int,QVariant)));

    d->resourceManagerAcyclicConnector.connectForwardVoid(
         this, SIGNAL(sigInternalRequestColorToResourceManager()),
         this, SLOT(slotProposeCurrentColorToResourceManager()));

    KisAcyclicSignalConnector *resetConnector = d->resourceManagerAcyclicConnector.createCoordinatedConnector();
    resetConnector->connectForwardVoid(
         this, SIGNAL(sigInternalRecoverColorInResourceManager()),
         this, SLOT(slotRecoverColorInResourceManager()));

    // configure GUI

    d->ui.reset(new Ui_KoFillConfigWidget());
    d->ui->setupUi(this);

    d->group = new QButtonGroup(this);
    d->group->setExclusive(true);

    d->ui->btnNoFill->setIcon(QPixmap((const char **) buttonnone));
    d->group->addButton(d->ui->btnNoFill, None);

    d->ui->btnSolidFill->setIcon(QPixmap((const char **) buttonsolid));
    d->group->addButton(d->ui->btnSolidFill, Solid);

    d->ui->btnGradientFill->setIcon(QPixmap((const char **) buttongradient));
    d->group->addButton(d->ui->btnGradientFill, Gradient);

    d->ui->btnPatternFill->setIcon(QPixmap((const char **) buttonpattern));
    d->group->addButton(d->ui->btnPatternFill, Pattern);
    d->ui->btnPatternFill->setVisible(false);

    if (fillVariant == KoFlake::Fill) {
        // FIXME: different button
        d->ui->btnMeshFill->setIcon(QPixmap((const char**) buttonpattern));
        d->group->addButton(d->ui->btnMeshFill, MeshGradient);
    } else {
        d->ui->btnMeshFill->setVisible(false);
    }

    d->colorAction = new KoColorPopupAction(d->ui->btnChooseSolidColor);
    d->colorAction->setToolTip(i18n("Change the filling color"));
    d->colorAction->setCurrentColor(Qt::white);

    d->ui->btnChooseSolidColor->setDefaultAction(d->colorAction);
    d->ui->btnChooseSolidColor->setPopupMode(QToolButton::InstantPopup);
    d->ui->btnSolidColorSample->setIcon(KisIconUtils::loadIcon("krita_tool_color_sampler"));

    // TODO: for now the color sampling button is disabled!
    d->ui->btnSolidColorSample->setEnabled(false);
    d->ui->btnSolidColorSample->setVisible(false);

    connect(d->colorAction, &KoColorPopupAction::colorChanged, [this](KoColor color) {
        d->colorChangedCompressor.start({color.toQColor(), d->fillVariant});
    });

    connect(d->ui->btnChooseSolidColor, SIGNAL(iconSizeChanged()), d->colorAction, SLOT(updateIcon()));

    connect(d->group, SIGNAL(idClicked(int)), SLOT(styleButtonPressed(int)));

    connect(d->group, SIGNAL(idClicked(int)), SLOT(slotUpdateFillTitle()));

    slotUpdateFillTitle();
    styleButtonPressed(d->group->checkedId());


    // Gradient selector
    d->ui->wdgGradientEditor->setCompactMode(true);
    d->ui->wdgGradientEditor->setCanvasResourcesInterface(canvas->resourceManager()->canvasResourcesInterface());
    connect(d->ui->wdgGradientEditor, SIGNAL(sigGradientChanged()), &d->gradientChangedCompressor, SLOT(start()));
    connect(&d->gradientChangedCompressor, SIGNAL(timeout()), SLOT(activeGradientChanged()));

    d->gradientAction = new KoResourcePopupAction(ResourceType::Gradients, canvas->resourceManager()->canvasResourcesInterface(), d->ui->btnChoosePredefinedGradient);

    d->gradientAction->setToolTip(i18n("Change filling gradient"));
    d->ui->btnChoosePredefinedGradient->setDefaultAction(d->gradientAction);
    d->ui->btnChoosePredefinedGradient->setPopupMode(QToolButton::InstantPopup);

    connect(d->gradientAction, SIGNAL(resourceSelected(QSharedPointer<KoShapeBackground>)),
            SLOT(gradientResourceChanged()));
    connect(d->ui->btnChoosePredefinedGradient, SIGNAL(iconSizeChanged()), d->gradientAction, SLOT(updateIcon()));

    d->ui->btnSaveGradient->setIcon(KisIconUtils::loadIcon("document-save"));
    connect(d->ui->btnSaveGradient, SIGNAL(clicked()), SLOT(slotSavePredefinedGradientClicked()));

    connect(d->ui->cmbGradientRepeat, SIGNAL(currentIndexChanged(int)), SLOT(slotGradientRepeatChanged()));
    connect(d->ui->cmbGradientType, SIGNAL(currentIndexChanged(int)), SLOT(slotGradientTypeChanged()));

    // meshgradient
    connect(d->ui->meshStopColorButton, SIGNAL(changed(const KoColor&)), this, SLOT(slotMeshHandleColorChanged(const KoColor&)));

    d->ui->spinbRows->setRange(1, 20);
    d->ui->spinbColumns->setRange(1, 20);
    connect(d->ui->spinbRows, SIGNAL(valueChanged(int)), SLOT(slotMeshGradientChanged()));
    connect(d->ui->spinbColumns, SIGNAL(valueChanged(int)), SLOT(slotMeshGradientChanged()));
    connect(d->ui->cmbSmoothingType, SIGNAL(currentIndexChanged(int)), SLOT(slotMeshGradientShadingChanged(int)));

    // initialize deactivation locks
    d->deactivationLocks.push_back(KisAcyclicSignalConnector::Blocker(d->resourceManagerAcyclicConnector));


/*
    // Pattern selector
    d->patternAction = new KoResourcePopupAction(ResourceType::Patterns, d->colorButton);
    d->patternAction->setToolTip(i18n("Change the filling pattern"));
    connect(d->patternAction, SIGNAL(resourceSelected(QSharedPointer<KoShapeBackground>)), this, SLOT(patternChanged(QSharedPointer<KoShapeBackground>)));
    connect(d->colorButton, SIGNAL(iconSizeChanged()), d->patternAction, SLOT(updateIcon()));
*/


}

KoFillConfigWidget::~KoFillConfigWidget()
{
    delete d;
}

void KoFillConfigWidget::activate()
{
    KIS_SAFE_ASSERT_RECOVER_NOOP(!d->deactivationLocks.empty());
    d->deactivationLocks.clear();

    if (!d->noSelectionTrackingMode) {
        d->shapeChangedCompressor.start();
    } else {
        loadCurrentFillFromResourceServer();
    }

    updateWidgetComponentVisibility();
}

void KoFillConfigWidget::deactivate()
{
    Q_EMIT sigInternalRecoverColorInResourceManager();

    KIS_SAFE_ASSERT_RECOVER_NOOP(d->deactivationLocks.empty());
    d->deactivationLocks.push_back(KisAcyclicSignalConnector::Blocker(d->resourceManagerAcyclicConnector));
}

void KoFillConfigWidget::forceUpdateOnSelectionChanged()
{
    d->shapeChangedCompressor.start();
}

void KoFillConfigWidget::setSelectedMeshGradientHandle(const SvgMeshPosition &position)
{
    d->meshposition = position;
    updateMeshGradientUI();
}

void KoFillConfigWidget::setNoSelectionTrackingMode(bool value)
{
    d->noSelectionTrackingMode = value;
    if (!d->noSelectionTrackingMode) {
        d->shapeChangedCompressor.start();
    }
}

void KoFillConfigWidget::slotUpdateFillTitle()
{
    QString text = d->group->checkedButton() ? d->group->checkedButton()->text() : QString();
    text.replace('&', QString());
    d->ui->lblFillTitle->setText(text);
}

void KoFillConfigWidget::slotCanvasResourceChanged(int key, const QVariant &value)
{
    if (key == KoCanvasResource::ForegroundColor
        || key == KoCanvasResource::BackgroundColor) {

        KoColor color = value.value<KoColor>();

        const int checkedId = d->group->checkedId();

        if (checkedId < 0 || checkedId == None || checkedId == Solid) {

            d->group->button(Solid)->setChecked(true);
            d->selectedFillIndex = Solid;

            KoFlake::FillVariant colorSlot = (key == KoCanvasResource::ForegroundColor)
                                               ? KoFlake::Fill
                                               : KoFlake::StrokeFill;

            if (key == d->fillVariant) {
                d->colorAction->setCurrentColor(color);
            }
            colorChanged({color.toQColor(), colorSlot});
        } else if (checkedId == Gradient && key == KoCanvasResource::ForegroundColor) {
            d->ui->wdgGradientEditor->notifyGlobalColorChanged(color);
        }
    } else if (key == KoCanvasResource::CurrentGradient) {
        KoResourceSP gradient = value.value<KoAbstractGradientSP>();
        const int checkedId = d->group->checkedId();

        if (gradient && (checkedId < 0 || checkedId == None || checkedId == Gradient)) {
            d->group->button(Gradient)->setChecked(true);
            d->gradientAction->setCurrentResource(gradient);
        }
    }
}

QList<KoShape*> KoFillConfigWidget::currentShapes()
{
    return d->canvas->selectedShapesProxy()->selection()->selectedEditableShapes();
}

int KoFillConfigWidget::selectedFillIndex() {
    return d->selectedFillIndex;
}

void KoFillConfigWidget::styleButtonPressed(int buttonId)
{
    QList<KoShape*> shapes = currentShapes();

    switch (buttonId) {
        case KoFillConfigWidget::None:
            noColorSelected();
            break;
        case KoFillConfigWidget::Solid:
            colorChanged({d->colorAction->currentColor(), d->fillVariant});
            break;
        case KoFillConfigWidget::Gradient:
            if (d->activeGradient) {
                setNewGradientBackgroundToShape();
                updateGradientSaveButtonAvailability();
            } else {
                gradientResourceChanged();
            }
            break;
        case KoFillConfigWidget::Pattern:
            // Only select mode in the widget, don't set actual pattern :/
            //d->colorButton->setDefaultAction(d->patternAction);
            //patternChanged(d->patternAction->currentBackground());
            break;
        case KoFillConfigWidget::MeshGradient:
            if (d->activeMeshGradient) {
                setNewMeshGradientBackgroundToShape();
            } else {
                createNewMeshGradientBackground();
                setNewMeshGradientBackgroundToShape();
            }
            break;
    }


    // update tool option fields with first selected object
    if (shapes.isEmpty() == false) {
        KoShape *firstShape = shapes.first();
        updateUiFromFillType(firstShape);
    }

    updateWidgetComponentVisibility();
}

KoShapeStrokeSP KoFillConfigWidget::createShapeStroke()
{
    KoShapeStrokeSP stroke(new KoShapeStroke());
    KIS_ASSERT_RECOVER_RETURN_VALUE(d->fillVariant == KoFlake::StrokeFill, stroke);

    switch (d->group->checkedId()) {
    case KoFillConfigWidget::None:
        stroke->setColor(Qt::transparent);
        break;
    case KoFillConfigWidget::Solid:
        stroke->setColor(d->colorAction->currentColor());
        break;
    case KoFillConfigWidget::Gradient: {
        QScopedPointer<QGradient> g(d->activeGradient->toQGradient());
        QBrush newBrush = *g;
        stroke->setLineBrush(newBrush);
        stroke->setColor(Qt::transparent);
        break;
    }
    case KoFillConfigWidget::Pattern:
        break;
    }

    return stroke;
}

void KoFillConfigWidget::noColorSelected()
{
    QList<KoShape*> selectedShapes = currentShapes();
    if (selectedShapes.isEmpty()) {
        Q_EMIT sigFillChanged();
        return;
    }

    KoShapeFillWrapper wrapper(selectedShapes, d->fillVariant);
    KUndo2Command *command = wrapper.setColor(QColor());

    if (command) {
        d->canvas->addCommand(command);
    }

    Q_EMIT sigFillChanged();
}

void KoFillConfigWidget::colorChanged(std::pair<QColor, KoFlake::FillVariant> resource)
{
    QColor color = resource.first;
    KoFlake::FillVariant fillVariant = resource.second;
    if (!color.isValid()) {
        return;
    }

    QList<KoShape*> selectedShapes = currentShapes();
    if (selectedShapes.isEmpty()) {
        Q_EMIT sigInternalRequestColorToResourceManager();
        Q_EMIT sigFillChanged();
        return;
    }

    d->overriddenColorFromProvider[fillVariant] = boost::none;

    KoShapeFillWrapper wrapper(selectedShapes, fillVariant);

    KUndo2Command *command = wrapper.setColor(color);
    if (command) {
        d->canvas->addCommand(command);
    }

    // only returns true if it is a stroke object that has a 0 for line width
    if (wrapper.hasZeroLineWidth() ) {
         KUndo2Command *lineCommand = wrapper.setLineWidth(1.0);
         if (lineCommand) {
             d->canvas->addCommand(lineCommand);
         }

         // * line to test out
         QColor solidColor = d->colorAction->currentColor();
         solidColor.setAlpha(255);
         command = wrapper.setColor(solidColor);
         if (command) {
             d->canvas->addCommand(command);
         }

    }

    Q_EMIT sigFillChanged();
    Q_EMIT sigInternalRequestColorToResourceManager();
}

void KoFillConfigWidget::slotProposeCurrentColorToResourceManager()
{
    const int checkedId = d->group->checkedId();

    auto uploadColorToResourceManager = [this](KoCanvasResource::CanvasResourceId res,
                              KoFlake::FillVariant var, KoColor &color) {
        if (!d->overriddenColorFromProvider[var]) {
            d->overriddenColorFromProvider[var] =
                d->canvas->resourceManager()->resource(res).value<KoColor>();
        }

        /**
         * Don't let opacity leak to our resource manager system
         *
         * NOTE: theoretically, we could guarantee it on a level of the
         * resource manager itself,
         */
        color.setOpacity(OPACITY_OPAQUE_U8);
        d->canvas->resourceManager()->setResource(res, QVariant::fromValue(color));
    };

    auto uploadColorFromShape = [&](KoCanvasResource::CanvasResourceId res, KoFlake::FillVariant fill) {
        KoShapeFillWrapper wrapper(currentShapes(), fill);
        if (!wrapper.color().isValid()) {
            return;
        }
        KoColor color;
        color.fromQColor(wrapper.color());
        uploadColorToResourceManager(res, fill, color);
    };

    if (checkedId == Solid) {
        if (currentShapes().isEmpty()) {
            KoCanvasResource::CanvasResourceId res =
                (d->fillVariant == KoFlake::Fill) ? KoCanvasResource::ForegroundColor
                                                  : KoCanvasResource::BackgroundColor;
            KoColor color = d->colorAction->currentKoColor();
            uploadColorToResourceManager(res, d->fillVariant, color);
        } else {
            uploadColorFromShape(KoCanvasResource::BackgroundColor, KoFlake::StrokeFill);
            uploadColorFromShape(KoCanvasResource::ForegroundColor, KoFlake::Fill);
        }
    } else if (checkedId == Gradient) {
        if (boost::optional<KoColor> gradientColor =
                d->ui->wdgGradientEditor->currentActiveStopColor()) {
            KoColor color = *gradientColor;
            KoCanvasResource::CanvasResourceId res =
                (d->fillVariant == KoFlake::Fill) ? KoCanvasResource::ForegroundColor
                                                  : KoCanvasResource::BackgroundColor;
            uploadColorToResourceManager(res, d->fillVariant, color);
        }
    }
}

void KoFillConfigWidget::slotRecoverColorInResourceManager()
{
    auto checkAndRecover = [this](KoCanvasResource::CanvasResourceId res,
                               KoFlake::FillVariant var) {
        if (d->overriddenColorFromProvider[var]) {
            d->canvas->resourceManager()->setResource(
                res, QVariant::fromValue(*d->overriddenColorFromProvider[var]));
            d->overriddenColorFromProvider[var] = boost::none;
        }
    };

    checkAndRecover(KoCanvasResource::BackgroundColor, KoFlake::StrokeFill);
    checkAndRecover(KoCanvasResource::ForegroundColor, KoFlake::Fill);
}

void KoFillConfigWidget::slotSavePredefinedGradientClicked()
{
    KoResourceServerProvider *serverProvider = KoResourceServerProvider::instance();
    auto server = serverProvider->gradientServer();

    const QString defaultGradientNamePrefix = i18nc("default prefix for the saved gradient", "gradient");
    const QString saveLocation = server->saveLocation();

    QString name = d->activeGradient->name().isEmpty() ? defaultGradientNamePrefix : d->activeGradient->name();
    QFileInfo fileInfo(saveLocation + name.split(" ").join("_") + d->activeGradient->defaultFileExtension());
    bool fileOverWriteAccepted = false;

    while(!fileOverWriteAccepted) {
        name = QInputDialog::getText(this,
                                     i18nc("@title:window", "Save Gradient"),
                                     i18n("Enter gradient name:"),
                                     QLineEdit::Normal, name);
        if (name.isNull() || name.isEmpty()) {
            return;
        } else {
            fileInfo = QFileInfo(saveLocation + name.split(" ").join("_") + d->activeGradient->defaultFileExtension());
            if (fileInfo.exists()) {
                int res = QMessageBox::warning(this, i18nc("@title:window", "Name Already Exists")
                                                            , i18n("The name '%1' already exists, do you wish to overwrite it?", name)
                                                            , QMessageBox::Yes | QMessageBox::No);
                if (res == QMessageBox::Yes) fileOverWriteAccepted = true;
            } else {
                fileOverWriteAccepted = true;
            }
        }
    }

    d->activeGradient->setName(name);
    d->activeGradient->setFilename(name.split(" ").join("_") + d->activeGradient->defaultFileExtension());

    KoAbstractGradientSP newGradient = d->activeGradient->clone().dynamicCast<KoAbstractGradient>();

    KisResourceUserOperations::addResourceWithUserInput(this, newGradient);

    d->gradientAction->setCurrentResource(newGradient);
}

void KoFillConfigWidget::activeGradientChanged()
{
    setNewGradientBackgroundToShape();
    updateGradientSaveButtonAvailability();

    Q_EMIT sigInternalRequestColorToResourceManager();
}

void KoFillConfigWidget::gradientResourceChanged()
{
    QSharedPointer<KoGradientBackground> bg =
        qSharedPointerDynamicCast<KoGradientBackground>(
            d->gradientAction->currentBackground());

    updateGradientUi(bg->gradient());

    setNewGradientBackgroundToShape();
    updateGradientSaveButtonAvailability();
}

void KoFillConfigWidget::slotGradientTypeChanged()
{
    QGradient::Type type =
        d->ui->cmbGradientType->currentIndex() == 0 ?
            QGradient::LinearGradient : QGradient::RadialGradient;

    d->activeGradient->setType(type);
    activeGradientChanged();
}

void KoFillConfigWidget::slotGradientRepeatChanged()
{
    QGradient::Spread spread =
        QGradient::Spread(d->ui->cmbGradientRepeat->currentIndex());

    d->activeGradient->setSpread(spread);
    activeGradientChanged();
}

void KoFillConfigWidget::updateGradientUi(const QGradient *gradient)
{
    KisSignalsBlocker b1(d->ui->wdgGradientEditor,
                         d->ui->cmbGradientType,
                         d->ui->cmbGradientRepeat);

    d->ui->wdgGradientEditor->setGradient(0);

    d->activeGradient = KoStopGradient::fromQGradient(gradient);

    d->ui->wdgGradientEditor->setGradient(d->activeGradient);
    d->ui->cmbGradientType->setCurrentIndex(d->activeGradient->type() != QGradient::LinearGradient);
    d->ui->cmbGradientRepeat->setCurrentIndex(int(d->activeGradient->spread()));
}

void KoFillConfigWidget::setNewGradientBackgroundToShape()
{
    QList<KoShape*> selectedShapes = currentShapes();
    if (selectedShapes.isEmpty()) {
        Q_EMIT sigFillChanged();
        return;
    }

    KoShapeFillWrapper wrapper(selectedShapes, d->fillVariant);
    QScopedPointer<QGradient> srcQGradient(d->activeGradient->toQGradient());
    KUndo2Command *command = wrapper.applyGradientStopsOnly(srcQGradient.data());

    if (command) {
        d->canvas->addCommand(command);
    }

    Q_EMIT sigFillChanged();
}

void KoFillConfigWidget::updateGradientSaveButtonAvailability()
{
    bool savingEnabled = false;

    QScopedPointer<QGradient> currentGradient(d->activeGradient->toQGradient());
    QSharedPointer<KoShapeBackground> bg = d->gradientAction->currentBackground();
    if (bg) {
        QSharedPointer<KoGradientBackground> resourceBackground =
            qSharedPointerDynamicCast<KoGradientBackground>(bg);

        savingEnabled = resourceBackground->gradient()->stops() != currentGradient->stops();
        savingEnabled |= resourceBackground->gradient()->type() != currentGradient->type();
        savingEnabled |= resourceBackground->gradient()->spread() != currentGradient->spread();
    }

    d->ui->btnSaveGradient->setEnabled(savingEnabled);
}

void KoFillConfigWidget::patternChanged(QSharedPointer<KoShapeBackground>  background)
{
    Q_UNUSED(background);

#if 0
    QSharedPointer<KoPatternBackground> patternBackground = qSharedPointerDynamicCast<KoPatternBackground>(background);
    if (! patternBackground) {
        return;
    }

    QList<KoShape*> selectedShapes = currentShapes();
    if (selectedShapes.isEmpty()) {
        return;
    }

    QSharedPointer<KoPatternBackground> fill(new KoPatternBackground());
    fill->setPattern(patternBackground->pattern());
    d->canvas->addCommand(new KoShapeBackgroundCommand(selectedShapes, fill));

#endif
}

void KoFillConfigWidget::slotMeshGradientChanged()
{
    createNewDefaultMeshGradientBackground();
    setNewMeshGradientBackgroundToShape();
    d->meshposition = SvgMeshPosition();
    Q_EMIT sigMeshGradientResetted();
}

void KoFillConfigWidget::slotMeshGradientShadingChanged(int index)
{
    d->activeMeshGradient->setType(static_cast<SvgMeshGradient::Shading>(index));
    setNewMeshGradientBackgroundToShape();
}

void KoFillConfigWidget::slotMeshHandleColorChanged(const KoColor &c)
{
    QList<KoShape*> selectedShapes = currentShapes();
    KIS_SAFE_ASSERT_RECOVER_RETURN(!selectedShapes.isEmpty());

    KoShapeFillWrapper wrapper(selectedShapes, d->fillVariant);
    const SvgMeshGradient *gradient = wrapper.meshgradient();

    // if we changed the handle, the gradient *has* to exist
    KIS_SAFE_ASSERT_RECOVER_RETURN(gradient);

    if (d->meshposition.isValid()) {
        // We don't have any signals firing when we change the structure (i.e position of stops etc) of a
        // meshgradient. So, activeMeshGradient isn't updated when this happens. Hence we update it here and
        // then modify the color.
        d->activeMeshGradient.reset(new SvgMeshGradient(*gradient));

        d->activeMeshGradient->getMeshArray()->modifyColor(d->meshposition, c.toQColor());
        setNewMeshGradientBackgroundToShape();
    }
    return;
}

void KoFillConfigWidget::loadCurrentFillFromResourceServer()
{
    {
        KoColor color = d->canvas->resourceManager()->foregroundColor();
        if (d->group->checkedId() == -1 || d->group->checkedId() == None) {
            d->group->button(Solid)->setChecked(true);
        }
        d->selectedFillIndex = Solid;
        d->colorAction->setCurrentColor(color);
    }

    Q_FOREACH (QAbstractButton *button, d->group->buttons()) {
        button->setEnabled(true);
    }

    Q_EMIT sigFillChanged();
}

void KoFillConfigWidget::createNewMeshGradientBackground()
{
    QList<KoShape*> selectedShapes = currentShapes();
    if (selectedShapes.isEmpty()) {
        return;
    }

    KoShapeFillWrapper wrapper(selectedShapes, d->fillVariant);
    const SvgMeshGradient *g = wrapper.meshgradient();
    if (g) {
        d->activeMeshGradient.reset(new SvgMeshGradient(*g));
    } else {
        createNewDefaultMeshGradientBackground();
    }

    updateMeshGradientUI();
}

void KoFillConfigWidget::createNewDefaultMeshGradientBackground()
{
    QList<KoShape*> selectedShapes = currentShapes();
    if (selectedShapes.isEmpty()) {
        return;
    }

    // use this for mesh creation
    QSizeF maxSize;
    for (const auto& shape: selectedShapes) {
        QSizeF size = shape->boundingRect().size();
        if (size.height() > maxSize.height()) {
            maxSize.rheight() = size.height();
        }
        if (size.width() > maxSize.width()) {
            maxSize.rwidth() = size.width();
        }
    }

    SvgMeshGradient *gradient = new SvgMeshGradient;

    QColor color =  d->canvas->resourceManager()->resource(KoFlake::Background).value<KoColor>().toQColor();

    int nrows = d->ui->spinbRows->value();
    int ncols = d->ui->spinbColumns->value();

    if (d->ui->cmbSmoothingType->currentIndex()) {
        gradient->setType(SvgMeshGradient::BICUBIC);
    } else {
        gradient->setType(SvgMeshGradient::BILINEAR);
    }

    gradient->getMeshArray()->createDefaultMesh(nrows, ncols, color, maxSize);
    gradient->setGradientUnits(KoFlake::ObjectBoundingBox);
    d->activeMeshGradient.reset(gradient);
}

void KoFillConfigWidget::setNewMeshGradientBackgroundToShape()
{
    QList<KoShape*> selectedShapes = currentShapes();
    // if called by "manager"
    if (selectedShapes.isEmpty()) {
        Q_EMIT sigFillChanged();
        return;
    }

    KoShapeFillWrapper wrapper(selectedShapes, d->fillVariant);

    KUndo2Command *command = wrapper.setMeshGradient(d->activeMeshGradient.data(), QTransform());
    if (command) {
        d->canvas->addCommand(command);
    }

    Q_EMIT sigFillChanged();
}

void KoFillConfigWidget::updateMeshGradientUI()
{
    if (!d->activeMeshGradient) return;

    KisSignalsBlocker b(d->ui->spinbRows,
                        d->ui->spinbColumns,
                        d->ui->cmbSmoothingType,
                        d->ui->meshStopColorButton);

    SvgMeshArray *mesharray = d->activeMeshGradient->getMeshArray().data();
    d->ui->spinbRows->setValue(mesharray->numRows());
    d->ui->spinbColumns->setValue(mesharray->numColumns());
    d->ui->cmbSmoothingType->setCurrentIndex(d->activeMeshGradient->type());
    if (d->meshposition.isValid()) {
        QColor qc = d->activeMeshGradient->getMeshArray()->getStop(d->meshposition).color;

        KoColor c = d->ui->meshStopColorButton->color();
        c.fromQColor(qc);

        d->ui->meshStopColorButton->setColor(c);
        d->ui->meshStopColorButton->setDisabled(false);
    } else {
        d->ui->meshStopColorButton->setDisabled(true);
    }
}

void KoFillConfigWidget::shapeChanged()
{
    if (d->noSelectionTrackingMode) return;

    QList<KoShape*> shapes = currentShapes();

    bool shouldUploadColorToResourceManager = false;

    // Disable the buttons if there aren't any selected shapes or we have a several shapes with different
    // gradient backgrounds.
    if (shapes.isEmpty() ||
        (shapes.size() > 1 && KoShapeFillWrapper(shapes, d->fillVariant).isMixedFill())) {

        Q_FOREACH (QAbstractButton *button, d->group->buttons()) {
            button->setEnabled(!shapes.isEmpty());
        }
    } else {
        // only one vector object selected
        Q_FOREACH (QAbstractButton *button, d->group->buttons()) {
            button->setEnabled(true);
        }

        // update active index of shape
        KoShape *shape = shapes.first();
        updateUiFromFillType(shape); // updates tool options fields

        shouldUploadColorToResourceManager = true;
    }

    // updates the UI
    d->group->button(d->selectedFillIndex)->setChecked(true);

    updateWidgetComponentVisibility();
    slotUpdateFillTitle();

    if (shouldUploadColorToResourceManager) {
        Q_EMIT sigInternalRequestColorToResourceManager();
    } else {
        Q_EMIT sigInternalRecoverColorInResourceManager();
    }
}

void KoFillConfigWidget::updateUiFromFillType(KoShape *shape)
{
    KIS_SAFE_ASSERT_RECOVER_RETURN(shape);
    KoShapeFillWrapper wrapper(shape, d->fillVariant);

    switch (wrapper.type()) {
        case KoFlake::None:
             d->selectedFillIndex = KoFillConfigWidget::None;
            break;
        case KoFlake::Solid: {
            d->selectedFillIndex = KoFillConfigWidget::Solid;
            QColor color = wrapper.color();
            if (color.alpha() > 0) {
                d->colorAction->setCurrentColor(wrapper.color());
            }
            break;
        }
        case KoFlake::Gradient:
            d->selectedFillIndex = KoFillConfigWidget::Gradient;
            updateGradientUi(wrapper.gradient());
            updateGradientSaveButtonAvailability();
            break;
        case KoFlake::Pattern:
            d->selectedFillIndex = KoFillConfigWidget::Pattern;
            break;
        case KoFlake::MeshGradient:
            d->selectedFillIndex = KoFillConfigWidget::MeshGradient;
            createNewMeshGradientBackground();
            break;
    }
}


void KoFillConfigWidget::updateWidgetComponentVisibility()
{
    // The UI is showing/hiding things like this because the 'stacked widget' isn't very flexible
    // and makes it difficult to put anything underneath it without a lot empty space

    // hide everything first
    d->ui->wdgGradientEditor->setVisible(false);
    d->ui->btnChoosePredefinedGradient->setVisible(false);
    d->ui->btnChooseSolidColor->setVisible(false);
    d->ui->typeLabel->setVisible(false);
    d->ui->repeatLabel->setVisible(false);
    d->ui->cmbGradientRepeat->setVisible(false);
    d->ui->cmbGradientType->setVisible(false);
    d->ui->btnSolidColorSample->setVisible(false);
    d->ui->btnSaveGradient->setVisible(false);
    d->ui->gradientTypeLine->setVisible(false);
    d->ui->soldStrokeColorLabel->setVisible(false);
    d->ui->presetLabel->setVisible(false);
    d->ui->stopColorLabel->setVisible(false);
    d->ui->meshStopColorButton->setVisible(false);
    d->ui->rowsLabel->setVisible(false);
    d->ui->spinbRows->setVisible(false);
    d->ui->columnsLabel->setVisible(false);
    d->ui->spinbColumns->setVisible(false);
    d->ui->smoothingTypeLabel->setVisible(false);
    d->ui->cmbSmoothingType->setVisible(false);

    // keep options hidden if no vector shapes are selected
    if(currentShapes().isEmpty()) {
        return;
    }


    switch (d->selectedFillIndex) {
        case KoFillConfigWidget::None:
            break;
        case KoFillConfigWidget::Solid:
            d->ui->btnChooseSolidColor->setVisible(true);
            d->ui->btnSolidColorSample->setVisible(false);
            d->ui->soldStrokeColorLabel->setVisible(true);
            break;
        case KoFillConfigWidget::Gradient:
            d->ui->wdgGradientEditor->setVisible(true);
            d->ui->btnChoosePredefinedGradient->setVisible(true);
            d->ui->typeLabel->setVisible(true);
            d->ui->repeatLabel->setVisible(true);
            d->ui->cmbGradientRepeat->setVisible(true);
            d->ui->cmbGradientType->setVisible(true);
            d->ui->btnSaveGradient->setVisible(true);
            d->ui->gradientTypeLine->setVisible(true);
            d->ui->presetLabel->setVisible(true);
            break;
        case KoFillConfigWidget::Pattern:
            break;
        case KoFillConfigWidget::MeshGradient:
            d->ui->stopColorLabel->setVisible(true);
            d->ui->meshStopColorButton->setVisible(true);
            d->ui->rowsLabel->setVisible(true);
            d->ui->spinbRows->setVisible(true);
            d->ui->columnsLabel->setVisible(true);
            d->ui->spinbColumns->setVisible(true);
            d->ui->smoothingTypeLabel->setVisible(true);
            d->ui->cmbSmoothingType->setVisible(true);
            d->ui->meshStopColorButton->setAlphaChannelEnabled(true);
            break;
    }

}
