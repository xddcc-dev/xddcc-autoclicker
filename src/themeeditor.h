#pragma once

#include <QDialog>
#include <QTextEdit>
#include <QTimer>
#include <QLabel>
#include "thememanager.h"

// live preview widget, paints a mock UI using theme colors
class ThemePreviewWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ThemePreviewWidget(QWidget *parent = nullptr);
    void setTheme(const Theme &t);

protected:
    void paintEvent(QPaintEvent *e) override;

private:
    Theme m_theme;
};

// dialog with a json editor on the left and live preview on the right
class ThemeEditorDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ThemeEditorDialog(ThemeManager *tm, QWidget *parent = nullptr);
    QString savedThemeName() const { return m_savedName; }

private slots:
    void onTextChanged();
    void onDebounceTimeout();
    void saveTheme();

private:
    Theme parseCurrentJson(bool &ok);

    ThemeManager      *m_themeManager;
    QTextEdit         *m_editor;
    ThemePreviewWidget *m_preview;
    QLabel            *m_errorLabel;
    QTimer            *m_debounce;
    QString            m_savedName;

    // the template used for brand new themes
    static QString blankTemplate();
};
