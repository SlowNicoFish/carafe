#pragma once

#include <QObject>
#include <QString>

struct AppSettings {
    QString defaultProton;
    QString steamgridApiKey;
    QString defaultLaunchArgs;
    QString defaultWrapperCommand;

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

    /** Fast JSON-only load; never touches the keyring. */
    static AppSettings loadBasic();
    /** Reads the SteamGridDB API key from the keyring (may block briefly). */
    static QString loadApiKey(const QString &jsonFallback);
    bool        save(const AppSettings &settings) const;

    /** Returns true when the system keyring is usable at run time. */
    static bool keyringAvailable();

private:
    static QString settingsPath();
};
