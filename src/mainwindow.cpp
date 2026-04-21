#include "mainwindow.h"
#include "windowpicker.h"
#include "themeeditor.h"
#include "positionpicker.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QFrame>
#include <QMouseEvent>
#include <QCloseEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QGroupBox>
#include <QButtonGroup>
#include <QScrollArea>
#include <QPainter>
#include <QTimer>
#include <QScrollBar>
#include <QApplication>
#include <QJsonDocument>
#include <QJsonObject>

#ifdef _WIN32
#include <windows.h>
#endif

MainWindow::MainWindow(ThemeManager *tm, QWidget *parent)
    : QMainWindow(parent), m_themeManager(tm)
{
    // frameless but keep WS_MINIMIZEBOX so taskbar works
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window |
                   Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint);
    setAttribute(Qt::WA_TranslucentBackground, false);
    resize(960, 640);
    setWindowTitle("XDDCC AutoClicker");

    m_engine = new ClickEngine(this);
    connect(m_engine, &ClickEngine::started,          this, &MainWindow::onStarted);
    connect(m_engine, &ClickEngine::stopped,          this, &MainWindow::onStopped);
    connect(m_engine, &ClickEngine::clickPerformed,   this, &MainWindow::onClick);
    connect(m_engine, &ClickEngine::statsUpdated,     this, &MainWindow::onStatsUpdated);
    connect(m_engine, &ClickEngine::timeLimitReached, this, [this]{
        QMessageBox::information(this, "Time Limit", "Click time limit reached.");
    });

    // set up tray icon (shown when hidden)
    m_trayIcon = new QSystemTrayIcon(QApplication::windowIcon(), this);
    m_trayIcon->setToolTip("XDDCC AutoClicker");
    auto *trayMenu = new QMenu(this);
    trayMenu->addAction("Show", this, &MainWindow::toggleHideMode);
    trayMenu->addSeparator();
    trayMenu->addAction("Quit", qApp, &QApplication::quit);
    m_trayIcon->setContextMenu(trayMenu);
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason r){
        if (r == QSystemTrayIcon::Trigger) toggleHideMode();
    });

    buildUi();

    connect(m_themeManager, &ThemeManager::themeChanged, this, &MainWindow::onThemeChanged);
    onThemeChanged(m_themeManager->currentTheme());

    loadSettings();
    pushEngineSettings();
    registerHotkey();
    registerHideHotkey();
}

MainWindow::~MainWindow()
{
    unregisterHotkey();
    unregisterHideHotkey();
}

void MainWindow::buildUi()
{
    // starburst background layer
    m_starburst = new StarburstWidget(this);
    m_starburst->setEnabled(false);
    m_starburst->lower();
    m_starburst->hide();

    // particle overlay layer
    m_particles = new ParticleWidget(this);
    m_particles->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_particles->hide();

    m_outerWidget = new QWidget(this);
    m_outerWidget->setObjectName("centralRoot");
    m_outerWidget->setAttribute(Qt::WA_TranslucentBackground, true);
    setCentralWidget(m_outerWidget);

    m_outerLayout = new QVBoxLayout(m_outerWidget);
    m_outerLayout->setContentsMargins(0, 0, 0, 0);
    m_outerLayout->setSpacing(0);

    m_header = buildHeader();
    m_outerLayout->addWidget(m_header);

    // body: tab bar + stack (for top position, tab bar is above stack)
    m_bodyWidget = new QWidget();
    m_bodyLayout = new QHBoxLayout(m_bodyWidget);
    m_bodyLayout->setContentsMargins(0, 0, 0, 0);
    m_bodyLayout->setSpacing(0);

    m_tabBarWidget = buildTabBar();

    m_stack = new QStackedWidget();
    m_stack->addWidget(buildMainPage());
    m_settings = new SettingsDialog();
    connect(m_settings, &SettingsDialog::settingsChanged,      this, &MainWindow::onSettingsChanged);
    connect(m_settings, &SettingsDialog::exportConfigRequested, this, &MainWindow::exportConfig);
    connect(m_settings, &SettingsDialog::importConfigRequested, this, &MainWindow::importConfig);
    m_stack->addWidget(m_settings);
    m_stack->addWidget(buildThemesPage());
    m_stack->addWidget(buildAboutPage());

    m_bodyLayout->addWidget(m_stack, 1);
    m_outerLayout->addWidget(m_tabBarWidget);

    // separator under tab bar
    auto *sep = new QFrame();
    sep->setObjectName("separator");
    sep->setFixedHeight(1);
    m_outerLayout->addWidget(sep);

    m_outerLayout->addWidget(m_bodyWidget, 1);

    m_bottomBar = buildBottomBar();
    m_outerLayout->addWidget(m_bottomBar);
}

// --- Header ---

QWidget* MainWindow::buildHeader()
{
    auto *w = new QWidget();
    w->setObjectName("header");
    w->setFixedHeight(44);
    // note: caller assigns return value to m_header
    auto *l = new QHBoxLayout(w);
    l->setContentsMargins(16, 0, 8, 0);
    l->setSpacing(8);

    auto *logo = new QLabel("XDDCC");
    logo->setObjectName("headerLogo");
    logo->setStyleSheet("font-weight: 800; font-size: 15px; background: transparent;");
    auto *slash = new QLabel("/");
    slash->setObjectName("muted");
    slash->setStyleSheet("background: transparent; color: inherit;");
    auto *sub = new QLabel("AutoClicker");
    sub->setObjectName("muted");
    sub->setStyleSheet("background: transparent;");

    l->addWidget(logo);
    l->addWidget(slash);
    l->addWidget(sub);
    l->addStretch(1);

    QFont tbFont("Segoe UI", 11);

    m_minBtn = new QPushButton(QString::fromUtf8("\xe2\x80\x94"));
    m_minBtn->setObjectName("titleBtn");
    m_minBtn->setFixedSize(36, 28);
    m_minBtn->setFont(tbFont);
    connect(m_minBtn, &QPushButton::clicked, this, &QMainWindow::showMinimized);

    m_closeBtn = new QPushButton(QString::fromUtf8("\xc3\x97")); // × U+00D7
    m_closeBtn->setObjectName("closeBtn");
    m_closeBtn->setFixedSize(36, 28);
    m_closeBtn->setFont(tbFont);
    connect(m_closeBtn, &QPushButton::clicked, this, &QMainWindow::close);

    l->addWidget(m_minBtn);
    l->addWidget(m_closeBtn);
    return w;
}

