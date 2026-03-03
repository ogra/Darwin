#include "MixerChannelWidget.h"
#include "KnobWidget.h"
#include "FaderWidget.h"
#include "LevelMeterWidget.h"
#include "CustomTooltip.h"
#include "IconButton.h"
#include "Track.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QPushButton>
#include <QMenu>
#include <QAction>
#include <QCursor>
#include <QProgressDialog>
#include <QApplication>
#include <QMouseEvent>
#include <QDateTime>
#include <QFileInfo>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QTimer>
#include <cmath>
#include "../plugins/VST3Scanner.h"
#include "../plugins/VST3PluginInstance.h"
#include "common/ThemeManager.h"

namespace {
    static QVector<VST3PluginInfo> s_cachedPlugins;
    static bool s_hasScanned = false;
}

MixerChannelWidget::MixerChannelWidget(int trackNumber, const QString &trackName, Track* track, QWidget *parent)
    : QWidget(parent)
    , m_track(track)
    , m_levelMeterL(nullptr)
    , m_levelMeterR(nullptr)
    , m_fxContainer(nullptr)
    , m_fxLayout(nullptr)
    , m_scanner(nullptr)
{
    connect(&Darwin::ThemeManager::instance(), &Darwin::ThemeManager::themeChanged, this, &MixerChannelWidget::applyTheme);

    setFixedWidth(120); // Matched closely to image
    
    // マスターチャンネル判定（番号が負またはトラック名が "Master"）
    bool isMaster = (trackNumber < 0) || (trackName.compare("Master", Qt::CaseInsensitive) == 0);
    bool isFolder = m_track && m_track->isFolder();
    
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 0, 8, 16); // px-2 pb-4
    layout->setSpacing(8);

    // フォルダトラック: 上部にフォルダカラーのアクセントバーを表示
    if (isFolder) {
        QWidget* folderAccent = new QWidget(this);
        folderAccent->setFixedHeight(4);
        folderAccent->setStyleSheet(QString(
            "background-color: %1; border-radius: 2px;"
        ).arg(m_track->color().name()));
        layout->addWidget(folderAccent);
    }

    // Fader Section (Top)
    QFrame* faderSection = new QFrame(this);
    faderSection->setObjectName("faderSection");
    faderSection->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

    // フォルダトラックは背景を少し変えてグループ感を出す
    if (isFolder) {
        faderSection->setStyleSheet(QString(
            "#faderSection {"
            "  background-color: %1;"
            "  border: 1px solid %2;"
            "  border-radius: 12px;"
            "}"
        ).arg(m_track->color().lighter(185).name(),
              m_track->color().lighter(160).name()));
    } else {
        faderSection->setStyleSheet(QString(
            "#faderSection {"
            "  background-color: %1;"
            "  border: 1px solid %2;"
            "  border-radius: 12px;"
            "}"
        ).arg(Darwin::ThemeManager::instance().panelBackgroundColor().name(),
              Darwin::ThemeManager::instance().borderColor().name()));
    }

    QHBoxLayout* faderLayout = new QHBoxLayout(faderSection);
    faderLayout->setContentsMargins(12, 16, 12, 16); // padding around fader and meter
    faderLayout->setSpacing(6);

    m_levelMeterL = new LevelMeterWidget(faderSection);
    faderLayout->addWidget(m_levelMeterL);

    FaderWidget* fader = new FaderWidget(faderSection);
    if (m_track) {
        // linear gain → dB → スライダー位置 に変換して初期値を設定
        float initDb  = FaderWidget::linearToDb(static_cast<float>(m_track->volume()));
        float initPos = FaderWidget::dbToPosition(initDb);
        fader->setValue(initPos);
        // スライダー位置(0〜1) → dB → linear gain → Track::setVolume
        connect(fader, &FaderWidget::valueChanged, m_track, [this](float pos) {
            float db     = FaderWidget::positionToDb(pos);
            float linear = FaderWidget::dbToLinear(db);
            m_track->setVolume(static_cast<double>(linear));
        });
    } else if (isMaster) {
        // マスターはデフォルト 0dB（linear = 1.0）
        fader->setValue(FaderWidget::dbToPosition(0.0f));
    }
    faderLayout->addWidget(fader);

    m_levelMeterR = new LevelMeterWidget(faderSection);
    faderLayout->addWidget(m_levelMeterR);

    layout->addWidget(faderSection);

    // Value Label for Fader（dB表示）
    auto makeDbText = [](float pos) -> QString {
        float db = FaderWidget::positionToDb(pos);
        if (std::isinf(db)) return QString("-\u221e dB"); // -∞ dB
        return QString("%1%2 dB")
            .arg(db >= 0.0f ? "+" : "")
            .arg(db, 0, 'f', 1);
    };
    QLabel* volLabel = new QLabel(makeDbText(fader->value()), this);
    volLabel->setObjectName("volLabel");
    volLabel->setAlignment(Qt::AlignCenter);
    volLabel->setStyleSheet(QString("font-family: 'Segoe UI', sans-serif; font-size: 9px; color: %1;")
        .arg(Darwin::ThemeManager::instance().secondaryTextColor().name()));
    layout->addWidget(volLabel);

    connect(fader, &FaderWidget::valueChanged, [volLabel, makeDbText](float val) {
        volLabel->setText(makeDbText(val));
    });

    // FX Slots Container
    m_fxContainer = new QWidget(this);
    m_fxLayout = new QVBoxLayout(m_fxContainer);
    m_fxLayout->setContentsMargins(0,0,0,0);
    m_fxLayout->setSpacing(4);

    if (m_track) {
        connect(m_track, &Track::propertyChanged, this, &MixerChannelWidget::updateFxSlots);
    }
    updateFxSlots();
    
    layout->addWidget(m_fxContainer);

    // Knobs Area (Bottom Box)
    QWidget *knobsContainer = new QWidget(this);
    knobsContainer->setObjectName("knobsContainer");
    knobsContainer->setFixedHeight(76); // increased height for labels
    knobsContainer->setStyleSheet(QString(
        "#knobsContainer {"
        "  background-color: %1;"
        "  border: 1px solid %2;"
        "  border-radius: 4px;"
        "}"
    ).arg(Darwin::ThemeManager::instance().panelBackgroundColor().name(),
          Darwin::ThemeManager::instance().borderColor().name()));
    
    QHBoxLayout *knobsLayout = new QHBoxLayout(knobsContainer);
    knobsLayout->setContentsMargins(4, 4, 4, 4);
    knobsLayout->setSpacing(4);
    knobsLayout->setAlignment(Qt::AlignHCenter);
    
    // Timing Knob + Label
    QWidget* timingBlock = new QWidget(knobsContainer);
    QVBoxLayout* timingBlockLayout = new QVBoxLayout(timingBlock);
    timingBlockLayout->setContentsMargins(0,0,0,0);
    timingBlockLayout->setSpacing(2);
    KnobWidget *knobTiming = new KnobWidget("TIMING", timingBlock);
    // モデルからタイミングオフセット値を初期化（-100〜+100ms → ノブ0〜1に変換）
    if (m_track) {
        float initKnobVal = static_cast<float>((m_track->timingOffsetMs() + 100.0) / 200.0);
        knobTiming->setValue(qBound(0.0f, initKnobVal, 1.0f));
    } else {
        knobTiming->setValue(0.5f); // 0.5 = 中央 (0ms)
    }
    QLabel* timingValueLabel = new QLabel("0.0ms", timingBlock);
    timingValueLabel->setObjectName("timingValueLabel");
    timingValueLabel->setAlignment(Qt::AlignCenter);
    timingValueLabel->setStyleSheet(QString("font-size: 8px; color: %1;")
        .arg(Darwin::ThemeManager::instance().secondaryTextColor().name()));
    timingBlockLayout->addWidget(knobTiming);
    timingBlockLayout->addWidget(timingValueLabel);
    // ノブ操作 → モデルへ接続（0〜1 → -100〜+100ms）
    connect(knobTiming, &KnobWidget::valueChanged, [this, timingValueLabel](float val){
        float ms = (val - 0.5f) * 200.0f;
        timingValueLabel->setText(QString::number(ms, 'f', 1) + "ms");
        if (m_track) {
            m_track->setTimingOffsetMs(static_cast<double>(ms));
        }
    });
    
    // Pan Knob + Label
    QWidget* panBlock = new QWidget(knobsContainer);
    QVBoxLayout* panBlockLayout = new QVBoxLayout(panBlock);
    panBlockLayout->setContentsMargins(0,0,0,0);
    panBlockLayout->setSpacing(2);
    KnobWidget *knobPan = new KnobWidget("PAN", panBlock);
    QLabel* panValueLabel = new QLabel("C", panBlock);
    panValueLabel->setObjectName("panValueLabel");
    panValueLabel->setAlignment(Qt::AlignCenter);
    panValueLabel->setStyleSheet(QString("font-size: 8px; color: %1;")
        .arg(Darwin::ThemeManager::instance().secondaryTextColor().name()));
    
    if (m_track) {
        // pan is -1.0 to 1.0 (L to R), knob is 0.0 to 1.0
        knobPan->setValue((m_track->pan() + 1.0f) * 0.5f);
        connect(knobPan, &KnobWidget::valueChanged, m_track, [this](float val){
            m_track->setPan((val * 2.0f) - 1.0f);
        });
    } else {
        knobPan->setValue(0.5f); // Center
    }

    panBlockLayout->addWidget(knobPan);
    panBlockLayout->addWidget(panValueLabel);
    
    auto updatePanLabel = [panValueLabel](float val) {
        float panVal = (val * 2.0f) - 1.0f; // -1 to 1
        if (qAbs(panVal) < 0.01f) {
            panValueLabel->setText("C");
        } else if (panVal < 0) {
            panValueLabel->setText(QString("L%1").arg(static_cast<int>(-panVal * 100)));
        } else {
            panValueLabel->setText(QString("R%1").arg(static_cast<int>(panVal * 100)));
        }
    };
    // Initialize label
    updatePanLabel(knobPan->value());
    connect(knobPan, &KnobWidget::valueChanged, updatePanLabel);

    knobsLayout->addWidget(timingBlock);
    knobsLayout->addWidget(panBlock);

    layout->addWidget(knobsContainer);

    // Initial label text
    timingValueLabel->setText("0.0ms");

    // Track Name
    QString trName = m_track ? m_track->name() : trackName;
    QString labelText = isMaster ? "MASTER" : trName;
    
    if (isFolder) {
        // フォルダトラック: シェブロン + 名前 + 子トラック数を表示
        QWidget* folderNameArea = new QWidget(this);
        QVBoxLayout* folderNameLayout = new QVBoxLayout(folderNameArea);
        folderNameLayout->setContentsMargins(0, 0, 0, 0);
        folderNameLayout->setSpacing(2);

        // シェブロン + フォルダ名を横並び
        QWidget* chevronRow = new QWidget(folderNameArea);
        QHBoxLayout* chevronRowLayout = new QHBoxLayout(chevronRow);
        chevronRowLayout->setContentsMargins(0, 0, 0, 0);
        chevronRowLayout->setSpacing(2);

        // 展開/折りたたみシェブロンボタン
        IconButton* chevronBtn = new IconButton(IconButton::Chevron, chevronRow);
        chevronBtn->setCheckable(true);
        chevronBtn->setInitialFolderState(m_track->isFolderExpanded());
        chevronBtn->setStyleSheet(
            "QPushButton { border: none; background: transparent; }"
            "QPushButton:hover { background: rgba(0,0,0,0.06); border-radius: 4px; }"
        );
        connect(chevronBtn, &QPushButton::clicked, this, [this]() {
            emit folderToggleRequested(m_track);
        });
        chevronRowLayout->addWidget(chevronBtn);

        // フォルダ名ラベル
        QLabel* nameLabel = new QLabel(trName, chevronRow);
        nameLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        CustomTooltip::attach(nameLabel);
        nameLabel->setStyleSheet(QString(
            "font-family: 'Segoe UI', sans-serif;"
            "font-size: 10px;"
            "font-weight: 800;"
            "color: %1;"
            "letter-spacing: 0.5px;"
        ).arg(m_track->color().darker(140).name()));
        chevronRowLayout->addWidget(nameLabel, 1);

        folderNameLayout->addWidget(chevronRow);

        layout->addWidget(folderNameArea);
    } else {
        // 通常トラック / マスター
        QLabel *nameLabel = new QLabel(labelText, this);
        nameLabel->setObjectName("trackNameLabel");
        nameLabel->setAlignment(Qt::AlignCenter);
        CustomTooltip::attach(nameLabel);
        nameLabel->setStyleSheet(QString(
            "font-family: 'Segoe UI', sans-serif;"
            "font-size: 10px;"
            "font-weight: 800;"
            "color: %1;"
            "letter-spacing: 0.5px;"
            "margin-top: 4px;"
        ).arg(Darwin::ThemeManager::instance().textColor().name()));
        
        layout->addWidget(nameLabel);
    }
}

