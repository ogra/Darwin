#include "UndoCommands.h"
#include "Project.h"
#include "Track.h"
#include "Clip.h"
#include "Note.h"
#include "VST3PluginInstance.h"

#include <QDebug>

// ===== ノート追加コマンド =====

AddNoteCommand::AddNoteCommand(Clip* clip, int pitch, qint64 startTick,
                               qint64 durationTicks, int velocity,
                               QUndoCommand* parent)
    : QUndoCommand("Add Note", parent)
    , m_clip(clip)
    , m_note(nullptr)
    , m_pitch(pitch)
    , m_startTick(startTick)
    , m_durationTicks(durationTicks)
    , m_velocity(velocity)
    , m_firstRedo(true)
{
}

void AddNoteCommand::undo()
{
    if (m_note && m_clip) {
        m_clip->removeNote(m_note);
        // removeNote は deleteLater を呼ぶので、undo後に再利用不可
        // → redo時に新規作成する
        m_note = nullptr;
    }
}

void AddNoteCommand::redo()
{
    if (m_clip) {
        m_note = m_clip->addNote(m_pitch, m_startTick, m_durationTicks, m_velocity);
    }
}

// ===== ノート削除コマンド =====

RemoveNoteCommand::RemoveNoteCommand(Clip* clip, Note* note,
                                     QUndoCommand* parent)
    : QUndoCommand("Remove Note", parent)
    , m_clip(clip)
    , m_note(note)
    , m_pitch(note->pitch())
    , m_startTick(note->startTick())
    , m_durationTicks(note->durationTicks())
    , m_velocity(note->velocity())
    , m_ownsNote(false)
{
}

void RemoveNoteCommand::undo()
{
    if (m_clip) {
        // ノートを再作成して追加
        m_note = m_clip->addNote(m_pitch, m_startTick, m_durationTicks, m_velocity);
        m_ownsNote = false;
    }
}

void RemoveNoteCommand::redo()
{
    if (m_clip && m_note) {
        m_clip->removeNote(m_note);
        m_note = nullptr;
        m_ownsNote = false;
    }
}

// ===== ノート移動コマンド =====

MoveNoteCommand::MoveNoteCommand(Note* note, int newPitch,
                                 qint64 newStartTick, QUndoCommand* parent)
    : QUndoCommand("Move Note", parent)
    , m_note(note)
    , m_oldPitch(note->pitch())
    , m_oldStartTick(note->startTick())
    , m_newPitch(newPitch)
    , m_newStartTick(newStartTick)
{
}

void MoveNoteCommand::undo()
{
    m_note->setPitch(m_oldPitch);
    m_note->setStartTick(m_oldStartTick);
}

void MoveNoteCommand::redo()
{
    m_note->setPitch(m_newPitch);
    m_note->setStartTick(m_newStartTick);
}

bool MoveNoteCommand::mergeWith(const QUndoCommand* other)
{
    if (other->id() != id()) return false;
    auto* cmd = static_cast<const MoveNoteCommand*>(other);
    if (cmd->m_note != m_note) return false;
    m_newPitch = cmd->m_newPitch;
    m_newStartTick = cmd->m_newStartTick;
    return true;
}

// ===== ノートリサイズコマンド =====

ResizeNoteCommand::ResizeNoteCommand(Note* note, qint64 newDurationTicks,
                                     QUndoCommand* parent)
    : QUndoCommand("Resize Note", parent)
    , m_note(note)
    , m_oldDurationTicks(note->durationTicks())
    , m_newDurationTicks(newDurationTicks)
{
}

void ResizeNoteCommand::undo()
{
    m_note->setDurationTicks(m_oldDurationTicks);
}

void ResizeNoteCommand::redo()
{
    m_note->setDurationTicks(m_newDurationTicks);
}

bool ResizeNoteCommand::mergeWith(const QUndoCommand* other)
{
    if (other->id() != id()) return false;
    auto* cmd = static_cast<const ResizeNoteCommand*>(other);
    if (cmd->m_note != m_note) return false;
    m_newDurationTicks = cmd->m_newDurationTicks;
    return true;
}

// ===== ベロシティ変更コマンド =====

ChangeVelocityCommand::ChangeVelocityCommand(Note* note, int newVelocity,
                                             QUndoCommand* parent)
    : QUndoCommand("Change Velocity", parent)
    , m_note(note)
    , m_oldVelocity(note->velocity())
    , m_newVelocity(newVelocity)
{
}

void ChangeVelocityCommand::undo()
{
    m_note->setVelocity(m_oldVelocity);
}

void ChangeVelocityCommand::redo()
{
    m_note->setVelocity(m_newVelocity);
}

bool ChangeVelocityCommand::mergeWith(const QUndoCommand* other)
{
    if (other->id() != id()) return false;
    auto* cmd = static_cast<const ChangeVelocityCommand*>(other);
    if (cmd->m_note != m_note) return false;
    m_newVelocity = cmd->m_newVelocity;
    return true;
}

// ===== クリップ追加コマンド =====

AddClipCommand::AddClipCommand(Track* track, qint64 startTick,
                               qint64 durationTicks, QUndoCommand* parent)
    : QUndoCommand("Add Clip", parent)
    , m_track(track)
    , m_clip(nullptr)
    , m_startTick(startTick)
    , m_durationTicks(durationTicks)
    , m_firstRedo(true)
{
}

void AddClipCommand::undo()
{
    if (m_clip && m_track) {
        m_track->removeClip(m_clip);
        m_clip = nullptr;
    }
}

