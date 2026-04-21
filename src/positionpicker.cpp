#include "positionpicker.h"
#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QApplication>
#include <QScreen>

PositionPicker::PositionPicker(QWidget *parent)
    : QDialog(parent, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);
    setCursor(Qt::CrossCursor);

    // cover all screens
    QRect total;
    for (QScreen *s : QApplication::screens())
        total = total.united(s->geometry());
    setGeometry(total);
}

void PositionPicker::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    // semi-transparent dark overlay
    p.fillRect(rect(), QColor(0, 0, 0, 90));

    // crosshair
    p.setPen(QPen(Qt::white, 1, Qt::DashLine));
    p.drawLine(m_current.x(), 0, m_current.x(), height());
    p.drawLine(0, m_current.y(), width(), m_current.y());

    // coordinate label
    QString label = QString("X: %1  Y: %2  — click to pick, Esc to cancel")
        .arg(m_current.x()).arg(m_current.y());
    QFont f = p.font();
    f.setPointSize(11);
    f.setBold(true);
    p.setFont(f);

    QRect tr(m_current.x() + 12, m_current.y() - 28, 320, 24);
    if (tr.right() > width() - 10)  tr.moveLeft(m_current.x() - tr.width() - 12);
    if (tr.top() < 4)                tr.moveTop(m_current.y() + 8);

    p.setPen(Qt::black);
    p.drawText(tr.adjusted(1,1,1,1), label);
    p.setPen(Qt::white);
    p.drawText(tr, label);
}

void PositionPicker::mouseMoveEvent(QMouseEvent *e)
{
    m_current = e->globalPosition().toPoint();
    update();
}

void PositionPicker::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        m_pos = e->globalPosition().toPoint();
        accept();
    }
}

void PositionPicker::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Escape)
        reject();
    else
        QDialog::keyPressEvent(e);
}
