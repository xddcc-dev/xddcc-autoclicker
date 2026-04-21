#include "themeeditor.h"

#include <QPainter>
#include <QPainterPath>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QSplitter>
#include <QFont>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonParseError>

// theme preview

ThemePreviewWidget::ThemePreviewWidget(QWidget *parent) : QWidget(parent)
{
    setMinimumSize(320, 400);
}

void ThemePreviewWidget::setTheme(const Theme &t)
{
    m_theme = t;
    update();
}

void ThemePreviewWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    auto c = [&](const QString &key, const QString &fb = "#888888") {
        return QColor(m_theme.colors.value(key, fb));
    };

    int r = m_theme.borderRadius;
    int w = width(), h = height();

    // background fill
    p.fillRect(rect(), c("background"));

    // mock header bar
    QRect header(0, 0, w, 36);
    p.fillRect(header, c("surface"));
    p.setPen(c("text"));
    QFont bold = p.font();
    bold.setBold(true);
    bold.setPointSize(10);
    p.setFont(bold);
    p.drawText(header.adjusted(12, 0, 0, 0), Qt::AlignVCenter | Qt::AlignLeft, "XDDCC AutoClicker");

    // mock tab bar
    QRect tabBar(0, 36, w, 28);
    p.fillRect(tabBar, c("background"));
    QStringList tabs = {"Main", "Settings", "Themes", "About"};
    int tx = 12;
    QFont tabFont = p.font();
    tabFont.setBold(false);
    tabFont.setPointSize(8);
    p.setFont(tabFont);
    for (int i = 0; i < tabs.size(); ++i) {
        QRect tr(tx, 36, 54, 28);
        if (i == 0) {
            p.setPen(c("primary"));
            // underline for active
            p.drawLine(tx, 63, tx + 54, 63);
        } else {
            p.setPen(c("textMuted"));
        }
        p.drawText(tr, Qt::AlignCenter, tabs[i]);
        tx += 56;
    }

    // separator line
    p.setPen(c("border"));
    p.drawLine(0, 64, w, 64);

    // mock card 1, top (start button + counter)
    int cardX = 16, cardY = 76, cardW = w - 32, cardH = 72;
    QPainterPath card1;
    card1.addRoundedRect(cardX, cardY, cardW, cardH, r, r);
    p.fillPath(card1, c("surface"));
    p.setPen(c("border"));
    p.drawPath(card1);

    // counter text
    p.setPen(c("accent"));
    QFont bigFont = p.font();
    bigFont.setPointSize(18);
    bigFont.setBold(true);
    p.setFont(bigFont);
    p.drawText(QRect(cardX + 14, cardY + 8, 80, cardH - 16), Qt::AlignVCenter | Qt::AlignLeft, "0");

    // mock start button
    QRect startBtn(cardX + cardW - 100, cardY + 14, 84, 44);
    QPainterPath btnPath;
    btnPath.addRoundedRect(startBtn, r, r);
    p.fillPath(btnPath, c("primary"));
    p.setPen(c("background")); // text on button
    QFont btnFont = p.font();
    btnFont.setPointSize(9);
    btnFont.setBold(true);
    p.setFont(btnFont);
    p.drawText(startBtn, Qt::AlignCenter, "START");

    // mock card 2, config area
    int c2Y = cardY + cardH + 10, c2H = h - c2Y - 44;
    QPainterPath card2;
    card2.addRoundedRect(cardX, c2Y, cardW, c2H, r, r);
    p.fillPath(card2, c("surface"));
    p.setPen(c("border"));
    p.drawPath(card2);

    QFont rowFont = p.font();
    rowFont.setPointSize(7);
    rowFont.setBold(false);
    p.setFont(rowFont);

    int rowY = c2Y + 12;
    int rowH = 18;
    QStringList rows = {"Interval (ms)", "Mouse button", "Click mode", "Max clicks"};
    for (const QString &row : rows) {
        p.setPen(c("textMuted"));
        p.drawText(QRect(cardX + 12, rowY, 90, rowH), Qt::AlignVCenter | Qt::AlignLeft, row);

        // mock input box
        QPainterPath inputPath;
        QRect inputRect(cardX + 100, rowY + 1, 80, rowH - 4);
        inputPath.addRoundedRect(inputRect, 2, 2);
        p.fillPath(inputPath, c("background"));
        p.setPen(c("border"));
        p.drawPath(inputPath);

        rowY += rowH + 6;
    }

    // mock status bar
    QRect sb(0, h - 28, w, 28);
    p.fillRect(sb, c("surface"));
    p.setPen(c("border"));
    p.drawLine(0, h - 28, w, h - 28);
    QFont sbFont = p.font();
    sbFont.setPointSize(7);
    p.setFont(sbFont);
    p.setPen(c("textMuted"));
    p.drawText(sb.adjusted(12, 0, 0, 0), Qt::AlignVCenter | Qt::AlignLeft, "Idle    |   0.0 CPS   |   00:00");
}


