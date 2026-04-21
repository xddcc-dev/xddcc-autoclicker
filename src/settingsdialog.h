#pragma once
#include <QWidget>
#include <QComboBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>

class SettingsDialog : public QWidget
{
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent = nullptr);

    // getters used by MainWindow
    int     clickMethodIndex()   const;
    QString hotkeyText()         const;
    QString hideHotkeyText()     const;
    bool    startMinimized()     const;
    bool    showInTray()         const;
    bool    clickSound()         const;
    QString clickSoundFile()     const;
    bool    performanceMode()    const;
    bool    alwaysOnTop()        const;
    bool    pauseOnMouseMove()   const;
    bool    stopAtScreenEdge()   const;
    bool    showCounterInTitle() const;
    int     focusDelayMs()       const;
    int     windowOpacity()      const;
    int     tabPosition()        const; // 0=top 1=left 2=right

    // setters used by QSettings restore
    void setClickMethodIndex(int i);
    void setHotkeyText(const QString &s);
    void setHideHotkeyText(const QString &s);
    void setStartMinimized(bool v);
    void setShowInTray(bool v);
    void setClickSound(bool v);
    void setClickSoundFile(const QString &s);
    void setPerformanceMode(bool v);
    void setAlwaysOnTop(bool v);
    void setPauseOnMouseMove(bool v);
    void setStopAtScreenEdge(bool v);
    void setShowCounterInTitle(bool v);
    void setFocusDelayMs(int v);
    void setWindowOpacity(int v);
    void setTabPosition(int v);

signals:
    void settingsChanged();
    void clickMethodChanged(int idx);
    void hotkeyChanged(const QString &text);
    void exportConfigRequested();
    void importConfigRequested();

private:
    QComboBox   *m_clickMethod;
    QLineEdit   *m_hotkey;
    QLineEdit   *m_hideHotkey;
    QCheckBox   *m_startMinimized;
    QCheckBox   *m_tray;
    QCheckBox   *m_sound;
    QPushButton *m_soundBrowse;
    QLabel      *m_soundPath;
    QCheckBox   *m_perf;
    QCheckBox   *m_alwaysOnTop;
    QCheckBox   *m_pauseOnMove;
    QCheckBox   *m_stopAtEdge;
    QCheckBox   *m_titleCounter;
    QSpinBox    *m_focusDelay;
    QSpinBox    *m_opacity;
    QComboBox   *m_tabPosition;
};
