#pragma once

#include <QWidget>
#include <QTimer>
#include <QtMath>
#include <limits>

class FaderWidget : public QWidget
{
    Q_OBJECT
public:
    explicit FaderWidget(QWidget *parent = nullptr);
    
    // スライダー位置を設定（0.0 〜 1.0）
    void setValue(float value);
    float value() const { return m_value; }

    // ── dB 変換ユーティリティ（static） ──────────────────────
    // dBレンジ: -∞（無音）〜 +30 dB
    // スライダー位置 0.0 = -∞ dB（無音）、1.0 = +30 dB

    /** スライダー位置(0〜1) を dB値(-∞〜+30) に変換
     *  線形マッピング: pos=0→-∞, pos=0.5→0dB, pos=1.0→+30dB */
    static float positionToDb(float pos)
    {
        if (pos <= 0.0f) return -std::numeric_limits<float>::infinity();
        // 線形カーブ: db = pos * 60 - 30
        return pos * 60.0f - 30.0f;
    }

    /** dB値(-∞〜+30) をスライダー位置(0〜1) に変換 */
    static float dbToPosition(float db)
    {
        if (db <= -90.0f) return 0.0f; // 実質 -∞
        // 線形逆変換: pos = (db + 30) / 60
        float pos = (db + 30.0f) / 60.0f;
        return qBound(0.0f, pos, 1.0f);
    }

    /** dB値 を linear gain に変換 */
    static float dbToLinear(float db)
    {
        if (db <= -90.0f) return 0.0f;
        return qPow(10.0f, db / 20.0f);
    }

    /** linear gain を dB値 に変換 */
    static float linearToDb(float linear)
    {
        if (linear <= 0.0f) return -std::numeric_limits<float>::infinity();
        return 20.0f * log10f(linear);
    }

signals:
    void valueChanged(float value);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    float m_value;        // 実際の値 0.0 to 1.0（スライダー位置）
    float m_displayValue; // 描画用の補間値（スムーズアニメーション）
    bool m_isDragging;
    QTimer m_animTimer;

    void tickAnimation();
};