ThemeEditorDialog::ThemeEditorDialog(ThemeManager *tm, QWidget *parent)
    : QDialog(parent), m_themeManager(tm)
{
    setWindowTitle("Theme Editor");
    resize(900, 580);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(12, 12, 12, 8);
    root->setSpacing(8);

    // top label
    auto *titleLbl = new QLabel("Edit theme JSON on the left — live preview updates on the right.");
    titleLbl->setStyleSheet("color: #888; font-size: 12px;");
    root->addWidget(titleLbl);

    // splitter
    auto *splitter = new QSplitter(Qt::Horizontal);
    splitter->setHandleWidth(6);

    // left, JSON editor
    auto *leftBox = new QWidget();
    auto *leftLayout = new QVBoxLayout(leftBox);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(4);

    auto *jsonLbl = new QLabel("JSON");
    jsonLbl->setStyleSheet("font-weight: 700; font-size: 12px;");
    leftLayout->addWidget(jsonLbl);

    m_editor = new QTextEdit();
    m_editor->setAcceptRichText(false);
    QFont mono("Consolas");
    mono.setStyleHint(QFont::Monospace);
    mono.setPointSize(10);
    m_editor->setFont(mono);
    m_editor->setPlainText(blankTemplate());
    leftLayout->addWidget(m_editor, 1);

    m_errorLabel = new QLabel();
    m_errorLabel->setStyleSheet("color: #cc3333; font-size: 11px;");
    m_errorLabel->setWordWrap(true);
    leftLayout->addWidget(m_errorLabel);

    splitter->addWidget(leftBox);

    // right, preview
    auto *rightBox = new QWidget();
    auto *rightLayout = new QVBoxLayout(rightBox);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(4);

    auto *prevLbl = new QLabel("Preview");
    prevLbl->setStyleSheet("font-weight: 700; font-size: 12px;");
    rightLayout->addWidget(prevLbl);

    m_preview = new ThemePreviewWidget();
    rightLayout->addWidget(m_preview, 1);

    splitter->addWidget(rightBox);
    splitter->setSizes({460, 360});
    root->addWidget(splitter, 1);

    // buttons
    auto *btnRow = new QHBoxLayout();
    btnRow->setSpacing(8);
    auto *saveBtn   = new QPushButton("Save to Library");
    auto *cancelBtn = new QPushButton("Cancel");
    btnRow->addStretch(1);
    btnRow->addWidget(saveBtn);
    btnRow->addWidget(cancelBtn);
    root->addLayout(btnRow);

    // debounce timer so preview doesn't update every keypress
    m_debounce = new QTimer(this);
    m_debounce->setSingleShot(true);
    m_debounce->setInterval(350);
    connect(m_editor,   &QTextEdit::textChanged, this, &ThemeEditorDialog::onTextChanged);
    connect(m_debounce, &QTimer::timeout,        this, &ThemeEditorDialog::onDebounceTimeout);
    connect(saveBtn,    &QPushButton::clicked,   this, &ThemeEditorDialog::saveTheme);
    connect(cancelBtn,  &QPushButton::clicked,   this, &QDialog::reject);

    // initial preview
    onDebounceTimeout();
}

void ThemeEditorDialog::onTextChanged()
{
    m_debounce->start();
}

void ThemeEditorDialog::onDebounceTimeout()
{
    bool ok = false;
    Theme t = parseCurrentJson(ok);
    if (ok) {
        m_errorLabel->clear();
        m_preview->setTheme(t);
    }
}

Theme ThemeEditorDialog::parseCurrentJson(bool &ok)
{
    ok = false;
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(
        m_editor->toPlainText().toUtf8(), &err);

    if (err.error != QJsonParseError::NoError) {
        m_errorLabel->setText(QString("JSON error at offset %1: %2")
            .arg(err.offset).arg(err.errorString()));
        return {};
    }

    Theme t = ThemeManager::parseThemeJson(doc.object());
    if (t.name.isEmpty()) {
        m_errorLabel->setText("Theme must have a non-empty \"name\" field.");
        return {};
    }

    ok = true;
    return t;
}

void ThemeEditorDialog::saveTheme()
{
    bool ok = false;
    Theme t = parseCurrentJson(ok);
    if (!ok) {
        QMessageBox::warning(this, "Save Failed", "Fix the JSON errors before saving.");
        return;
    }

    // write to a temp QJsonDocument string and load via the manager
    QJsonDocument doc(ThemeManager::themeToJson(t));
    QByteArray data = doc.toJson();

    // use a temp resource-style load path hack — load from raw bytes
    // add to manager via the parse result directly
    m_themeManager->addTheme(t);
    m_savedName = t.name;
    accept();
}

QString ThemeEditorDialog::blankTemplate()
{
    return R"({
    "name": "My Theme",
    "author": "you",
    "version": "1.0",
    "description": "My custom theme",
    "colors": {
        "background": "#1a1a1a",
        "surface":    "#242424",
        "primary":    "#5588ff",
        "primaryHover": "#4477ee",
        "accent":     "#88aaff",
        "text":       "#f0f0f0",
        "textMuted":  "#888888",
        "border":     "#3a3a3a",
        "success":    "#44bb88",
        "danger":     "#dd4444",
        "warning":    "#ddaa44"
    },
    "gradients": {
        "header": ["#1a1a1a", "#242424"],
        "button": ["#5588ff", "#4477ee"],
        "active": ["#44bb88", "#338866"]
    },
    "borderRadius": 6,
    "fontFamily": "Segoe UI",
    "fontSize": 13,
    "animations": {
        "enabled": true,
        "duration": 160,
        "hoverGlow": true,
        "pulseOnClick": true
    }
})";
}
