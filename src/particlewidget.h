#pragma once
#include <QWidget>
#include <QTimer>
#include <QJsonObject>
#include <QColor>
#include <QVector>
#include <QPointF>

struct Particle {
    QPointF pos;
    QPointF vel;
    float   life    = 0.f;
    float   maxLife = 5.f;
    float   size    = 2.f;
};

class ParticleWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ParticleWidget(QWidget *parent = nullptr);
    void configure(const QJsonObject &cfg);
    void setActive(bool on);

protected:
    void paintEvent(QPaintEvent *) override;
    void resizeEvent(QResizeEvent *) override;

private slots:
    void tick();

private:
    void spawnParticle();

    QTimer  m_timer;
    QColor  m_color    = Qt::white;
    int     m_maxCount = 35;
    float   m_speed    = 0.45f;
    float   m_size     = 2.2f;
    float   m_lifetime = 5.0f;
    int     m_spawnRate = 1;   // particles per frame
    bool    m_active   = false;
    int     m_frameCounter = 0;

    QVector<Particle> m_particles;
};
