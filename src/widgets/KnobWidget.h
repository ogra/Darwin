#pragma once
#include <QWidget>
#include <QTimer>

class KnobWidget : public QWidget
{
    Q_OBJECT
public:
    explicit KnobWidget(const QString &label, QWidget *parent = nullptr);
    void setValue(float value); // 0.0 to 1.0
    float value() const { return m_value; }

signals:
    void valueChanged(float value);

protected:
    void paintEvent(QPaintEvent *event) override;
    QSize sizeHint() const override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    QString m_label;
    float m_value;        // 実際の値 0.0 to 1.0
    float m_displayValue; // 描画用の補間値
    bool m_isDragging;
    QPoint m_lastMousePos;
    QTimer m_animTimer;

    void tickAnimation();
};