void MixerChannelWidget::applyTheme()
{
    bool isFolder = m_track && m_track->isFolder();

    // Fader Section
    QFrame* faderSection = findChild<QFrame*>("faderSection");
    if (faderSection) {
        if (!isFolder) {
            faderSection->setStyleSheet(QString(
                "#faderSection {"
                "  background-color: %1;"
                "  border: 1px solid %2;"
                "  border-radius: 12px;"
                "}"
            ).arg(Darwin::ThemeManager::instance().panelBackgroundColor().name(),
                  Darwin::ThemeManager::instance().borderColor().name()));
        }
    }

    // Knobs Container
    QWidget* knobsContainer = findChild<QWidget*>("knobsContainer");
    if (knobsContainer) {
        knobsContainer->setStyleSheet(QString(
            "#knobsContainer {"
            "  background-color: %1;"
            "  border: 1px solid %2;"
            "  border-radius: 4px;"
            "}"
        ).arg(Darwin::ThemeManager::instance().panelBackgroundColor().name(),
              Darwin::ThemeManager::instance().borderColor().name()));
    }

    // テキストラベルのテーマ更新
    const QString secColor = Darwin::ThemeManager::instance().secondaryTextColor().name();
    const QString textColor = Darwin::ThemeManager::instance().textColor().name();
    if (QLabel* lbl = findChild<QLabel*>("trackNameLabel"))
        lbl->setStyleSheet(QString("font-family: 'Segoe UI', sans-serif; font-size: 10px; font-weight: 800; color: %1; letter-spacing: 0.5px; margin-top: 4px;").arg(textColor));
    if (QLabel* lbl = findChild<QLabel*>("volLabel"))
        lbl->setStyleSheet(QString("font-family: 'Segoe UI', sans-serif; font-size: 9px; color: %1;").arg(secColor));
    if (QLabel* lbl = findChild<QLabel*>("timingValueLabel"))
        lbl->setStyleSheet(QString("font-size: 8px; color: %1;").arg(secColor));
    if (QLabel* lbl = findChild<QLabel*>("panValueLabel"))
        lbl->setStyleSheet(QString("font-size: 8px; color: %1;").arg(secColor));
    // FX Slots are re-rendered based on state or we update them
    updateFxSlots();
}

