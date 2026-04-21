#include "particlewidget.h"
#include <QPainter>
#include <QRandomGenerator>
#include <QJsonObject>
#include <cmath>

ParticleWidget::ParticleWidget(QWidget *parent) : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_NoSystemBackground);
    setAutoFillBackground(false);

    m_timer.setInterval(16); // ~60fps
    connect(&m_timer, &QTimer::timeout, this, &ParticleWidget::tick);
}

void ParticleWidget::configure(const QJsonObject &cfg)
{
    m_color     = QColor(cfg.value("color").toString("#ffffff"));
    m_maxCount  = cfg.value("count").toInt(35);
    m_speed     = (float)cfg.value("speed").toDouble(0.45);
    m_size      = (float)cfg.value("size").toDouble(2.2);
    m_lifetime  = (float)cfg.value("lifetime").toDouble(5.0);
    m_spawnRate = cfg.value("spawnRate").toInt(1);
    update();
}

void ParticleWidget::setActive(bool on)
{
    m_active = on;
    if (on) {
        m_particles.clear();
        m_timer.start();
    } else {
        m_timer.stop();
        m_particles.clear();
        update();
    }
}

void ParticleWidget::tick()
{
    if (width() <= 0 || height() <= 0) return;

    float dt = 0.016f; // seconds per frame at 60fps

    // spawn new particles
    m_frameCounter++;
    if (m_frameCounter % qMax(1, 4 / m_spawnRate) == 0) {
        if (m_particles.size() < m_maxCount) {
            spawnParticle();
        }
    }

    // update existing particles
    for (int i = m_particles.size() - 1; i >= 0; --i) {
        Particle &p = m_particles[i];
        p.life += dt;
        p.pos  += p.vel;

        // small horizontal drift
        p.vel.setX(p.vel.x() + (QRandomGenerator::global()->bounded(100) - 50) * 0.0002f);

        if (p.life >= p.maxLife || p.pos.y() < -10) {
            m_particles.removeAt(i);
        }
    }

    update();
}

void ParticleWidget::spawnParticle()
{
    Particle p;
    auto *rng = QRandomGenerator::global();
    p.pos.setX(rng->bounded(width()));
    p.pos.setY(height() + rng->bounded(20));
    float baseSpeed = m_speed + rng->bounded(100) * 0.003f;
    p.vel.setX((rng->bounded(100) - 50) * 0.005f);
    p.vel.setY(-baseSpeed);
    p.maxLife = m_lifetime * (0.7f + rng->bounded(100) * 0.006f);
    p.life    = 0.f;
    p.size    = m_size * (0.6f + rng->bounded(100) * 0.008f);
    m_particles.append(p);
}

void ParticleWidget::paintEvent(QPaintEvent *)
{
    if (!m_active || m_particles.isEmpty()) return;

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    for (const Particle &pt : m_particles) {
        float t = pt.life / pt.maxLife;

        // fade in during first 20%, fade out during last 30%
        float alpha = 1.f;
        if (t < 0.2f)       alpha = t / 0.2f;
        else if (t > 0.7f)  alpha = 1.f - (t - 0.7f) / 0.3f;
        alpha = qBound(0.f, alpha, 1.f);

        QColor col = m_color;
        col.setAlphaF(alpha * 0.7f);
        p.setBrush(col);
        p.setPen(Qt::NoPen);
        float r = pt.size * 0.5f;
        p.drawEllipse(pt.pos, r, r);
    }
}

void ParticleWidget::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
}