// --- Tab bar ---

QWidget* MainWindow::buildTabBar()
{
    auto *w = new QWidget();
    w->setObjectName("tabBar");
    w->setFixedHeight(40);

    auto *l = new QHBoxLayout(w);
    l->setContentsMargins(12, 0, 12, 0);
    l->setSpacing(2);

    QStringList labels = { "Main", "Settings", "Themes", "About" };
    for (int i = 0; i < labels.size(); ++i) {
        auto *btn = new QPushButton(labels[i]);
        btn->setObjectName("tabBtn");
        btn->setCheckable(true);
        btn->setMinimumWidth(80);
        btn->setFixedHeight(38);
        if (i == 0) btn->setChecked(true);
        connect(btn, &QPushButton::clicked, this, [this, i]{ onNavClicked(i); });
        m_navButtons.append(btn);
        l->addWidget(btn);
    }
    l->addStretch(1);

    // sliding underline indicator
    m_tabIndicator = new QWidget(w);
    m_tabIndicator->setObjectName("tabIndicator");
    m_tabIndicator->setFixedHeight(2);
    m_tabIndicator->raise();

    m_tabIndAnim = new QPropertyAnimation(m_tabIndicator, "geometry", this);
    m_tabIndAnim->setDuration(200);
    m_tabIndAnim->setEasingCurve(QEasingCurve::OutCubic);

    QTimer::singleShot(0, this, [this]{ updateTabIndicator(0, false); });
    return w;
}

// --- Main page ---

