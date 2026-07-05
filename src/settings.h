#pragma once

#include <QObject>
#include <QString>

struct AppSettings {
    QString defaultProton;
    QString steamgridApiKey;
    QString defaultLaunchArgs;
    QString extraProtonPaths;   // newline-separated list

    static AppSettings defaults() { return {}; }
};

/**
 * Loads and saves application settings.
 *
 * Non-sensitive fields are persisted in a JSON file under
 * ~/.local/share/io.marlonn.carafe/settings.json.
 *
 * The SteamGridDB API key is stored in the system keyring (KWallet /
 * Freedesktop Secret Service) when available, with a fallback to the
 * JSON file.
 */
class SettingsStore : public QObject
{
    Q_OBJECT

public:
    explicit SettingsStore(QObject *parent = nullptr);

    static AppSettings load();
    bool        save(const AppSettings &settings) const;

    /** Returns true when the system keyring is usable at run time. */
    static bool keyringAvailable();

private:
    static QString settingsPath();
};
