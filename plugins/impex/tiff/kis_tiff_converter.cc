/*
 *  SPDX-FileCopyrightText: 2005-2006 Cyrille Berger <cberger@cberger.net>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_tiff_converter.h"

#include <stdio.h>

#include <QApplication>
#include <QFile>

#include <QFileInfo>

#include <KoDocumentInfo.h>
#include <KoUnit.h>

#include <KoColorModelStandardIds.h>
#include <KoColorSpace.h>
#include <KoColorSpaceRegistry.h>

#include <KisDocument.h>
#include <KoColorProfile.h>
#include <kis_group_layer.h>
#include <kis_image.h>
#include <kis_layer.h>
#include <kis_paint_layer.h>
#include <kis_transaction.h>
#include <kis_transform_worker.h>

#include "kis_assert.h"
#include "kis_buffer_stream.h"
#include "kis_global.h"
#include "kis_tiff_reader.h"
#include "kis_tiff_writer_visitor.h"
#include "kis_tiff_ycbcr_reader.h"

#include <KisImportExportAdditionalChecks.h>

#if TIFFLIB_VERSION < 20111221
typedef size_t tmsize_t;
#endif

namespace
{
QPair<QString, QString> getColorSpaceForColorType(uint16_t sampletype, uint16_t color_type, uint16_t color_nb_bits, TIFF *image, uint16_t &nbchannels, uint16_t &extrasamplescount, uint8_t &destDepth)
{
    const int bits32 = 32;
    const int bits16 = 16;
    const int bits8 = 8;

    if (sampletype == SAMPLEFORMAT_INT) {
        dbgFile << "Detected signed TIFF image" << color_type << color_nb_bits;
    }

    if (color_type == PHOTOMETRIC_MINISWHITE || color_type == PHOTOMETRIC_MINISBLACK) {
        if (nbchannels == 0)
            nbchannels = 1;
        extrasamplescount = nbchannels - 1; // FIX the extrasamples count in case of
        if (sampletype == SAMPLEFORMAT_IEEEFP) {
            if (color_nb_bits == 16) {
#ifdef HAVE_OPENEXR
                destDepth = 16;
                return QPair<QString, QString>(GrayAColorModelID.id(), Float16BitsColorDepthID.id());
#endif
            } else if (color_nb_bits == 32) {
                destDepth = 32;
                return QPair<QString, QString>(GrayAColorModelID.id(), Float32BitsColorDepthID.id());
            }
            return QPair<QString, QString>(); // sanity check; no support for float of higher or lower bit depth
        }
        if (color_nb_bits <= 8) {
            destDepth = 8;
            return QPair<QString, QString>(GrayAColorModelID.id(), Integer8BitsColorDepthID.id());
        } else /* if (color_nb_bits == bits16) */ {
            destDepth = 16;
            return QPair<QString, QString>(GrayAColorModelID.id(), Integer16BitsColorDepthID.id());
        }

    } else if (color_type == PHOTOMETRIC_RGB /*|| color_type == */) {
        if (nbchannels == 0)
            nbchannels = 3;
        extrasamplescount = nbchannels - 3; // FIX the extrasamples count in case of
        if (sampletype == SAMPLEFORMAT_IEEEFP) {
            if (color_nb_bits == 16) {
#ifdef HAVE_OPENEXR
                destDepth = 16;
                return QPair<QString, QString>(RGBAColorModelID.id(), Float16BitsColorDepthID.id());
#endif
            } else if (color_nb_bits == 32) {
                destDepth = 32;
                return QPair<QString, QString>(RGBAColorModelID.id(), Float32BitsColorDepthID.id());
            }
            return QPair<QString, QString>(); // sanity check; no support for float of higher or lower bit depth
        } else {
            if (color_nb_bits <= 8) {
                destDepth = 8;
                return QPair<QString, QString>(RGBAColorModelID.id(), Integer8BitsColorDepthID.id());
            } else /* if (color_nb_bits == bits16) */ {
                destDepth = 16;
                return QPair<QString, QString>(RGBAColorModelID.id(), Integer16BitsColorDepthID.id());
            }
        }
    } else if (color_type == PHOTOMETRIC_YCBCR) {
        if (nbchannels == 0)
            nbchannels = 3;
        extrasamplescount = nbchannels - 3; // FIX the extrasamples count in case of
        if (sampletype == SAMPLEFORMAT_IEEEFP) {
            if (color_nb_bits == 16) {
#ifdef HAVE_OPENEXR
                destDepth = 16;
                return QPair<QString, QString>(YCbCrAColorModelID.id(), Float16BitsColorDepthID.id());
#endif
            } else if (color_nb_bits == 32) {
                destDepth = 32;
                return QPair<QString, QString>(YCbCrAColorModelID.id(), Float32BitsColorDepthID.id());
            }
            return QPair<QString, QString>(); // sanity check; no support for float of higher or lower bit depth
        } else {
            if (color_nb_bits <= 8) {
                destDepth = 8;
                return QPair<QString, QString>(YCbCrAColorModelID.id(), Integer8BitsColorDepthID.id());
            } else /* if (color_nb_bits == bits16) */ {
                destDepth = 16;
                return QPair<QString, QString>(YCbCrAColorModelID.id(), Integer16BitsColorDepthID.id());
            }
        }
        if (color_nb_bits <= 8) {
            destDepth = 8;
            return QPair<QString, QString>(YCbCrAColorModelID.id(), Integer8BitsColorDepthID.id());
        } else if (color_nb_bits == bits16) {
            destDepth = 16;
            return QPair<QString, QString>(YCbCrAColorModelID.id(), Integer16BitsColorDepthID.id());
        } else {
            return QPair<QString, QString>(); // sanity check; no support integers of higher bit depth
        }
    } else if (color_type == PHOTOMETRIC_SEPARATED) {
        if (nbchannels == 0)
            nbchannels = 4;
        // SEPARATED is in general CMYK but not always, so we check
        uint16_t inkset;
        if ((TIFFGetField(image, TIFFTAG_INKSET, &inkset) == 0)) {
            dbgFile << "Image does not define the inkset.";
            inkset = 2;
        }
        if (inkset != INKSET_CMYK) {
            dbgFile << "Unsupported inkset (right now, only CMYK is supported)";
            char **ink_names;
            uint16_t numberofinks;
            if (TIFFGetField(image, TIFFTAG_INKNAMES, &ink_names) == 1 && TIFFGetField(image, TIFFTAG_NUMBEROFINKS, &numberofinks) == 1) {
                dbgFile << "Inks are :";
                for (uint32_t i = 0; i < numberofinks; i++) {
                    dbgFile << ink_names[i];
                }
            } else {
                dbgFile << "inknames are not defined !";
                // To be able to read stupid adobe files, if there are no information about inks and four channels, then it's a CMYK file :
                if (nbchannels - extrasamplescount != 4) {
                    return QPair<QString, QString>();
                }
                // else - assume it's CMYK and proceed
            }
        }
        if (sampletype == SAMPLEFORMAT_IEEEFP) {
            if (color_nb_bits == 16) {
#ifdef HAVE_OPENEXR
                destDepth = 16;
                return QPair<QString, QString>(CMYKAColorModelID.id(), Float16BitsColorDepthID.id());
#endif
            } else if (color_nb_bits == 32) {
                destDepth = 32;
                return QPair<QString, QString>(CMYKAColorModelID.id(), Float32BitsColorDepthID.id());
            }
            return QPair<QString, QString>(); // sanity check; no support for float of higher or lower bit depth
        }
        if (color_nb_bits <= 8) {
            destDepth = 8;
            return QPair<QString, QString>(CMYKAColorModelID.id(), Integer8BitsColorDepthID.id());
        } else if (color_nb_bits == 16) {
            destDepth = 16;
            return QPair<QString, QString>(CMYKAColorModelID.id(), Integer16BitsColorDepthID.id());
        } else {
            return QPair<QString, QString>(); // no support for other bit depths
        }
    } else if (color_type == PHOTOMETRIC_CIELAB || color_type == PHOTOMETRIC_ICCLAB) {
        if (nbchannels == 0)
            nbchannels = 3;
        extrasamplescount = nbchannels - 3; // FIX the extrasamples count

        switch (color_nb_bits) {
        case bits32: {
            destDepth = bits32;
            if (sampletype == SAMPLEFORMAT_IEEEFP) {
                return QPair<QString, QString>(LABAColorModelID.id(), Float32BitsColorDepthID.id());
            } else {
                return QPair<QString, QString>(); // no support for other bit depths
            }
        }
        case bits16: {
            destDepth = bits16;
            if (sampletype == SAMPLEFORMAT_IEEEFP) {
#ifdef HAVE_OPENEXR
                return QPair<QString, QString>(LABAColorModelID.id(), Float16BitsColorDepthID.id());
#endif
            } else {
                return QPair<QString, QString>(LABAColorModelID.id(), Integer16BitsColorDepthID.id());
            }
            return QPair<QString, QString>(); // no support for other bit depths
        }
        case bits8: {
            destDepth = bits8;
            return QPair<QString, QString>(LABAColorModelID.id(), Integer8BitsColorDepthID.id());
        }
        default: {
            return QPair<QString, QString>();
        }
        }
    } else if (color_type == PHOTOMETRIC_PALETTE) {
        destDepth = 16;
        if (nbchannels == 0)
            nbchannels = 2;
        extrasamplescount = nbchannels - 2; // FIX the extrasamples count
        // <-- we will convert the index image to RGBA16 as the palette is always on 16bits colors
        return QPair<QString, QString>(RGBAColorModelID.id(), Integer16BitsColorDepthID.id());
    }
    return QPair<QString, QString>();
}

