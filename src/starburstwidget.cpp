#include "starburstwidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QtMath>
#include <cmath>
// im not even sure if this works anymore
StarburstWidget::StarburstWidget(QWidget *parent) : QWidget(parent)
{
    setAttribute(Qt::WA_StyledBackground, false);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void StarburstWidget::setColors(const QColor &c1, const QColor &c2)
{
    m_c1 = c1;
    m_c2 = c2;
    update();
}

void StarburstWidget::setEnabled(bool on)
{
    m_active = on;
    update();
}

void StarburstWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);

    if (!m_active) {
        // just fill with c1 as a plain background when disabled
        p.fillRect(rect(), m_c1);
        return;
    }

    p.setRenderHint(QPainter::Antialiasing);

    // starburst: alternating wedges from the center outward
    // the wedges are narrower at the center and widen at the edges
    QPointF center(width() / 2.0, height() / 2.0);
    // radius big enough to reach all four corners
    double radius = std::hypot(width(), height());

    const int numWedges = 18; // more wedges = finer starburst
    const double step = 360.0 / numWedges;

    for (int i = 0; i < numWedges; ++i) {
        double startDeg = i * step - 90.0; // start from top
        double endDeg   = startDeg + step;

        double startRad = qDegreesToRadians(startDeg);
        double endRad   = qDegreesToRadians(endDeg);

        // each wedge is a triangle-ish polygon from center to the arc edge
        QPolygonF wedge;
        wedge << center;

        // fan out along the arc edge with a few points for smooth-ish look
        const int fanSteps = 6;
        for (int j = 0; j <= fanSteps; ++j) {
            double angle = startRad + (endRad - startRad) * j / double(fanSteps);
            wedge << QPointF(
                center.x() + radius * std::cos(angle),
                center.y() + radius * std::sin(angle)
            );
        }

        p.setPen(Qt::NoPen);
        p.setBrush(i % 2 == 0 ? m_c1 : m_c2);
        p.drawPolygon(wedge);
    }

    // soft circular overlay in the center so cards are readable on top
    QRadialGradient fog(center, radius * 0.55);
    fog.setColorAt(0.0, QColor(0, 0, 0, 100));
    fog.setColorAt(1.0, QColor(0, 0, 0, 0));
    p.fillRect(rect(), fog);
}
