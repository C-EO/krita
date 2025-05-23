/*
 * SPDX-FileCopyrightText: 2018 Boudewijn Rempt <boud@valdyas.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KISRESOURCELOCATOR_H
#define KISRESOURCELOCATOR_H

#include <QObject>
#include <QScopedPointer>
#include <QStringList>
#include <QString>

#include "kritaresources_export.h"

#include <KisResourceStorage.h>


/**
 * The KisResourceLocator class locates all resource storages (folders,
 * bundles, various adobe resource libraries) in the resource location.
 *
 * The resource location is always a writable folder.
 *
 * There is one resource locator which is owned by the QApplication
 * object.
 *
 * The resource location is configurable, but there is only one location
 * where Krita will look for resources.
 */
class KRITARESOURCES_EXPORT KisResourceLocator : public QObject
{
    Q_OBJECT
public:

    // The configuration key that holds the resource location
    // for this installation of Krita. The location is
    // QStandardPaths::AppDataLocation by default, but that
    // can be changed.
    static const QString resourceLocationKey;

    static KisResourceLocator *instance();

    ~KisResourceLocator();

    enum class LocatorError {
        Ok,
        LocationReadOnly,
        CannotCreateLocation,
        CannotInitializeDb,
        CannotSynchronizeDb
    };

    /**
     * @brief initialize Setup the resource locator for use.
     *
     * @param installationResourcesLocation the place where the resources
     * that come packaged with Krita reside.
     */
    LocatorError initialize(const QString &installationResourcesLocation);

    /**
     * @brief errorMessages
     * @return
     */
    QStringList errorMessages() const;

    /**
     * @brief resourceLocationBase is the place where all resource storages (folder,
     * bundles etc. are located. This is a writable place.
     * @return the base location for all storages.
     */
    QString resourceLocationBase() const;

    /**
     * @brief purge purges the local resource cache
     */
    void purge(const QString &storageLocation);

    /**
     * @brief addStorage Adds a new resource storage to the database. The storage is
     * will be marked as not pre-installed. If there is already a storage with the
     * given location, it will first be removed.
     * @param storageLocation a unique name for the given storage
     * @param storage a storage object
     * @return true if the storage has been added successfully
     */
    bool addStorage(const QString &storageLocation, KisResourceStorageSP storage);

    /**
     * @brief removeStorage removes the temporary storage from the database
     * @param storageLocation the unique name of the storage
     * @return true is successful.
     */
    bool removeStorage(const QString &storageLocation);

    /**
     * @brief hasStorage can be used to check whether the given storage already exists
     * @param storageLocation the name of the storage
     * @return true if the storage is known
     */
    bool hasStorage(const QString &storageLocation);


    /**
     * @brief saveTags saves all tags to .tag files in the resource folder
     */
    static void saveTags();

    /**
     * Remove the given tag from the cache
     */
    void purgeTag(const QString tagUrl, const QString resourceType);

    /**
     * Returns the full file path of the resource if it has any
     * separate physical representation on the disk
     */
    QString filePathForResource(KoResourceSP resource);

    /// This updates the "fontregistry" storage. Called when the font directories change;
    void updateFontStorage();

Q_SIGNALS:

    void progressMessage(const QString&);

    /// Emitted whenever a storage is added
    void storageAdded(const QString &location);

    /// Emitted whenever a storage is removed
    void storageRemoved(const QString &location);

    /// Emitted when the locator needs to add an embedded resource
    void beginExternalResourceImport(const QString &resourceType, int numResources);

    /// Emitted when the locator finished importing the embedded resource
    void endExternalResourceImport(const QString &resourceType);

    /// Emitted when the locator needs to add an embedded resource
    void beginExternalResourceRemove(const QString &resourceType, const QVector<int> resourceIds);

    /// Emitted when the locator finished importing the embedded resource
    void endExternalResourceRemove(const QString &resourceType);

    /// Emitted when a resource changes its active state
    void resourceActiveStateChanged(const QString &resourceType, int resourceId);

    /// Emitted when a storage is resynchronized using KisresourceCacheDb::synchronizeStorage()
    ///
    /// if \p isBulkResynchronization then this resynchronization happened as a part
    /// of bulk resynchronization at the start of Krita. At the end of this bulk
    /// action storagesBulkSynchronizationFinished() will be emitted as well.
    void storageResynchronized(const QString &storage, bool isBulkResynchronization);

    /// Emitted when bulk-synchronization of all the storages has been finished
    ///
    /// \see storageResynchronized
    void storagesBulkSynchronizationFinished();

private:

    friend class KisAllTagsModel;
    friend class KisTagResourceModel;
    friend class KisAllResourcesModel;
    friend class KisAllTagResourceModel;
    friend class KisStorageModel;
    friend class TestResourceLocator;
    friend class TestResourceModel;
    friend class Resource;
    friend class KisResourceCacheDb;
    friend class KisStorageFilterProxyModel;
    friend class KisResourceQueryMapper;
    friend class KisResourceUserOperations;
    friend class KisBrushTypeMetaDataFixup;
    friend class KisResourceThumbnailCache;

    /// @return true if the resource is present in the cache, false if it hasn't been loaded
    bool resourceCached(QString storageLocation, const QString &resourceType, const QString &filename) const;

    /**
     * @brief resource finds a physical resource in one of the storages
     * @param storageLocation the storage containing the resource. If empty,
     * this is the folder storage.
     *
     * Note that the resource does not have the version or id field set, so this cannot be used directly,
     * but only through KisResourceModel.
     *
     * @param resourceType the type of the resource
     * @param filename the filename of the resource including extension, but without
     * any paths
     * @return A resource if found, or 0
     */
    KoResourceSP resource(QString storageLocation, const QString &resourceType, const QString &filename);

