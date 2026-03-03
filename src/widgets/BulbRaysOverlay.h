#pragma once
#include <QWidget>

/**
 * @brief 電球ボタンの周囲に放射状の線を描画するオーバーレイウィジェット
 *
 * m_themeToggleBtn の子として配置し、ライトモード時は progress=1.0 で
 * 線が点灯した状態を維持する。マウスイベントは透過する。
 */
class BulbRaysOverlay : public QWidget
{
    Q_OBJECT
public:
    explicit BulbRaysOverlay(QWidget* parent = nullptr);

    void   setProgress(qreal progress);
    qreal  progress() const { return m_progress; }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    qreal m_progress = 0.0;
};
