/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef MEDIAPROXY_H
#define MEDIAPROXY_H

#include <QObject>
#include <QPalette>
#include <QQmlParserStatus>
#include <QSize>
#include <QUrl>

#include "../provider/providertype.h"

/**
 * A proxy class that converts a provider url to a real resource url.
 */
class MediaProxy : public QObject, public QQmlParserStatus, public Provider
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    /**
     * Package path from the saved configuration, can be an image file, a url with
     * "image://" scheme or a folder (KPackage).
     */
    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)

    /**
     * The real path of the image
     * e.g. /home/kde/Pictures/image.png
     *      image://package/get? (KPackage)
     */
    Q_PROPERTY(QUrl modelImage READ modelImage NOTIFY modelImageChanged)

    Q_PROPERTY(QSize targetSize READ targetSize WRITE setTargetSize NOTIFY targetSizeChanged)

public:
    explicit MediaProxy(QObject *parent = nullptr);

    void classBegin() override;
    void componentComplete() override;

    QString source() const;
    void setSource(const QString &url);

    QUrl modelImage() const;

    QSize targetSize() const;
    void setTargetSize(const QSize &size);

    Provider::Type providerType() const;

    Q_INVOKABLE void openModelImage();

    Q_INVOKABLE void useSingleImageDefaults();

    static QUrl formatUrl(const QUrl &url);

Q_SIGNALS:
    void sourceChanged();
    void modelImageChanged();
    void targetSizeChanged(const QSize &size);

    /**
     * Emitted when system color scheme changes. The frontend is required to
     * reload the wallpaper even if the image path is not changed.
     */
    void colorSchemeChanged();

private Q_SLOTS:
    /**
     * Switches to dark-colored wallpaper if available when system color
     * scheme is dark.
     *
     * @since 5.26
     */
    void slotSystemPaletteChanged(const QPalette &palette);

private:
    inline bool isDarkColorScheme(const QPalette &palette = {}) const noexcept;

    Provider::Type determineType(const QUrl &url);

    QUrl findPreferredImageInPackage();
    void updateModelImage();

    bool m_ready = false;

    QUrl m_source;
    QUrl m_modelImage;
    Provider::Type m_providerType = Provider::Type::Unknown;

    QSize m_targetSize;

    bool m_isDarkColorScheme;
};

#endif // MEDIAPROXY_H
