/*
 *  SPDX-FileCopyrightText: 2015 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef __KIS_IMAGE_ANIMATION_INTERFACE_H
#define __KIS_IMAGE_ANIMATION_INTERFACE_H

#include <QObject>
#include <QScopedPointer>

#include "kis_types.h"
#include "kritaimage_export.h"

class KisUpdatesFacade;
class KisTimeSpan;
class KisKeyframeChannel;
class KoColor;
class KisRegion;
class KisLockFrameGenerationLock;

namespace KisLayerUtils {
    struct SwitchFrameCommand;
}

class KRITAIMAGE_EXPORT KisImageAnimationInterface : public QObject
{
    Q_OBJECT

public:
    KisImageAnimationInterface(KisImage *image);
    KisImageAnimationInterface(const KisImageAnimationInterface &rhs, KisImage *newImage);
    ~KisImageAnimationInterface() override;

    /**
     * Returns true of the image has at least one animated layer
     */
    bool hasAnimation() const;

    /**
     * Returns currently active frame of the underlying image. Some strokes
     * can override this value and it will report a different value.
     */
    int currentTime() const;

    /**
     * Same as currentTime, except it isn't changed when background strokes
     * are running.
     */
    int currentUITime() const;

    /**
     * While any non-current frame is being regenerated by the
     * strategy, the image is kept in a special state, named
     * 'externalFrameActive'. Is this state the following applies:
     *
     * 1) All the animated paint devices switch its state into
     *    frameId() defined by global time.
     *
     * 2) All animation-not-capable devices switch to a temporary
     *    content device, which *is in undefined state*. The stroke
     *    should regenerate the image projection manually.
     */
    bool externalFrameActive() const;

    void requestTimeSwitchWithUndo(int time);

    void requestTimeSwitchNonGUI(int time, bool useUndo = false);

    /**
     * Start a background thread that will recalculate some extra frame.
     * The result will be reported using two types of signals:
     *
     * 1) KisImage::sigImageUpdated() will be emitted for every chunk
     *    of updated area.
     *
     * 2) sigFrameReady() will be emitted in the end of the operation.
     *    IMPORTANT: to get the result you must connect to this signal
     *    with Qt::DirectConnection and fetch the result from
     *    frameProjection().  After the signal handler is exited, the
     *    data will no longer be available.
     *
     * 3) The passed lock will be released when the stroke is finished
     *    execution (and the strategy is destroyed)
     */
    void requestFrameRegeneration(int frameId, const KisRegion &dirtyRegion, bool isCancellable, KisLockFrameGenerationLock &&lock);

    void notifyNodeChanged(const KisNode *node, const QRect &rect, bool recursive);
    void notifyNodeChanged(const KisNode *node, const QVector<QRect> &rects, bool recursive);
    void invalidateFrames(const KisTimeSpan &range, const QRect &rect);
    void invalidateFrame(const int time, KisNodeSP target);

    /**
     * Changes the default color of the "external frame" projection of
     * the image's root layer. Please note that this command should be
     * executed from a context of an exclusive job!
     */
    void setDefaultProjectionColor(const KoColor &color);

    /**
     * @brief documentPlaybackRange
     * @return A KisTimeSpan reflecting actual document time range. This is
     * the actual play back range associated with a krita document.
     */
    const KisTimeSpan& documentPlaybackRange() const;
    void setDocumentRange(const KisTimeSpan range);

    /**
     * @brief activePlaybackRange
     * @return Returns the current clip range that the user wishes play through.
     * Takes into account selection range when available as a custom loop override.
     * Should be used in the PlaybackEngine to determine proper loop points.
     */
    const KisTimeSpan &activePlaybackRange() const;
    void setActivePlaybackRange(const KisTimeSpan range);

    int framerate() const;

    QString exportSequenceFilePath();
    void setExportSequenceFilePath(const QString &filePath);

    QString exportSequenceBaseName();
    void setExportSequenceBaseName(const QString &baseName);

    int exportInitialFrameNumber();
    void setExportInitialFrameNumber(const int frameNum);

    QSet<int> activeLayerSelectedTimes();
    void setActiveLayerSelectedTimes(const QSet<int> &times);

    KisImageWSP image() const;

    int totalLength();

    /**
     * Blocks background processes like frame cache populator from starting the
     * generation process, hence giving priority to the interactive frame
     * generation methods.
     *
     * This method is **not** blocking, it just forbids further
     * actions. If there is any backround action is running, it
     * continues to run. Use lockFrameGeneration() to wait
     * for completion of such actions.
     *
     * \see KisBlockBackgroundFrameGenerationLock for RAII wrapper
     */
    void blockBackgroundFrameGeneration();

    /**
     * Unblocks background generation process.
     *
     * \see blockBackgroundFrameGeneration()
     */
    void unblockBackgroundFrameGeneration();

    /**
     * Reports if background generation process is blocked
     *
     * \see blockBackgroundFrameGeneration()
     */
    bool backgroundFrameGenerationBlocked() const;

    /**
     * Acquire an exclusive lock for the frame generation process
     * initiated by requestFrameRegeneration().
     *
     * It is impossible to execute multiple background frame
     * generation processes on a single image, because the
     * image returns the result using global signals. Hence
     * the initiator of the generation should acquire the lock
     * first and pass it to requestFrameRegeneration(). The lock
     * will be automatically released when the frame generation
     * process is ended and all the signals are emitted.
     *
     * Calling to lockFrameGeneration() may block until the
     * currently executing frame generation process is running.
     *
     * \see KisLockFrameGenerationLock for RAII wrapper
     */
    void lockFrameGeneration();

    /**
     * Release frame generation lock
     *
     * \see lockFrameGeneration()
     */
    void unlockFrameGeneration();

    /**
     * Try to acquire frame generation lock
     *
     * \see lockFrameGeneration()
     */
    bool tryLockFrameGeneration();

    enum SwitchTimeAsyncOption {
        STAO_NONE = 0,
        STAO_USE_UNDO = 1 << 1,
        STAO_FORCE_REGENERATION = 1 << 2
    };
    Q_DECLARE_FLAGS(SwitchTimeAsyncFlags, SwitchTimeAsyncOption);

