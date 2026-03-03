/**
 * @file ScanSpinnerWidget.cpp
 * @brief VST3スキャン中のスピナーウィジェット実装
 */
#include "SourceView.h"
#include <QPainter>

// ─── ScanSpinnerWidget ───────────────────────────────────

ScanSpinnerWidget::ScanSpinnerWidget(QWidget* parent) : QWidget(parent)
{
    setFixedSize(20, 20);
    connect(&m_timer, &QTimer::timeout, this, [this](){
        m_angle = (m_angle + 12) % 360;
        update();
    });
    hide();
}

void ScanSpinnerWidget::start()
{
    m_spinning = true;
    m_angle = 0;
    m_timer.start(25); // ~40fps
    show();
}

void ScanSpinnerWidget::stop()
{
    m_spinning = false;
    m_timer.stop();
    hide();
}

void ScanSpinnerWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    
    QRectF r(2, 2, width() - 4, height() - 4);
    
    // Draw spinning arc with gradient sweep
    QPen pen(QColor("#FF3366"), 2.0, Qt::SolidLine, Qt::RoundCap);
    p.setPen(pen);
    p.drawArc(r, m_angle * 16, 270 * 16);
    
    // Draw trailing faded arc
    QPen fadePen(QColor(255, 51, 102, 60), 2.0, Qt::SolidLine, Qt::RoundCap);
    p.setPen(fadePen);
    p.drawArc(r, (m_angle + 270) * 16, 90 * 16);
}
