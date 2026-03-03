#include "Note.h"
#include <QJsonObject>

Note::Note(int pitch, qint64 startTick, qint64 durationTicks, int velocity, QObject* parent)
    : QObject(parent)
    , m_pitch(pitch)
    , m_startTick(startTick)
    , m_durationTicks(durationTicks)
    , m_velocity(velocity)
{
}

void Note::setPitch(int pitch)
{
    if (m_pitch != pitch) {
        m_pitch = qBound(0, pitch, 127);
        emit changed();
    }
}

void Note::setStartTick(qint64 startTick)
{
    if (m_startTick != startTick) {
        m_startTick = qMax(0LL, startTick);
        emit changed();
    }
}

void Note::setDurationTicks(qint64 durationTicks)
{
    if (m_durationTicks != durationTicks) {
        m_durationTicks = qMax(1LL, durationTicks);
        emit changed();
    }
}

void Note::setVelocity(int velocity)
{
    if (m_velocity != velocity) {
        m_velocity = qBound(0, velocity, 127);
        emit changed();
    }
}

QString Note::pitchName() const
{
    static const QStringList noteNames = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    int octave = (m_pitch / 12) - 1;
    int noteIndex = m_pitch % 12;
    return noteNames[noteIndex] + QString::number(octave);
}

QJsonObject Note::toJson() const
{
    QJsonObject json;
    json["pitch"] = m_pitch;
    json["startTick"] = m_startTick;
    json["durationTicks"] = m_durationTicks;
    json["velocity"] = m_velocity;
    return json;
}

Note* Note::fromJson(const QJsonObject& json, QObject* parent)
{
    int pitch = json["pitch"].toInt(60);
    qint64 startTick = static_cast<qint64>(json["startTick"].toDouble(0));
    qint64 durationTicks = static_cast<qint64>(json["durationTicks"].toDouble(480));
    int velocity = json["velocity"].toInt(100);
    return new Note(pitch, startTick, durationTicks, velocity, parent);
}
