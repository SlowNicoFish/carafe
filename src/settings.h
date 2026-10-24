#pragma once

#include <QJsonObject>
#include <QObject>
#include <QString>

struct AppSettings {
    QString defaultProton;
    QString steamgridApiKey;
    bool removeSteamgridApiKey = false;
    QString defaultLaunchArgs;
    QString defaultWrapperCommand;

    static AppSettings defaults() { return {}; }
};

/**
 * Loads and saves application settings.
 *
 * Non-sensitive fields are persisted in a JSON file under the platform's
 * application configuration directory.
 *
 * The SteamGridDB API key is stored only in KWallet. It is never written to
 * the JSON settings file.
 */
class SettingsStore : public QObject {
    Q_OBJECT

public:
    explicit SettingsStore(QObject *parent = nullptr);

    static AppSettings loadBasic();
    /** Reads the SteamGridDB API key from KWallet (may block briefly). */
    static QString loadApiKey();
    /** Returns whether the SteamGridDB API key exists in KWallet. */
    static bool hasApiKey();
    bool save(const AppSettings &settings) const;

    /** Returns true when the system keyring is usable at run time. */
    static bool keyringAvailable();

private:
    static QString settingsPath();
    static bool writeSettingsFile(const QJsonObject &obj);
};
