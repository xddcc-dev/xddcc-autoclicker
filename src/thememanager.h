#pragma once

#include <QObject>
#include <QJsonObject>
#include <QMap>
#include <QStringList>
#include <QColor>

struct Theme {
    QString name;
    QString author;
    QString version;
    QString description;
    QMap<QString, QString> colors;
    QMap<QString, QStringList> gradients;
    int  borderRadius    = 8;
    QString fontFamily   = "Segoe UI";
    int  fontSize        = 13;
    bool animationsEnabled = true;
    int  animationDuration = 200;
    bool hoverGlow       = true;
    bool pulseOnClick    = true;
    bool        starburst      = false;
    QJsonObject particleConfig;       // particles section from the JSON
    QJsonObject raw;
};

class ThemeManager : public QObject
{
    Q_OBJECT
public:
    explicit ThemeManager(QObject *parent = nullptr);

    void loadBuiltInThemes();
    bool loadThemeFromFile(const QString &path);
    bool saveThemeToFile(const Theme &theme, const QString &path);
    bool applyTheme(const QString &name);
    void addTheme(const Theme &theme); // add programmatically (from editor)

    QStringList themeNames() const;
    Theme theme(const QString &name) const;
    Theme currentTheme() const  { return m_current; }
    QString currentThemeName() const { return m_current.name; }

    static QString buildStylesheet(const Theme &theme);
    static Theme parseThemeJson(const QJsonObject &obj);
    static QJsonObject themeToJson(const Theme &theme);

signals:
    void themeChanged(const Theme &theme);

private:
    QMap<QString, Theme> m_themes;
    Theme m_current;
};
