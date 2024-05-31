/*
    localegenhelper.cpp
    SPDX-FileCopyrightText: 2021 Han Young <hanyoung@protonmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "localegenhelper.h"
#include "localegenhelperadaptor.h"

#include <KLocalizedString>

#include <QDBusConnection>
#include <QDebug>

#include <chrono>

using namespace Qt::StringLiterals;

LocaleGenHelper::LocaleGenHelper()
    : m_authority(PolkitQt1::Authority::instance())
{
    new LocaleGenHelperAdaptor(this);
    if (!QDBusConnection::systemBus().registerService(QStringLiteral("org.kde.localegenhelper"))) {
        qWarning() << "another helper is already running";
        QCoreApplication::instance()->exit();
    }
    if (!QDBusConnection::systemBus().registerObject(QStringLiteral("/LocaleGenHelper"), this)) {
        qWarning() << "unable to register service interface to dbus";
        QCoreApplication::instance()->exit();
    }
    connect(m_authority, &PolkitQt1::Authority::checkAuthorizationFinished, this, &LocaleGenHelper::enableLocalesPrivate);
    connect(&m_timer, &QTimer::timeout, this, [] {
        QCoreApplication::instance()->exit();
    });
    exitAfterTimeOut();
}

void LocaleGenHelper::enableLocales(const QStringList &locales)
{
    qDebug() << locales;
    if (m_timer.isActive()) {
        m_timer.stop();
    }
    if (m_isGenerating) {
        Q_EMIT error(i18n("Another process is already running, please retry later"));
        exitAfterTimeOut();
        return;
    }
    processLocales(locales);
    m_isGenerating = true;
    if (shouldGenerate()) {
        m_authority->checkAuthorization(QStringLiteral("org.kde.localegenhelper.enableLocales"),
                                        PolkitQt1::SystemBusNameSubject(message().service()),
                                        PolkitQt1::Authority::AllowUserInteraction);
    } else {
        exitAfterTimeOut();
        Q_EMIT success();
    }
}

void LocaleGenHelper::enableLocalesPrivate(PolkitQt1::Authority::Result result)
{
    qDebug() << result;
    if (result != PolkitQt1::Authority::Result::Yes) {
        Q_EMIT error(i18n("Unauthorized to edit locale configuration file"));
        exitAfterTimeOut();
        return;
    }

    // if success, handleLocaleGen will call exit
    if (editLocaleGen()) {
        exitAfterTimeOut();
    }
}

bool LocaleGenHelper::shouldGenerate()
{
    QFile localegen(QStringLiteral("/etc/locale.gen"));
    if (!localegen.open(QIODevice::ReadOnly)) {
        return false;
    }
    m_alreadyEnabled.clear();
    while (!localegen.atEnd()) {
        QString locale = QString::fromLocal8Bit(localegen.readLine().simplified());
        if (!m_comment && locale == u"# generated by KDE Plasma Region & Language KCM") {
            m_comment = true;
        }
        if (locale.isEmpty() || locale.front() == u'#') {
            continue;
        }
        const QList<QStringView> localeAndCharset = QStringView(locale).split(u' ');
        if (localeAndCharset.size() != 2 || localeAndCharset.at(1) != u"UTF-8") {
            continue;
        } else {
            QString localeNameWithoutCharset = localeAndCharset.front().toString().remove(".UTF-8"_L1);
            m_alreadyEnabled.insert(localeNameWithoutCharset);
        }
    }
    for (const auto &locale : std::as_const(m_locales)) {
        if (locale == QLatin1Char('C')) {
            continue;
        }
        if (m_alreadyEnabled.count(locale) == 0) {
            return true;
        }
    }
    return false;
}

bool LocaleGenHelper::editLocaleGen()
{
    bool result = false;
    QFile localegen(QStringLiteral("/etc/locale.gen"));
    if (!localegen.open(QIODevice::Append)) {
        Q_EMIT error(i18n("Can't open file `/etc/locale.gen`"));
        return result;
    }
    for (const auto &locale : std::as_const(m_locales)) {
        if (m_alreadyEnabled.count(locale) || locale == QLatin1Char('C')) {
            continue;
        }
        // start at newline first time
        if (!m_comment) {
            localegen.write("\n# generated by KDE Plasma Region & Language KCM\n");
            m_comment = true;
        }
        localegen.write(locale.toUtf8() + ".UTF-8 UTF-8\n"_ba);
    }

    QString localeGenPath = QStandardPaths::findExecutable(QStringLiteral("locale-gen"));
    if (localeGenPath.isEmpty()) {
        localeGenPath = QStandardPaths::findExecutable(QStringLiteral("locale-gen"),
                                                       {
                                                           QStringLiteral("/usr/sbin"),
                                                           QStringLiteral("/sbin"),
                                                           QStringLiteral("/usr/local/sbin"),
                                                       });
    }
    if (!localeGenPath.isEmpty()) {
        auto *process = new QProcess(this);
        process->setProgram(localeGenPath);
        connect(process, &QProcess::finished, this, [this, process](int statusCode, QProcess::ExitStatus status) {
            handleLocaleGen(statusCode, status, process);
        });
        process->start();
        result = true;
    } else {
        Q_EMIT error(i18n("Can't locate executable `locale-gen`"));
    }
    return result;
}

void LocaleGenHelper::handleLocaleGen(int statusCode, QProcess::ExitStatus status, QProcess *process)
{
    Q_UNUSED(status)
    if (statusCode == 0) {
        Q_EMIT success();
    } else {
        QString all_error;
        if (!process) {
            all_error = i18n("Unknown");
        } else {
            all_error.append(QString::fromLocal8Bit(process->readAllStandardOutput()));
            all_error.append(QLatin1Char('\n'));
            all_error.append(QString::fromLocal8Bit(process->readAllStandardError()));
        }
        Q_EMIT error(all_error);
    }
    exitAfterTimeOut();
}

void LocaleGenHelper::exitAfterTimeOut()
{
    m_isGenerating = false;
    m_timer.start(30s);
}

void LocaleGenHelper::processLocales(const QStringList &locales)
{
    QStringList processedLocales = locales;
    for (auto &locale : processedLocales) {
        locale.remove(".UTF-8"_L1);
        if (locale == QLatin1Char('C')) {
            continue;
        }
    }
    m_locales = std::move(processedLocales);
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    LocaleGenHelper generator;
    return app.exec();
}
