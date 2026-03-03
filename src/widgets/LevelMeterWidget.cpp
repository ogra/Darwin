#include "LevelMeterWidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QTimer>
#include <QtMath>
#include <algorithm>
#include "common/ThemeManager.h"

LevelMeterWidget::LevelMeterWidget(QWidget *parent)
    : QWidget(parent)
    , m_level(0.0f)
    , m_displayLevel(0.0f)
    , m_peakHold(0.0f)
{
    setMinimumSize(4, 110);
    setMaximumWidth(8);
    
    // Setup decay timer (approx 60fps)
    QTimer* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this]() {
        // Decay (falling speed)
        float decayFactor = 0.90f; 
        
        bool changed = false;
        if (m_displayLevel > m_level) {
            m_displayLevel *= decayFactor;
            if (m_displayLevel < m_level) m_displayLevel = m_level;
            changed = true;
        } else if (m_displayLevel < m_level) {
            m_displayLevel = m_level;
            changed = true;
        }
        
        // Decay the target level slightly too so peaks don't stick forever if no audio comes in
        m_level *= 0.95f;
        if (m_level < 0.001f) m_level = 0.0f;
        
        // Peak hold decay (slower)
        m_peakHold -= 0.005f;
        if (m_displayLevel >= m_peakHold) m_peakHold = m_displayLevel;
        if (m_peakHold < 0.0f) m_peakHold = 0.0f;

        if (changed || m_peakHold > 0.0f) {
            update();
        }
    });
    timer->start(16);

    // テーマ変更時に再描画
    connect(&Darwin::ThemeManager::instance(), &Darwin::ThemeManager::themeChanged,
            this, [this]() { update(); });
}

void LevelMeterWidget::setLevel(float level)
{
    // Capture highest peak between timer ticks
    m_level = std::max(m_level, level);
}

QSize LevelMeterWidget::sizeHint() const
{
    return QSize(6, 120);
}

void LevelMeterWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    int w = width();
    int h = height();

    // Background Container (Flat style)
    p.setPen(Qt::NoPen);
    QPainterPath bgPath;
    bgPath.addRoundedRect(0, 0, w, h, 2, 2);
    p.fillPath(bgPath, Darwin::ThemeManager::instance().borderColor());

    float constrainedLevel = std::min(1.0f, m_displayLevel);
    float visualLevel = std::sqrt(constrainedLevel);
    
    float constrainedPeak = std::min(1.0f, m_peakHold);
    float visualPeak = std::sqrt(constrainedPeak);

    if (visualLevel > 0.0f) {
        int fillHeight = static_cast<int>(h * visualLevel);
        int fillY = h - fillHeight;
        
        QPainterPath fillPath;
        fillPath.addRoundedRect(0, fillY, w, fillHeight, 2, 2);
        
        // Soft Modern Gradient (green -> yellow -> red)
        QLinearGradient grad(0, h, 0, 0); // Bottom to top over the whole height
        grad.setColorAt(0.0, QColor("#10b981")); // Emerald Green
        grad.setColorAt(0.7, QColor("#f59e0b")); // Amber Yellow
        grad.setColorAt(0.95, QColor("#ef4444")); // Rose Red
        
        p.fillPath(fillPath, grad);
    }
    
    // Draw Peak hold line
    if (visualPeak > 0.0f) {
        int peakY = h - static_cast<int>(h * visualPeak);
        peakY = std::max(0, peakY); // clamp top
        p.fillRect(0, peakY, w, 2, QColor("#94a3b8")); // Slate
    }
    
    // Clip indicator if exceeded 1.0
    if (m_displayLevel > 1.0f) {
        p.fillRect(0, 0, w, 3, QColor("#ef4444")); // Solid red block at top
    }
}