void MixerChannelWidget::setLevel(float left, float right)
{
    if (m_levelMeterL) {
        m_levelMeterL->setLevel(left);
    }
    if (m_levelMeterR) {
        m_levelMeterR->setLevel(right);
    }
}

void MixerChannelWidget::updateFxSlots()
{
    // Clear all existing widgets in fxLayout
    QLayoutItem *child;
    while ((child = m_fxLayout->takeAt(0)) != nullptr) {
        if (child->widget()) {
            child->widget()->deleteLater();
        }
        delete child;
    }
    
    // Create buttons for each loaded FX
    if (m_track) {
        const auto& fxPlugins = m_track->fxPlugins();
        for (int i = 0; i < fxPlugins.size(); ++i) {
            VST3PluginInstance* fx = fxPlugins.at(i);
            QPushButton *btn = new QPushButton(fx->pluginName(), m_fxContainer);
            btn->setFixedHeight(22);
            btn->setCursor(Qt::PointingHandCursor);
            
            if (fx->isBypassed()) {
                QString bg = Darwin::ThemeManager::instance().isDarkMode() ? "#334155" : "#f1f5f9";
                QString fg = Darwin::ThemeManager::instance().isDarkMode() ? "#94a3b8" : "#94a3b8";
                QString border = Darwin::ThemeManager::instance().isDarkMode() ? "#475569" : "#cbd5e1";
                btn->setStyleSheet(QString(
                    "QPushButton {"
                    "  background-color: %1;" 
                    "  color: %2;" 
                    "  border: 1px solid %3;"
                    "  border-radius: 2px;"
                    "  font-family: 'Segoe UI', sans-serif;"
                    "  font-size: 10px;"
                    "  font-weight: 700;"
                    "}"
                    "QPushButton:hover { background-color: %3; }"
                    "QPushButton:pressed { background-color: %3; color: %2; }"
                    "QPushButton:focus { outline: none; }"
                ).arg(bg, fg, border));
            } else {
                QString bg = Darwin::ThemeManager::instance().isDarkMode() ? "#4c1d95" : "#ffe4e6";
                QString fg = Darwin::ThemeManager::instance().isDarkMode() ? "#ddd6fe" : "#fb7185";
                QString border = Darwin::ThemeManager::instance().isDarkMode() ? "#6d28d9" : "#fb7185";
                btn->setStyleSheet(QString(
                    "QPushButton {"
                    "  background-color: %1;" 
                    "  color: %2;" 
                    "  border: 1px solid %3;"
                    "  border-radius: 2px;"
                    "  font-family: 'Segoe UI', sans-serif;"
                    "  font-size: 10px;"
                    "  font-weight: 700;"
                    "}"
                    "QPushButton:hover { background-color: %3; }"
                    "QPushButton:pressed { background-color: %3; color: %2; }"
                    "QPushButton:focus { outline: none; }"
                ).arg(bg, fg, border));
            }
            
            m_fxLayout->addWidget(btn);
            
            // Event filter for double-click removal and long-press bypass
            btn->setProperty("fxIndex", i);
            btn->installEventFilter(this);
            
            // Clicks request editor to open in Inspector
            connect(btn, &QPushButton::clicked, this, [this, fx, btn]() {
                if (btn->property("longPressTriggered").toBool()) {
                    btn->setProperty("longPressTriggered", false);
                    return;
                }
                emit pluginEditorRequested(fx);
            });
        }
    }
    
    // Re-add the "+" button
    QPushButton *addFxBtn = new QPushButton("+", m_fxContainer);
    addFxBtn->setFixedHeight(20);
    addFxBtn->setCursor(Qt::PointingHandCursor);
    QString addBg = Darwin::ThemeManager::instance().isDarkMode() ? "#1e293b" : "#f8fafc";
    QString addHover = Darwin::ThemeManager::instance().isDarkMode() ? "#334155" : "#f1f5f9";
    QString addBorder = Darwin::ThemeManager::instance().isDarkMode() ? "#475569" : "#cbd5e1";
    QString addFg = Darwin::ThemeManager::instance().isDarkMode() ? "#94a3b8" : "#94a3b8";

    addFxBtn->setStyleSheet(QString(
        "QPushButton {"
        "  border: 1px dashed %3;" 
        "  border-radius: 2px;"
        "  color: %4;" 
        "  font-weight: bold;"
        "  background-color: %1;"
        "}"
        "QPushButton:hover { background-color: %2; color: #64748b; }"
    ).arg(addBg, addHover, addBorder, addFg));
    connect(addFxBtn, &QPushButton::clicked, this, &MixerChannelWidget::showFxPluginMenu);
    m_fxLayout->addWidget(addFxBtn);
    
    // 追加アニメーション: 新しく追加されたFXスロットを浮かび上がらせる
    if (m_pendingFxHighlight >= 0) {
        animateFxAppear(m_pendingFxHighlight);
        m_pendingFxHighlight = -1;
    }
}