QWidget* MainWindow::buildMainPage()
{
    auto *scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    auto *page = new QWidget();
    scroll->setWidget(page);

    auto *root = new QVBoxLayout(page);
    root->setContentsMargins(24, 20, 24, 20);
    root->setSpacing(14);

    // top card — start button + counter
    auto *topCard = new QFrame();
    topCard->setObjectName("card");
    auto *topRow = new QHBoxLayout(topCard);
    topRow->setContentsMargins(24, 20, 24, 20);
    topRow->setSpacing(24);

    auto *counterBox = new QVBoxLayout();
    counterBox->setSpacing(2);
    auto *cLbl = new QLabel("Total Clicks");
    cLbl->setObjectName("muted"); cLbl->setStyleSheet("background: transparent;");
    m_counter = new QLabel("0");
    m_counter->setObjectName("counter"); m_counter->setStyleSheet("background: transparent;");
    counterBox->addWidget(cLbl);
    counterBox->addWidget(m_counter);
    topRow->addLayout(counterBox, 1);

    m_startBtn = new QPushButton("START");
    m_startBtn->setObjectName("startBtn");
    m_startBtn->setMinimumSize(110, 44);
    m_startBtn->setMaximumSize(160, 52);
    connect(m_startBtn, &QPushButton::clicked, this, &MainWindow::toggleClicking);
    topRow->addWidget(m_startBtn);
    root->addWidget(topCard);

    // config card
    auto *cfg = new QFrame();
    cfg->setObjectName("card");
    auto *cfgLayout = new QGridLayout(cfg);
    cfgLayout->setContentsMargins(24, 20, 24, 20);
    cfgLayout->setSpacing(12);
    cfgLayout->setColumnStretch(2, 1);

    auto makeLbl = [](const QString &text) -> QLabel* {
        auto *l = new QLabel(text);
        l->setStyleSheet("background: transparent;");
        return l;
    };

    // interval
    cfgLayout->addWidget(makeLbl("Interval (ms)"), 0, 0);
    m_intervalSpin = new QSpinBox();
    m_intervalSpin->setRange(1, 60000); m_intervalSpin->setValue(100); m_intervalSpin->setFixedWidth(90);
    m_intervalSlider = new QSlider(Qt::Horizontal);
    m_intervalSlider->setRange(1, 2000); m_intervalSlider->setValue(100);
    connect(m_intervalSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int v){
        m_intervalSlider->blockSignals(true);
        m_intervalSlider->setValue(qMin(v, m_intervalSlider->maximum()));
        m_intervalSlider->blockSignals(false);
        pushEngineSettings();
    });
    connect(m_intervalSlider, &QSlider::valueChanged, this, [this](int v){
        m_intervalSpin->blockSignals(true); m_intervalSpin->setValue(v); m_intervalSpin->blockSignals(false);
        pushEngineSettings();
    });
    cfgLayout->addWidget(m_intervalSpin, 0, 1);
    cfgLayout->addWidget(m_intervalSlider, 0, 2);

    // randomize
    m_randomize = new QCheckBox("Randomize interval  ±");
    m_randomize->setStyleSheet("background: transparent;");
    m_jitterSpin = new QSpinBox();
    m_jitterSpin->setRange(0, 5000); m_jitterSpin->setValue(10);
    m_jitterSpin->setSuffix(" ms"); m_jitterSpin->setFixedWidth(90);
    connect(m_randomize, &QCheckBox::toggled, this, [this]{ pushEngineSettings(); });
    connect(m_jitterSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]{ pushEngineSettings(); });
    cfgLayout->addWidget(m_randomize, 1, 0);
    cfgLayout->addWidget(m_jitterSpin, 1, 1);

    // mouse button
    cfgLayout->addWidget(makeLbl("Mouse button"), 2, 0);
    {
        auto *row = new QHBoxLayout(); row->setSpacing(12);
        m_btnLeft = new QRadioButton("Left"); m_btnRight = new QRadioButton("Right"); m_btnMiddle = new QRadioButton("Middle");
        m_btnLeft->setStyleSheet("background:transparent;"); m_btnRight->setStyleSheet("background:transparent;"); m_btnMiddle->setStyleSheet("background:transparent;");
        m_btnLeft->setChecked(true);
        auto *bg = new QButtonGroup(this); bg->addButton(m_btnLeft); bg->addButton(m_btnRight); bg->addButton(m_btnMiddle);
        row->addWidget(m_btnLeft); row->addWidget(m_btnRight); row->addWidget(m_btnMiddle); row->addStretch(1);
        auto *rw = new QWidget(); rw->setLayout(row); rw->setStyleSheet("background:transparent;");
        cfgLayout->addWidget(rw, 2, 1, 1, 2);
        connect(m_btnLeft, &QRadioButton::toggled, this, [this]{ pushEngineSettings(); });
        connect(m_btnRight, &QRadioButton::toggled, this, [this]{ pushEngineSettings(); });
        connect(m_btnMiddle, &QRadioButton::toggled, this, [this]{ pushEngineSettings(); });
    }

    // click mode
    cfgLayout->addWidget(makeLbl("Click mode"), 3, 0);
    {
        auto *row = new QHBoxLayout(); row->setSpacing(12);
        m_modeSingle = new QRadioButton("Single"); m_modeDouble = new QRadioButton("Double"); m_modeHold = new QRadioButton("Hold");
        m_modeSingle->setStyleSheet("background:transparent;"); m_modeDouble->setStyleSheet("background:transparent;"); m_modeHold->setStyleSheet("background:transparent;");
        m_modeSingle->setChecked(true);
        auto *bg = new QButtonGroup(this); bg->addButton(m_modeSingle); bg->addButton(m_modeDouble); bg->addButton(m_modeHold);
        row->addWidget(m_modeSingle); row->addWidget(m_modeDouble); row->addWidget(m_modeHold); row->addStretch(1);
        auto *rw = new QWidget(); rw->setLayout(row); rw->setStyleSheet("background:transparent;");
        cfgLayout->addWidget(rw, 3, 1, 1, 2);
        connect(m_modeSingle, &QRadioButton::toggled, this, [this]{ pushEngineSettings(); });
        connect(m_modeDouble, &QRadioButton::toggled, this, [this]{ pushEngineSettings(); });
        connect(m_modeHold, &QRadioButton::toggled, this, [this]{ pushEngineSettings(); });
    }

    // click position with screen picker
    cfgLayout->addWidget(makeLbl("Click position"), 4, 0);
    {
        auto *row = new QHBoxLayout(); row->setSpacing(8);
        m_posCursor = new QRadioButton("Cursor"); m_posFixed = new QRadioButton("Fixed XY");
        m_posCursor->setStyleSheet("background:transparent;"); m_posFixed->setStyleSheet("background:transparent;");
        m_posCursor->setChecked(true);
        auto *bg = new QButtonGroup(this); bg->addButton(m_posCursor); bg->addButton(m_posFixed);
        m_fixedX = new QSpinBox(); m_fixedX->setRange(-20000, 20000); m_fixedX->setPrefix("X: "); m_fixedX->setFixedWidth(80);
        m_fixedY = new QSpinBox(); m_fixedY->setRange(-20000, 20000); m_fixedY->setPrefix("Y: "); m_fixedY->setFixedWidth(80);
        auto *pickBtn = new QPushButton("Pick on screen");
        row->addWidget(m_posCursor); row->addWidget(m_posFixed);
        row->addWidget(m_fixedX); row->addWidget(m_fixedY);
        row->addWidget(pickBtn); row->addStretch(1);
        auto *rw = new QWidget(); rw->setLayout(row); rw->setStyleSheet("background:transparent;");
        cfgLayout->addWidget(rw, 4, 1, 1, 2);
        connect(m_posCursor, &QRadioButton::toggled, this, [this]{ pushEngineSettings(); });
        connect(m_posFixed, &QRadioButton::toggled, this, [this]{ pushEngineSettings(); });
        connect(m_fixedX, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]{ pushEngineSettings(); });
        connect(m_fixedY, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]{ pushEngineSettings(); });
        connect(pickBtn, &QPushButton::clicked, this, [this]{
            PositionPicker picker(this);
            if (picker.exec() == QDialog::Accepted) {
                m_posFixed->setChecked(true);
                m_fixedX->setValue(picker.pickedPos().x());
                m_fixedY->setValue(picker.pickedPos().y());
                pushEngineSettings();
            }
        });
    }

    // max clicks
    cfgLayout->addWidget(makeLbl("Max clicks  (0 = unlimited)"), 5, 0);
    m_maxClicks = new QSpinBox();
    m_maxClicks->setRange(0, 10000000); m_maxClicks->setFixedWidth(110);
    connect(m_maxClicks, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]{ pushEngineSettings(); });
    cfgLayout->addWidget(m_maxClicks, 5, 1);

    // time limit
    cfgLayout->addWidget(makeLbl("Time limit  (0 = unlimited)"), 6, 0);
    m_timeLimitSpin = new QSpinBox();
    m_timeLimitSpin->setRange(0, 86400); m_timeLimitSpin->setSuffix(" s"); m_timeLimitSpin->setFixedWidth(110);
    connect(m_timeLimitSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]{ pushEngineSettings(); });
    cfgLayout->addWidget(m_timeLimitSpin, 6, 1);

    // hotkey display
    cfgLayout->addWidget(makeLbl("Hotkey"), 7, 0);
    m_hotkeyLabel = new QLabel("F6  (start/stop)");
    m_hotkeyLabel->setObjectName("muted"); m_hotkeyLabel->setStyleSheet("background: transparent;");
    cfgLayout->addWidget(m_hotkeyLabel, 7, 1);

    // target window
    cfgLayout->addWidget(makeLbl("Target window"), 8, 0);
    {
        auto *row = new QHBoxLayout(); row->setSpacing(8);
        m_pickWindowBtn = new QPushButton("Pick Window...");
        m_targetLabel = new QLabel("Any window");
        m_targetLabel->setObjectName("muted"); m_targetLabel->setStyleSheet("background: transparent;");
        row->addWidget(m_pickWindowBtn); row->addWidget(m_targetLabel, 1);
        auto *rw = new QWidget(); rw->setLayout(row); rw->setStyleSheet("background:transparent;");
        cfgLayout->addWidget(rw, 8, 1, 1, 2);
        connect(m_pickWindowBtn, &QPushButton::clicked, this, &MainWindow::openWindowPicker);
    }

    root->addWidget(cfg);
    root->addStretch(1);
    return scroll;
}

// --- Themes page ---

