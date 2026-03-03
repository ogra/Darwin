#pragma once

#include <QUndoCommand>
#include <QList>
#include <QJsonObject>
#include <QJsonArray>
#include <QColor>

class Project;
class Track;
class Clip;
class Note;

/**
 * @brief ノート追加コマンド
 */
class AddNoteCommand : public QUndoCommand
{
public:
    AddNoteCommand(Clip* clip, int pitch, qint64 startTick, qint64 durationTicks,
                   int velocity, QUndoCommand* parent = nullptr);
    void undo() override;
    void redo() override;

    Note* createdNote() const { return m_note; }

private:
    Clip* m_clip;
    Note* m_note;
    int m_pitch;
    qint64 m_startTick;
    qint64 m_durationTicks;
    int m_velocity;
    bool m_firstRedo;
};

/**
 * @brief ノート削除コマンド
 */
class RemoveNoteCommand : public QUndoCommand
{
public:
    RemoveNoteCommand(Clip* clip, Note* note, QUndoCommand* parent = nullptr);
    void undo() override;
    void redo() override;

private:
    Clip* m_clip;
    Note* m_note;
    int m_pitch;
    qint64 m_startTick;
    qint64 m_durationTicks;
    int m_velocity;
    bool m_ownsNote;
};

/**
 * @brief ノート移動コマンド
 */
class MoveNoteCommand : public QUndoCommand
{
public:
    MoveNoteCommand(Note* note, int newPitch, qint64 newStartTick,
                    QUndoCommand* parent = nullptr);
    void undo() override;
    void redo() override;
    int id() const override { return 1001; } // マージ可能
    bool mergeWith(const QUndoCommand* other) override;

private:
    Note* m_note;
    int m_oldPitch;
    qint64 m_oldStartTick;
    int m_newPitch;
    qint64 m_newStartTick;
};

/**
 * @brief ノートリサイズコマンド
 */
class ResizeNoteCommand : public QUndoCommand
{
public:
    ResizeNoteCommand(Note* note, qint64 newDurationTicks,
                      QUndoCommand* parent = nullptr);
    void undo() override;
    void redo() override;
    int id() const override { return 1002; }
    bool mergeWith(const QUndoCommand* other) override;

private:
    Note* m_note;
    qint64 m_oldDurationTicks;
    qint64 m_newDurationTicks;
};

/**
 * @brief ノートベロシティ変更コマンド
 */
class ChangeVelocityCommand : public QUndoCommand
{
public:
    ChangeVelocityCommand(Note* note, int newVelocity,
                          QUndoCommand* parent = nullptr);
    void undo() override;
    void redo() override;
    int id() const override { return 1003; }
    bool mergeWith(const QUndoCommand* other) override;

private:
    Note* m_note;
    int m_oldVelocity;
    int m_newVelocity;
};

/**
 * @brief クリップ追加コマンド
 */
class AddClipCommand : public QUndoCommand
{
public:
    AddClipCommand(Track* track, qint64 startTick, qint64 durationTicks,
                   QUndoCommand* parent = nullptr);
    void undo() override;
    void redo() override;

    Clip* createdClip() const { return m_clip; }

private:
    Track* m_track;
    Clip* m_clip;
    qint64 m_startTick;
    qint64 m_durationTicks;
    bool m_firstRedo;
};

/**
 * @brief クリップ削除コマンド
 */
class RemoveClipCommand : public QUndoCommand
{
public:
    RemoveClipCommand(Track* track, Clip* clip,
                      QUndoCommand* parent = nullptr);
    void undo() override;
    void redo() override;

private:
    Track* m_track;
    Clip* m_clip;
    QJsonObject m_clipData; // シリアライズしたクリップデータ
    bool m_ownsClip;
};

/**
 * @brief クリップ移動コマンド
 */
class MoveClipCommand : public QUndoCommand
{
public:
    MoveClipCommand(Clip* clip, qint64 newStartTick,
                    QUndoCommand* parent = nullptr);
    void undo() override;
    void redo() override;
    int id() const override { return 1004; }
    bool mergeWith(const QUndoCommand* other) override;

private:
    Clip* m_clip;
    qint64 m_oldStartTick;
    qint64 m_newStartTick;
};

/**
 * @brief クリップリサイズコマンド
 */
class ResizeClipCommand : public QUndoCommand
{
public:
    ResizeClipCommand(Clip* clip, qint64 newDurationTicks,
                      QUndoCommand* parent = nullptr);
    void undo() override;
    void redo() override;
    int id() const override { return 1005; }
    bool mergeWith(const QUndoCommand* other) override;

private:
    Clip* m_clip;
    qint64 m_oldDurationTicks;
    qint64 m_newDurationTicks;
};

/**
 * @brief トラック追加コマンド
 */
class AddTrackCommand : public QUndoCommand
{
public:
    AddTrackCommand(Project* project, const QString& name,
                    QUndoCommand* parent = nullptr);
    void undo() override;
    void redo() override;

    Track* createdTrack() const { return m_track; }

private:
    Project* m_project;
    Track* m_track;
    QString m_name;
    bool m_firstRedo;
};

/**
 * @brief トラック削除コマンド
 */
class RemoveTrackCommand : public QUndoCommand
{
public:
    RemoveTrackCommand(Project* project, Track* track,
                       QUndoCommand* parent = nullptr);
    void undo() override;
    void redo() override;

private:
    Project* m_project;
    Track* m_track;
    QJsonObject m_trackData;
    int m_trackIndex;
    bool m_ownsTrack;
};
