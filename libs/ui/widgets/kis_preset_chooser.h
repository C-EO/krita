/*
 *  SPDX-FileCopyrightText: 2002 Patrick Julien <freak@codepimps.org>
 *  SPDX-FileCopyrightText: 2011 Silvio Heinrich <plassy@web.de>
 *  SPDX-FileCopyrightText: 2011 José Luis Vergara <pentalis@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_PRESET_CHOOSER_H_
#define KIS_PRESET_CHOOSER_H_

#include <QWidget>
#include <QPointer>

#include <KoResource.h>
#include <KoID.h>
#include "kis_signal_auto_connection.h"
#include "ResourceListViewModes.h"

class KoAbstractResourceServerAdapter;
class KisPresetDelegate;
class KisResourceItemChooser;


#include <kritaui_export.h>

/**
 * A special type of item chooser that can contain extra widgets that show
 * more information about the currently selected item. Reimplement update()
 * to extract that information and fill the appropriate widgets.
 */
class KRITAUI_EXPORT KisPresetChooser : public QWidget
{
    Q_OBJECT

public:

    KisPresetChooser(QWidget *parent = 0);
    ~KisPresetChooser() override;

    enum ViewMode {
        THUMBNAIL, /// Shows thumbnails
        DETAIL  /// Shows thumbnails with text next to it
    };

    /// Sets a list of resources in the paintop list, when ever user press enter in the lineedit of paintop_presets_popup Class
    void setViewMode(ViewMode mode);
    void setViewModeToThumbnail();
    void setViewModeToDetail();

    void setCurrentResource(KoResourceSP resource);

    KoResourceSP currentResource() const;

    /// Sets the visibility of tagging klineEdits
    void showTaggingBar(bool show);

    KisResourceItemChooser *itemChooser();

    void setPresetFilter(const QString& paintOpId);

    /// get the base size for the icons. Used by the slider in the view options
    int iconSize();

Q_SIGNALS:
    void resourceSelected(KoResourceSP resource);
    void resourceClicked(KoResourceSP resource);

public Q_SLOTS:
    void updateViewSettings();

    /// sets the icon size. Used by slider in view menu
    void setIconSize(int newSize);

    /// saves the icon size for the presets. called by the horizontal slider release event.
    void saveIconSize();

    /// Shows/hides brush names when KisResourceItemListView used by KisResourceItemChooser
    /// changes it's ListViewMode.
    /// If this method is removed then the Details view brush names will still be displayed when
    /// KisResourceItemChooser is in horizontal layout.
    void showHideBrushNames(ListViewMode newViewMode);

private Q_SLOTS:
    void notifyConfigChanged();
    void slotResourceWasSelected(KoResourceSP resource);
    void slotCurrentPresetChanged();

private:
    KisResourceItemChooser *m_chooser {0};
    KisPresetDelegate* m_delegate {0};
    ViewMode m_mode;

    class PaintOpFilterModel;
    QPointer<PaintOpFilterModel> m_paintOpFilterModel;

    KisSignalAutoConnectionsStore m_currentPresetConnections;
};

#endif

