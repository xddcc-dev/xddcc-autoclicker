#include "thememanager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QApplication>
#include <QDir>

ThemeManager::ThemeManager(QObject *parent) : QObject(parent) {}

void ThemeManager::loadBuiltInThemes()
{
    QStringList files = {
        ":/themes/default.json",
        ":/themes/xddcc.json"
    };
    for (const QString &f : files) {
        loadThemeFromFile(f);
    }
}

bool ThemeManager::loadThemeFromFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return false;
    QByteArray data = file.readAll();
    file.close();

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError) return false;

    Theme t = parseThemeJson(doc.object());
    if (t.name.isEmpty()) return false;
    m_themes[t.name] = t;
    return true;
}

bool ThemeManager::saveThemeToFile(const Theme &theme, const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    QJsonDocument doc(themeToJson(theme));
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

Theme ThemeManager::parseThemeJson(const QJsonObject &obj)
{
    Theme t;
    t.raw = obj;
    t.name = obj.value("name").toString();
    t.author = obj.value("author").toString();
    t.version = obj.value("version").toString();

    QJsonObject colors = obj.value("colors").toObject();
    for (auto it = colors.begin(); it != colors.end(); ++it) {
        t.colors[it.key()] = it.value().toString();
    }

    QJsonObject grads = obj.value("gradients").toObject();
    for (auto it = grads.begin(); it != grads.end(); ++it) {
        QJsonArray arr = it.value().toArray();
        QStringList stops;
        for (const auto &v : arr) stops << v.toString();
        t.gradients[it.key()] = stops;
    }

    t.borderRadius = obj.value("borderRadius").toInt(8);
    t.fontFamily   = obj.value("fontFamily").toString("Segoe UI");
    t.fontSize     = obj.value("fontSize").toInt(13);
    t.description  = obj.value("description").toString();
    t.starburst      = obj.value("starburst").toBool(false);
    t.particleConfig = obj.value("particles").toObject();

    QJsonObject anims = obj.value("animations").toObject();
    t.animationsEnabled = anims.value("enabled").toBool(true);
    t.animationDuration = anims.value("duration").toInt(200);
    t.hoverGlow         = anims.value("hoverGlow").toBool(true);
    t.pulseOnClick      = anims.value("pulseOnClick").toBool(true);

    return t;
}

QJsonObject ThemeManager::themeToJson(const Theme &theme)
{
    QJsonObject root;
    root["name"] = theme.name;
    root["author"] = theme.author;
    root["version"] = theme.version;

    QJsonObject colors;
    for (auto it = theme.colors.begin(); it != theme.colors.end(); ++it)
        colors[it.key()] = it.value();
    root["colors"] = colors;

    QJsonObject grads;
    for (auto it = theme.gradients.begin(); it != theme.gradients.end(); ++it) {
        QJsonArray arr;
        for (const QString &s : it.value()) arr.append(s);
        grads[it.key()] = arr;
    }
    root["gradients"] = grads;

    root["borderRadius"] = theme.borderRadius;
    root["fontFamily"] = theme.fontFamily;
    root["fontSize"] = theme.fontSize;

    QJsonObject anims;
    anims["enabled"] = theme.animationsEnabled;
    anims["duration"] = theme.animationDuration;
    anims["hoverGlow"] = theme.hoverGlow;
    anims["pulseOnClick"] = theme.pulseOnClick;
    root["animations"] = anims;
    return root;
}

QStringList ThemeManager::themeNames() const { return m_themes.keys(); }
Theme ThemeManager::theme(const QString &name) const { return m_themes.value(name); }
void ThemeManager::addTheme(const Theme &t) { if (!t.name.isEmpty()) m_themes[t.name] = t; }

bool ThemeManager::applyTheme(const QString &name)
{
    if (!m_themes.contains(name)) return false;
    m_current = m_themes[name];
    QString ss = buildStylesheet(m_current);
    if (qApp) qApp->setStyleSheet(ss);
    emit themeChanged(m_current);
    return true;
}

QString ThemeManager::buildStylesheet(const Theme &t)
{
    auto c = [&](const QString &k, const QString &fallback = "#000000") {
        return t.colors.value(k, fallback);
    };

    QString buttonGrad1 = t.gradients.value("button", {c("primary"), c("primaryHover")}).value(0, c("primary"));
    QString buttonGrad2 = t.gradients.value("button", {c("primary"), c("primaryHover")}).value(1, c("primaryHover"));

    // subtle hover: just 10% lighter than the base button color not blindingly bright
    QColor hoverColor = QColor(buttonGrad1).lighter(110);
    QString subtleHover = hoverColor.name();
    QColor pressColor = QColor(buttonGrad1).darker(110);
    QString subtlePress = pressColor.name();

    int r = t.borderRadius;

    QString ss;
    // base rule, everything inherits font and color, bg defaults to main bg
    ss += QString("* { font-family: '%1'; font-size: %2px; color: %3; background: %4; }\n")
          .arg(t.fontFamily).arg(t.fontSize).arg(c("text")).arg(c("background"));

    ss += QString("QMainWindow, QDialog { background: %1; }\n").arg(c("background"));

    ss += QString("QWidget#card, QFrame#card { background: %1; border: 1px solid %2; border-radius: %3px; }\n")
          .arg(c("surface"), c("border")).arg(r);

    ss += QString("QPushButton { background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 %1, stop:1 %2); "
                  "color: white; border: none; border-radius: %3px; padding: 8px 16px; font-weight: 600; }\n")
          .arg(buttonGrad1, buttonGrad2).arg(r);
    // hover is only slightly brighter
    ss += QString("QPushButton:hover { background: %1; }\n").arg(subtleHover);
    ss += QString("QPushButton:pressed { background: %1; }\n").arg(subtlePress);
    ss += QString("QPushButton:disabled { background: %1; color: %2; }\n").arg(c("border"), c("textMuted"));

    ss += QString("QPushButton#sidebarBtn { background: transparent; color: %1; text-align: left; padding: 10px 14px; border-radius: %2px; }\n")
          .arg(c("textMuted")).arg(r);
    ss += QString("QPushButton#sidebarBtn:hover { background: %1; color: %2; }\n").arg(c("surface"), c("text"));
    ss += QString("QPushButton#sidebarBtn:checked { background: %1; color: white; }\n").arg(c("primary"));

    ss += QString("QPushButton#titleBtn { background: transparent; color: %1; border-radius: 4px; padding: 4px 8px; }\n").arg(c("textMuted"));
    ss += QString("QPushButton#titleBtn:hover { background: %1; color: white; }\n").arg(c("surface"));
    ss += QString("QPushButton#closeBtn:hover { background: %1; color: white; }\n").arg(c("danger"));

    ss += QString("QPushButton#startBtn { font-size: %1px; font-weight: 700; border-radius: %2px; }\n")
          .arg(t.fontSize + 4).arg(r + 2);
    // active = stop nono
    ss += QString("QPushButton#startBtn[active=true] { background: %1; }\n").arg(c("danger"));

    ss += QString("QLabel { background: transparent; color: %1; }\n").arg(c("text"));
    ss += QString("QLabel#muted { color: %1; }\n").arg(c("textMuted"));
    ss += QString("QLabel#heading { font-size: %1px; font-weight: 700; color: %2; }\n").arg(t.fontSize + 8).arg(c("text"));
    ss += QString("QLabel#counter { font-size: 48px; font-weight: 800; color: %1; }\n").arg(c("accent"));

    ss += QString("QSpinBox, QLineEdit, QComboBox { background: %1; color: %2; border: 1px solid %3; border-radius: %4px; padding: 4px 8px; }\n")
          .arg(c("surface"), c("text"), c("border")).arg(r/2 + 2);
    ss += QString("QSpinBox:focus, QLineEdit:focus, QComboBox:focus { border: 1px solid %1; }\n").arg(c("primary"));

    // flat spinbox buttons, gets rid of the non modern old ahh look
    ss += QString("QSpinBox::up-button, QSpinBox::down-button { "
                  "background: %1; border: none; border-left: 1px solid %2; width: 18px; }\n")
          .arg(c("border"), c("border"));
    ss += QString("QSpinBox::up-button { border-bottom: 1px solid %1; }\n").arg(c("border"));
    ss += QString("QSpinBox::up-button:hover, QSpinBox::down-button:hover { background: %1; }\n").arg(c("primary"));
    ss += QString("QSpinBox::up-arrow   { width: 7px; height: 7px; border-left: 4px solid transparent; "
                  "border-right: 4px solid transparent; border-bottom: 5px solid %1; }\n").arg(c("text"));
    ss += QString("QSpinBox::down-arrow { width: 7px; height: 7px; border-left: 4px solid transparent; "
                  "border-right: 4px solid transparent; border-top: 5px solid %1; }\n").arg(c("text"));

    ss += QString("QComboBox::drop-down { background: %1; border: none; border-left: 1px solid %2; width: 22px; }\n")
          .arg(c("border"), c("border"));
    ss += QString("QComboBox::drop-down:hover { background: %1; }\n").arg(c("primary"));
    ss += QString("QComboBox::down-arrow { border-left: 4px solid transparent; border-right: 4px solid transparent; "
                  "border-top: 5px solid %1; width: 0; height: 0; }\n").arg(c("text"));
    ss += QString("QComboBox QAbstractItemView { background: %1; color: %2; selection-background-color: %3; border: 1px solid %4; }\n")
          .arg(c("surface"), c("text"), c("primary"), c("border"));

    ss += QString("QSlider::groove:horizontal { background: %1; height: 6px; border-radius: 3px; }\n").arg(c("border"));
    ss += QString("QSlider::handle:horizontal { background: %1; width: 16px; height: 16px; margin: -6px 0; border-radius: 8px; }\n").arg(c("primary"));
    ss += QString("QSlider::sub-page:horizontal { background: %1; border-radius: 3px; }\n").arg(c("primary"));

    ss += QString("QRadioButton, QCheckBox { color: %1; spacing: 8px; background: transparent; }\n").arg(c("text"));
    ss += QString("QRadioButton::indicator, QCheckBox::indicator { width: 16px; height: 16px; border: 1px solid %1; border-radius: 8px; background: %2; }\n")
          .arg(c("border"), c("surface"));
    ss += QString("QCheckBox::indicator { border-radius: 3px; }\n");
    ss += QString("QRadioButton::indicator:checked, QCheckBox::indicator:checked { background: %1; border: 1px solid %1; }\n").arg(c("primary"));

    ss += QString("QFrame { background: transparent; }\n");
    ss += QString("QFrame#separator { background: %1; max-height: 1px; }\n").arg(c("border"));

    // scroll areas, kill the default white viewport
    ss += QString("QScrollArea { background: %1; border: none; }\n").arg(c("background"));
    ss += QString("QAbstractScrollArea { background: %1; border: none; }\n").arg(c("background"));
    ss += QString("QAbstractScrollArea > QWidget { background: %1; }\n").arg(c("background"));
    ss += QString("QScrollBar:vertical { background: %1; width: 8px; border-radius: 4px; border: none; }\n").arg(c("background"));
    ss += QString("QScrollBar::handle:vertical { background: %1; border-radius: 4px; min-height: 20px; border: none; }\n").arg(c("border"));
    ss += QString("QScrollBar::add-line, QScrollBar::sub-line { background: transparent; height: 0; border: none; }\n");

    // custom status bar widget at the bottom
    ss += QString("QWidget#statusBar { background: %1; border-top: 1px solid %2; }\n")
          .arg(c("surface"), c("border"));
    ss += QString("QStatusBar { background: %1; color: %2; border-top: 1px solid %3; font-size: %4px; }\n")
          .arg(c("surface"), c("textMuted"), c("border")).arg(t.fontSize - 1);

    ss += QString("QListWidget { background: %1; border: 1px solid %2; border-radius: %3px; color: %4; }\n")
          .arg(c("surface"), c("border")).arg(r).arg(c("text"));
    ss += QString("QListWidget::item { padding: 6px; }\n");
    ss += QString("QListWidget::item:selected { background: %1; color: white; }\n").arg(c("primary"));

    ss += QString("QWidget#themeCard { background: %1; border: 2px solid %2; border-radius: %3px; }\n")
          .arg(c("surface"), c("border")).arg(r);
    ss += QString("QWidget#themeCardActive { background: %1; border: 2px solid %2; border-radius: %3px; }\n")
          .arg(c("surface"), c("primary")).arg(r);

    ss += QString("QWidget#header { background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 %1, stop:1 %2); }\n")
          .arg(t.gradients.value("header", {c("background"), c("surface")}).value(0, c("background")),
               t.gradients.value("header", {c("background"), c("surface")}).value(1, c("surface")));

    // tab bar
    ss += QString("QWidget#tabBar { background: %1; }\n").arg(c("background"));
    ss += QString("QWidget#tabIndicator { background: %1; border: none; }\n").arg(c("primary"));
    ss += QString("QPushButton#tabBtn { background: transparent; color: %1; border: none; border-bottom: 2px solid transparent; "
                  "border-radius: 0px; padding: 6px 14px; font-weight: 600; font-size: %2px; }\n")
          .arg(c("textMuted")).arg(t.fontSize);
    ss += QString("QPushButton#tabBtn:hover { color: %1; background: transparent; }\n").arg(c("text"));
    ss += QString("QPushButton#tabBtn:checked { color: %1; border-bottom: 2px solid %1; background: transparent; }\n").arg(c("primary"));

    return ss;
}
