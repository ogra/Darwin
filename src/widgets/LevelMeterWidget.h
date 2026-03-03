#pragma once

#include <QWidget>

class LevelMeterWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LevelMeterWidget(QWidget *parent = nullptr);
    
    void setLevel(float level);

protected:
    void paintEvent(QPaintEvent *event) override;
    QSize sizeHint() const override;

private:
    float m_level; 
    
    // Smooth decay values
    float m_displayLevel;
    
    // Peak hold
    float m_peakHold;
};