public Q_SLOTS:

    /**
     * Switches current frame (synchronously) and starts an
     * asynchronous regeneration of the entire image.
     */
    void switchCurrentTimeAsync(int frameId, SwitchTimeAsyncFlags options = STAO_NONE);

    void setDocumentRangeStartFrame(int column);
    void setDocumentRangeEndFrame(int column);

    void setFramerate(int fps);

Q_SIGNALS:
    /**
     * @brief sigFrameReady notifies when an External frame has been regenerated and is available.
     * @param time -- frame index
     *
     * Used for background processing of frames where we want to ensure that an external frame has
     * been fully processed before updating.
     *
     */
    void sigFrameReady(int time);

    /**
     * @brief sigFrameRegenerated notifies when internal frame has been fully regenerated.
     * @param time
     *
     * Used to notify switchCurrentTimeAsync clients that the frame is visible to the user.
     * Only notifies when internal frame regeneration occurs, not external.
     * Currently used in AnimationPlayer to update what it considers to be the "visible" frame
     */
    void sigFrameRegenerated(int time);

    /**
     * @brief sigFrameRegenerationSkipped notified when async frame changes are skipped.
     * @param time
     *
     * Skipping frame regeneration occurs when the contents of the frame are deemed unimportant
     * and not work updating the canvas for (generally for image-wide hold frames, for example.)
     */
    void sigFrameRegenerationSkipped(int time);

    void sigFrameCancelled();
    void sigUiTimeChanged(int newTime);
    void sigFramesChanged(const KisTimeSpan &range, const QRect &rect);

    void sigInternalRequestTimeSwitch(int frameId, bool useUndo);

    void sigFramerateChanged();
    void sigPlaybackRangeChanged();
    void sigDocumentRangeChanged();

    void sigKeyframeAdded(const KisKeyframeChannel* channel, int time);
    void sigKeyframeRemoved(const KisKeyframeChannel* channel, int time);

private:
    // interface for:
    friend class KisRegenerateFrameStrokeStrategy;
    friend class KisSuspendProjectionUpdatesStrokeStrategy; //TODO These friend classes are ugly. Let's refactor after Krita 5 release.
    friend class KisAnimationFrameCacheTest;
    friend struct KisLayerUtils::SwitchFrameCommand;
    friend class KisImageTest;
    void saveAndResetCurrentTime(int frameId, int *savedValue);
    void restoreCurrentTime(int *savedValue);
    void notifyFrameReady();
    void notifyFrameCancelled();
    void notifyFrameRegenerated();
    bool requiresOnionSkinRendering();

    KisUpdatesFacade* updatesFacade() const;

    void blockFrameInvalidation(bool value);

    friend class KisSwitchTimeStrokeStrategy;
    friend class TransformStrokeStrategy;
    void explicitlySetCurrentTime(int frameId);
    struct Private;
    const QScopedPointer<Private> m_d;
};

#endif /* __KIS_IMAGE_ANIMATION_INTERFACE_H */
