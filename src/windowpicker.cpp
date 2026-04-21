#include "windowpicker.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidgetItem>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#endif

#ifdef _WIN32
static QString processNameFromHwnd(HWND hwnd)
{
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!h) return QString();
    wchar_t buf[MAX_PATH] = {0};
    DWORD size = MAX_PATH;
    QString out;
    if (QueryFullProcessImageNameW(h, 0, buf, &size)) {
        QString full = QString::fromWCharArray(buf);
        out = full.section('\\', -1);
    }
    CloseHandle(h);
    return out;
}

struct EnumCtx { QList<WindowInfo> *list; };

static BOOL CALLBACK enumProc(HWND hwnd, LPARAM lparam)
{
    if (!IsWindowVisible(hwnd)) return TRUE;
    int len = GetWindowTextLengthW(hwnd);
    if (len == 0) return TRUE;

    std::wstring title(len + 1, L'\0');
    GetWindowTextW(hwnd, &title[0], len + 1);
    QString qtitle = QString::fromWCharArray(title.c_str());
    if (qtitle.trimmed().isEmpty()) return TRUE;

    EnumCtx *ctx = reinterpret_cast<EnumCtx*>(lparam);
    WindowInfo info;
    info.hwnd = (void*)hwnd;
    info.title = qtitle;
    info.processName = processNameFromHwnd(hwnd);
    ctx->list->append(info);
    return TRUE;
}
#endif

WindowPicker::WindowPicker(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("Pick Target Window");
    resize(560, 440);

    auto *root = new QVBoxLayout(this);

    auto *header = new QLabel("Select a window to target. Clicks will only fire when it's in the foreground.");
    header->setObjectName("muted");
    header->setWordWrap(true);
    root->addWidget(header);

    m_list = new QListWidget(this);
    root->addWidget(m_list, 1);

    m_preview = new QLabel("No selection");
    m_preview->setObjectName("muted");
    root->addWidget(m_preview);

    auto *btnRow = new QHBoxLayout();
    m_refreshBtn = new QPushButton("Refresh");
    m_okBtn = new QPushButton("OK");
    m_cancelBtn = new QPushButton("Cancel");
    btnRow->addWidget(m_refreshBtn);
    btnRow->addStretch(1);
    btnRow->addWidget(m_okBtn);
    btnRow->addWidget(m_cancelBtn);
    root->addLayout(btnRow);

    connect(m_refreshBtn, &QPushButton::clicked, this, &WindowPicker::refresh);
    connect(m_okBtn, &QPushButton::clicked, this, &WindowPicker::onAccept);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_list, &QListWidget::itemSelectionChanged, this, &WindowPicker::onSelectionChanged);

    refresh();
}

void WindowPicker::refresh()
{
    m_list->clear();
    m_windows.clear();
#ifdef _WIN32
    EnumCtx ctx{ &m_windows };
    EnumWindows(enumProc, reinterpret_cast<LPARAM>(&ctx));
#endif
    for (const auto &w : m_windows) {
        QString label = QString("%1  —  %2").arg(w.title, w.processName.isEmpty() ? "?" : w.processName);
        m_list->addItem(label);
    }
}

void WindowPicker::onSelectionChanged()
{
    int idx = m_list->currentRow();
    if (idx < 0 || idx >= m_windows.size()) {
        m_preview->setText("No selection");
        return;
    }
    const auto &w = m_windows[idx];
    m_preview->setText(QString("HWND: 0x%1  |  %2").arg((quintptr)w.hwnd, 0, 16).arg(w.processName));
}

void WindowPicker::onAccept()
{
    int idx = m_list->currentRow();
    if (idx < 0 || idx >= m_windows.size()) { reject(); return; }
    m_selected = m_windows[idx];
    accept();
}
