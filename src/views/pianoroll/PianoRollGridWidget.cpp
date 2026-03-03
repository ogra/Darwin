/**
 * @file PianoRollGridWidget.cpp
 * @brief PianoRollGridWidget のコンストラクタ、セットアップ、ユーティリティ、アニメーション
 *
 * 描画・入力処理はそれぞれ分割ファイルに配置:
 *   - PianoRollGridWidget_Painting.cpp
 *   - PianoRollGridWidget_Input.cpp
 */
#include "PianoRollGridWidget.h"
#include "Clip.h"
#include "Note.h"
#include "Project.h"
#include "Track.h"
#include <QPainter>
#include <QScrollArea>
#include <QScrollBar>
#include "common/Constants.h"
#include "common/BurstAnimationHelper.h"
#include "common/ThemeManager.h"

using namespace Darwin;
static const double BASE_PIXELS_PER_BAR = PIXELS_PER_BAR;

PianoRollGridWidget::PianoRollGridWidget(QWidget *parent) 
    : QWidget(parent)
    , m_playheadPosition(0)
    , m_activeClip(nullptr)
    , m_project(nullptr)
    , m_zoomLevel(1.0)
    , m_selectedNote(nullptr)
    , m_isDragging(false)
    , m_isResizing(false)
    , m_isResizingLeft(false)
    , m_isZooming(false)
    , m_longPressActive(false)
    , m_prevSelectedNote(nullptr)
{
    setMinimumHeight(ROW_HEIGHT * NUM_ROWS);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    
    // 長押しタイマー設定（300msでズームモードに入る）
    m_longPressTimer.setSingleShot(true);
    m_longPressTimer.setInterval(300);
    connect(&m_longPressTimer, &QTimer::timeout, this, &PianoRollGridWidget::onLongPressTimeout);
    
    // アニメーションタイマー（60fps）
    m_animClock.start();
    connect(&m_animTimer, &QTimer::timeout, this, &PianoRollGridWidget::tickAnimations);
    m_animTimer.setInterval(16);

    // テーマ切替時に再描画
    connect(&Darwin::ThemeManager::instance(), &Darwin::ThemeManager::themeChanged,
            this, [this]() { update(); });

    updateGeometry();
}

void PianoRollGridWidget::onLongPressTimeout()
{
    // ラバーバンド範囲選択中は長押しズームに入らない
    if (m_isRubberBanding) return;

    // 長押し判定：マウスが大きく動いていなければズームモード開始
    m_longPressActive = true;
    m_isZooming = true;
    m_isDragging = false;
    m_isResizing = false;
    m_isRubberBanding = false;
    m_selectedNote = nullptr;
    setCursor(Qt::SizeHorCursor);
    update();
}

QSize PianoRollGridWidget::sizeHint() const
{
    int w = computeRequiredWidth();
    return QSize(w, ROW_HEIGHT * NUM_ROWS);
}

int PianoRollGridWidget::computeRequiredWidth() const
{
    int minWidth = static_cast<int>(MIN_BARS * BASE_PIXELS_PER_BAR * m_zoomLevel);
    
    qint64 maxTick = 0;
    if (m_project) {
        for (Track* track : m_project->tracks()) {
            for (Clip* clip : track->clips()) {
                qint64 endTick = clip->startTick() + clip->durationTicks();
                if (endTick > maxTick) maxTick = endTick;
            }
        }
    }
    if (m_activeClip) {
        qint64 clipEnd = m_activeClip->startTick() + m_activeClip->durationTicks();
        if (clipEnd > maxTick) maxTick = clipEnd;
    }
    if (m_playheadPosition > maxTick) maxTick = m_playheadPosition;
    
    int contentWidth = static_cast<int>(maxTick * pixelsPerTick()) + static_cast<int>(8 * BASE_PIXELS_PER_BAR * m_zoomLevel);
    return qMax(minWidth, contentWidth);
}

void PianoRollGridWidget::updateDynamicSize()
{
    int newWidth = computeRequiredWidth();
    setMinimumWidth(newWidth);
    resize(newWidth, height());
    updateGeometry();
    update();
}

void PianoRollGridWidget::ensurePlayheadVisible()
{
    if (!m_scrollArea) return;
    
    int playheadX = static_cast<int>(m_playheadPosition * pixelsPerTick());
    QScrollBar* hBar = m_scrollArea->horizontalScrollBar();
    if (!hBar) return;
    
    int viewportWidth = m_scrollArea->viewport()->width();
    int scrollPos = hBar->value();
    
    int rightThreshold = scrollPos + static_cast<int>(viewportWidth * 0.8);
    int leftThreshold = scrollPos + static_cast<int>(viewportWidth * 0.2);
    
    if (playheadX > rightThreshold) {
        hBar->setValue(playheadX - static_cast<int>(viewportWidth * 0.8));
    } else if (playheadX < leftThreshold) {
        hBar->setValue(playheadX - static_cast<int>(viewportWidth * 0.2));
    }
}

