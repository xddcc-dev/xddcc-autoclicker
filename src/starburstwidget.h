#pragma once
#include <QWidget>
#include <QColor>

// paints the red+blue starburst pattern as a full background
class StarburstWidget : public QWidget
{
    Q_OBJECT
public:
    explicit StarburstWidget(QWidget *parent = nullptr);
    void setColors(const QColor &c1, const QColor &c2);
    void setEnabled(bool on);

protected:
    void paintEvent(QPaintEvent *e) override;

private:
    bool   m_active = false;
    QColor m_c1     = QColor("#e01414");
    QColor m_c2     = QColor("#1a5fff");
};
