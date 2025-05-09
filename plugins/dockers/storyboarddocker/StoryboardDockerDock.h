/*
 *  SPDX-FileCopyrightText: 2020 Saurabh Kumar <saurabhk660@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef _STORYBOARDDOCKER_DOCK_H_
#define _STORYBOARDDOCKER_DOCK_H_

#include <QDockWidget>
#include <QButtonGroup>
#include <QItemSelection>
#include <QPointer>

#include <kis_mainwindow_observer.h>
#include <kis_canvas2.h>
#include <kis_action.h>
#include <boost/optional.hpp>
#include "DlgExportStoryboard.h"
#include <QDomElement>

class Ui_WdgStoryboardDock;
class CommentMenu;
class ArrangeMenu;
class StoryboardCommentModel;
class StoryboardModel;
class StoryboardDelegate;
class KisNodeManager;
class QPrinter;

class StoryboardDockerDock : public QDockWidget, public KisMainwindowObserver{
    Q_OBJECT
public:
    StoryboardDockerDock();
    ~StoryboardDockerDock() override;
    QString observerName() override { return "StoryboardDockerDock"; }
    void setCanvas(KoCanvasBase *canvas) override;
    void unsetCanvas() override;
    void setViewManager(KisViewManager* kisview) override;

    struct ExportPageShot {
        boost::optional<QRectF> cutNameRect;
        boost::optional<QRectF> cutNumberRect;
        boost::optional<QRectF> cutDurationRect;
        boost::optional<QRectF> cutImageRect;
        QMap<QString, QRectF> commentRects;

        ExportPageShot()
            : commentRects() {
        }
    };

    struct ExportPage {
        QVector<ExportPageShot> elements;
        boost::optional<QRectF> pageTimeRect;
        boost::optional<QRectF> pageNumberRect;
        boost::optional<QDomDocument> svg;

        ExportPage()
            : elements()
        {}
    };

private Q_SLOTS:
    /**
     * @brief sets the image in Model to nullptr if there is no canvas set or no KisImage
     */
    void notifyImageDeleted();
    /**
     * @brief sets the KisDocument's storyboardItemList to be the same as StoryboardModel's storyboardItemList
     * and KisDocument's storyboardCommentList to be the same as CommentModel's commentList
     */
    void slotUpdateDocumentList();
    /**
     * @brief sets the StoryboardModel's storyboardItemList to be the same as KisDocument's storyboardItemList
     */
    void slotUpdateStoryboardModelList();
    /**
     * @brief sets the CommentModel's comment list to be the same as KisDocument's storyboardCommentList
     */
    void slotUpdateCommentModelList();


    /**
     * @brief calls @c slotExport(ExportFormat) with PDF parameter.
     */
    void slotExportAsPdf();

    /**
     * @brief calls @c slotExport(ExportFormat) with SVG parameter.
     */
    void slotExportAsSvg();

    /**
     * @brief Creates the @c DlgExportStoryboard and performs the actual export.
     * @sa DlgExportStoryboard
     */
    void slotExport(ExportFormat format);

    /**
     * @brief called when lock toggle button is clicked.
     * @param value The new lock toggle value.
     */
    void slotLockClicked(bool);

    /**
     * @brief called when a mode option is selected in @c Arrange menu.
     * @param button The button selected.
     */
    void slotModeChanged(QAbstractButton*);

    /**
     * @brief called when a view option is selected in @c Arrange menu.
     * @param button The button selected.
     */
    void slotViewChanged(QAbstractButton*);

    /**
     * @brief called to update minimum width on reaction to model changes.
     * Should change based on available content.
     */
    void slotUpdateMinimumWidth();

    /**
     * @brief called to reflect changes to the model.
     */
    void slotModelChanged();

private:
    ExportPage getPageLayout(int rows, int columns, const QRect& imageSize, const QRect& pageRect, const QFontMetrics& painter);
    ExportPage getPageLayout(QString layoutSvgFileName, QPrinter *printer);

    QString buildDurationString(int seconds, int frames);

private:
    KisCanvas2* m_canvas;
    KisNodeManager* m_nodeManager;
    QScopedPointer<Ui_WdgStoryboardDock> m_ui;

    QMenu *m_exportMenu;
    KisAction *m_exportAsPdfAction;
    KisAction *m_exportAsSvgAction;

    QPointer<StoryboardCommentModel> m_commentModel;
    CommentMenu *m_commentMenu;

    KisAction *m_lockAction;

    ArrangeMenu *m_arrangeMenu;

    QButtonGroup *m_modeGroup;
    QButtonGroup *m_viewGroup;

    QSharedPointer<StoryboardModel> m_storyboardModel;
    QPointer<StoryboardDelegate> m_storyboardDelegate;
};

#endif