void MixerChannelWidget::showFxPluginMenu()
{
    if (!m_track) return;
    
    if (!s_hasScanned) {
        QProgressDialog progress("Scanning VST3 Plugins...", "Cancel", 0, 0, this);
        progress.setWindowModality(Qt::WindowModal);
        progress.setMinimumDuration(0);
        progress.setValue(1); // Force show
        progress.show();
        QApplication::processEvents();

        if (!m_scanner) {
            m_scanner = new VST3Scanner(this);
        }
        
        // false = get all plugins, not just instruments
        s_cachedPlugins = m_scanner->scan(false); 
        s_hasScanned = true;
        
        progress.close();
        QApplication::processEvents(); // Allow progress dialog to fully close and focus to return
    }

    QMenu* menu = new QMenu(this);
    menu->setAttribute(Qt::WA_DeleteOnClose);
    {
        bool isDark = Darwin::ThemeManager::instance().isDarkMode();
        QString menuBg     = isDark ? "#1e293b" : "#ffffff";
        QString menuFg     = isDark ? "#e2e8f0" : "#1e293b";
        QString menuBorder = isDark ? "#334155" : "#cbd5e1";
        QString menuSel    = isDark ? "#2d3748" : "#e2e8f0";
        menu->setStyleSheet(QString(
            "QMenu { "
            "  background-color: %1; "
            "  color: %2; "
            "  border: 1px solid %3; "
            "  border-radius: 4px; "
            "} "
            "QMenu::item {"
            "  padding: 6px 24px 6px 16px;"
            "} "
            "QMenu::item:selected {"
            "  background-color: %4;"
            "}"
        ).arg(menuBg, menuFg, menuBorder, menuSel));
    }
    
    for (const auto& info : s_cachedPlugins) {
        if (info.isEffect) {
            qDebug() << "Adding FX Plugin to Menu:" << info.name << "Path:" << info.path;
            
            // Set a fallback name if empty
            QString displayName = info.name.trimmed();
            if (displayName.isEmpty()) {
                displayName = QFileInfo(info.path).baseName();
            }
            if (displayName.isEmpty()) {
                displayName = "Unknown FX";
            }
            
            QAction* action = menu->addAction(displayName);
            connect(action, &QAction::triggered, this, [this, info]() {
                // FX追加前のインデックスを記録
                m_pendingFxHighlight = m_track->fxPlugins().size();
                m_track->addFxPlugin(info.path);
            });
        }
    }
    
    if (menu->isEmpty()) {
        QAction* emptyAct = menu->addAction("No FX Plugins Found");
        emptyAct->setEnabled(false);
    }
    
    menu->exec(QCursor::pos());
}