double PianoRollGridWidget::pixelsPerTick() const
{
    return (BASE_PIXELS_PER_BAR * m_zoomLevel) / TICKS_PER_BAR;
}

qint64 PianoRollGridWidget::gridQuantize() const
{
    // ズームレベルに応じたグリッド量子化単位
    double ppt = pixelsPerTick();
    double pixelsPerBeat = TICKS_PER_BEAT * ppt;
    
    if (pixelsPerBeat >= 200) return TICKS_PER_BEAT / 8; // 32分音符 (60 ticks)
    if (pixelsPerBeat >= 100) return TICKS_PER_BEAT / 4; // 16分音符 (120 ticks)
    if (pixelsPerBeat >= 50)  return TICKS_PER_BEAT / 2; // 8分音符 (240 ticks)
    return TICKS_PER_BEAT; // 4分音符 (480 ticks)
}

qint64 PianoRollGridWidget::snapTick(qint64 tick) const
{
    if (!m_project || !m_project->gridSnapEnabled()) return tick;
    qint64 q = gridQuantize();
    if (q <= 0) return tick;
    return ((tick + q / 2) / q) * q;
}

void PianoRollGridWidget::setPlayheadPosition(qint64 tickPosition)
{
    m_playheadPosition = tickPosition;
    
    // コンテンツ幅を動的に拡張
    int newWidth = computeRequiredWidth();
    if (newWidth > minimumWidth()) {
        setMinimumWidth(newWidth);
        resize(newWidth, height());
        updateGeometry();
    }
    
    // 再生ヘッドにスクロール追従
    ensurePlayheadVisible();
    
    update();
}

void PianoRollGridWidget::setProject(Project* project)
{
    m_project = project;
    
    if (m_project) {
        // 既存トラックのプロパティ変更（表示/非表示など）を監視
        for (Track* track : m_project->tracks()) {
            connect(track, &Track::propertyChanged, this, [this](){ update(); });
        }
        // 新規トラック追加時にも接続
        connect(m_project, &Project::trackAdded, this, [this](Track* track){
            connect(track, &Track::propertyChanged, this, [this](){ update(); });
            update();
        });
    }
    
    update();
}

void PianoRollGridWidget::setActiveClip(Clip* clip)
{
    // Disconnect old clip signals if needed
    if (m_activeClip) {
        disconnect(m_activeClip, nullptr, this, nullptr);
    }
    
    m_activeClip = clip;
    m_selectedNote = nullptr;
    m_selectedNotes.clear();
    
    if (m_activeClip) {
        connect(m_activeClip, &Clip::changed, this, [this](){ update(); });
        connect(m_activeClip, &Clip::noteAdded, this, [this](Note*){ update(); });
        connect(m_activeClip, &Clip::noteRemoved, this, [this](Note* note){
            if (m_selectedNote == note) m_selectedNote = nullptr;
            update();
        });
    }
    
    update();
}

// ===== アニメーション実装 =====

void PianoRollGridWidget::startNoteAnim(Note* note, NoteAnim::Type type)
{
    NoteAnim anim;
    anim.startMs = m_animClock.elapsed();
    anim.progress = 0.0f;
    anim.type = type;
    m_noteAnims[note] = anim;
    
    if (!m_animTimer.isActive()) {
        m_animTimer.start();
    }
}

void PianoRollGridWidget::startBurstAnim(const QRectF& rect, const QColor& color)
{
    BurstAnimation::spawnBurst(rect, color, -1,
                               m_animClock.elapsed(),
                               BurstAnimation::pianoRollParams(),
                               m_burstGhosts, m_particles);
    if (!m_animTimer.isActive()) {
        m_animTimer.start();
    }
}

void PianoRollGridWidget::drawBurstEffects(QPainter& p)
{
    BurstAnimation::drawBurstEffects(p, m_burstGhosts, m_particles,
                                     BurstAnimation::pianoRollParams().roundedRadius);
}

void PianoRollGridWidget::tickAnimations()
{
    qint64 now = m_animClock.elapsed();
    bool anyActive = false;
    
    QList<Note*> toRemove;
    for (auto it = m_noteAnims.begin(); it != m_noteAnims.end(); ++it) {
        NoteAnim& anim = it.value();
        float durationMs = (anim.type == NoteAnim::PopIn) ? 250.0f : 300.0f;
        float elapsed = static_cast<float>(now - anim.startMs);
        anim.progress = qBound(0.0f, elapsed / durationMs, 1.0f);
        
        if (anim.progress >= 1.0f) {
            toRemove.append(it.key());
        } else {
            anyActive = true;
        }
    }
    
    for (Note* n : toRemove) {
        m_noteAnims.remove(n);
    }
    
    // バースト共通エンジンで更新
    if (BurstAnimation::tickBurst(now, BurstAnimation::pianoRollParams(),
                                   m_burstGhosts, m_particles)) {
        anyActive = true;
    }
    
    if (!anyActive && m_noteAnims.isEmpty()) {
        m_animTimer.stop();
    }
    
    update();
}
