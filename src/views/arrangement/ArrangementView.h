#pragma once
#include <QWidget>

#include <QMap>
#include <QPoint>
#include <QTimer>
#include <QPushButton>

class Project;
class Track;
class QVBoxLayout;
class ArrangementGridWidget;
class QGraphicsOpacityEffect;
class QPropertyAnimation;

class QScrollArea;
class QScrollBar;


class IconButton;

class ArrangementView : public QWidget
{
    Q_OBJECT
public:
    ArrangementView(QWidget *parent = nullptr);
    
    void setProject(Project* project);
    ArrangementGridWidget* gridWidget() const { return m_grid; }
    class TimelineWidget* timelineWidget() const { return reinterpret_cast<TimelineWidget*>(m_timeline); }
    QScrollBar* horizontalScrollBar() const;
    
    Track* selectedTrack() const { return m_selectedTrack; }
    void selectTrack(Track* track);

signals:
    void trackSelected(Track* track);

public slots:
    void onTrackAdded(Track* track);
    void onTrackRemoved(Track* track);
    void rebuildHeaders();
    
protected:
    void paintEvent(QPaintEvent *event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
    
private:
    void startLongPressTimer(Track* track, const QPoint& globalPos);
    void cancelLongPress();
    void beginTrackDrag(Track* track);
    void updateTrackDrag(const QPoint& globalPos);
    void finishTrackDrag();
    QWidget* createTrackHeader(Track* track);
    QWidget* createFolderHeader(Track* folder);

    ArrangementGridWidget* m_grid;
    QScrollArea* m_gridScroll;
    QScrollArea* m_headerScroll = nullptr;
    QWidget*     m_headerContent   = nullptr;
    QWidget*     m_topContainer    = nullptr;   ///< +Trackボタンを含む上部バー
    QWidget*     m_cornerSpacer    = nullptr;   ///< コーナースペーサー
    QPushButton* m_addTrackBtn     = nullptr;
    QPushButton* m_addFolderBtn    = nullptr;
    Project* m_project;
    QVBoxLayout* m_headersLayout;
    QWidget* m_scrollContent;
    Track* m_selectedTrack;
    QMap<Track*, QWidget*> m_trackHeaders;
    QWidget* m_timeline;
    IconButton* m_snapBtn = nullptr;
    bool m_initializing = false;  // setProject初期化中はアニメーションをスキップ

    // ── トラックD&Dリオーダー ──
    QTimer m_longPressTimer;
    Track* m_longPressTrack = nullptr;
    QPoint m_longPressStartPos;
    
    bool m_isDraggingTrack = false;
    Track* m_dragTrack = nullptr;
    QWidget* m_dragGhost = nullptr;          // 浮遊するゴースト
    int m_dragInsertIndex = -1;              // 挿入先インデックス
    QWidget* m_dropIndicator = nullptr;      // 挿入ライン表示
    QPoint m_dragOffset;                     // ヘッダ上でのオフセット
    Track* m_dragTargetFolder = nullptr;     // フォルダドロップ先
    QWidget* m_folderHighlight = nullptr;    // フォルダハイライト表示
};