bool MixerChannelWidget::eventFilter(QObject *watched, QEvent *event)
{
    QPushButton* btn = qobject_cast<QPushButton*>(watched);
    if (!btn) return QWidget::eventFilter(watched, event);

    if (event->type() == QEvent::MouseButtonDblClick) {
        bool ok;
        int idx = btn->property("fxIndex").toInt(&ok);
        if (ok && m_track) {
            // エディタが開いている場合は先に閉じる（use-after-free防止）
            emit pluginEditorRequested(nullptr);
            // はじけ飛ぶアニメーションを再生してから削除
            animateFxBurst(btn);
            // 削除はアニメーション後に実行
            QTimer::singleShot(250, this, [this, idx]() {
                if (m_track && idx < m_track->fxPlugins().size()) {
                    m_track->removeFxPlugin(idx);
                }
            });
            return true;
        }
    } else if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent* me = static_cast<QMouseEvent*>(event);
        if (me->button() == Qt::LeftButton) {
            btn->setProperty("pressTime", QDateTime::currentMSecsSinceEpoch());
            btn->setProperty("longPressTriggered", false);
        }
    } else if (event->type() == QEvent::MouseButtonRelease) {
        QMouseEvent* me = static_cast<QMouseEvent*>(event);
        if (me->button() == Qt::LeftButton) {
            qint64 pressTime = btn->property("pressTime").toLongLong();
            if (pressTime > 0 && (QDateTime::currentMSecsSinceEpoch() - pressTime > 400)) {
                // It's a long press
                btn->setProperty("longPressTriggered", true);
                bool ok;
                int idx = btn->property("fxIndex").toInt(&ok);
                if (ok && m_track) {
                    if (idx >= 0 && idx < m_track->fxPlugins().size()) {
                        VST3PluginInstance* p = m_track->fxPlugins().at(idx);
                        p->setBypassed(!p->isBypassed());
                        updateFxSlots();
                    }
                }
                // Do not return true, so the button visual state doesn't get stuck.
                // The clicked handler checks "longPressTriggered" and ignores it.
            }
            btn->setProperty("pressTime", 0);
        }
    }

    return QWidget::eventFilter(watched, event);
}

