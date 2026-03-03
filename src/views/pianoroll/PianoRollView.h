#pragma once
#include <QWidget>

class PianoRollGridWidget;
class VelocityLaneWidget;
class QScrollArea;
class QScrollBar;
class Project;
class Clip;

class PianoRollView : public QWidget
{
    Q_OBJECT
public:
    PianoRollView(QWidget *parent = nullptr);
    
    PianoRollGridWidget* gridWidget() const { return m_grid; }
    VelocityLaneWidget* velocityWidget() const { return m_velocityLane; }
    
    QScrollBar* horizontalScrollBar() const;
    
    void setProject(Project* project);
    void setActiveClip(Clip* clip);

private slots:
    void applyTheme();

private:
    Project* m_project;
    PianoRollGridWidget* m_grid;
    VelocityLaneWidget* m_velocityLane;
    
    QScrollArea* m_keysScrollArea;
    QScrollArea* m_gridScrollArea;
    QScrollArea* m_velocityScrollArea;
    QWidget* m_keysWidget = nullptr;   ///< 鍵盤ラベルの親ウィジェット（テーマ変更時に再スタイル適用）
};
