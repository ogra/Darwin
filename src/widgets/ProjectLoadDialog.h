#pragma once
#include <QDialog>
#include <QElapsedTimer>
#include <QTimer>
#include <QColor>
#include <QString>

class ProjectLoadDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ProjectLoadDialog(const QString& projectName, QWidget* parent = nullptr);

    void showSuccess(int trackCount);
    void showFailure(const QString& reason);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void tick();

    QString m_projectName;
    QTimer m_timer;
    QElapsedTimer m_clock;

    // スピナー角度
    float m_spinnerAngle = 0.0f;

    // プログレスバー
    float m_progressSweep = 0.0f;

    enum Stage { Loading, Success, Failure, FadeOut };
    Stage m_stage = Loading;
    qint64 m_stageStartMs = 0;

    int m_trackCount = 0;
    QString m_failReason;

    float m_overlayAlpha = 1.0f;
};
