#include "clickengine.h"
#include <QRandomGenerator>
#include <QCursor>
#include <QScreen>
#include <QApplication>

#ifdef _WIN32
#include <windows.h>
#endif

#if defined(__linux__)
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>
static Display *g_display = nullptr;
static void ensureDisplay() {
    if (!g_display) g_display = XOpenDisplay(nullptr);
}
#endif

ClickEngine::ClickEngine(QObject *parent) : QObject(parent)
{
    m_timer.setTimerType(Qt::PreciseTimer);
    connect(&m_timer, &QTimer::timeout, this, &ClickEngine::onTick);

    m_statsTimer.setInterval(250);
    connect(&m_statsTimer, &QTimer::timeout, this, &ClickEngine::onStatsTick);

    m_focusDelayTimer.setSingleShot(true);
    connect(&m_focusDelayTimer, &QTimer::timeout, this, &ClickEngine::onFocusDelayDone);
}

ClickEngine::~ClickEngine() { stop(); }

void ClickEngine::setInterval(int ms)          { m_interval = qMax(1, ms); }
void ClickEngine::setRandomize(bool e, int j)  { m_randomize = e; m_jitter = qMax(0, j); }
void ClickEngine::setClickButton(ClickButton b){ m_button = b; }
void ClickEngine::setClickMode(ClickMode m)    { m_mode = m; }
void ClickEngine::setClickPosition(ClickPosition p, QPoint xy) { m_posMode = p; m_fixedPos = xy; }
void ClickEngine::setClickMethod(ClickMethod m){ m_method = m; }
void ClickEngine::setMaxClicks(int max)        { m_maxClicks = qMax(0, max); }
void ClickEngine::setTimeLimit(int s)          { m_timeLimit = qMax(0, s); }
void ClickEngine::setTargetWindow(void* hwnd)  { m_targetHwnd = hwnd; }
void ClickEngine::setPauseOnMouseMove(bool on) { m_pauseOnMove = on; }
void ClickEngine::setStopAtEdge(bool on)       { m_stopAtEdge = on; }
void ClickEngine::setFocusDelay(int ms)        { m_focusDelayMs = qMax(0, ms); }

void ClickEngine::setSoundEnabled(bool on)     { m_soundEnabled = on; }
void ClickEngine::setSoundFile(const QString &path)
{
    if (path.isEmpty()) return;
    m_sound.setSource(QUrl::fromLocalFile(path));
    m_sound.setVolume(1.0f);
}

double ClickEngine::currentCPS() const { return m_cps; }
qint64 ClickEngine::elapsedMs()  const { return m_running ? m_elapsed.elapsed() : 0; }

int ClickEngine::nextIntervalMs()
{
    int target = m_interval;
    if (m_randomize && m_jitter > 0)
        target += QRandomGenerator::global()->bounded(-m_jitter, m_jitter + 1);
    target = qMax(1, target);

    // pay back any lateness from the previous tick
    int adjusted = qMax(1, target - m_debtMs);
    m_debtMs = qMax(0, m_debtMs - (target - adjusted));
    m_lastInterval = adjusted;
    return adjusted;
}

void ClickEngine::start()
{
    if (m_running) return;
    m_running = true;
    m_clickCount = 0;
    m_lastCountForCps = 0;
    m_elapsed.restart();
    m_lastCpsTimestamp = 0;
    m_debtMs = 0;
    m_lastCursorPos = QCursor::pos();

    if (m_focusDelayMs > 0) {
        m_focusDelayTimer.start(m_focusDelayMs);
    } else {
        onFocusDelayDone();
    }
}

void ClickEngine::onFocusDelayDone()
{
    m_tickTimer.restart();
    m_timer.start(nextIntervalMs());
    m_statsTimer.start();
    emit started();
}

void ClickEngine::stop()
{
    if (!m_running) return;
    m_running = false;
    m_timer.stop();
    m_statsTimer.stop();
    m_focusDelayTimer.stop();
    emit stopped();
}

void ClickEngine::resetCounter()
{
    m_clickCount = 0;
    emit clickPerformed(0);
}

void ClickEngine::onTick()
{
    if (!m_running) return;

    // measure actual elapsed since last tick to compute debt
    qint64 actual = m_tickTimer.elapsed();
    m_debtMs = (int)qMax(0LL, actual - (qint64)m_lastInterval);
    m_tickTimer.restart();

#ifdef _WIN32
    if (m_targetHwnd) {
        HWND fg = GetForegroundWindow();
        if (fg != (HWND)m_targetHwnd) {
            m_debtMs = 0; // dont accumulate debt while waiting for window
            m_timer.start(nextIntervalMs());
            return;
        }
    }
#endif

    // pause if mouse moved (optional)
    if (m_pauseOnMove) {
        QPoint cur = QCursor::pos();
        if (cur != m_lastCursorPos) {
            m_lastCursorPos = cur;
            m_debtMs = 0;
            m_timer.start(nextIntervalMs());
            return;
        }
    }

    // stop at screen edge (optional)
    if (m_stopAtEdge) {
        QPoint cur = QCursor::pos();
        QRect screen = QApplication::primaryScreen()->geometry();
        if (cur.x() <= screen.left()  || cur.y() <= screen.top() ||
            cur.x() >= screen.right() || cur.y() >= screen.bottom()) {
            stop();
            return;
        }
    }

    doClick();

    if (m_soundEnabled) {
        m_sound.play();
    }

    m_clickCount++;
    emit clickPerformed(m_clickCount);

    if (m_maxClicks > 0 && m_clickCount >= m_maxClicks) {
        stop();
        return;
    }

    if (m_timeLimit > 0 && m_elapsed.elapsed() >= (qint64)m_timeLimit * 1000) {
        stop();
        emit timeLimitReached();
        return;
    }

    m_lastCursorPos = QCursor::pos();
    m_timer.start(nextIntervalMs());
}

