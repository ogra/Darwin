#pragma once
#include <QLabel>
#include <QPropertyAnimation>
#include <QTimer>

class CustomTooltip : public QLabel {
    Q_OBJECT
public:
    static void showText(const QPoint& globalPos, const QString& text);
    static void hideText();
    static void attach(QWidget* w);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    explicit CustomTooltip();
    static CustomTooltip* instance();

    QPropertyAnimation* m_fadeAnim;
    QTimer* m_hideTimer;
};
