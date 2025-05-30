/*
 *  SPDX-FileCopyrightText: 2024 Wolthera van Hövell tot Westerflier <griffinvalley@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "TagFilterProxyModelQmlWrapper.h"

#include <KisResourceModel.h>
#include <KisResourceModelProvider.h>
#include <KisTagFilterResourceProxyModel.h>
#include <KisResourceMetaDataModel.h>
#include <KisTagModel.h>
#include <KisTagResourceModel.h>
#include <kis_signal_compressor.h>
#include <KisResourceSearchBoxFilter.h>

#include <resources/KoFontFamily.h>

struct TagFilterProxyModelQmlWrapper::Private {
    Private(QObject *parent = nullptr)
        : tagFilterProxyModel(new FontFamilyTagFilterModel(parent))
        , tagModel(new KisTagModel(ResourceType::FontFamilies, parent))
        , compressor(KisSignalCompressor(100, KisSignalCompressor::POSTPONE, parent))
    {
        allResourceModel = KisResourceModelProvider::resourceModel(ResourceType::FontFamilies);
        tagFilterProxyModel->sort(KisAbstractResourceModel::Name);
        tagFilterProxyModel->setTagFilter(tagModel->tagForIndex(tagModel->index(0, 0)));
    }
    FontFamilyTagFilterModel *tagFilterProxyModel {nullptr};
    KisAllResourcesModel *allResourceModel {nullptr};
    KisTagModel *tagModel {nullptr};
    KisSignalCompressor compressor;
    QString currentSearchText;
    KoResourceSP currentResource;
};

TagFilterProxyModelQmlWrapper::TagFilterProxyModelQmlWrapper(QObject *parent)
    : QObject(parent)
    , d(new Private(parent))
{

    connect(&d->compressor, SIGNAL(timeout()), this, SLOT(setSearchTextOnModel()));
}

TagFilterProxyModelQmlWrapper::~TagFilterProxyModelQmlWrapper()
{
}

QAbstractItemModel *TagFilterProxyModelQmlWrapper::model() const
{
    return d->tagFilterProxyModel;
}

QAbstractItemModel *TagFilterProxyModelQmlWrapper::tagModel() const
{
    return d->tagModel;
}

void TagFilterProxyModelQmlWrapper::tagActivated(const int &row)
{
    QModelIndex idx = d->tagModel->index(row, 0);
    if (idx.isValid()) {
        KisTagSP tag = d->tagModel->tagForIndex(idx);
        if (tag == d->tagFilterProxyModel->currentTagFilter()) return;
        d->tagFilterProxyModel->setTagFilter(tag);
        emit activeTagChanged();
    }
}

int TagFilterProxyModelQmlWrapper::currentTag() const
{
    return d->tagModel->indexForTag(d->tagFilterProxyModel->currentTagFilter()).row();
}

QString TagFilterProxyModelQmlWrapper::searchText() const
{
    return d->currentSearchText;
}

void TagFilterProxyModelQmlWrapper::setSearchText(const QString &text)
{
    if (d->currentSearchText == text) {
        return;
    }
    d->currentSearchText = text;
    emit searchTextChanged();
    d->compressor.start();
}

void TagFilterProxyModelQmlWrapper::setSearchInTag(const bool &newSearchInTag)
{
    if (d->tagFilterProxyModel->filterInCurrentTag() != newSearchInTag) {
        d->tagFilterProxyModel->setFilterInCurrentTag(newSearchInTag);
        emit searchInTagChanged();
    }
}

bool TagFilterProxyModelQmlWrapper::searchInTag()
{
    return d->tagFilterProxyModel->filterInCurrentTag();
}

void TagFilterProxyModelQmlWrapper::addNewTag(const QString &newTagName, const int &resourceIndex)
{
    KisTagSP tagsp = d->tagModel->tagForUrl(newTagName);
    QModelIndex resourceIdx = d->tagFilterProxyModel->index(resourceIndex, 0);
    if (tagsp.isNull()) {
        QVector<KoResourceSP> vec;
        tagsp = d->tagModel->addTag(newTagName, false, vec);

    }
    // TODO: figure out how to get a tag reactivated again, without doing too much code duplication :|
    if (resourceIdx.isValid()) {
        int resourceId = d->tagFilterProxyModel->data(resourceIdx, Qt::UserRole + KisResourceModel::Id).toInt();
        d->tagFilterProxyModel->tagResources(tagsp, {resourceId});
    }

}

void TagFilterProxyModelQmlWrapper::tagResource(const int &tagIndex, const int &resourceIndex)
{
    QModelIndex idx = d->tagModel->index(tagIndex, 0);
    QModelIndex resourceIdx = d->tagFilterProxyModel->index(resourceIndex, 0);
    KisTagSP tagsp;
    if (idx.isValid()) {
        tagsp = d->tagModel->tagForIndex(idx);
    }
    if (tagsp && resourceIdx.isValid()) {
        int resourceId = d->tagFilterProxyModel->data(resourceIdx, Qt::UserRole + KisResourceModel::Id).toInt();
        d->tagFilterProxyModel->tagResources(tagsp, {resourceId});
    }
}

void TagFilterProxyModelQmlWrapper::untagResource(const int &tagIndex, const int &resourceIndex)
{
    QModelIndex idx = d->tagModel->index(tagIndex, 0);
    QModelIndex resourceIdx = d->tagFilterProxyModel->index(resourceIndex, 0);
    KisTagSP tagsp;
    if (idx.isValid()) {
        tagsp = d->tagModel->tagForIndex(idx);
    }
    if (tagsp && resourceIdx.isValid()) {
        int resourceId = d->tagFilterProxyModel->data(resourceIdx, Qt::UserRole + KisResourceModel::Id).toInt();
        d->tagFilterProxyModel->untagResources(tagsp, {resourceId});
    }
}

QString TagFilterProxyModelQmlWrapper::localizedNameFromMetadata(
        const QMap<QString, QVariant> &metadata,
        const QStringList &locales,
        const QString &fallBack)
{
    const QVariantMap localizedNames = metadata.value("localized_font_family").toMap();
    QString name = fallBack.isEmpty()? localizedNames.value("en").toString(): fallBack;

    Q_FOREACH(const QString locale, locales) {
        const QLocale l(locale);
        bool found = false;
        Q_FOREACH(const QString key, localizedNames.keys()) {
            if (QLocale(key) == l) {
                name = localizedNames.value(key, name).toString();
                found = true;
                break;
            }
        }
        if (found) break;
    }
    return name;
}

QVariantMap TagFilterProxyModelQmlWrapper::metadataForIndex(const int &resourceIndex) const
{
    QModelIndex resourceIdx = d->tagFilterProxyModel->index(resourceIndex, 0);
    // NOTE: KisTagFilterProxyModel has this weird thing where it switches between source models depending
    // on whether filtering by tag happens. This somehow causes index() to not always work.
    return d->tagFilterProxyModel->data(resourceIdx, Qt::UserRole + KisResourceModel::MetaData).toMap();
}

#include <KoWritingSystemUtils.h>
QString TagFilterProxyModelQmlWrapper::localizedSampleFromMetadata(const QMap<QString, QVariant> &metadata, const QStringList &locales, const QString &fallBack)
{
    QString sample = fallBack;
    QVariantMap samples = metadata.value("sample_svg").toMap();
    if (!samples.isEmpty()) sample = samples.value("s_Latn", samples.values().first()).toString();
    if (!locales.isEmpty()) {
        Q_FOREACH(const QString locale, locales) {
            const QString tag = KoWritingSystemUtils::sampleTagForQLocale(QLocale (locale));
            if (samples.keys().contains(tag) ) {
                sample = samples.value(tag).toString();
                break;
            }
        }
    }
    return sample;
}

QVariantList TagFilterProxyModelQmlWrapper::taggedResourceModel (const int &resourceIndex) const
{
    QVariantList taggedResourceModel;
    QModelIndex resourceIdx = d->tagFilterProxyModel->index(resourceIndex, 0);
    KisTagSP tagsp;
    for (int i = 0; i< d->tagModel->rowCount(); i++) {
        QModelIndex idx = d->tagModel->index(i, 0);
        tagsp = d->tagModel->tagForIndex(idx);
        bool visible = tagsp->id() >= 0;
        bool enabled = false;
        if (resourceIdx.isValid()) {
            int resourceId = d->tagFilterProxyModel->data(resourceIdx, Qt::UserRole + KisResourceModel::Id).toInt();
            enabled = d->tagFilterProxyModel->isResourceTagged(tagsp, resourceId) > 0 && visible;
        }
        QVariantMap tag { {"name", tagsp->name()}, {"value", i}, {"visible", visible}, {"enabled", enabled} };
        taggedResourceModel.append(tag);
    }
    return taggedResourceModel;
}

bool TagFilterProxyModelQmlWrapper::showResourceTagged(const int &tagIndex, const int &resourceIndex) const
{
    QModelIndex idx = d->tagModel->index(tagIndex, 0);
    QModelIndex resourceIdx = d->tagFilterProxyModel->index(resourceIndex, 0);
    KisTagSP tagsp;
    if (idx.isValid()) {
        tagsp = d->tagModel->tagForIndex(idx);
    }
    if (tagsp) {
        if (tagsp->id() < 0) return false;
        return true;
        if (resourceIdx.isValid()) {
            int resourceId = d->tagFilterProxyModel->data(resourceIdx, Qt::UserRole + KisResourceModel::Id).toInt();
            return d->tagFilterProxyModel->isResourceTagged(tagsp, resourceId);
        }
    }
    return false;
}

int TagFilterProxyModelQmlWrapper::currentIndex() const
{
    return d->tagFilterProxyModel->indexForResource(d->currentResource).row();
}

void TagFilterProxyModelQmlWrapper::setCurrentIndex(const int &index)
{
    QModelIndex resourceIdx = d->tagFilterProxyModel->index(index, 0);
    if (!resourceIdx.isValid()) return;
    KoResourceSP newResource = d->tagFilterProxyModel->resourceForIndex(resourceIdx);
    if (newResource != d->currentResource) {
        d->currentResource = newResource;
        emit currentResourceChanged();
    }
}

void TagFilterProxyModelQmlWrapper::setResourceToFileName(const QString &filename)
{
    KoResourceSP resource = d->currentResource;
    QVector<KoResourceSP> resources = d->allResourceModel->resourcesForFilename(filename);
    if (!resources.isEmpty()) {
        resource = resources.first();
    }
    if (resource != d->currentResource) {
        d->currentResource = resource;
        emit currentResourceChanged();
    }
}

QString TagFilterProxyModelQmlWrapper::resourceFilename()
{
    return d->currentResource? d->currentResource->filename(): "";
}

void TagFilterProxyModelQmlWrapper::setSearchTextOnModel()
{
    d->tagFilterProxyModel->setSearchText(d->currentSearchText);
    emit modelSortUpdated();
}

FontFamilyTagFilterModel::FontFamilyTagFilterModel(QObject *parent)
    : KisTagFilterResourceProxyModel(ResourceType::FontFamilies, parent)
{
}

bool FontFamilyTagFilterModel::additionalResourceNameChecks(const QModelIndex &index, const KisResourceSearchBoxFilter *filter) const
{
    bool match = false;
    if (index.isValid()) {
        const QStringList resourceTags = sourceModel()->data(index, Qt::UserRole + KisAbstractResourceModel::Tags).toStringList();

        KisResourceMetaDataModel *metadataModel = KisResourceModelProvider::resourceMetadataModel();
        const int resourceId = sourceModel()->data(index, Qt::UserRole + KisResourceModel::Id).toInt();
        const QVariantMap localizedNames = metadataModel->metaDataValue(resourceId, "localized_font_family").toMap();

        Q_FOREACH(const QVariant localizedName, localizedNames.values()) {
            match = filter->matchesResource(localizedName.toString(), resourceTags);
            if (match) {
                break;
            }
        }
    }
    return match;
}
