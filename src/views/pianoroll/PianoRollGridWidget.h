#pragma once
#include <QWidget>
#include <QTimer>
#include <QMap>
#include <QElapsedTimer>
#include "common/BurstAnimationHelper.h"

class Clip;
class Note;
class Project;
class QScrollArea;

class PianoRollGridWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PianoRollGridWidget(QWidget *parent = nullptr);
    
    void setScrollArea(QScrollArea* scrollArea) { m_scrollArea = scrollArea; }
    
signals:
    void requestSeek(qint64 tickPosition);

public slots:
    void setPlayheadPosition(qint64 tickPosition);
    void setActiveClip(Clip* clip);
    void setProject(Project* project);
    
private slots:
    void onLongPressTimeout();
    
protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    QSize sizeHint() const override;
    
private:
    double pixelsPerTick() const;
    qint64 gridQuantize() const;
    /** tickをグリッドにスナップ（Project::gridSnapEnabled チェック付き） */
    qint64 snapTick(qint64 tick) const;
    int computeRequiredWidth() const;
    void updateDynamicSize();
    void ensurePlayheadVisible();
    
    qint64 m_playheadPosition;
    Clip* m_activeClip;
    Project* m_project;
    QScrollArea* m_scrollArea = nullptr;
    
    double m_zoomLevel;
    
    // Interaction state
    Note* m_selectedNote;
    QList<Note*> m_selectedNotes;   // 複数選択
    bool m_isDragging;
    bool m_isDraggingPlayhead = false;
    bool m_isResizing;              // 右端リサイズ
    bool m_isResizingLeft;          // 左端リサイズ
    bool m_isZooming;
    bool m_longPressActive;
    QTimer m_longPressTimer;
    QPoint m_lastMousePos;
    QPoint m_pressPos;
    
    // 範囲選択（ラバーバンド）
    bool m_isRubberBanding = false;
    QPoint m_rubberBandOrigin;
    QRect m_rubberBandRect;

    // ===== アニメーション =====
    struct NoteAnim {
        qint64 startMs;
        float progress;
        enum Type { PopIn, SelectGlow } type;
    };
    QMap<Note*, NoteAnim> m_noteAnims;
    QElapsedTimer m_animClock;
    QTimer m_animTimer;
    Note* m_prevSelectedNote;

    // バースト共通エンジン
    QList<BurstAnimation::BurstGhost> m_burstGhosts;
    QList<BurstAnimation::Particle>   m_particles;

    void startNoteAnim(Note* note, NoteAnim::Type type);
    void startBurstAnim(const QRectF& rect, const QColor& color);
    void tickAnimations();
    void drawBurstEffects(QPainter& p);
    
    static constexpr int MIN_BARS = 64;
};