QWidget* MainWindow::buildThemesPage()
{
    auto *page = new QWidget();
    auto *root = new QVBoxLayout(page);
    root->setContentsMargins(24, 20, 24, 20);
    root->setSpacing(14);

    auto *title = new QLabel("Themes");
    title->setObjectName("heading"); title->setStyleSheet("background: transparent;");
    root->addWidget(title);

    auto *btnRow = new QHBoxLayout();
    m_importThemeBtn = new QPushButton("Import .xtheme");
    m_exportThemeBtn = new QPushButton("Export current");
    auto *createBtn  = new QPushButton("Create Custom");
    btnRow->addWidget(m_importThemeBtn); btnRow->addWidget(m_exportThemeBtn); btnRow->addWidget(createBtn); btnRow->addStretch(1);
    root->addLayout(btnRow);

    connect(m_importThemeBtn, &QPushButton::clicked, this, &MainWindow::importTheme);
    connect(m_exportThemeBtn, &QPushButton::clicked, this, &MainWindow::exportTheme);
    connect(createBtn, &QPushButton::clicked, this, [this]{
        ThemeEditorDialog editor(m_themeManager, this);
        if (editor.exec() == QDialog::Accepted) {
            m_themeManager->applyTheme(editor.savedThemeName());
            refreshThemeGrid();
        }
    });

    auto *scroll = new QScrollArea();
    scroll->setWidgetResizable(true); scroll->setFrameShape(QFrame::NoFrame);
    m_themeGrid = new QWidget();
    scroll->setWidget(m_themeGrid);
    root->addWidget(scroll, 1);

    refreshThemeGrid();
    return page;
}

void MainWindow::refreshThemeGrid()
{
    if (m_themeGrid->layout()) {
        QLayoutItem *item;
        while ((item = m_themeGrid->layout()->takeAt(0)) != nullptr) {
            if (item->widget()) item->widget()->deleteLater();
            delete item;
        }
        delete m_themeGrid->layout();
    }

    auto *grid = new QGridLayout(m_themeGrid);
    grid->setContentsMargins(4, 4, 4, 4); grid->setSpacing(12);

    auto names = m_themeManager->themeNames();
    int col = 0, row = 0;
    for (const QString &name : names) {
        Theme t  = m_themeManager->theme(name);
        bool active = (name == m_themeManager->currentThemeName());
        auto *card = new QWidget();
        card->setObjectName(active ? "themeCardActive" : "themeCard");
        card->setMinimumSize(220, 140);
        auto *cl = new QVBoxLayout(card);
        cl->setContentsMargins(14, 14, 14, 14); cl->setSpacing(6);

        auto *nameLbl = new QLabel(t.name);
        nameLbl->setStyleSheet("font-weight: 700; font-size: 14px; background: transparent;");
        cl->addWidget(nameLbl);

        auto *authLbl = new QLabel(QString("by %1").arg(t.author));
        authLbl->setObjectName("muted"); authLbl->setStyleSheet("background: transparent;");
        cl->addWidget(authLbl);

        auto *sw = new QHBoxLayout(); sw->setSpacing(4);
        for (const QString &k : QStringList{"background","surface","primary","accent","success","danger"}) {
            auto *s = new QLabel();
            s->setFixedSize(20, 20);
            s->setStyleSheet(QString("background: %1; border-radius: 3px; border: 1px solid rgba(128,128,128,0.3);")
                             .arg(t.colors.value(k, "#000000")));
            sw->addWidget(s);
        }
        sw->addStretch(1); cl->addLayout(sw);

        auto *apply = new QPushButton(active ? "Active" : "Apply");
        apply->setEnabled(!active);
        connect(apply, &QPushButton::clicked, this, [this, name]{
            m_themeManager->applyTheme(name);
            refreshThemeGrid();
        });
        cl->addWidget(apply);

        grid->addWidget(card, row, col);
        if (++col >= 3) { col = 0; row++; }
    }
    grid->setRowStretch(row + 1, 1);
    grid->setColumnStretch(3, 1);
}

// --- About page ---

QWidget* MainWindow::buildAboutPage()
{
    auto *page = new QWidget();
    auto *root = new QVBoxLayout(page);
    root->setContentsMargins(24, 20, 24, 20); root->setSpacing(14);

    auto *title = new QLabel("About");
    title->setObjectName("heading"); title->setStyleSheet("background: transparent;");
    root->addWidget(title);

    auto *card = new QFrame();
    card->setObjectName("card");
    auto *cl = new QVBoxLayout(card);
    cl->setContentsMargins(24, 24, 24, 24); cl->setSpacing(6);

    auto *appName = new QLabel("XDDCC AutoClicker");
    appName->setStyleSheet("font-size: 20px; font-weight: 800; background: transparent;");
    auto *ver  = new QLabel("Version 1.0");
    ver->setObjectName("muted"); ver->setStyleSheet("background: transparent;");
    auto *auth = new QLabel("Author: Pomni");
    auth->setObjectName("muted"); auth->setStyleSheet("background: transparent;");
    auto *desc = new QLabel("A simple clean autoclicker");
    desc->setWordWrap(true); desc->setStyleSheet("background: transparent;");

    cl->addWidget(appName); cl->addWidget(ver); cl->addWidget(auth);
    cl->addSpacing(8); cl->addWidget(desc); cl->addStretch(1);

    root->addWidget(card); root->addStretch(1);
    return page;
}

// --- Bottom bar ---

QWidget* MainWindow::buildBottomBar()
{
    auto *bar = new QWidget();
    bar->setObjectName("statusBar");
    bar->setFixedHeight(30);
    auto *l = new QHBoxLayout(bar);
    l->setContentsMargins(14, 0, 14, 0); l->setSpacing(0);

    m_activeIndicator = new QLabel("RUNNING");
    m_activeIndicator->setObjectName("activeIndicator");
    m_activeIndicator->setStyleSheet("color: #10b981; font-weight: 700; font-size: 11px; background: transparent;");
    m_activeIndicator->setVisible(false);

    m_statusText  = new QLabel("Idle");  m_statusText->setObjectName("muted");
    m_statusText->setStyleSheet("background: transparent; font-size: 12px;");
    m_cpsLabel    = new QLabel("0.0 CPS"); m_cpsLabel->setObjectName("muted");
    m_cpsLabel->setStyleSheet("background: transparent; font-size: 12px;");
    m_elapsedLabel = new QLabel("00:00"); m_elapsedLabel->setObjectName("muted");
    m_elapsedLabel->setStyleSheet("background: transparent; font-size: 12px;");

    auto mkSep = []{ auto *s = new QLabel("|"); s->setObjectName("muted"); s->setStyleSheet("background:transparent; padding: 0 8px; font-size:12px;"); return s; };

    l->addWidget(m_activeIndicator); l->addSpacing(10); l->addWidget(m_statusText);
    l->addStretch(1);
    l->addWidget(m_cpsLabel); l->addWidget(mkSep()); l->addWidget(m_elapsedLabel); l->addWidget(mkSep());
    auto *hkLbl = new QLabel("F6 toggle"); hkLbl->setObjectName("muted"); hkLbl->setStyleSheet("background:transparent; font-size:12px;");
    l->addWidget(hkLbl);
    return bar;
}