// ── Burst animation on FX delete ──────────────────────────────
void MixerChannelWidget::animateFxBurst(QPushButton* btn)
{
    if (!btn) return;

    // Opacity fade-out
    auto* eff = new QGraphicsOpacityEffect(btn);
    btn->setGraphicsEffect(eff);

    auto* fadeAnim = new QPropertyAnimation(eff, "opacity", btn);
    fadeAnim->setDuration(250);
    fadeAnim->setStartValue(1.0);
    fadeAnim->setEndValue(0.0);
    fadeAnim->setEasingCurve(QEasingCurve::OutCubic);
    fadeAnim->start(QAbstractAnimation::DeleteWhenStopped);

    // Scale-up effect via stylesheet max-height trickery:
    // We animate the button geometry to expand slightly from center
    QRect startGeo = btn->geometry();
    QRect endGeo = startGeo.adjusted(-6, -4, 6, 4);

    auto* geoAnim = new QPropertyAnimation(btn, "geometry", btn);
    geoAnim->setDuration(250);
    geoAnim->setStartValue(startGeo);
    geoAnim->setEndValue(endGeo);
    geoAnim->setEasingCurve(QEasingCurve::OutCubic);
    geoAnim->start(QAbstractAnimation::DeleteWhenStopped);

    // Brief color flash before disappearing
    QString origStyle = btn->styleSheet();
    btn->setStyleSheet(origStyle + " background-color: rgba(255,120,80,200); border: 1px solid rgba(255,180,100,220);");
    QTimer::singleShot(120, btn, [btn, origStyle]() {
        if (btn) btn->setStyleSheet(origStyle);
    });
}