void ClickEngine::onStatsTick()
{
    qint64 now = m_elapsed.elapsed();
    qint64 dt  = now - m_lastCpsTimestamp;
    if (dt > 0) {
        int diff = m_clickCount - m_lastCountForCps;
        m_cps = (double)diff * 1000.0 / (double)dt;
        m_lastCountForCps  = m_clickCount;
        m_lastCpsTimestamp = now;
    }
    emit statsUpdated(m_cps, now);
}

// windows click
#ifdef _WIN32

static void sendInputClick(ClickButton btn, ClickMode mode)
{
    DWORD down = MOUSEEVENTF_LEFTDOWN, up = MOUSEEVENTF_LEFTUP;
    if (btn == ClickButton::Right)  { down = MOUSEEVENTF_RIGHTDOWN;  up = MOUSEEVENTF_RIGHTUP; }
    if (btn == ClickButton::Middle) { down = MOUSEEVENTF_MIDDLEDOWN; up = MOUSEEVENTF_MIDDLEUP; }

    INPUT in[2] = {};
    in[0].type = INPUT_MOUSE; in[0].mi.dwFlags = down;
    in[1].type = INPUT_MOUSE; in[1].mi.dwFlags = up;

    if (mode == ClickMode::Hold) { SendInput(1, &in[0], sizeof(INPUT)); return; }
    SendInput(2, in, sizeof(INPUT));
    if (mode == ClickMode::Double) { Sleep(1); SendInput(2, in, sizeof(INPUT)); }
}

static void mouseEventClick(ClickButton btn, ClickMode mode)
{
    DWORD down = MOUSEEVENTF_LEFTDOWN, up = MOUSEEVENTF_LEFTUP;
    if (btn == ClickButton::Right)  { down = MOUSEEVENTF_RIGHTDOWN;  up = MOUSEEVENTF_RIGHTUP; }
    if (btn == ClickButton::Middle) { down = MOUSEEVENTF_MIDDLEDOWN; up = MOUSEEVENTF_MIDDLEUP; }

    mouse_event(down, 0, 0, 0, 0);
    if (mode != ClickMode::Hold) {
        mouse_event(up, 0, 0, 0, 0);
        if (mode == ClickMode::Double) { Sleep(1); mouse_event(down, 0, 0, 0, 0); mouse_event(up, 0, 0, 0, 0); }
    }
}

static void postMessageClick(HWND hwnd, QPoint pos, ClickButton btn, ClickMode mode)
{
    if (!hwnd) hwnd = GetForegroundWindow();
    if (!hwnd) return;

    UINT dm = WM_LBUTTONDOWN, um = WM_LBUTTONUP; WPARAM wp = MK_LBUTTON;
    if (btn == ClickButton::Right)  { dm = WM_RBUTTONDOWN; um = WM_RBUTTONUP; wp = MK_RBUTTON; }
    if (btn == ClickButton::Middle) { dm = WM_MBUTTONDOWN; um = WM_MBUTTONUP; wp = MK_MBUTTON; }

    POINT p{pos.x(), pos.y()};
    ScreenToClient(hwnd, &p);
    LPARAM lp = MAKELPARAM(p.x, p.y);

    PostMessageW(hwnd, dm, wp, lp);
    if (mode != ClickMode::Hold) {
        PostMessageW(hwnd, um, 0, lp);
        if (mode == ClickMode::Double) { PostMessageW(hwnd, dm, wp, lp); PostMessageW(hwnd, um, 0, lp); }
    }
}
#endif

void ClickEngine::doClick()
{
#ifdef _WIN32
    QPoint pos = (m_posMode == ClickPosition::Fixed) ? m_fixedPos : QCursor::pos();

    if (m_posMode == ClickPosition::Fixed &&
        (m_method == ClickMethod::SendInput || m_method == ClickMethod::MouseEvent))
        SetCursorPos(pos.x(), pos.y());

    switch (m_method) {
        case ClickMethod::SendInput:   sendInputClick(m_button, m_mode);  break;
        case ClickMethod::MouseEvent:  mouseEventClick(m_button, m_mode); break;
        case ClickMethod::PostMessage: postMessageClick((HWND)m_targetHwnd, pos, m_button, m_mode); break;
    }

#elif defined(__linux__)
    ensureDisplay();
    if (!g_display) return;

    unsigned int btn = Button1;
    if (m_button == ClickButton::Right)  btn = Button3;
    if (m_button == ClickButton::Middle) btn = Button2;

    if (m_posMode == ClickPosition::Fixed)
        XTestFakeMotionEvent(g_display, -1, m_fixedPos.x(), m_fixedPos.y(), 0);

    auto press   = [&]{ XTestFakeButtonEvent(g_display, btn, True,  0); XFlush(g_display); };
    auto release = [&]{ XTestFakeButtonEvent(g_display, btn, False, 0); XFlush(g_display); };

    if (m_mode == ClickMode::Hold) { press(); }
    else { press(); release(); if (m_mode == ClickMode::Double) { usleep(1000); press(); release(); } }
#endif
}
