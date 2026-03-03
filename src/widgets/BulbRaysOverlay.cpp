#include "BulbRaysOverlay.h"
#include <QPainter>
#include <QtMath>

static constexpr int   OVERLAY_SIZE  = 38;
static constexpr qreal INNER_RADIUS  = 13.0;  // ボタン外周直ぐ外（ボタン半径13px）
static constexpr qreal MAX_RAY_LEN   = 4.0;
static constexpr qreal RAY_WIDTH     = 1.8;

// 左上3本・右上3本
static constexpr qreal ANGLES_DEG[6] = { -150.0, -130.0, -110.0, -70.0, -50.0, -30.0 };

BulbRaysOverlay::BulbRaysOverlay(QWidget* parent)
    : QWidget(parent)
    , m_progress(0.0)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(OVERLAY_SIZE, OVERLAY_SIZE);
}

void BulbRaysOverlay::setProgress(qreal progress)
{
    m_progress = qBound(0.0, progress, 1.0);
    update();
}

void BulbRaysOverlay::paintEvent(QPaintEvent*)
{
    if (m_progress <= 0.0) return;

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // 電球の円形部分の中心: ボタン中央より少し上（アイコン内の球体位置に合わせる）
    const qreal cx     = width()  / 2.0;
    const qreal cy     = height() / 2.0 - 3.0;
    const qreal rayLen = MAX_RAY_LEN * m_progress;

    const int    alpha    = static_cast<int>(m_progress * 210);
    const QColor rayColor(0xfb, 0xbf, 0x24, alpha); // アンバー

    QPen pen(rayColor);
    pen.setWidthF(RAY_WIDTH);
    pen.setCapStyle(Qt::RoundCap);
    p.setPen(pen);

    for (int i = 0; i < 6; ++i) {
        const qreal angle = qDegreesToRadians(ANGLES_DEG[i]);
        const qreal cosA  = qCos(angle);
        const qreal sinA  = qSin(angle);
        const QPointF p1(cx + cosA * INNER_RADIUS,            cy + sinA * INNER_RADIUS);
        const QPointF p2(cx + cosA * (INNER_RADIUS + rayLen), cy + sinA * (INNER_RADIUS + rayLen));
        p.drawLine(p1, p2);
    }
}
