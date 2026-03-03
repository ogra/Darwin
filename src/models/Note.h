#pragma once

#include <QObject>
#include <QJsonObject>

/**
 * @brief MIDIノートを表すクラス
 */
class Note : public QObject
{
    Q_OBJECT

public:
    explicit Note(int pitch, qint64 startTick, qint64 durationTicks, int velocity = 100, QObject* parent = nullptr);
    ~Note() override = default;

    // Getters
    int pitch() const { return m_pitch; }
    qint64 startTick() const { return m_startTick; }
    qint64 durationTicks() const { return m_durationTicks; }
    int velocity() const { return m_velocity; }

    // Setters
    void setPitch(int pitch);
    void setStartTick(qint64 startTick);
    void setDurationTicks(qint64 durationTicks);
    void setVelocity(int velocity);

    // Utility
    qint64 endTick() const { return m_startTick + m_durationTicks; }
    QString pitchName() const;

    // シリアライズ
    QJsonObject toJson() const;
    static Note* fromJson(const QJsonObject& json, QObject* parent = nullptr);

signals:
    void changed();

private:
    int m_pitch;           // MIDI pitch (0-127)
    qint64 m_startTick;    // Start position in ticks
    qint64 m_durationTicks; // Duration in ticks
    int m_velocity;        // Velocity (0-127)
};