void AddClipCommand::redo()
{
    if (m_track) {
        m_clip = m_track->addClip(m_startTick, m_durationTicks);
    }
}

// ===== クリップ削除コマンド =====

RemoveClipCommand::RemoveClipCommand(Track* track, Clip* clip,
                                     QUndoCommand* parent)
    : QUndoCommand("Remove Clip", parent)
    , m_track(track)
    , m_clip(clip)
    , m_ownsClip(false)
{
    // クリップデータをシリアライズして保持（undo時に復元）
    m_clipData = clip->toJson();
}

void RemoveClipCommand::undo()
{
    if (m_track) {
        // シリアライズデータからクリップを復元
        Clip* restored = Clip::fromJson(m_clipData, m_track);
        m_track->clips(); // 直接追加
        // Track::addClip相当の処理
        // 注: Trackのclips listは公開されていないので addClip を使って再作成
        m_clip = m_track->addClip(restored->startTick(), restored->durationTicks());
        // ノートをコピー
        for (const Note* note : restored->notes()) {
            m_clip->addNote(note->pitch(), note->startTick(), note->durationTicks(), note->velocity());
        }
        delete restored;
        m_ownsClip = false;
    }
}

void RemoveClipCommand::redo()
{
    if (m_track && m_clip) {
        m_track->removeClip(m_clip);
        m_clip = nullptr;
        m_ownsClip = false;
    }
}

// ===== クリップ移動コマンド =====

MoveClipCommand::MoveClipCommand(Clip* clip, qint64 newStartTick,
                                 QUndoCommand* parent)
    : QUndoCommand("Move Clip", parent)
    , m_clip(clip)
    , m_oldStartTick(clip->startTick())
    , m_newStartTick(newStartTick)
{
}

void MoveClipCommand::undo()
{
    m_clip->setStartTick(m_oldStartTick);
}

void MoveClipCommand::redo()
{
    m_clip->setStartTick(m_newStartTick);
}

bool MoveClipCommand::mergeWith(const QUndoCommand* other)
{
    if (other->id() != id()) return false;
    auto* cmd = static_cast<const MoveClipCommand*>(other);
    if (cmd->m_clip != m_clip) return false;
    m_newStartTick = cmd->m_newStartTick;
    return true;
}

// ===== クリップリサイズコマンド =====

ResizeClipCommand::ResizeClipCommand(Clip* clip, qint64 newDurationTicks,
                                     QUndoCommand* parent)
    : QUndoCommand("Resize Clip", parent)
    , m_clip(clip)
    , m_oldDurationTicks(clip->durationTicks())
    , m_newDurationTicks(newDurationTicks)
{
}

void ResizeClipCommand::undo()
{
    m_clip->setDurationTicks(m_oldDurationTicks);
}

void ResizeClipCommand::redo()
{
    m_clip->setDurationTicks(m_newDurationTicks);
}

bool ResizeClipCommand::mergeWith(const QUndoCommand* other)
{
    if (other->id() != id()) return false;
    auto* cmd = static_cast<const ResizeClipCommand*>(other);
    if (cmd->m_clip != m_clip) return false;
    m_newDurationTicks = cmd->m_newDurationTicks;
    return true;
}

// ===== トラック追加コマンド =====

AddTrackCommand::AddTrackCommand(Project* project, const QString& name,
                                 QUndoCommand* parent)
    : QUndoCommand("Add Track", parent)
    , m_project(project)
    , m_track(nullptr)
    , m_name(name)
    , m_firstRedo(true)
{
}

void AddTrackCommand::undo()
{
    if (m_track && m_project) {
        m_project->removeTrack(m_track);
        m_track = nullptr;
    }
}

void AddTrackCommand::redo()
{
    if (m_project) {
        m_track = m_project->addTrack(m_name);
    }
}

// ===== トラック削除コマンド =====

RemoveTrackCommand::RemoveTrackCommand(Project* project, Track* track,
                                       QUndoCommand* parent)
    : QUndoCommand("Remove Track", parent)
    , m_project(project)
    , m_track(track)
    , m_trackIndex(project->tracks().indexOf(track))
    , m_ownsTrack(false)
{
    m_trackData = track->toJson();
}

void RemoveTrackCommand::undo()
{
    if (m_project) {
        // トラックをシリアライズデータから復元
        Track* restored = Track::fromJson(m_trackData, m_project);
        // プロジェクトにトラックを追加
        m_track = m_project->addTrack(restored->name());
        // 属性をコピー
        m_track->setInstrumentName(restored->instrumentName());
        m_track->setVisible(restored->isVisible());
        m_track->setMuted(restored->isMuted());
        m_track->setSolo(restored->isSolo());
        m_track->setVolume(restored->volume());
        m_track->setPan(restored->pan());
        m_track->setColor(restored->color());

        // プラグインを復元
        if (restored->hasPlugin()) {
            m_track->loadPlugin(restored->pluginInstance()->pluginPath());
        }

        // クリップを復元
        for (const Clip* srcClip : restored->clips()) {
            Clip* newClip = m_track->addClip(srcClip->startTick(), srcClip->durationTicks());
            for (const Note* note : srcClip->notes()) {
                newClip->addNote(note->pitch(), note->startTick(),
                                note->durationTicks(), note->velocity());
            }
        }

        delete restored;
        m_ownsTrack = false;
    }
}

void RemoveTrackCommand::redo()
{
    if (m_project && m_track) {
        m_project->removeTrack(m_track);
        m_track = nullptr;
    }
}