template<template<typename> class T> KisTIFFPostProcessor *makePostProcessor(uint32_t nbsamples, QPair<QString, QString> id)
{
    if (id.second == Integer8BitsColorDepthID.id()) {
        return new T<uint8_t>(nbsamples);
    } else if (id.second == Integer16BitsColorDepthID.id()) {
        return new T<uint16_t>(nbsamples);
#ifdef HAVE_OPENEXR
    } else if (id.second == Float16BitsColorDepthID.id()) {
        return new T<half>(nbsamples);
#endif
    } else if (id.second == Float32BitsColorDepthID.id()) {
        return new T<float>(nbsamples);
    } else {
        KIS_ASSERT(false && "TIFF does not support this bit depth!");
        return nullptr;
    }
}

template<template<typename> class T> KisTIFFReaderBase *makeReader(uint32_t nbsamples, QPair<QString, QString> id)
{
    if (id.second == Integer8BitsColorDepthID.id()) {
        return new T<uint8_t>(nbsamples);
    } else if (id.second == Integer16BitsColorDepthID.id()) {
        return new T<uint16_t>(nbsamples);
#ifdef HAVE_OPENEXR
    } else if (id.second == Float16BitsColorDepthID.id()) {
        return new T<half>(nbsamples);
#endif
    } else if (id.second == Float32BitsColorDepthID.id()) {
        return new T<float>(nbsamples);
    } else {
        KIS_ASSERT(false && "TIFF does not support this bit depth!");
        return nullptr;
    }
}
}

