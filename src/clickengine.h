#pragma once

#include <QObject>
#include <QTimer>
#include <QElapsedTimer>
#include <QPoint>
#include <QSoundEffect>

#ifdef _WIN32
#include <windows.h>
#endif

enum class ClickMethod   { SendInput, MouseEvent, PostMessage };
enum class ClickButton   { Left, Right, Middle };
enum class ClickMode     { Single, Double, Hold };
enum class ClickPosition { Cursor, Fixed };

class ClickEngine : public QObject
{
    Q_OBJECT
public:
    explicit ClickEngine(QObject *parent = nullptr);
    ~ClickEngine();

    void setInterval(int ms);
    void setRandomize(bool enabled, int jitter);
    void setClickButton(ClickButton b);
    void setClickMode(ClickMode m);
    void setClickPosition(ClickPosition p, QPoint fixedXY = QPoint(0,0));
    void setClickMethod(ClickMethod m);
    void setMaxClicks(int max);        // 0 = unlimited
    void setTimeLimit(int seconds);    // 0 = unlimited
    void setTargetWindow(void* hwnd);  // nullptr = any
    void setSoundEnabled(bool on);
    void setSoundFile(const QString &path);
    void setPauseOnMouseMove(bool on);
    void setStopAtEdge(bool on);
    void setFocusDelay(int ms);

    bool   isRunning()   const { return m_running; }
    int    clickCount()  const { return m_clickCount; }
    double currentCPS()  const;
    qint64 elapsedMs()   const;

public slots:
    void start();
    void stop();
    void resetCounter();

signals:
    void started();
    void stopped();
    void clickPerformed(int total);
    void statsUpdated(double cps, qint64 elapsedMs);
    void timeLimitReached();

private slots:
    void onTick();
    void onStatsTick();
    void onFocusDelayDone();

private:
    void   doClick();
    int    nextIntervalMs();

    QTimer        m_timer;
    QTimer        m_statsTimer;
    QTimer        m_focusDelayTimer;
    QElapsedTimer m_elapsed;
    QElapsedTimer m_tickTimer;   // measures actual time between ticks for compensation

    QSoundEffect  m_sound;

    bool   m_running       = false;
    int    m_interval      = 100;
    bool   m_randomize     = false;
    int    m_jitter        = 10;
    ClickButton   m_button   = ClickButton::Left;
    ClickMode     m_mode     = ClickMode::Single;
    ClickPosition m_posMode  = ClickPosition::Cursor;
    QPoint m_fixedPos;
    ClickMethod   m_method   = ClickMethod::SendInput;
    int    m_maxClicks     = 0;
    int    m_timeLimit     = 0;   // seconds, 0 = off
    int    m_clickCount    = 0;
    void  *m_targetHwnd    = nullptr;
    bool   m_soundEnabled  = false;
    bool   m_pauseOnMove   = false;
    bool   m_stopAtEdge    = false;
    int    m_focusDelayMs  = 0;

    // timing compensation — track lateness and pay it back on next tick
    int    m_debtMs        = 0;
    int    m_lastInterval  = 100;
    QPoint m_lastCursorPos;

    int    m_lastCountForCps   = 0;
    qint64 m_lastCpsTimestamp  = 0;
    double m_cps               = 0.0;
};
