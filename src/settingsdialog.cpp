#include "settingsdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QFrame>
#include <QFileDialog>

static QLabel *ml(const QString &text)
{
    auto *l = new QLabel(text);
    l->setStyleSheet("background: transparent;");
    return l;
}

SettingsDialog::SettingsDialog(QWidget *parent) : QWidget(parent)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 20);
    root->setSpacing(14);

    auto *title = new QLabel("Settings");
    title->setObjectName("heading");
    title->setStyleSheet("background: transparent;");
    root->addWidget(title);

    // click sectionnn
    auto *clickCard = new QFrame();
    clickCard->setObjectName("card");
    auto *clickForm = new QFormLayout(clickCard);
    clickForm->setContentsMargins(20, 16, 20, 16);
    clickForm->setSpacing(10);
    clickForm->setLabelAlignment(Qt::AlignLeft);

    m_clickMethod = new QComboBox();
    m_clickMethod->addItems({"SendInput (recommended)", "mouse_event (legacy)", "PostMessage (background)"});
    clickForm->addRow(ml("Click method:"), m_clickMethod);

    m_hotkey = new QLineEdit("F6");
    m_hotkey->setMaxLength(8);
    m_hotkey->setFixedWidth(80);
    clickForm->addRow(ml("Start/stop hotkey:"), m_hotkey);

    m_hideHotkey = new QLineEdit("F8");
    m_hideHotkey->setMaxLength(8);
    m_hideHotkey->setFixedWidth(80);
    clickForm->addRow(ml("Hide/show hotkey:"), m_hideHotkey);

    m_focusDelay = new QSpinBox();
    m_focusDelay->setRange(0, 5000);
    m_focusDelay->setValue(0);
    m_focusDelay->setSuffix(" ms");
    m_focusDelay->setFixedWidth(100);
    clickForm->addRow(ml("Delay after window focus:"), m_focusDelay);

    root->addWidget(clickCard);

    // behavior
    auto *behavCard = new QFrame();
    behavCard->setObjectName("card");
    auto *behavForm = new QFormLayout(behavCard);
    behavForm->setContentsMargins(20, 16, 20, 16);
    behavForm->setSpacing(10);
    behavForm->setLabelAlignment(Qt::AlignLeft);

    m_perf = new QCheckBox("High resolution timer");
    m_perf->setChecked(true);
    m_perf->setStyleSheet("background: transparent;");
    behavForm->addRow(ml("Performance:"), m_perf);

    m_pauseOnMove = new QCheckBox("Pause while mouse is moving");
    m_pauseOnMove->setStyleSheet("background: transparent;");
    behavForm->addRow(ml("Auto-pause:"), m_pauseOnMove);

    m_stopAtEdge = new QCheckBox("Stop when cursor hits screen edge");
    m_stopAtEdge->setStyleSheet("background: transparent;");
    behavForm->addRow(ml("Screen edge:"), m_stopAtEdge);

    root->addWidget(behavCard);

    // ui
    auto *uiCard = new QFrame();
    uiCard->setObjectName("card");
    auto *uiForm = new QFormLayout(uiCard);
    uiForm->setContentsMargins(20, 16, 20, 16);
    uiForm->setSpacing(10);
    uiForm->setLabelAlignment(Qt::AlignLeft);

    m_startMinimized = new QCheckBox("Start minimized to tray");
    m_startMinimized->setStyleSheet("background: transparent;");
    uiForm->addRow(ml("Startup:"), m_startMinimized);

    m_tray = new QCheckBox("Show icon in system tray");
    m_tray->setStyleSheet("background: transparent;");
    uiForm->addRow(ml("Tray:"), m_tray);

    m_alwaysOnTop = new QCheckBox("Keep window always on top");
    m_alwaysOnTop->setStyleSheet("background: transparent;");
    uiForm->addRow(ml("Window:"), m_alwaysOnTop);

    m_titleCounter = new QCheckBox("Show click count in title bar");
    m_titleCounter->setStyleSheet("background: transparent;");
    uiForm->addRow("", m_titleCounter);

    m_tabPosition = new QComboBox();
    m_tabPosition->addItems({"Top", "Left", "Right"});
    uiForm->addRow(ml("Tab position:"), m_tabPosition);

    m_opacity = new QSpinBox();
    m_opacity->setRange(30, 100);
    m_opacity->setValue(100);
    m_opacity->setSuffix(" %");
    m_opacity->setFixedWidth(80);
    uiForm->addRow(ml("Window opacity:"), m_opacity);

    // sound, off by default, with file picker (the sound is broken anyway, someone should fix it)
    m_sound = new QCheckBox("Play sound on each click");
    m_sound->setChecked(false);
    m_sound->setStyleSheet("background: transparent;");

    m_soundBrowse = new QPushButton("Browse...");
    m_soundPath   = new QLabel("No file selected");
    m_soundPath->setObjectName("muted");
    m_soundPath->setStyleSheet("background: transparent; font-size: 11px;");
    m_soundPath->setEnabled(false);
    m_soundBrowse->setEnabled(false);

    auto *soundRow = new QHBoxLayout();
    soundRow->setSpacing(8);
    soundRow->addWidget(m_sound);
    soundRow->addWidget(m_soundBrowse);
    soundRow->addWidget(m_soundPath, 1);
    auto *soundW = new QWidget(); soundW->setLayout(soundRow); soundW->setStyleSheet("background: transparent;");
    uiForm->addRow(ml("Sound:"), soundW);

    root->addWidget(uiCard);

    // config stuff
    auto *cfgCard = new QFrame();
    cfgCard->setObjectName("card");
    auto *cfgLayout = new QHBoxLayout(cfgCard);
    cfgLayout->setContentsMargins(20, 16, 20, 16);
    cfgLayout->setSpacing(10);

    auto *exportBtn = new QPushButton("Export Config...");
    auto *importBtn = new QPushButton("Import Config...");
    cfgLayout->addWidget(ml("Config file:"));
    cfgLayout->addWidget(exportBtn);
    cfgLayout->addWidget(importBtn);
    cfgLayout->addStretch(1);

    connect(exportBtn, &QPushButton::clicked, this, [this]{ emit exportConfigRequested(); });
    connect(importBtn, &QPushButton::clicked, this, [this]{ emit importConfigRequested(); });

    root->addWidget(cfgCard);
    root->addStretch(1);

    // wire signals
    auto emit_changed = [this]{ emit settingsChanged(); };

    connect(m_clickMethod, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int i){ emit clickMethodChanged(i); emit settingsChanged(); });
    connect(m_hotkey,      &QLineEdit::textChanged, this, [this](const QString &t){ emit hotkeyChanged(t); emit settingsChanged(); });
    connect(m_hideHotkey,  &QLineEdit::textChanged, this, emit_changed);
    connect(m_focusDelay,  QOverload<int>::of(&QSpinBox::valueChanged), this, emit_changed);
    connect(m_perf,        &QCheckBox::toggled, this, emit_changed);
    connect(m_pauseOnMove, &QCheckBox::toggled, this, emit_changed);
    connect(m_stopAtEdge,  &QCheckBox::toggled, this, emit_changed);
    connect(m_startMinimized, &QCheckBox::toggled, this, emit_changed);
    connect(m_tray,        &QCheckBox::toggled, this, emit_changed);
    connect(m_alwaysOnTop, &QCheckBox::toggled, this, emit_changed);
    connect(m_titleCounter,&QCheckBox::toggled, this, emit_changed);
    connect(m_tabPosition, QOverload<int>::of(&QComboBox::currentIndexChanged), this, emit_changed);
    connect(m_opacity,     QOverload<int>::of(&QSpinBox::valueChanged), this, emit_changed);

    connect(m_sound, &QCheckBox::toggled, this, [this](bool on){
        m_soundBrowse->setEnabled(on);
        m_soundPath->setEnabled(on);
        emit settingsChanged();
    });
    connect(m_soundBrowse, &QPushButton::clicked, this, [this]{
        QString file = QFileDialog::getOpenFileName(nullptr, "Pick click sound", {}, "Audio (*.mp3 *.wav *.ogg)");
        if (!file.isEmpty()) { m_soundPath->setText(file); emit settingsChanged(); }
    });
}

