#pragma once

#include <QRectF>
#include <QColor>
#include <QList>
#include <QPainter>
#include <QtMath>
#include <cstdlib>

/**
 * @brief バーストアニメーション（削除時のはじけ飛ぶエフェクト）の共通エンジン
 *
 * ArrangementGridWidget と PianoRollGridWidget で完全に重複していた
 * バーストゴースト＋パーティクルの生成・描画・更新ロジックを集約。
 * パラメータの差異（パーティクル数、速度、重力 等）は BurstParams で吸収する。
 */
namespace BurstAnimation {

// ── イージング関数 ──────────────────────────
inline float easeOutBack(float t) {
    const float c1 = 1.70158f;
    const float c3 = c1 + 1.0f;
    return 1.0f + c3 * qPow(t - 1.0f, 3) + c1 * qPow(t - 1.0f, 2);
}

inline float easeOutCubic(float t) {
    return 1.0f - qPow(1.0f - t, 3);
}

// ── データ構造 ──────────────────────────────
struct BurstGhost {
    QRectF rect;
    QColor color;
    qint64 startMs;
    float  progress;
    int    extra;       // ArrangementGrid では trackIndex、他では未使用（-1）
};

struct Particle {
    float  x, y;
    float  vx, vy;
    float  size;
    float  alpha;
    QColor color;
    qint64 startMs;
};

// ── 生成パラメータ ──────────────────────────
struct BurstParams {
    int    particleMin   = 8;     // パーティクル数の下限
    int    particleRange = 5;     // 追加乱数の範囲（min + rand()%range）
    float  speed         = 2.0f;  // 初速ベース
    float  speedRand     = 0.04f; // 初速乱数係数
    float  baseSize      = 3.0f;  // パーティクルサイズベース
    float  sizeRand      = 0.04f; // サイズ乱数係数
    float  ghostDurationMs  = 400.0f;  // ゴーストの寿命
    float  particleDurationMs = 500.0f; // パーティクルの寿命
    float  gravity       = 0.15f; // 重力加速度（フレーム毎）
    float  ghostScale    = 0.4f;  // ゴースト膨張量
    int    roundedRadius = 4;     // 角丸半径
};

// Arrangement 用デフォルト
inline BurstParams arrangementParams() {
    return BurstParams{8, 5, 2.0f, 0.04f, 3.0f, 0.04f, 400.0f, 500.0f, 0.15f, 0.4f, 4};
}

// PianoRoll 用デフォルト
inline BurstParams pianoRollParams() {
    return BurstParams{6, 5, 1.5f, 0.03f, 2.0f, 0.03f, 350.0f, 450.0f, 0.12f, 0.5f, 3};
}

// ── 生成 ────────────────────────────────────
inline void spawnBurst(const QRectF& rect, const QColor& color, int extra,
                       qint64 nowMs, const BurstParams& params,
                       QList<BurstGhost>& ghosts, QList<Particle>& particles)
{
    BurstGhost ghost;
    ghost.rect     = rect;
    ghost.color    = color;
    ghost.startMs  = nowMs;
    ghost.progress = 0.0f;
    ghost.extra    = extra;
    ghosts.append(ghost);

    int particleCount = params.particleMin + (std::rand() % params.particleRange);
    float hw = static_cast<float>(rect.width())  * 0.5f;
    float hh = static_cast<float>(rect.height()) * 0.5f;
    float cx = static_cast<float>(rect.center().x());
    float cy = static_cast<float>(rect.center().y());
    float perimeter = 2.0f * (hw + hh);

    for (int j = 0; j < particleCount; ++j) {
        Particle p;
        float d = (static_cast<float>(std::rand() % 1000) / 1000.0f) * perimeter;
        float nx, ny;
        if (d < hw) {
            p.x = cx - hw + d * 2.0f; p.y = cy - hh;
            nx = 0; ny = -1;
        } else if (d < hw + hh) {
            float t = d - hw;
            p.x = cx + hw; p.y = cy - hh + t * 2.0f;
            nx = 1; ny = 0;
        } else if (d < 2 * hw + hh) {
            float t = d - hw - hh;
            p.x = cx + hw - t * 2.0f; p.y = cy + hh;
            nx = 0; ny = 1;
        } else {
            float t = d - 2 * hw - hh;
            p.x = cx - hw; p.y = cy + hh - t * 2.0f;
            nx = -1; ny = 0;
        }
        float speed  = params.speed + (std::rand() % 100) * params.speedRand;
        float jitter = ((std::rand() % 100) - 50) * 0.02f;
        p.vx    = (nx + jitter) * speed;
        p.vy    = (ny + jitter) * speed;
        p.size  = params.baseSize + (std::rand() % 100) * params.sizeRand;
        p.alpha = 1.0f;
        p.color = color.lighter(120 + std::rand() % 60);
        p.startMs = ghost.startMs;
        particles.append(p);
    }
}

// ── 描画 ────────────────────────────────────
inline void drawBurstEffects(QPainter& painter,
                             const QList<BurstGhost>& ghosts,
                             const QList<Particle>& particles,
                             int roundedRadius)
{
    painter.setRenderHint(QPainter::Antialiasing, true);

    for (const BurstGhost& ghost : ghosts) {
        float t = ghost.progress;
        float scale   = 1.0f + t * 0.4f;   // ghostScale は tick 側で反映
        float opacity = 1.0f - easeOutCubic(t);
        if (opacity <= 0.01f) continue;

        painter.save();
        QPointF center = ghost.rect.center();
        painter.translate(center);
        painter.scale(scale, scale);
        painter.translate(-center);
        painter.setOpacity(opacity * 0.6f);

        QColor fill = ghost.color;
        fill.setAlphaF(0.5f);
        painter.setPen(Qt::NoPen);
        painter.setBrush(fill);
        painter.drawRoundedRect(ghost.rect, roundedRadius, roundedRadius);
        painter.restore();
    }

    for (const Particle& pt : particles) {
        if (pt.alpha <= 0.01f) continue;
        painter.save();
        painter.setOpacity(pt.alpha);
        painter.setPen(Qt::NoPen);
        painter.setBrush(pt.color);
        painter.drawEllipse(QPointF(pt.x, pt.y), pt.size, pt.size);
        painter.restore();
    }

    painter.setRenderHint(QPainter::Antialiasing, false);
}

// ── 更新（毎フレーム呼ぶ） ──────────────────
inline bool tickBurst(qint64 nowMs, const BurstParams& params,
                      QList<BurstGhost>& ghosts, QList<Particle>& particles)
{
    bool anyActive = false;

    for (int i = ghosts.size() - 1; i >= 0; --i) {
        float elapsed = static_cast<float>(nowMs - ghosts[i].startMs);
        ghosts[i].progress = qBound(0.0f, elapsed / params.ghostDurationMs, 1.0f);
        if (ghosts[i].progress >= 1.0f) {
            ghosts.removeAt(i);
        } else {
            anyActive = true;
        }
    }

    for (int i = particles.size() - 1; i >= 0; --i) {
        float elapsed = static_cast<float>(nowMs - particles[i].startMs);
        float t = qBound(0.0f, elapsed / params.particleDurationMs, 1.0f);
        particles[i].x += particles[i].vx;
        particles[i].y += particles[i].vy;
        particles[i].vy += params.gravity;
        particles[i].alpha = 1.0f - easeOutCubic(t);
        particles[i].size *= 0.97f;
        if (t >= 1.0f || particles[i].alpha <= 0.01f) {
            particles.removeAt(i);
        } else {
            anyActive = true;
        }
    }

    return anyActive;
}

} // namespace BurstAnimation