// --- Nav / slide transition ---

void MainWindow::updateTabIndicator(int idx, bool animate)
{
    if (!m_tabIndicator || idx < 0 || idx >= m_navButtons.size()) return;
    QPushButton *btn = m_navButtons[idx];
    QPoint btnPos = btn->mapTo(m_tabBarWidget, QPoint(0, 0));
    QRect target(btnPos.x(), m_tabBarWidget->height() - 2, btn->width(), 2);

    if (animate && m_tabIndAnim) {
        m_tabIndAnim->stop();
        m_tabIndAnim->setStartValue(m_tabIndicator->geometry());
        m_tabIndAnim->setEndValue(target);
        m_tabIndAnim->start();
    } else {
        m_tabIndicator->setGeometry(target);
    }
}

void MainWindow::onNavClicked(int idx)
{
    int from = m_stack->currentIndex();
    if (from == idx || m_sliding) return;

    for (int i = 0; i < m_navButtons.size(); ++i)
        m_navButtons[i]->setChecked(i == idx);
    updateTabIndicator(idx, true);

    // slide via pixmap overlay
    QPixmap fromPix = m_stack->currentWidget()->grab();
    m_stack->setCurrentIndex(idx);
    QPixmap toPix = m_stack->currentWidget()->grab();

    int dir = (idx > from) ? 1 : -1;
    auto *overlay = new QWidget(m_stack);
    overlay->setGeometry(m_stack->rect());
    overlay->raise(); overlay->show();

    auto *fromLbl = new QLabel(overlay);
    fromLbl->setPixmap(fromPix);
    fromLbl->setGeometry(0, 0, overlay->width(), overlay->height());

    auto *toLbl = new QLabel(overlay);
    toLbl->setPixmap(toPix);
    toLbl->setGeometry(dir * overlay->width(), 0, overlay->width(), overlay->height());

    auto *outAnim = new QPropertyAnimation(fromLbl, "pos", overlay);
    outAnim->setDuration(200); outAnim->setEndValue(QPoint(-dir * overlay->width(), 0));
    outAnim->setEasingCurve(QEasingCurve::OutCubic);

    auto *inAnim = new QPropertyAnimation(toLbl, "pos", overlay);
    inAnim->setDuration(200); inAnim->setEndValue(QPoint(0, 0));
    inAnim->setEasingCurve(QEasingCurve::OutCubic);

    auto *group = new QParallelAnimationGroup(overlay);
    group->addAnimation(outAnim); group->addAnimation(inAnim);

    m_sliding = true;
    connect(group, &QParallelAnimationGroup::finished, this, [this, overlay]{
        overlay->deleteLater(); m_sliding = false;
    });
    group->start();
}

// --- Engine ---

void MainWindow::toggleClicking()
{
    if (m_engine->isRunning()) m_engine->stop();
    else { pushEngineSettings(); m_engine->start(); }
}

void MainWindow::pushEngineSettings()
{
    m_engine->setInterval(m_intervalSpin->value());
    m_engine->setRandomize(m_randomize->isChecked(), m_jitterSpin->value());

    ClickButton b = ClickButton::Left;
    if (m_btnRight->isChecked())  b = ClickButton::Right;
    if (m_btnMiddle->isChecked()) b = ClickButton::Middle;
    m_engine->setClickButton(b);

    ClickMode m = ClickMode::Single;
    if (m_modeDouble->isChecked()) m = ClickMode::Double;
    if (m_modeHold->isChecked())   m = ClickMode::Hold;
    m_engine->setClickMode(m);

    ClickPosition p = m_posFixed->isChecked() ? ClickPosition::Fixed : ClickPosition::Cursor;
    m_engine->setClickPosition(p, QPoint(m_fixedX->value(), m_fixedY->value()));

    int methodIdx = m_settings ? m_settings->clickMethodIndex() : 0;
    ClickMethod method = ClickMethod::SendInput;
    if (methodIdx == 1) method = ClickMethod::MouseEvent;
    if (methodIdx == 2) method = ClickMethod::PostMessage;
    m_engine->setClickMethod(method);

    m_engine->setMaxClicks(m_maxClicks->value());
    m_engine->setTimeLimit(m_timeLimitSpin->value());
    m_engine->setTargetWindow(m_targetHwnd);

    if (m_settings) {
        m_engine->setSoundEnabled(m_settings->clickSound());
        m_engine->setSoundFile(m_settings->clickSoundFile());
        m_engine->setPauseOnMouseMove(m_settings->pauseOnMouseMove());
        m_engine->setStopAtEdge(m_settings->stopAtScreenEdge());
        m_engine->setFocusDelay(m_settings->focusDelayMs());
    }
}

// --- Signals ---

void MainWindow::onStarted()
{
    m_startBtn->setText("STOP");
    m_startBtn->setProperty("active", true);
    m_startBtn->style()->unpolish(m_startBtn);
    m_startBtn->style()->polish(m_startBtn);
    m_statusText->setText("Running");
    m_activeIndicator->setVisible(true);
}

void MainWindow::onStopped()
{
    m_startBtn->setText("START");
    m_startBtn->setProperty("active", false);
    m_startBtn->style()->unpolish(m_startBtn);
    m_startBtn->style()->polish(m_startBtn);
    m_statusText->setText("Idle");
    m_activeIndicator->setVisible(false);
}

void MainWindow::onClick(int total)
{
    m_counter->setText(QString::number(total));
    if (m_settings && m_settings->showCounterInTitle())
        setWindowTitle(QString("XDDCC AutoClicker — %1 clicks").arg(total));
}

void MainWindow::onStatsUpdated(double cps, qint64 elapsedMs)
{
    m_cpsLabel->setText(QString("%1 CPS").arg(cps, 0, 'f', 1));
    qint64 secs = elapsedMs / 1000;
    m_elapsedLabel->setText(QString("%1:%2")
        .arg(secs / 60, 2, 10, QChar('0'))
        .arg(secs % 60, 2, 10, QChar('0')));
}