    /**
     * @brief resourceForId returns the resource with the given id, or 0 if no such resource exists.
     * The resource object will have its id set but not its version.
     * @param resourceId the id
     */
    KoResourceSP resourceForId(int resourceId);

    /**
     * @brief setResourceActive
     * @param resourceId
     * @param active shows if the resource should be set as active or not
     * @return
     */
    bool setResourceActive(int resourceId, bool active);

    /**
     * @brief importResourceFromFile
     * @param resourceType
     * @param fileName
     * @param storageLocation: optional, the storage where the resource will be stored. Empty means in the default Folder storage.
     * @return the imported resource, which has been added to the database and the cache
     */
    KoResourceSP importResourceFromFile(const QString &resourceType, const QString &fileName, const bool allowOverwrite, const QString &storageLocation = QString());

    /**
     * @brief importResource
     * @param resourceType
     * @param fileName: filename that should be assigned to the resource
     * @param device: QIODevice where the resource should be loaded from
     * @param storageLocation: optional, the storage where the resource will be stored. Empty means in the default Folder storage.
     * @return the imported resource, which has been added to the database and the cache
     */
    KoResourceSP importResource(const QString &resourceType, const QString &fileName, QIODevice *device, const bool allowOverwrite, const QString &storageLocation = QString());

    /**
     * @brief return whether importing will overwrite some existing resource
     * @param resourceType
     * @param fileName: filename that should be assigned to the resource
     * @param storageLocation: optional, the storage where the resource will be stored. Empty means in the default Folder storage.
     */
    bool importWillOverwriteResource(const QString &resourceType, const QString &fileName, const QString &storageLocation = QString()) const;

    /**
     * @brief exportResource
     * @param resource resource to be exported
     * @param device: QIODevice where the resource should be loaded to
     * @return true if the resource has been exported successfully
     */
    bool exportResource(KoResourceSP resource, QIODevice *device);

    /**
     * @brief addResource adds the given resource to the database and potentially a storage
     * @param resourceType the type of the resource
     * @param resource the actual resource object
     * @param storageLocation the storage where the resource will be saved. By default this is the default folder storage.
     * @return true if successful
     */
    bool addResource(const QString &resourceType, const KoResourceSP resource, const QString &storageLocation = QString());

    /**
     * @brief updateResource
     * @param resourceType
     * @param resource
     * @return
     */
    bool updateResource(const QString &resourceType, const KoResourceSP resource);

    /**
     * @brief Reloads the resource from its persistent storage
     * @param resourceType the type of the resource
     * @param resource the actual resource object
     * @return true if reloading was successful. When returned false,
     *         \p resource is kept unchanged
     */
    bool reloadResource(const QString &resourceType, const KoResourceSP resource);

    /**
     * @brief metaDataForResource
     * @param id
     * @return
     */
    QMap<QString, QVariant> metaDataForResource(int id) const;

    /**
     * @brief setMetaDataForResource
     * @param id
     * @param map
     * @return
     */
    bool setMetaDataForResource(int id, QMap<QString, QVariant> map) const;

    /**
     * @brief metaDataForStorage
     * @param storage
     * @return
     */
    QMap<QString, QVariant> metaDataForStorage(const QString &storageLocation) const;

    /**
     * @brief setMetaDataForStorage
     * @param storage
     * @param map
     */
    void setMetaDataForStorage(const QString &storageLocation, QMap<QString, QVariant> map) const;

    /**
     * Loads all the resources required by \p resource into the cache
     *
     * loadRequiredResources() also loads embedded resources and adds them
     * into the database.
     */
    void loadRequiredResources(KoResourceSP resource);

    /**
     * @brief tagForUrl create a tag from the database
     * @param tagUrl the url
     * @return a complete tag with all translated names and comments.
     */
    KisTagSP tagForUrl(const QString &tagUrl, const QString resourceType);

    /**
     * @brief tagForUrlNoCache create a tag from the database, don't use cache
     * @param tagUrl url of the tag
     * @param resourceType resource type of the tag
     * @return
     */
    static KisTagSP tagForUrlNoCache(const QString &tagUrl, const QString resourceType);

    KisResourceLocator(QObject *parent);
    KisResourceLocator(const KisResourceLocator&);
    KisResourceLocator operator=(const KisResourceLocator&);

    enum class InitializationStatus {
        Unknown,      // We don't know whether Krita has run on this system for this resource location yet
        Initialized,  // Everything is ready to start synchronizing the database
        FirstRun,     // Krita hasn't run for this resource location yet
        FirstUpdate,  // Krita was installed, but it's a version from before the resource locator existed, only user-defined resources are present
        Updating      // Krita is updating from an older version with resource locator
    };

    LocatorError firstTimeInstallation(InitializationStatus initializationStatus, const QString &installationResourcesLocation);

    // Synchronize on restarting Krita to see whether the user has added any storages or resources to the resources location
    bool synchronizeDb();

    void findStorages();
    QList<KisResourceStorageSP> storages() const;

    KisResourceStorageSP storageByLocation(const QString &location) const;
    KisResourceStorageSP folderStorage() const;
    KisResourceStorageSP memoryStorage() const;
    KisResourceStorageSP fontStorage() const;

    struct ResourceStorage {
        QString storageLocation;
        QString resourceType;
        QString resourceFileName;
     };

    friend class KisMyPaintPaintOpPreset;

    ResourceStorage getResourceStorage(int resourceId) const;
    QString makeStorageLocationAbsolute(QString storageLocation) const;
    QString makeStorageLocationRelative(QString location) const;

    class Private;
    QScopedPointer<Private> d;
};

#endif // KISRESOURCELOCATOR_H