KisPropertiesConfigurationSP KisTIFFOptions::toProperties() const
{
    QHash<int, int> compToIndex;
    compToIndex[COMPRESSION_NONE] = 0;
    compToIndex[COMPRESSION_JPEG] = 1;
    compToIndex[COMPRESSION_DEFLATE] = 2;
    compToIndex[COMPRESSION_LZW] = 3;
    compToIndex[COMPRESSION_PIXARLOG] = 8;

    KisPropertiesConfigurationSP cfg = new KisPropertiesConfiguration();

    cfg->setProperty("compressiontype", compToIndex.value(compressionType, 0));
    cfg->setProperty("predictor", predictor - 1);
    cfg->setProperty("alpha", alpha);
    cfg->setProperty("flatten", flatten);
    cfg->setProperty("quality", jpegQuality);
    cfg->setProperty("deflate", deflateCompress);
    cfg->setProperty("pixarlog", pixarLogCompress);
    cfg->setProperty("saveProfile", saveProfile);

    return cfg;
}

void KisTIFFOptions::fromProperties(KisPropertiesConfigurationSP cfg)
{
    QHash<int, int> indexToComp;
    indexToComp[0] = COMPRESSION_NONE;
    indexToComp[1] = COMPRESSION_JPEG;
    indexToComp[2] = COMPRESSION_DEFLATE;
    indexToComp[3] = COMPRESSION_LZW;
    indexToComp[4] = COMPRESSION_PIXARLOG;

    // old value that might be still stored in a config (remove after Krita 5.0 :) )
    indexToComp[8] = COMPRESSION_PIXARLOG;

    compressionType = static_cast<quint16>(indexToComp.value(cfg->getInt("compressiontype", 0), COMPRESSION_NONE));

    predictor = static_cast<quint16>(cfg->getInt("predictor", 0)) + 1;
    alpha = cfg->getBool("alpha", true);
    flatten = cfg->getBool("flatten", true);
    jpegQuality = static_cast<quint16>(cfg->getInt("quality", 80));
    deflateCompress = static_cast<quint16>(cfg->getInt("deflate", 6));
    pixarLogCompress = static_cast<quint16>(cfg->getInt("pixarlog", 6));
    saveProfile = cfg->getBool("saveProfile", true);
}

KisTIFFConverter::KisTIFFConverter(KisDocument *doc)
{
    m_doc = doc;
    m_stop = false;

    TIFFSetWarningHandler(0);
    TIFFSetErrorHandler(0);
}

KisTIFFConverter::~KisTIFFConverter()
{
}

KisImportExportErrorCode KisTIFFConverter::decode(const QString &filename)
{
    dbgFile << "Start decoding TIFF File";
    // Opent the TIFF file
    TIFF *image = 0;

    if (!KisImportExportAdditionalChecks::doesFileExist(filename)) {
        return ImportExportCodes::FileNotExist;
    }
    if (!KisImportExportAdditionalChecks::isFileReadable(filename)) {
        return ImportExportCodes::NoAccessToRead;
    }

    if ((image = TIFFOpen(QFile::encodeName(filename), "r")) == 0) {
        dbgFile << "Could not open the file, either it does not exist, either it is not a TIFF :" << filename;
        return (ImportExportCodes::FileFormatIncorrect);
    }
    do {
        dbgFile << "Read new sub-image";
        KisImportExportErrorCode result = readTIFFDirectory(image);
        if (!result.isOk()) {
            return result;
        }
    } while (TIFFReadDirectory(image));
    // Freeing memory
    TIFFClose(image);
    return ImportExportCodes::OK;
}

