#pragma once

#include <QDialog>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>

struct WindowInfo {
    void* hwnd = nullptr; // HWND
    QString title;
    QString processName;
};

class WindowPicker : public QDialog
{
    Q_OBJECT
public:
    explicit WindowPicker(QWidget *parent = nullptr);

    WindowInfo selectedWindow() const { return m_selected; }

private slots:
    void refresh();
    void onAccept();
    void onSelectionChanged();

private:
    QListWidget *m_list;
    QPushButton *m_refreshBtn;
    QPushButton *m_okBtn;
    QPushButton *m_cancelBtn;
    QLabel *m_preview;

    WindowInfo m_selected;
    QList<WindowInfo> m_windows;
};