void MainWindow::openWindowPicker()
{
    WindowPicker picker(this);
    if (picker.exec() == QDialog::Accepted) {
        auto info = picker.selectedWindow();
        m_targetHwnd = info.hwnd;
        m_targetLabel->setText(QString("%1  (%2)").arg(info.title, info.processName));
        pushEngineSettings();
    }
}

void MainWindow::onThemeChanged(const Theme &t)
{
    applyStarburst(t);
    applyParticles(t);
    refreshThemeGrid();
}

void MainWindow::applyStarburst(const Theme &t)
{
    if (!m_starburst) return;
    if (t.starburst) {
        m_starburst->setColors(QColor(t.colors.value("primary","#ff2222")),
                               QColor(t.colors.value("accent","#3388ff")));
        m_starburst->setEnabled(true);
        m_starburst->setGeometry(0, 0, width(), height());
        m_starburst->show(); m_starburst->raise();
        centralWidget()->raise();
    } else {
        m_starburst->setEnabled(false);
        m_starburst->hide();
    }
}

void MainWindow::applyParticles(const Theme &t)
{
    if (!m_particles) return;
    QJsonObject cfg = t.particleConfig;
    if (!cfg.isEmpty() && cfg.value("enabled").toBool(false)) {
        m_particles->configure(cfg);
        m_particles->setGeometry(0, 0, width(), height());
        m_particles->show();
        m_particles->raise();
        centralWidget()->raise();
        m_particles->setActive(true);
    } else {
        m_particles->setActive(false);
        m_particles->hide();
    }
}

void MainWindow::importTheme()
{
    QString file = QFileDialog::getOpenFileName(this, "Import Theme", {}, "XTheme (*.xtheme *.json)");
    if (file.isEmpty()) return;
    if (m_themeManager->loadThemeFromFile(file)) { refreshThemeGrid(); QMessageBox::information(this, "Import", "Theme imported."); }
    else QMessageBox::warning(this, "Import", "Failed to load theme file.");
}

void MainWindow::exportTheme()
{
    Theme t = m_themeManager->currentTheme();
    QString file = QFileDialog::getSaveFileName(this, "Export Theme", t.name + ".xtheme", "XTheme (*.xtheme)");
    if (!file.isEmpty() && !m_themeManager->saveThemeToFile(t, file))
        QMessageBox::warning(this, "Export", "Failed to save theme.");
}

// --- Settings changes ---

void MainWindow::onSettingsChanged()
{
    m_hotkeyStr     = m_settings->hotkeyText();
    m_hideHotkeyStr = m_settings->hideHotkeyText();
    m_hotkeyLabel->setText(QString("%1  (start/stop)").arg(m_hotkeyStr));

    unregisterHotkey(); registerHotkey();
    unregisterHideHotkey(); registerHideHotkey();

    // always on top
    bool aot = m_settings->alwaysOnTop();
    setWindowFlag(Qt::WindowStaysOnTopHint, aot);
    show();

    // window opacity
    setWindowOpacity(m_settings->windowOpacity() / 100.0);

    // tab position
    applyTabPosition(m_settings->tabPosition());

    pushEngineSettings();
}

void MainWindow::applyTabPosition(int pos)
{
    // pos 0 = top (default, already built), 1 = left, 2 = right
    // For simplicity, just move the tab bar widget in the layout.
    // Remove tabBar from outerLayout and re-add in correct position.
    if (!m_tabBarWidget || !m_bodyWidget || !m_bodyLayout) return;

    // clear the body layout
    while (m_bodyLayout->count())
        m_bodyLayout->takeAt(0)->widget();

    m_outerLayout->removeWidget(m_tabBarWidget);

    if (pos == 0) {
        // top: tabBar in outerLayout above bodyWidget
        m_tabBarWidget->setFixedHeight(40);
        m_tabBarWidget->setMaximumWidth(QWIDGETSIZE_MAX);
        m_outerLayout->insertWidget(1, m_tabBarWidget);
        m_bodyLayout->addWidget(m_stack, 1);
    } else if (pos == 1) {
        // left: tabBar on left side in bodyLayout
        m_tabBarWidget->setFixedWidth(120);
        m_tabBarWidget->setMaximumHeight(QWIDGETSIZE_MAX);
        m_tabBarWidget->setFixedHeight(QWIDGETSIZE_MAX);
        m_bodyLayout->addWidget(m_tabBarWidget);
        m_bodyLayout->addWidget(m_stack, 1);
    } else {
        // right
        m_tabBarWidget->setFixedWidth(120);
        m_bodyLayout->addWidget(m_stack, 1);
        m_bodyLayout->addWidget(m_tabBarWidget);
    }
}

// --- Settings persistence ---

void MainWindow::saveSettings()
{
    m_settings_store.setValue("theme",        m_themeManager->currentThemeName());
    m_settings_store.setValue("interval",     m_intervalSpin->value());
    m_settings_store.setValue("jitter",       m_jitterSpin->value());
    m_settings_store.setValue("randomize",    m_randomize->isChecked());
    m_settings_store.setValue("btnLeft",      m_btnLeft->isChecked());
    m_settings_store.setValue("btnRight",     m_btnRight->isChecked());
    m_settings_store.setValue("btnMiddle",    m_btnMiddle->isChecked());
    m_settings_store.setValue("modeSingle",   m_modeSingle->isChecked());
    m_settings_store.setValue("modeDouble",   m_modeDouble->isChecked());
    m_settings_store.setValue("modeHold",     m_modeHold->isChecked());
    m_settings_store.setValue("posCursor",    m_posCursor->isChecked());
    m_settings_store.setValue("fixedX",       m_fixedX->value());
    m_settings_store.setValue("fixedY",       m_fixedY->value());
    m_settings_store.setValue("maxClicks",    m_maxClicks->value());
    m_settings_store.setValue("timeLimit",    m_timeLimitSpin->value());

    if (m_settings) {
        m_settings_store.setValue("clickMethod",    m_settings->clickMethodIndex());
        m_settings_store.setValue("hotkey",         m_settings->hotkeyText());
        m_settings_store.setValue("hideHotkey",     m_settings->hideHotkeyText());
        m_settings_store.setValue("startMinimized", m_settings->startMinimized());
        m_settings_store.setValue("tray",           m_settings->showInTray());
        m_settings_store.setValue("sound",          m_settings->clickSound());
        m_settings_store.setValue("soundFile",      m_settings->clickSoundFile());
        m_settings_store.setValue("perf",           m_settings->performanceMode());
        m_settings_store.setValue("alwaysOnTop",    m_settings->alwaysOnTop());
        m_settings_store.setValue("pauseOnMove",    m_settings->pauseOnMouseMove());
        m_settings_store.setValue("stopAtEdge",     m_settings->stopAtScreenEdge());
        m_settings_store.setValue("titleCounter",   m_settings->showCounterInTitle());
        m_settings_store.setValue("focusDelay",     m_settings->focusDelayMs());
        m_settings_store.setValue("opacity",        m_settings->windowOpacity());
        m_settings_store.setValue("tabPos",         m_settings->tabPosition());
    }
}