KisImportExportErrorCode KisTIFFConverter::readTIFFDirectory(TIFF *image)
{
    // Read information about the tiff
    uint32_t width, height;
    if (TIFFGetField(image, TIFFTAG_IMAGEWIDTH, &width) == 0) {
        dbgFile << "Image does not define its width";
        TIFFClose(image);
        return ImportExportCodes::FileFormatIncorrect;
    }

    if (TIFFGetField(image, TIFFTAG_IMAGELENGTH, &height) == 0) {
        dbgFile << "Image does not define its height";
        TIFFClose(image);
        return ImportExportCodes::FileFormatIncorrect;
    }

    float xres;
    if (TIFFGetField(image, TIFFTAG_XRESOLUTION, &xres) == 0) {
        dbgFile << "Image does not define x resolution";
        // but we don't stop
        xres = 100;
    }

    float yres;
    if (TIFFGetField(image, TIFFTAG_YRESOLUTION, &yres) == 0) {
        dbgFile << "Image does not define y resolution";
        // but we don't stop
        yres = 100;
    }

    uint16_t depth;
    if ((TIFFGetField(image, TIFFTAG_BITSPERSAMPLE, &depth) == 0)) {
        dbgFile << "Image does not define its depth";
        depth = 1;
    }

    uint16_t sampletype;
    if ((TIFFGetField(image, TIFFTAG_SAMPLEFORMAT, &sampletype) == 0)) {
        dbgFile << "Image does not define its sample type";
        sampletype = SAMPLEFORMAT_UINT;
    }

    // Determine the number of channels (useful to know if a file has an alpha or not
    uint16_t nbchannels;
    if (TIFFGetField(image, TIFFTAG_SAMPLESPERPIXEL, &nbchannels) == 0) {
        dbgFile << "Image has an undefined number of samples per pixel";
        nbchannels = 0;
    }

    // Get the number of extrasamples and information about them
    uint16_t *sampleinfo = 0, extrasamplescount;
    if (TIFFGetField(image, TIFFTAG_EXTRASAMPLES, &extrasamplescount, &sampleinfo) == 0) {
        extrasamplescount = 0;
    }

    // Determine the colorspace
    uint16_t color_type;
    if (TIFFGetField(image, TIFFTAG_PHOTOMETRIC, &color_type) == 0) {
        dbgFile << "Image has an undefined photometric interpretation";
        color_type = PHOTOMETRIC_MINISWHITE;
    }

    uint8_t dstDepth = 0;
    QPair<QString, QString> colorSpaceIdTag = getColorSpaceForColorType(sampletype, color_type, depth, image, nbchannels, extrasamplescount, dstDepth);
    if (colorSpaceIdTag.first.isEmpty()) {
        dbgFile << "Image has an unsupported colorspace :" << color_type << " for this depth :" << depth;
        TIFFClose(image);
        return ImportExportCodes::FormatColorSpaceUnsupported;
    }
    dbgFile << "Colorspace is :" << colorSpaceIdTag.first << colorSpaceIdTag.second << " with a depth of" << depth << " and with a nb of channels of" << nbchannels;

    // Read image profile
    dbgFile << "Reading profile";
    const KoColorProfile *profile = 0;
    quint32 EmbedLen;
    quint8 *EmbedBuffer;

    if (TIFFGetField(image, TIFFTAG_ICCPROFILE, &EmbedLen, &EmbedBuffer) == 1) {
        dbgFile << "Profile found";
        QByteArray rawdata;
        rawdata.resize(static_cast<int>(EmbedLen));
        memcpy(rawdata.data(), EmbedBuffer, EmbedLen);
        profile = KoColorSpaceRegistry::instance()->createColorProfile(colorSpaceIdTag.first, colorSpaceIdTag.second, rawdata);
    }

    const QString colorSpaceId = KoColorSpaceRegistry::instance()->colorSpaceId(colorSpaceIdTag.first, colorSpaceIdTag.second);

    // Check that the profile is used by the color space
    if (profile && !KoColorSpaceRegistry::instance()->profileIsCompatible(profile, colorSpaceId)) {
        dbgFile << "The profile " << profile->name() << " is not compatible with the color space model " << colorSpaceIdTag.first << " " << colorSpaceIdTag.second;
        profile = 0;
    }

    // Do not use the linear gamma profile for 16 bits/channel by default, tiff files are usually created with
    // gamma correction. XXX: Should we ask the user?
    if (!profile) {
        dbgFile << "No profile found; trying to assign a default one.";
        if (colorSpaceIdTag.first == RGBAColorModelID.id()) {
            profile = KoColorSpaceRegistry::instance()->profileByName("sRGB-elle-V2-srgbtrc.icc");
        } else if (colorSpaceIdTag.first == GrayAColorModelID.id()) {
            profile = KoColorSpaceRegistry::instance()->profileByName("Gray-D50-elle-V2-srgbtrc.icc");
        } else if (colorSpaceIdTag.first == CMYKAColorModelID.id()) {
            profile = KoColorSpaceRegistry::instance()->profileByName("Chemical proof");
        } else if (colorSpaceIdTag.first == LABAColorModelID.id()) {
            profile = KoColorSpaceRegistry::instance()->profileByName("Lab identity build-in");
        }
        if (!profile) {
            dbgFile << "No suitable default profile found.";
        }
    }

    // Retrieve a pointer to the colorspace
    const KoColorSpace *cs = 0;
    if (profile && profile->isSuitableForOutput()) {
        dbgFile << "image has embedded profile:" << profile->name() << "";
        cs = KoColorSpaceRegistry::instance()->colorSpace(colorSpaceIdTag.first, colorSpaceIdTag.second, profile);
    } else {
        cs = KoColorSpaceRegistry::instance()->colorSpace(colorSpaceIdTag.first, colorSpaceIdTag.second, 0);
    }

    if (cs == 0) {
        dbgFile << "Colorspace" << colorSpaceIdTag.first << colorSpaceIdTag.second << " is not available, please check your installation.";
        TIFFClose(image);
        return ImportExportCodes::FormatColorSpaceUnsupported;
    }

    // Create the cmsTransform if needed
    KoColorTransformation *transform = 0;
    if (profile && !profile->isSuitableForOutput()) {
        dbgFile << "The profile can't be used in krita, need conversion";
        transform = KoColorSpaceRegistry::instance()
                        ->colorSpace(colorSpaceIdTag.first, colorSpaceIdTag.second, profile)
                        ->createColorConverter(cs, KoColorConversionTransformation::internalRenderingIntent(), KoColorConversionTransformation::internalConversionFlags());
    }

    // Check if there is an alpha channel
    int32_t alphapos = -1; // <- no alpha
    bool hasPremultipliedAlpha = false;
    // Check which extra is alpha if any
    dbgFile << "There are" << nbchannels << " channels and" << extrasamplescount << " extra channels";
    if (sampleinfo) { // index images don't have any sampleinfo, and therefore sampleinfo == 0
        for (uint16_t i = 0; i < extrasamplescount; i++) {
            dbgFile << "sample" << i << "extra sample count" << extrasamplescount << "color channel count" << (cs->colorChannelCount()) << "Number of channels" << nbchannels << "sample info" << sampleinfo[i];
            switch (sampleinfo[i]) {
            case EXTRASAMPLE_ASSOCALPHA:
                // The color values are already multiplied with the alpha value. This is reversed in the postprocessor.
                dbgPlugins << "Detected associated alpha @ " << i;
                hasPremultipliedAlpha = true;
                alphapos = extrasamplescount - 1U; // nbsamples - 1
                break;
            case EXTRASAMPLE_UNASSALPHA:
                // color values are not premultiplied with alpha, and can be used as they are.
                alphapos = i;
                break;
            case EXTRASAMPLE_UNSPECIFIED:
            default:
                qWarning() << "Extra sample type not defined for this file, assuming unassociated alpha.";
                alphapos = i;
                break;
            }

            if (sampleinfo[i] == EXTRASAMPLE_UNASSALPHA) {
                // color values are not premultiplied with alpha, and can be used as they are.
                alphapos = i;
            }
        }
    }

    dbgFile << "Alpha pos:" << alphapos;

    // Read META Information
    KoDocumentInfo *info = m_doc->documentInfo();
    char *text;
    if (TIFFGetField(image, TIFFTAG_ARTIST, &text) == 1) {
        info->setAuthorInfo("creator", text);
    }
    if (TIFFGetField(image, TIFFTAG_DOCUMENTNAME, &text) == 1) {
        info->setAboutInfo("title", text);
    }
    if (TIFFGetField(image, TIFFTAG_IMAGEDESCRIPTION, &text) == 1) {
        info->setAboutInfo("description", text);
    }

    uint16_t orientation = ORIENTATION_TOPLEFT;
    if (TIFFGetField(image, TIFFTAG_ORIENTATION, &orientation) == 0) {
        dbgFile << "Orientation not defined, assuming top left";
    }

    dbgFile << "Orientation:" << orientation;

    // Get the planar configuration
    uint16_t planarconfig;
    if (TIFFGetField(image, TIFFTAG_PLANARCONFIG, &planarconfig) == 0) {
        dbgFile << "Plannar configuration is not define";
        TIFFClose(image);
        return ImportExportCodes::FileFormatIncorrect;
    }
    // Creating the KisImageSP
    if (!m_image) {
        m_image = new KisImage(m_doc->createUndoStore(), static_cast<qint32>(width), static_cast<qint32>(height), cs, "built image");
        m_image->setResolution(POINT_TO_INCH(static_cast<qreal>(xres)), POINT_TO_INCH(static_cast<qreal>(yres))); // It is the "invert" macro because we convert from pointer-per-inchs to points
        Q_CHECK_PTR(m_image);
    } else {
        if (m_image->width() < static_cast<qint32>(width) || m_image->height() < static_cast<qint32>(height)) {
            qint32 newwidth = (m_image->width() < static_cast<qint32>(width)) ? static_cast<qint32>(width) : m_image->width();
            qint32 newheight = (m_image->height() < static_cast<qint32>(height)) ? static_cast<qint32>(height) : m_image->height();
            m_image->resizeImage(QRect(0, 0, newwidth, newheight));
        }
    }
    KisPaintLayer *layer = new KisPaintLayer(m_image.data(), m_image->nextLayerName(), quint8_MAX);
    tdata_t buf = 0;
    tdata_t *ps_buf = 0; // used only for planar configuration separated
    KisBufferStreamBase *tiffstream;

    KisTIFFReaderBase *tiffReader = 0;

    quint8 poses[5];
    KisTIFFPostProcessor *postprocessor = 0;

    // Configure poses
    uint16_t nbcolorsamples = nbchannels - extrasamplescount;
    switch (color_type) {
    case PHOTOMETRIC_MINISWHITE: {
        poses[0] = 0;
        poses[1] = 1;
        postprocessor = makePostProcessor<KisTIFFPostProcessorInvert>(nbcolorsamples, colorSpaceIdTag);
    } break;
    case PHOTOMETRIC_MINISBLACK: {
        poses[0] = 0;
        poses[1] = 1;
        postprocessor = makePostProcessor<KisTIFFPostProcessorDummy>(nbcolorsamples, colorSpaceIdTag);
    } break;
    case PHOTOMETRIC_CIELAB: {
        poses[0] = 0;
        poses[1] = 1;
        poses[2] = 2;
        poses[3] = 3;
        postprocessor = makePostProcessor<KisTIFFPostProcessorCIELABtoICCLAB>(nbcolorsamples, colorSpaceIdTag);
    } break;
    case PHOTOMETRIC_ICCLAB: {
        poses[0] = 0;
        poses[1] = 1;
        poses[2] = 2;
        poses[3] = 3;
        postprocessor = makePostProcessor<KisTIFFPostProcessorDummy>(nbcolorsamples, colorSpaceIdTag);
    } break;
    case PHOTOMETRIC_RGB: {
        if (sampletype == SAMPLEFORMAT_IEEEFP) {
            poses[2] = 2;
            poses[1] = 1;
            poses[0] = 0;
            poses[3] = 3;
        } else {
            poses[0] = 2;
            poses[1] = 1;
            poses[2] = 0;
            poses[3] = 3;
        }
        postprocessor = makePostProcessor<KisTIFFPostProcessorDummy>(nbcolorsamples, colorSpaceIdTag);
    } break;
    case PHOTOMETRIC_SEPARATED: {
        poses[0] = 0;
        poses[1] = 1;
        poses[2] = 2;
        poses[3] = 3;
        poses[4] = 4;
        postprocessor = makePostProcessor<KisTIFFPostProcessorDummy>(nbcolorsamples, colorSpaceIdTag);
    } break;
    default:
        break;
    }

    // Initisalize tiffReader
    uint16_t *lineSizeCoeffs = new uint16_t[nbchannels];
    uint16_t vsubsampling = 1;
    uint16_t hsubsampling = 1;
    for (uint32_t i = 0; i < nbchannels; i++) {
        lineSizeCoeffs[i] = 1;
    }
    if (color_type == PHOTOMETRIC_PALETTE) {
        uint16_t *red; // No need to free them they are free by libtiff
        uint16_t *green;
        uint16_t *blue;
        if ((TIFFGetField(image, TIFFTAG_COLORMAP, &red, &green, &blue)) == 0) {
            dbgFile << "Indexed image does not define a palette";
            TIFFClose(image);
            delete[] lineSizeCoeffs;
            return ImportExportCodes::FileFormatIncorrect;
        }

        tiffReader = new KisTIFFReaderFromPalette(layer->paintDevice(), red, green, blue, poses, alphapos, depth, sampletype, nbcolorsamples, extrasamplescount, hasPremultipliedAlpha, transform, postprocessor);
    } else if (color_type == PHOTOMETRIC_YCBCR) {
        TIFFGetFieldDefaulted(image, TIFFTAG_YCBCRSUBSAMPLING, &hsubsampling, &vsubsampling);
        lineSizeCoeffs[1] = hsubsampling;
        lineSizeCoeffs[2] = hsubsampling;
        uint16_t position;
        TIFFGetFieldDefaulted(image, TIFFTAG_YCBCRPOSITIONING, &position);
        if (dstDepth == 8) {
            tiffReader = new KisTIFFYCbCrReader<uint8_t>(
                layer->paintDevice(), static_cast<quint32>(layer->image()->width()), static_cast<quint32>(layer->image()->height()), poses, alphapos, depth, sampletype, nbcolorsamples, extrasamplescount, hasPremultipliedAlpha, transform, postprocessor, hsubsampling, vsubsampling);
        } else if (dstDepth == 16) {
            if (sampletype == SAMPLEFORMAT_IEEEFP) {
#ifdef HAVE_OPENEXR
                tiffReader = new KisTIFFYCbCrReader<half>(layer->paintDevice(),
                                                          static_cast<quint32>(layer->image()->width()),
                                                          static_cast<quint32>(layer->image()->height()),
                                                          poses,
                                                          alphapos,
                                                          depth,
                                                          sampletype,
                                                          nbcolorsamples,
                                                          extrasamplescount,
                                                          hasPremultipliedAlpha,
                                                          transform,
                                                          postprocessor,
                                                          hsubsampling,
                                                          vsubsampling);
#endif
            } else {
                tiffReader = new KisTIFFYCbCrReader<uint16_t>(layer->paintDevice(),
                                                              static_cast<quint32>(layer->image()->width()),
                                                              static_cast<quint32>(layer->image()->height()),
                                                              poses,
                                                              alphapos,
                                                              depth,
                                                              sampletype,
                                                              nbcolorsamples,
                                                              extrasamplescount,
                                                              hasPremultipliedAlpha,
                                                              transform,
                                                              postprocessor,
                                                              hsubsampling,
                                                              vsubsampling);
            }
        } else if (dstDepth == 32) {
            if (sampletype == SAMPLEFORMAT_IEEEFP) {
                tiffReader = new KisTIFFYCbCrReader<float>(layer->paintDevice(),
                                                           static_cast<quint32>(layer->image()->width()),
                                                           static_cast<quint32>(layer->image()->height()),
                                                           poses,
                                                           alphapos,
                                                           depth,
                                                           sampletype,
                                                           nbcolorsamples,
                                                           extrasamplescount,
                                                           hasPremultipliedAlpha,
                                                           transform,
                                                           postprocessor,
                                                           hsubsampling,
                                                           vsubsampling);
            } else {
                tiffReader = new KisTIFFYCbCrReader<uint32_t>(layer->paintDevice(),
                                                              static_cast<quint32>(layer->image()->width()),
                                                              static_cast<quint32>(layer->image()->height()),
                                                              poses,
                                                              alphapos,
                                                              depth,
                                                              sampletype,
                                                              nbcolorsamples,
                                                              extrasamplescount,
                                                              hasPremultipliedAlpha,
                                                              transform,
                                                              postprocessor,
                                                              hsubsampling,
                                                              vsubsampling);
            }
        }
    } else if (dstDepth == 8) {
        tiffReader = new KisTIFFReaderTarget<uint8_t>(layer->paintDevice(), poses, alphapos, depth, sampletype, nbcolorsamples, extrasamplescount, hasPremultipliedAlpha, transform, postprocessor, quint8_MAX);
    } else if (dstDepth == 16) {
        if (sampletype == SAMPLEFORMAT_IEEEFP) {
#ifdef HAVE_OPENEXR
            tiffReader = new KisTIFFReaderTarget<half>(layer->paintDevice(), poses, alphapos, depth, sampletype, nbcolorsamples, extrasamplescount, hasPremultipliedAlpha, transform, postprocessor, 1.0);
#endif
        } else {
            tiffReader = new KisTIFFReaderTarget<uint16_t>(layer->paintDevice(), poses, alphapos, depth, sampletype, nbcolorsamples, extrasamplescount, hasPremultipliedAlpha, transform, postprocessor, quint16_MAX);
        }
    } else if (dstDepth == 32) {
        if (sampletype == SAMPLEFORMAT_IEEEFP) {
            tiffReader = new KisTIFFReaderTarget<float>(layer->paintDevice(), poses, alphapos, depth, sampletype, nbcolorsamples, extrasamplescount, hasPremultipliedAlpha, transform, postprocessor, 1.0f);
        } else {
            tiffReader = new KisTIFFReaderTarget<uint32_t>(layer->paintDevice(), poses, alphapos, depth, sampletype, nbcolorsamples, extrasamplescount, hasPremultipliedAlpha, transform, postprocessor, std::numeric_limits<uint32_t>::max());
        }
    }

    if (!tiffReader) {
        delete postprocessor;
        delete[] lineSizeCoeffs;
        TIFFClose(image);
        dbgFile << "Image has an invalid/unsupported color type: " << color_type;
        return ImportExportCodes::FileFormatIncorrect;
    }

    if (TIFFIsTiled(image)) {
        dbgFile << "tiled image";
        uint32_t tileWidth, tileHeight;
        uint32_t x, y;
        TIFFGetField(image, TIFFTAG_TILEWIDTH, &tileWidth);
        TIFFGetField(image, TIFFTAG_TILELENGTH, &tileHeight);
        uint32_t linewidth = (tileWidth * depth * nbchannels) / 8;
        if (planarconfig == PLANARCONFIG_CONTIG) {
            buf = _TIFFmalloc(TIFFTileSize(image));
            if (depth < 16) {
                tiffstream = new KisBufferStreamContigBelow16((uint8_t *)buf, depth, linewidth);
            } else if (depth < 32) {
                tiffstream = new KisBufferStreamContigBelow32((uint8_t *)buf, depth, linewidth);
            } else {
                tiffstream = new KisBufferStreamContigAbove32((uint8_t *)buf, depth, linewidth);
            }
        } else {
            ps_buf = new tdata_t[nbchannels];
            tsize_t *lineSizes = new tsize_t[nbchannels];
            tmsize_t baseSize = TIFFTileSize(image);
            for (uint32_t i = 0; i < nbchannels; i++) {
                ps_buf[i] = _TIFFmalloc(baseSize);
                lineSizes[i] = tileWidth;
                ;
            }
            tiffstream = new KisBufferStreamSeparate(reinterpret_cast<uint8_t **>(ps_buf), nbchannels, depth, lineSizes);
            delete[] lineSizes;
        }
        dbgFile << linewidth << "" << nbchannels << "" << layer->paintDevice()->colorSpace()->colorChannelCount();
        for (y = 0; y < height; y += tileHeight) {
            for (x = 0; x < width; x += tileWidth) {
                dbgFile << "Reading tile x =" << x << " y =" << y;
                if (planarconfig == PLANARCONFIG_CONTIG) {
                    TIFFReadTile(image, buf, x, y, 0, (tsample_t)-1);
                } else {
                    for (uint16_t i = 0; i < nbchannels; i++) {
                        TIFFReadTile(image, ps_buf[i], x, y, 0, i);
                    }
                }
                uint32_t realTileWidth = (x + tileWidth) < width ? tileWidth : width - x;
                for (uint32_t yintile = 0; y + yintile < height && yintile < tileHeight / vsubsampling;) {
                    tiffReader->copyDataToChannels(x, y + yintile, realTileWidth, tiffstream);
                    yintile += 1;
                    tiffstream->moveToLine(yintile);
                }
                tiffstream->restart();
            }
        }
    } else {
        dbgFile << "striped image";
        tsize_t stripsize = TIFFStripSize(image);
        uint32_t rowsPerStrip;
        TIFFGetFieldDefaulted(image, TIFFTAG_ROWSPERSTRIP, &rowsPerStrip);
        dbgFile << rowsPerStrip << "" << height;
        rowsPerStrip = qMin(rowsPerStrip, height); // when TIFFNumberOfStrips(image) == 1 it might happen that rowsPerStrip is incorrectly set
        if (planarconfig == PLANARCONFIG_CONTIG) {
            buf = _TIFFmalloc(stripsize);
            if (depth < 16) {
                tiffstream = new KisBufferStreamContigBelow16(reinterpret_cast<uint8_t *>(buf), depth, stripsize / rowsPerStrip);
            } else if (depth < 32) {
                tiffstream = new KisBufferStreamContigBelow32(reinterpret_cast<uint8_t *>(buf), depth, stripsize / rowsPerStrip);
            } else {
                tiffstream = new KisBufferStreamContigAbove32(reinterpret_cast<uint8_t *>(buf), depth, stripsize / rowsPerStrip);
            }
        } else {
            ps_buf = new tdata_t[nbchannels];
            tsize_t scanLineSize = stripsize / rowsPerStrip;
            dbgFile << " scanLineSize for each plan =" << scanLineSize;
            tsize_t *lineSizes = new tsize_t[nbchannels];
            for (uint32_t i = 0; i < nbchannels; i++) {
                ps_buf[i] = _TIFFmalloc(stripsize);
                lineSizes[i] = scanLineSize / lineSizeCoeffs[i];
            }
            tiffstream = new KisBufferStreamSeparate(reinterpret_cast<uint8_t **>(ps_buf), nbchannels, depth, lineSizes);
            delete[] lineSizes;
        }

        dbgFile << "Scanline size =" << TIFFRasterScanlineSize(image) << " / strip size =" << TIFFStripSize(image) << " / rowsPerStrip =" << rowsPerStrip << " stripsize/rowsPerStrip =" << stripsize / rowsPerStrip;
        uint32_t y = 0;
        dbgFile << " NbOfStrips =" << TIFFNumberOfStrips(image) << " rowsPerStrip =" << rowsPerStrip << " stripsize =" << stripsize;
        for (uint32_t strip = 0; y < height; strip++) {
            if (planarconfig == PLANARCONFIG_CONTIG) {
                TIFFReadEncodedStrip(image, TIFFComputeStrip(image, y, 0), buf, (tsize_t)-1);
            } else {
                for (uint16_t i = 0; i < nbchannels; i++) {
                    TIFFReadEncodedStrip(image, TIFFComputeStrip(image, y, i), ps_buf[i], (tsize_t)-1);
                }
            }
            for (uint32_t yinstrip = 0; yinstrip < rowsPerStrip && y < height;) {
                uint32_t linesread = tiffReader->copyDataToChannels(0, y, width, tiffstream);
                y += linesread;
                yinstrip += linesread;
                tiffstream->moveToLine(yinstrip);
            }
            tiffstream->restart();
        }
    }
    tiffReader->finalize();
    delete[] lineSizeCoeffs;
    delete tiffReader;
    delete tiffstream;
    if (planarconfig == PLANARCONFIG_CONTIG) {
        _TIFFfree(buf);
    } else {
        for (uint32_t i = 0; i < nbchannels; i++) {
            _TIFFfree(ps_buf[i]);
        }
        delete[] ps_buf;
    }

    m_image->addNode(KisNodeSP(layer), m_image->rootLayer().data());

    // Process rotation before handing image over
    // https://developer.apple.com/documentation/imageio/cgimagepropertyorientation
    switch (orientation) {
    case ORIENTATION_TOPRIGHT:
        KisTransformWorker::mirrorX(layer->paintDevice());
        break;
    case ORIENTATION_BOTRIGHT:
        m_image->rotateImage(M_PI);
        break;
    case ORIENTATION_BOTLEFT:
        KisTransformWorker::mirrorY(layer->paintDevice());
        break;
    case ORIENTATION_LEFTTOP:
        m_image->rotateImage(M_PI / 2);
        KisTransformWorker::mirrorY(layer->paintDevice());
        break;
    case ORIENTATION_RIGHTTOP:
        m_image->rotateImage(M_PI / 2);
        break;
    case ORIENTATION_RIGHTBOT:
        m_image->rotateImage(M_PI / 2);
        KisTransformWorker::mirrorX(layer->paintDevice());
        break;
    case ORIENTATION_LEFTBOT:
        m_image->rotateImage(-M_PI / 2 + M_PI * 2);
        break;
    default:
        break;
    }

    return ImportExportCodes::OK;
}

