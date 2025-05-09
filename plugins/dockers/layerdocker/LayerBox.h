/*
 *  LayerBox.h - part of Krita aka Krayon aka KimageShop
 *
 *  SPDX-FileCopyrightText: 2002 Patrick Julien <freak@codepimps.org>
 *  SPDX-FileCopyrightText: 2006 Gábor Lehel <illissius@gmail.com>
 *  SPDX-FileCopyrightText: 2007 Thomas Zander <zander@kde.org>
 *  SPDX-FileCopyrightText: 2007-2009 Boudewijn Rempt <boud@valdyas.org>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_LAYERBOX_H
#define KIS_LAYERBOX_H

#include <QList>
#include <QDockWidget>
#include <QPointer>

#include <kis_debug.h>

#include <KoColorSpace.h>
#include <KoDockFactoryBase.h>

#include <kis_types.h>

#include "kis_action.h"
#include "KisViewManager.h"
#include "kis_mainwindow_observer.h"
#include "kis_signal_compressor.h"
#include "kis_layer_filter_widget.h"
#include "kis_signal_auto_connection.h"
#include <QSlider>
#include <QCheckBox>
#include <kis_slider_spin_box.h>
#include <QComboBox>
#include <kis_idle_watcher.h>

#include "kritalayerdocker_export.h"

class QModelIndex;

typedef QList<QModelIndex> QModelIndexList;

class QMenu;
class QAbstractButton;
class KoCompositeOp;
class KisCanvas2;
class KisNodeModel;
class KisNodeFilterProxyModel;
class Ui_WdgLayerBox;
class KisNodeJugglerCompressed;
class KisColorLabelSelectorWidgetMenuWrapper;
class QWidgetAction;
class KisKeyframeChannel;
class KisSelectionActionsAdapter;

/**
 * A widget that shows a visualization of the layer structure.
 *
 * The center of the layer box is KisNodeModel, which shows the actual layers.
 * This widget adds docking functionality and command buttons.
 *
 */
class KRITALAYERDOCKER_EXPORT LayerBox : public QDockWidget, public KisMainwindowObserver
{

    Q_OBJECT

public:

    LayerBox();
    ~LayerBox() override;
    QString observerName() override { return "LayerBox"; }
    /// reimplemented from KisMainwindowObserver
    void setViewManager(KisViewManager* kisview) override;
    void setCanvas(KoCanvasBase *canvas) override;
    void unsetCanvas() override;

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private Q_SLOTS:

    void notifyImageDeleted();

    void slotContextMenuRequested(const QPoint &pos, const QModelIndex &index);

    // From the node manager to the layerbox
    void slotSetCompositeOp(const KoCompositeOp* compositeOp);
    void slotSetOpacity(double opacity);
    void slotUpdateOpacitySlider(quint8 value);
    void updateUI();
    void setCurrentNode(KisNodeSP node);
    void slotModelReset();

    // from the layerbox to the node manager
    void slotRmClicked();
    void slotRaiseClicked();
    void slotLowerClicked();
    void slotPropertiesClicked();
    void slotChangeCloneSourceClicked();

    void slotCompositeOpChanged(int index);
    void slotOpacityChanged();
    void slotOpacitySliderMoved(qreal opacity);

    void slotCollapsed(const QModelIndex &index);
    void slotExpanded(const QModelIndex &index);

    void slotSelectOpaque();
    void slotNodeCollapsedChanged();

    void slotEditGlobalSelection(bool showSelections);
    void slotRenameCurrentNode();

    void slotAdjustCurrentBeforeRemoveRows(const QModelIndex &parent, int first, int last);
    void selectionChanged(const QModelIndexList &selection);
    void slotNodeManagerChangedSelection(const QList<KisNodeSP> &nodes);
    void slotColorLabelChanged(int index);
    void slotUpdateIcons();
    void toggleActiveLayerSolo();
    void updateLayerOpMenu(const QModelIndex &index, QMenu &menu);

    void slotAddLayerBnClicked();
    void slotLayerOpMenuOpened();
    void slotLayerOpMenuClosed();

    void updateAvailableLabels();
    void updateLayerFiltering();

    void slotUpdateThumbnailIconSize();
    void slotUpdateTreeIndentation();
    void slotUpdateLayerInfoTextStyle();
    void slotUpdateLayerInfoTextOpacity();
    void slotUpdateUseInlineLayerInfoText();
    void slotUpdateUseLayerSelectionCheckbox();

    void slotImageTimeChanged(int time);
    void slotForgetAboutSavedNodeBeforeEditSelectionMode();

Q_SIGNALS:
    void imageChanged();

private:
    inline void connectActionToButton(KisViewManager* view, QAbstractButton *button, const QString &id);
    inline void addActionToMenu(QMenu *menu, const QString &id);
    // reimp from KisNodeManager
    qint32 convertOpacityToInt(qreal opacity);

    KisNodeSP findNonHidableNode(KisNodeSP startNode);
private:

    QPointer<KisCanvas2> m_canvas;
    QScopedPointer<KisSelectionActionsAdapter> m_selectionActionsAdapter;
    QMenu *m_newLayerMenu;
    QMenu *m_opLayerMenu;
    KisImageWSP m_image;
    QPointer<KisNodeModel> m_nodeModel;
    QPointer<KisNodeFilterProxyModel> m_filteringModel;
    QPointer<KisNodeManager> m_nodeManager;
    QPointer<KisColorLabelSelectorWidgetMenuWrapper> m_colorSelector;
    QPointer<QWidgetAction> m_colorSelectorAction;
    Ui_WdgLayerBox* m_wdgLayerBox;
    int m_newOpacity;
    KisNodeSP m_changedOpacityNode;

    KisAction *m_removeAction;
    KisAction *m_propertiesAction;
    KisAction *m_changeCloneSourceAction;
    KisAction *m_layerToggleSolo;
    KisAction *m_showGlobalSelectionMask;
    KisSignalCompressor m_colorLabelCompressor;
    KisSignalCompressor m_thumbnailSizeCompressor;
    KisSignalCompressor m_treeIndentationCompressor;
    KisSignalCompressor m_infoTextOpacityCompressor;
    KisSignalCompressor m_nodeOpacityCompressor;

    KisLayerFilterWidget* layerFilterWidget;
    QSlider *thumbnailSizeSlider;
    QSlider *indentationSlider;
    QComboBox *infoTextCombobox;
    KisSliderSpinBox *infoTextOpacitySlider;
    QCheckBox *infoTextInlineChkbox;
    QCheckBox *layerSelectionCheckBox {nullptr};

    KisNodeSP m_activeNode;
    KisNodeWSP m_savedNodeBeforeEditSelectionMode;
    KisSignalAutoConnectionsStore m_activeNodeConnections;
};

class KRITALAYERDOCKER_EXPORT LayerBoxFactory : public KoDockFactoryBase
{

public:
    LayerBoxFactory() { }

    QString id() const override {
        return QString("KisLayerBox");
    }

    QDockWidget* createDockWidget() override {
        LayerBox * dockWidget = new LayerBox();

        dockWidget->setObjectName(id());

        return dockWidget;
    }

    DockPosition defaultDockPosition() const override {
        return DockRight;
    }

};

#endif // KIS_LAYERBOX_H