void MainWindow::loadSettings()
{
    auto s = [&](const QString &key, const QVariant &def){ return m_settings_store.value(key, def); };

    QString theme = s("theme", "Default").toString();
    m_themeManager->applyTheme(theme);

    m_intervalSpin->setValue(s("interval", 100).toInt());
    m_jitterSpin->setValue(s("jitter", 10).toInt());
    m_randomize->setChecked(s("randomize", false).toBool());

    if (s("btnRight",  false).toBool()) m_btnRight->setChecked(true);
    if (s("btnMiddle", false).toBool()) m_btnMiddle->setChecked(true);
    else                                m_btnLeft->setChecked(true);

    if (s("modeDouble", false).toBool()) m_modeDouble->setChecked(true);
    else if (s("modeHold", false).toBool()) m_modeHold->setChecked(true);
    else                                   m_modeSingle->setChecked(true);

    if (s("posCursor", true).toBool()) m_posCursor->setChecked(true);
    else                               m_posFixed->setChecked(true);

    m_fixedX->setValue(s("fixedX", 0).toInt());
    m_fixedY->setValue(s("fixedY", 0).toInt());
    m_maxClicks->setValue(s("maxClicks", 0).toInt());
    m_timeLimitSpin->setValue(s("timeLimit", 0).toInt());

    if (m_settings) {
        m_settings->setClickMethodIndex(s("clickMethod", 0).toInt());
        m_settings->setHotkeyText(s("hotkey", "F6").toString());
        m_settings->setHideHotkeyText(s("hideHotkey", "F8").toString());
        m_settings->setStartMinimized(s("startMinimized", false).toBool());
        m_settings->setShowInTray(s("tray", false).toBool());
        m_settings->setClickSound(s("sound", false).toBool());
        m_settings->setClickSoundFile(s("soundFile", "").toString());
        m_settings->setPerformanceMode(s("perf", true).toBool());
        m_settings->setAlwaysOnTop(s("alwaysOnTop", false).toBool());
        m_settings->setPauseOnMouseMove(s("pauseOnMove", false).toBool());
        m_settings->setStopAtScreenEdge(s("stopAtEdge", false).toBool());
        m_settings->setShowCounterInTitle(s("titleCounter", false).toBool());
        m_settings->setFocusDelayMs(s("focusDelay", 0).toInt());
        m_settings->setWindowOpacity(s("opacity", 100).toInt());
        m_settings->setTabPosition(s("tabPos", 0).toInt());

        m_hotkeyStr     = m_settings->hotkeyText();
        m_hideHotkeyStr = m_settings->hideHotkeyText();
        m_hotkeyLabel->setText(QString("%1  (start/stop)").arg(m_hotkeyStr));

        setWindowOpacity(m_settings->windowOpacity() / 100.0);
        applyTabPosition(m_settings->tabPosition());
    }
}

// --- Config export/import ---

void MainWindow::exportConfig()
{
    QString file = QFileDialog::getSaveFileName(this, "Export Config", "xddcc_config.json", "JSON (*.json)");
    if (file.isEmpty()) return;

    QJsonObject obj;
    obj["theme"]      = m_themeManager->currentThemeName();
    obj["interval"]   = m_intervalSpin->value();
    obj["jitter"]     = m_jitterSpin->value();
    obj["randomize"]  = m_randomize->isChecked();
    obj["btnType"]    = m_btnRight->isChecked() ? 1 : m_btnMiddle->isChecked() ? 2 : 0;
    obj["clickMode"]  = m_modeDouble->isChecked() ? 1 : m_modeHold->isChecked() ? 2 : 0;
    obj["posMode"]    = m_posFixed->isChecked() ? 1 : 0;
    obj["fixedX"]     = m_fixedX->value();
    obj["fixedY"]     = m_fixedY->value();
    obj["maxClicks"]  = m_maxClicks->value();
    obj["timeLimit"]  = m_timeLimitSpin->value();
    if (m_settings) {
        obj["clickMethod"]  = m_settings->clickMethodIndex();
        obj["hotkey"]       = m_settings->hotkeyText();
        obj["alwaysOnTop"]  = m_settings->alwaysOnTop();
        obj["opacity"]      = m_settings->windowOpacity();
        obj["tabPos"]       = m_settings->tabPosition();
    }

    QFile f(file);
    if (f.open(QIODevice::WriteOnly))
        f.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
}

void MainWindow::importConfig()
{
    QString file = QFileDialog::getOpenFileName(this, "Import Config", {}, "JSON (*.json)");
    if (file.isEmpty()) return;

    QFile f(file);
    if (!f.open(QIODevice::ReadOnly)) { QMessageBox::warning(this, "Import", "Could not open file."); return; }
    QJsonObject obj = QJsonDocument::fromJson(f.readAll()).object();
    if (obj.isEmpty()) { QMessageBox::warning(this, "Import", "Invalid config file."); return; }

    m_themeManager->applyTheme(obj.value("theme").toString("Default"));
    m_intervalSpin->setValue(obj.value("interval").toInt(100));
    m_jitterSpin->setValue(obj.value("jitter").toInt(10));
    m_randomize->setChecked(obj.value("randomize").toBool(false));

    int bt = obj.value("btnType").toInt(0);
    if (bt == 1) m_btnRight->setChecked(true); else if (bt == 2) m_btnMiddle->setChecked(true); else m_btnLeft->setChecked(true);

    int cm = obj.value("clickMode").toInt(0);
    if (cm == 1) m_modeDouble->setChecked(true); else if (cm == 2) m_modeHold->setChecked(true); else m_modeSingle->setChecked(true);

    if (obj.value("posMode").toInt(0) == 1) m_posFixed->setChecked(true); else m_posCursor->setChecked(true);
    m_fixedX->setValue(obj.value("fixedX").toInt(0));
    m_fixedY->setValue(obj.value("fixedY").toInt(0));
    m_maxClicks->setValue(obj.value("maxClicks").toInt(0));
    m_timeLimitSpin->setValue(obj.value("timeLimit").toInt(0));

    if (m_settings) {
        m_settings->setClickMethodIndex(obj.value("clickMethod").toInt(0));
        m_settings->setHotkeyText(obj.value("hotkey").toString("F6"));
        m_settings->setAlwaysOnTop(obj.value("alwaysOnTop").toBool(false));
        m_settings->setWindowOpacity(obj.value("opacity").toInt(100));
        m_settings->setTabPosition(obj.value("tabPos").toInt(0));
    }
    onSettingsChanged();
    pushEngineSettings();
    QMessageBox::information(this, "Import", "Config loaded.");
}