// get
int     SettingsDialog::clickMethodIndex()   const { return m_clickMethod->currentIndex(); }
QString SettingsDialog::hotkeyText()         const { return m_hotkey->text(); }
QString SettingsDialog::hideHotkeyText()     const { return m_hideHotkey->text(); }
bool    SettingsDialog::startMinimized()     const { return m_startMinimized->isChecked(); }
bool    SettingsDialog::showInTray()         const { return m_tray->isChecked(); }
bool    SettingsDialog::clickSound()         const { return m_sound->isChecked(); }
QString SettingsDialog::clickSoundFile()     const {
    QString s = m_soundPath->text();
    return (s == "No file selected") ? QString() : s;
}
bool    SettingsDialog::performanceMode()    const { return m_perf->isChecked(); }
bool    SettingsDialog::alwaysOnTop()        const { return m_alwaysOnTop->isChecked(); }
bool    SettingsDialog::pauseOnMouseMove()   const { return m_pauseOnMove->isChecked(); }
bool    SettingsDialog::stopAtScreenEdge()   const { return m_stopAtEdge->isChecked(); }
bool    SettingsDialog::showCounterInTitle() const { return m_titleCounter->isChecked(); }
int     SettingsDialog::focusDelayMs()       const { return m_focusDelay->value(); }
int     SettingsDialog::windowOpacity()      const { return m_opacity->value(); }
int     SettingsDialog::tabPosition()        const { return m_tabPosition->currentIndex(); }

// set
void SettingsDialog::setClickMethodIndex(int i)    { m_clickMethod->setCurrentIndex(i); }
void SettingsDialog::setHotkeyText(const QString &s){ m_hotkey->setText(s); }
void SettingsDialog::setHideHotkeyText(const QString &s){ m_hideHotkey->setText(s); }
void SettingsDialog::setStartMinimized(bool v)     { m_startMinimized->setChecked(v); }
void SettingsDialog::setShowInTray(bool v)         { m_tray->setChecked(v); }
void SettingsDialog::setClickSound(bool v)         { m_sound->setChecked(v); }
void SettingsDialog::setClickSoundFile(const QString &s){
    if (!s.isEmpty()) { m_soundPath->setText(s); }
}
void SettingsDialog::setPerformanceMode(bool v)    { m_perf->setChecked(v); }
void SettingsDialog::setAlwaysOnTop(bool v)        { m_alwaysOnTop->setChecked(v); }
void SettingsDialog::setPauseOnMouseMove(bool v)   { m_pauseOnMove->setChecked(v); }
void SettingsDialog::setStopAtScreenEdge(bool v)   { m_stopAtEdge->setChecked(v); }
void SettingsDialog::setShowCounterInTitle(bool v) { m_titleCounter->setChecked(v); }
void SettingsDialog::setFocusDelayMs(int v)        { m_focusDelay->setValue(v); }
void SettingsDialog::setWindowOpacity(int v)       { m_opacity->setValue(v); }
void SettingsDialog::setTabPosition(int v)         { m_tabPosition->setCurrentIndex(v); }
