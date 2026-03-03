#pragma once

#include <QWidget>
#include <QLabel>
#include <QPropertyAnimation>

class SplashWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SplashWidget(QWidget *parent = nullptr);
    ~SplashWidget();

signals:
    void finished();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QLabel* m_iconLabel;
    QPropertyAnimation* m_animation;
};
