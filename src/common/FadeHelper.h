#pragma once

#include <QWidget>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <functional>

/**
 * @brief フェードイン/アウトのヘルパー関数
 *
 * MainWindow::switchMode, SourceView, MixerChannelWidget 等で
 * 繰り返されるフェードアニメーションのボイラープレートを共通化する。
 */
namespace FadeHelper {

/**
 * @brief ウィジェットをフェードインさせる
 * @param widget     対象ウィジェット
 * @param durationMs アニメーション時間（ms）
 * @param curve      イージングカーブ（デフォルト OutQuad）
 * @param onFinished 完了コールバック（省略可）
 *
 * アニメーション完了後に graphicsEffect を自動除去する。
 */
inline void fadeIn(QWidget* widget, int durationMs = 300,
                   QEasingCurve curve = QEasingCurve::OutQuad,
                   std::function<void()> onFinished = nullptr)
{
    auto* eff = new QGraphicsOpacityEffect(widget);
    eff->setOpacity(0.0);
    widget->setGraphicsEffect(eff);

    auto* anim = new QPropertyAnimation(eff, "opacity", widget);
    anim->setDuration(durationMs);
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    anim->setEasingCurve(curve);
    QObject::connect(anim, &QPropertyAnimation::finished, widget, [widget, onFinished]() {
        widget->setGraphicsEffect(nullptr);
        if (onFinished) onFinished();
    });
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

/**
 * @brief ウィジェットをフェードアウトさせる
 * @param widget     対象ウィジェット
 * @param durationMs アニメーション時間（ms）
 * @param curve      イージングカーブ（デフォルト OutQuad）
 * @param onFinished 完了コールバック（省略可）
 */
inline void fadeOut(QWidget* widget, int durationMs = 300,
                    QEasingCurve curve = QEasingCurve::OutQuad,
                    std::function<void()> onFinished = nullptr)
{
    auto* eff = new QGraphicsOpacityEffect(widget);
    eff->setOpacity(1.0);
    widget->setGraphicsEffect(eff);

    auto* anim = new QPropertyAnimation(eff, "opacity", widget);
    anim->setDuration(durationMs);
    anim->setStartValue(1.0);
    anim->setEndValue(0.0);
    anim->setEasingCurve(curve);
    QObject::connect(anim, &QPropertyAnimation::finished, widget, [widget, onFinished]() {
        if (onFinished) onFinished();
    });
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

} // namespace FadeHelper
