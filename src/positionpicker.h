#pragma once
#include <QDialog>
#include <QPoint>

// full-screen transparent overlay, click anywhere to pick that coordinate
class PositionPicker : public QDialog
{
    Q_OBJECT
public:
    explicit PositionPicker(QWidget *parent = nullptr);
    QPoint pickedPos() const { return m_pos; }

protected:
    void paintEvent(QPaintEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void keyPressEvent(QKeyEvent *) override;

private:
    QPoint m_pos;
    QPoint m_current;
};
