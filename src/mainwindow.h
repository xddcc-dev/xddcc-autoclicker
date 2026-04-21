#pragma once

#include <QMainWindow>
#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QSpinBox>
#include <QSlider>
#include <QRadioButton>
#include <QCheckBox>
#include <QComboBox>
#include <QStackedWidget>
#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QLabel>
#include <QPoint>
#include <QSystemTrayIcon>
#include <QSettings>
#include <QMenu>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include "clickengine.h"
#include "thememanager.h"
#include "settingsdialog.h"
#include "starburstwidget.h"
#include "themeeditor.h"
#include "particlewidget.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(ThemeManager *tm, QWidget *parent = nullptr);
    ~MainWindow();

    void applyTabPosition(int pos); // 0=top 1=left 2=right

protected:
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
    void closeEvent(QCloseEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;
    void showEvent(QShowEvent *e) override;
    void changeEvent(QEvent *e) override;

private slots:
    void toggleClicking();
    void onStarted();
    void onStopped();
    void onClick(int total);
    void onStatsUpdated(double cps, qint64 elapsedMs);
    void onNavClicked(int idx);
    void openWindowPicker();
    void onThemeChanged(const Theme &t);
    void importTheme();
    void exportTheme();
    void onSettingsChanged();
    void updateTabIndicator(int idx, bool animate);
    void toggleHideMode();
    void saveSettings();
    void loadSettings();
    void exportConfig();
    void importConfig();

private:
    void buildUi();
    QWidget* buildHeader();
    QWidget* buildTabBar();
    QWidget* buildMainPage();
    QWidget* buildThemesPage();
    QWidget* buildAboutPage();
    QWidget* buildBottomBar();
    void refreshThemeGrid();
    void registerHotkey();
    void unregisterHotkey();
    void registerHideHotkey();
    void unregisterHideHotkey();
    void pushEngineSettings();
    void applyStarburst(const Theme &t);
    void applyParticles(const Theme &t);

    ThemeManager    *m_themeManager;
    ClickEngine     *m_engine;
    StarburstWidget *m_starburst  = nullptr;
    ParticleWidget  *m_particles  = nullptr;

    // header / window chrome
    QWidget     *m_header       = nullptr;
    QWidget     *m_tabBarWidget = nullptr;
    QWidget     *m_bottomBar    = nullptr;
    QPushButton *m_minBtn       = nullptr;
    QPushButton *m_closeBtn     = nullptr;

    // top-level layout containers (needed for tab repositioning)
    QWidget     *m_outerWidget  = nullptr;  // central widget
    QVBoxLayout *m_outerLayout  = nullptr;
    QWidget     *m_bodyWidget   = nullptr;  // sits between header and statusbar
    QHBoxLayout *m_bodyLayout   = nullptr;

    // tab buttons + indicator
    QList<QPushButton*> m_navButtons;
    QWidget            *m_tabIndicator = nullptr;
    QPropertyAnimation *m_tabIndAnim   = nullptr;

    // content stack
    QStackedWidget *m_stack;
    bool            m_sliding = false;

    // main page controls
    QPushButton  *m_startBtn;
    QLabel       *m_counter;
    QSpinBox     *m_intervalSpin;
    QSlider      *m_intervalSlider;
    QCheckBox    *m_randomize;
    QSpinBox     *m_jitterSpin;
    QRadioButton *m_btnLeft, *m_btnRight, *m_btnMiddle;
    QRadioButton *m_modeSingle, *m_modeDouble, *m_modeHold;
    QRadioButton *m_posCursor, *m_posFixed;
    QSpinBox     *m_fixedX, *m_fixedY;
    QSpinBox     *m_maxClicks;
    QSpinBox     *m_timeLimitSpin;
    QLabel       *m_hotkeyLabel;
    QPushButton  *m_pickWindowBtn;
    QLabel       *m_targetLabel;
    void         *m_targetHwnd  = nullptr;

    // themes page
    QWidget      *m_themeGrid;
    QPushButton  *m_importThemeBtn;
    QPushButton  *m_exportThemeBtn;

    // settings page
    SettingsDialog *m_settings;

    // bottom status bar
    QLabel *m_statusText;
    QLabel *m_cpsLabel;
    QLabel *m_elapsedLabel;
    QLabel *m_activeIndicator;

    // unused glow members kept to avoid linker issues
    QGraphicsDropShadowEffect *m_startGlow = nullptr;
    QPropertyAnimation        *m_glowAnim  = nullptr;

    // window drag
    QPoint m_dragPos;
    bool   m_dragging = false;

    // hotkeys
    int     m_hotkeyId     = 1;
    int     m_hideHotkeyId = 2;
    QString m_hotkeyStr    = "F6";
    QString m_hideHotkeyStr = "F8";

    // hide mode
    bool            m_hideMode  = false;
    QSystemTrayIcon *m_trayIcon = nullptr;

    // settings persistence
    QSettings m_settings_store{"XDDCC", "AutoClicker"};
};