// --- Hide mode ---

void MainWindow::toggleHideMode()
{
    m_hideMode = !m_hideMode;

#ifdef _WIN32
    HWND hwnd = (HWND)winId();
    LONG_PTR ex = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
    if (m_hideMode) {
        ex = (ex | WS_EX_TOOLWINDOW) & ~WS_EX_APPWINDOW;
        SetWindowLongPtr(hwnd, GWL_EXSTYLE, ex);
        setWindowTitle("System");
        ShowWindow(hwnd, SW_HIDE);
        ShowWindow(hwnd, SW_SHOW);
        m_trayIcon->show();
    } else {
        ex = (ex & ~WS_EX_TOOLWINDOW) | WS_EX_APPWINDOW;
        SetWindowLongPtr(hwnd, GWL_EXSTYLE, ex);
        setWindowTitle("XDDCC AutoClicker");
        ShowWindow(hwnd, SW_HIDE);
        ShowWindow(hwnd, SW_SHOW);
        m_trayIcon->hide();
    }
#endif
}

// --- Mouse drag ---

static bool isInteractive(QWidget *w)
{
    return qobject_cast<QPushButton*>(w)
        || qobject_cast<QAbstractSlider*>(w)
        || qobject_cast<QAbstractSpinBox*>(w)
        || qobject_cast<QAbstractButton*>(w)
        || qobject_cast<QLineEdit*>(w)
        || qobject_cast<QComboBox*>(w)
        || qobject_cast<QScrollBar*>(w);
}

void MainWindow::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        QWidget *hit = childAt(e->pos());
        bool onHeader    = hit && (hit == m_header    || hit->parent() == m_header);
        bool onTabBar    = hit && (hit == m_tabBarWidget || hit->parent() == m_tabBarWidget);
        bool onStatusBar = hit && (hit == m_bottomBar || hit->parent() == m_bottomBar);
        if ((onHeader || onTabBar || onStatusBar) && !isInteractive(hit)) {
            m_dragging = true;
            m_dragPos  = e->globalPosition().toPoint() - frameGeometry().topLeft();
            e->accept();
            return;
        }
    }
    QMainWindow::mousePressEvent(e);
}

void MainWindow::mouseMoveEvent(QMouseEvent *e)
{
    if (m_dragging && (e->buttons() & Qt::LeftButton)) {
        move(e->globalPosition().toPoint() - m_dragPos);
        e->accept(); return;
    }
    QMainWindow::mouseMoveEvent(e);
}

void MainWindow::mouseReleaseEvent(QMouseEvent *e)
{
    m_dragging = false;
    QMainWindow::mouseReleaseEvent(e);
}

// --- Resize / show events ---

void MainWindow::resizeEvent(QResizeEvent *e)
{
    if (m_starburst) m_starburst->setGeometry(0, 0, width(), height());
    if (m_particles) m_particles->setGeometry(0, 0, width(), height());
    QMainWindow::resizeEvent(e);
}

void MainWindow::showEvent(QShowEvent *e)
{
    QMainWindow::showEvent(e);
#ifdef _WIN32
    // ensure minimize box is set so taskbar button works correctly
    HWND hwnd = (HWND)winId();
    LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
    SetWindowLongPtr(hwnd, GWL_STYLE, style | WS_MINIMIZEBOX | WS_SYSMENU);
#endif
    // position indicator after window is visible
    QTimer::singleShot(0, this, [this]{ updateTabIndicator(m_stack->currentIndex(), false); });
}

void MainWindow::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::WindowStateChange) {
        // nothing special needed — Qt handles it
    }
    QMainWindow::changeEvent(e);
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    saveSettings();
    unregisterHotkey();
    unregisterHideHotkey();
    QMainWindow::closeEvent(e);
}

// --- Hotkeys ---

#ifdef _WIN32
static UINT parseHotkey(const QString &s, UINT *mods)
{
    *mods = 0;
    QString key = s.trimmed().toUpper();
    if (key.isEmpty()) return 0;
    if (key.startsWith("F") && key.size() <= 3) {
        int n = key.mid(1).toInt();
        if (n >= 1 && n <= 24) return VK_F1 + (n - 1);
    }
    if (key.size() == 1) return key.at(0).unicode();
    return 0;
}
#endif

void MainWindow::registerHotkey()
{
#ifdef _WIN32
    UINT mods = 0, vk = parseHotkey(m_hotkeyStr, &mods);
    if (vk) RegisterHotKey((HWND)winId(), m_hotkeyId, mods, vk);
#endif
}

void MainWindow::unregisterHotkey()
{
#ifdef _WIN32
    UnregisterHotKey((HWND)winId(), m_hotkeyId);
#endif
}

void MainWindow::registerHideHotkey()
{
#ifdef _WIN32
    UINT mods = 0, vk = parseHotkey(m_hideHotkeyStr, &mods);
    if (vk) RegisterHotKey((HWND)winId(), m_hideHotkeyId, mods, vk);
#endif
}

void MainWindow::unregisterHideHotkey()
{
#ifdef _WIN32
    UnregisterHotKey((HWND)winId(), m_hideHotkeyId);
#endif
}

bool MainWindow::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
{
#ifdef _WIN32
    MSG *msg = reinterpret_cast<MSG*>(message);
    if (msg->message == WM_HOTKEY) {
        if ((int)msg->wParam == m_hotkeyId)     { toggleClicking();  if (result) *result = 0; return true; }
        if ((int)msg->wParam == m_hideHotkeyId) { toggleHideMode();  if (result) *result = 0; return true; }
    }
#endif
    return QMainWindow::nativeEvent(eventType, message, result);
}
