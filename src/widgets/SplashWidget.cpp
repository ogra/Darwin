#include "SplashWidget.h"
#include <QPixmap>
#include <QPainter>
#include <QTimer>
#include <QApplication>
#include <QScreen>
#include <QPaintEvent>

SplashWidget::SplashWidget(QWidget *parent)
    : QWidget(parent)
{
    // スプラッシュスクリーンとして設定、枠なし、最前面
    setWindowFlags(Qt::SplashScreen | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    
    // スプラッシュの全体サイズ
    setFixedSize(400, 300);

    // プライマリスクリーンの中央に配置
    if (QScreen *screen = QApplication::primaryScreen()) {
        QRect screenGeometry = screen->geometry();
        int x = (screenGeometry.width() - width()) / 2;
        int y = (screenGeometry.height() - height()) / 2;
        move(x, y);
    }

    // アイコン表示用ラベル
    m_iconLabel = new QLabel(this);
    QPixmap icon(":/icons/darwin.png");
    // 高画質で少し大きめにロードしてジャギーを防ぐ
    icon = icon.scaled(200, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_iconLabel->setPixmap(icon);
    m_iconLabel->setScaledContents(true);

    int baseSize = 90;
    int maxSize = 110;

    int centerX = width() / 2;
    int centerY = height() / 2;

    QRect startRect(centerX - baseSize/2, centerY - baseSize/2, baseSize, baseSize);
    QRect endRect(centerX - maxSize/2, centerY - maxSize/2, maxSize, maxSize);

    m_iconLabel->setGeometry(startRect);

    // アニメーション設定：geometry（位置・サイズ）を変化させる
    m_animation = new QPropertyAnimation(m_iconLabel, "geometry", this);
    m_animation->setDuration(400); // 片道0.4秒
    m_animation->setStartValue(startRect);
    m_animation->setEndValue(endRect);
    m_animation->setEasingCurve(QEasingCurve::InOutSine);
    
    // アニメーション終了時に方向を一度だけ反転させて縮小、その後終了
    connect(m_animation, &QPropertyAnimation::finished, this, [this]() {
        if (m_animation->direction() == QAbstractAnimation::Forward) {
            // リバウンド（縮小）を開始
            m_animation->setDirection(QAbstractAnimation::Backward);
            m_animation->start();
        } else {
            // 縮小が終わったらスプラッシュを終了
            emit finished();
            close();
            deleteLater();
        }
    });
    
    m_animation->start();
}

SplashWidget::~SplashWidget()
{
}

void SplashWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // 背景を完全に透明にする（アルファ値を 0 に設定）
    painter.setBrush(QColor(0, 0, 0, 0));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(rect(), 15, 15);
}