KisImportExportErrorCode KisTIFFConverter::buildImage(const QString &filename)
{
    return decode(filename);
}

KisImageSP KisTIFFConverter::image()
{
    return m_image;
}

KisImportExportErrorCode KisTIFFConverter::buildFile(const QString &filename, KisImageSP kisimage, KisTIFFOptions options)
{
    dbgFile << "Start writing TIFF File";
    KIS_ASSERT_RECOVER_RETURN_VALUE(kisimage, ImportExportCodes::InternalError);

    // Open file for writing
    TIFF *image;
    if ((image = TIFFOpen(QFile::encodeName(filename), "w")) == 0) {
        dbgFile << "Could not open the file for writing" << filename;
        return ImportExportCodes::NoAccessToWrite;
    }

    // Set the document information
    KoDocumentInfo *info = m_doc->documentInfo();
    QString title = info->aboutInfo("title");
    if (!title.isEmpty()) {
        if (!TIFFSetField(image, TIFFTAG_DOCUMENTNAME, title.toLatin1().constData())) {
            TIFFClose(image);
            return ImportExportCodes::ErrorWhileWriting;
        }
    }
    QString abstract = info->aboutInfo("description");
    if (!abstract.isEmpty()) {
        if (!TIFFSetField(image, TIFFTAG_IMAGEDESCRIPTION, abstract.toLatin1().constData())) {
            TIFFClose(image);
            return ImportExportCodes::ErrorWhileWriting;
        }
    }
    QString author = info->authorInfo("creator");
    if (!author.isEmpty()) {
        if (!TIFFSetField(image, TIFFTAG_ARTIST, author.toLatin1().constData())) {
            TIFFClose(image);
            return ImportExportCodes::ErrorWhileWriting;
        }
    }

    dbgFile << "xres: " << INCH_TO_POINT(kisimage->xRes()) << " yres: " << INCH_TO_POINT(kisimage->yRes());
    if (!TIFFSetField(image, TIFFTAG_XRESOLUTION, INCH_TO_POINT(kisimage->xRes()))) { // It is the "invert" macro because we convert from pointer-per-inchs to points
        TIFFClose(image);
        return ImportExportCodes::ErrorWhileWriting;
    }
    if (!TIFFSetField(image, TIFFTAG_YRESOLUTION, INCH_TO_POINT(kisimage->yRes()))) {
        TIFFClose(image);
        return ImportExportCodes::ErrorWhileWriting;
    }

    KisGroupLayer *root = dynamic_cast<KisGroupLayer *>(kisimage->rootLayer().data());
    KIS_ASSERT_RECOVER(root)
    {
        TIFFClose(image);
        return ImportExportCodes::InternalError;
    }

    KisTIFFWriterVisitor *visitor = new KisTIFFWriterVisitor(image, &options);
    if (!(visitor->visit(root))) {
        TIFFClose(image);
        return ImportExportCodes::Failure;
    }

    TIFFClose(image);
    return ImportExportCodes::OK;
}

void KisTIFFConverter::cancel()
{
    m_stop = true;
}
