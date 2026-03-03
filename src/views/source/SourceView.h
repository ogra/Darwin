#pragma once

#include <QWidget>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QStackedWidget>
#include <QTimer>
#include <QElapsedTimer>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include "VST3Scanner.h"

class Project;
class Track;
class PluginEditorWidget;

// Custom spinning indicator widget for scan progress
class ScanSpinnerWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ScanSpinnerWidget(QWidget* parent = nullptr);
    void start();
    void stop();
    bool isSpinning() const { return m_spinning; }
protected:
    void paintEvent(QPaintEvent* event) override;
private:
    QTimer m_timer;
    int m_angle = 0;
    bool m_spinning = false;
};

// Overlay widget drawn inside the detail panel during plugin loading
class PluginLoadOverlay : public QWidget
{
    Q_OBJECT
public:
    explicit PluginLoadOverlay(QWidget* parent = nullptr);
    void startLoading(const QString& pluginName);
    void showSuccess();
    void showFailure(const QString& reason = QString());
    bool isActive() const { return isVisible(); }
signals:
    void finished();
protected:
    void paintEvent(QPaintEvent* event) override;
private:
    void tick();

    QString m_pluginName;
    QTimer  m_timer;
    QElapsedTimer m_clock;

    float m_spinnerAngle  = 0.0f;
    float m_progressSweep = 0.0f;
    float m_wavePhase     = 0.0f;

    enum Stage { Loading, Success, Failure, FadeOut };
    Stage  m_stage        = Loading;
    qint64 m_stageStartMs = 0;

    QString m_failReason;
    float   m_overlayAlpha = 0.0f;
};

class SourceView : public QWidget
{
    Q_OBJECT
public:
    SourceView(QWidget *parent = nullptr);
    
    void rescanPlugins();
    
protected:
    void resizeEvent(QResizeEvent* event) override;
    
signals:
    void instrumentSelected(const QString& instrumentName, const QString& path);
    void loadInstrumentRequested(const QString& instrumentName, const QString& path);

private slots:
    void onInstrumentClicked(QListWidgetItem* item);
    void onLoadButtonClicked();
    void onRescanClicked();

public slots:
    void applyTheme();

public:
    /** MainWindow から呼ばれるロード結果コールバック */
    void onPluginLoaded(bool success, const QString& error = QString());

private:
    void setupUi();
    void updateDetailCard(const VST3PluginInfo& info);
    void populateInstrumentList();
    void sortPluginsByUsage();
    void loadUsageData();
    void saveUsageData();
    void incrementUsageCount(const QString& pluginPath);
    
    void updatePluginList(const QVector<VST3PluginInfo>& plugins);
    
    // Animation helpers
    void startScanAnimation();
    void stopScanAnimation();
    void startStaggeredReveal();
    void revealNextItem();
    void animateDetailCard(const VST3PluginInfo& info);
    
    QListWidget* m_instrumentList;
    QLabel* m_statusLabel;
    QLabel* m_instNameLabel;
    QLabel* m_instDescLabel;
    QLabel* m_vendorLabel;
    QPushButton* m_loadBtn;
    QPushButton* m_rescanBtn;
    
    VST3Scanner* m_scanner;
    QVector<VST3PluginInfo> m_plugins;
    int m_selectedIndex;
    QSettings* m_settings;
    
    // Animation state
    ScanSpinnerWidget* m_scanSpinner;
    QTimer m_scanDotTimer;
    int m_scanDotCount = 0;
    
    QTimer m_itemRevealTimer;
    int m_itemRevealIndex = 0;
    
    bool m_isLoading = false;
    
    // Detail card animation
    QGraphicsOpacityEffect* m_detailFadeEffect;
    QWidget* m_detailContentWidget;
    QWidget* m_detailWidget;

    // Plugin load overlay
    PluginLoadOverlay* m_loadOverlay;
};