// ── Glow / float-up animation on FX add ──────────────────────
void MixerChannelWidget::animateFxAppear(int fxIndex)
{
    if (!m_fxLayout) return;

    // fxLayout items: [fx0, fx1, ..., fxN, "+"-button]
    // The new FX button is at index fxIndex
    if (fxIndex < 0 || fxIndex >= m_fxLayout->count()) return;
    QLayoutItem* item = m_fxLayout->itemAt(fxIndex);
    if (!item || !item->widget()) return;

    QPushButton* btn = qobject_cast<QPushButton*>(item->widget());
    if (!btn) return;

    // Start translucent + slightly below, animate to full opacity at correct position
    auto* eff = new QGraphicsOpacityEffect(btn);
    eff->setOpacity(0.0);
    btn->setGraphicsEffect(eff);

    // じんわりフェードイン
    auto* fadeIn = new QPropertyAnimation(eff, "opacity", btn);
    fadeIn->setDuration(500);
    fadeIn->setStartValue(0.0);
    fadeIn->setEndValue(1.0);
    fadeIn->setEasingCurve(QEasingCurve::InOutQuad);
    fadeIn->start(QAbstractAnimation::DeleteWhenStopped);

    // Glow highlight: temporarily set a bright border/background
    QString origStyle = btn->styleSheet();
    btn->setStyleSheet(origStyle + " border: 1px solid rgba(100,200,255,220); background-color: rgba(80,160,255,60);");

    // Fade glow away after animation settles
    QTimer::singleShot(500, btn, [btn, origStyle]() {
        if (!btn) return;
        // Animate glow fadeout by restoring style
        btn->setStyleSheet(origStyle + " border: 1px solid rgba(100,200,255,100); background-color: rgba(80,160,255,30);");
        QTimer::singleShot(300, btn, [btn, origStyle]() {
            if (btn) btn->setStyleSheet(origStyle);
        });
    });
}
