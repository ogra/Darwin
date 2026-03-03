#pragma once

#include <QString>
#include <QList>
#include <QFile>
#include <QFileInfo>
#include <QDataStream>
#include <QDebug>
#include <cmath>

/**
 * @brief Standard MIDI File (SMF) パーサー
 *
 * .mid / .midi ファイルを読み込み、ノート情報のリストに変換する。
 * フォーマット0/1に対応。全トラックの Note On/Off をマージして返す。
 */
class MidiFileParser
{
public:
    /// パースされたノート1つ分のデータ
    struct MidiNote {
        int pitch;          // 0-127
        qint64 startTick;   // プロジェクト基準の tick
        qint64 durationTicks;
        int velocity;       // 0-127
        int channel;        // 0-15
    };

    /// パース結果
    struct ParseResult {
        bool success = false;
        QString errorMessage;
        QString fileName;           // 拡張子なしのファイル名
        int ticksPerQuarterNote = 480; // MIDIファイルのPPQN
        QList<MidiNote> notes;
    };

    /**
     * @brief MIDIファイルをパースしてノート情報を取得する
     * @param filePath MIDIファイルパス
     * @param projectPPQN プロジェクトのPPQN（デフォルト480）
     * @return パース結果
     */
    static ParseResult parse(const QString& filePath, int projectPPQN = 480)
    {
        ParseResult result;
        result.fileName = QFileInfo(filePath).completeBaseName();

        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            result.errorMessage = QStringLiteral("ファイルを開けません: %1").arg(filePath);
            return result;
        }

        QByteArray data = file.readAll();
        file.close();

        if (data.size() < 14) {
            result.errorMessage = QStringLiteral("ファイルが小さすぎます");
            return result;
        }

        int pos = 0;

        // ── ヘッダーチャンク (MThd) の読み取り ──
        if (data.mid(pos, 4) != "MThd") {
            result.errorMessage = QStringLiteral("MIDIヘッダーが見つかりません");
            return result;
        }
        pos += 4;

        quint32 headerLength = readUint32(data, pos); pos += 4;
        if (headerLength < 6) {
            result.errorMessage = QStringLiteral("ヘッダー長が不正です");
            return result;
        }

        quint16 format = readUint16(data, pos); pos += 2;
        quint16 numTracks = readUint16(data, pos); pos += 2;
        quint16 division = readUint16(data, pos); pos += 2;

        // SMPTE形式は非対応（ティック/拍子のみ対応）
        if (division & 0x8000) {
            result.errorMessage = QStringLiteral("SMPTEタイムベースは非対応です");
            return result;
        }

        result.ticksPerQuarterNote = division;

        // ヘッダーの残りバイトをスキップ
        pos = 8 + static_cast<int>(headerLength);

        Q_UNUSED(format)

        // ── トラックチャンクの読み取り ──
        // 全トラックのノートを一つにマージ
        struct PendingNote {
            int pitch;
            qint64 startTick;
            int velocity;
            int channel;
        };

        for (int t = 0; t < numTracks && pos < data.size(); ++t) {
            if (pos + 8 > data.size()) break;
            if (data.mid(pos, 4) != "MTrk") {
                // トラックチャンクでなければスキップを試みる
                result.errorMessage = QStringLiteral("トラックチャンクが不正です (トラック %1)").arg(t);
                return result;
            }
            pos += 4;

            quint32 trackLength = readUint32(data, pos); pos += 4;
            int trackEnd = pos + static_cast<int>(trackLength);
            if (trackEnd > data.size()) trackEnd = data.size();

            // アクティブノートの追跡 (channel*128 + pitch をキーにする)
            QMap<int, PendingNote> activeNotes;
            qint64 absoluteTick = 0;
            quint8 runningStatus = 0;

            while (pos < trackEnd) {
                // デルタタイムの読み取り
                qint64 delta = readVariableLength(data, pos);
                absoluteTick += delta;

                if (pos >= trackEnd) break;

                quint8 statusByte = static_cast<quint8>(data[pos]);

                // メタイベント
                if (statusByte == 0xFF) {
                    pos++; // FF
                    if (pos >= trackEnd) break;
                    pos++; // メタタイプ
                    if (pos >= trackEnd) break;
                    int metaLen = static_cast<int>(readVariableLength(data, pos));
                    pos += metaLen;
                    continue;
                }

                // SysExイベント
                if (statusByte == 0xF0 || statusByte == 0xF7) {
                    pos++; // F0 or F7
                    int sysexLen = static_cast<int>(readVariableLength(data, pos));
                    pos += sysexLen;
                    continue;
                }

                // MIDIイベント
                quint8 status;
                quint8 data1;

                if (statusByte & 0x80) {
                    // 新しいステータスバイト
                    status = statusByte;
                    runningStatus = status;
                    pos++;
                    if (pos >= trackEnd) break;
                    data1 = static_cast<quint8>(data[pos]);
                    pos++;
                } else {
                    // ランニングステータス
                    status = runningStatus;
                    data1 = statusByte;
                    pos++;
                }

                quint8 eventType = status & 0xF0;
                int channel = status & 0x0F;

                // 2バイト目が必要なイベント
                quint8 data2 = 0;
                if (eventType != 0xC0 && eventType != 0xD0) {
                    if (pos >= trackEnd) break;
                    data2 = static_cast<quint8>(data[pos]);
                    pos++;
                }

                // Note On (velocity > 0)
                if (eventType == 0x90 && data2 > 0) {
                    int key = channel * 128 + data1;
                    // 既にアクティブなら閉じる
                    if (activeNotes.contains(key)) {
                        PendingNote& pending = activeNotes[key];
                        qint64 dur = absoluteTick - pending.startTick;
                        if (dur > 0) {
                            MidiNote note;
                            note.pitch = pending.pitch;
                            note.startTick = scaleTick(pending.startTick, division, projectPPQN);
                            note.durationTicks = scaleTick(dur, division, projectPPQN);
                            note.velocity = pending.velocity;
                            note.channel = pending.channel;
                            result.notes.append(note);
                        }
                        activeNotes.remove(key);
                    }
                    PendingNote pending;
                    pending.pitch = data1;
                    pending.startTick = absoluteTick;
                    pending.velocity = data2;
                    pending.channel = channel;
                    activeNotes[key] = pending;
                }
                // Note Off、または velocity=0 の Note On
                else if (eventType == 0x80 || (eventType == 0x90 && data2 == 0)) {
                    int key = channel * 128 + data1;
                    if (activeNotes.contains(key)) {
                        PendingNote& pending = activeNotes[key];
                        qint64 dur = absoluteTick - pending.startTick;
                        if (dur > 0) {
                            MidiNote note;
                            note.pitch = pending.pitch;
                            note.startTick = scaleTick(pending.startTick, division, projectPPQN);
                            note.durationTicks = scaleTick(dur, division, projectPPQN);
                            note.velocity = pending.velocity;
                            note.channel = pending.channel;
                            result.notes.append(note);
                        }
                        activeNotes.remove(key);
                    }
                }
                // 他のイベント（CC, PitchBend等）はノートに影響しないためスキップ
            }

            // トラック終了時に残っているアクティブノートを閉じる
            for (auto it = activeNotes.begin(); it != activeNotes.end(); ++it) {
                const PendingNote& pending = it.value();
                qint64 dur = absoluteTick - pending.startTick;
                if (dur > 0) {
                    MidiNote note;
                    note.pitch = pending.pitch;
                    note.startTick = scaleTick(pending.startTick, division, projectPPQN);
                    note.durationTicks = scaleTick(dur, division, projectPPQN);
                    note.velocity = pending.velocity;
                    note.channel = pending.channel;
                    result.notes.append(note);
                }
            }

            pos = trackEnd;
        }

        // スタートtickでソート
        std::sort(result.notes.begin(), result.notes.end(),
                  [](const MidiNote& a, const MidiNote& b) {
                      return a.startTick < b.startTick;
                  });

        result.success = true;
        return result;
    }

private:
    /// ビッグエンディアン 32bit 読み取り
    static quint32 readUint32(const QByteArray& data, int pos)
    {
        if (pos + 4 > data.size()) return 0;
        return (static_cast<quint8>(data[pos]) << 24)
             | (static_cast<quint8>(data[pos + 1]) << 16)
             | (static_cast<quint8>(data[pos + 2]) << 8)
             | static_cast<quint8>(data[pos + 3]);
    }

    /// ビッグエンディアン 16bit 読み取り
    static quint16 readUint16(const QByteArray& data, int pos)
    {
        if (pos + 2 > data.size()) return 0;
        return (static_cast<quint8>(data[pos]) << 8)
             | static_cast<quint8>(data[pos + 1]);
    }

    /// MIDI可変長数値の読み取り（posを進める）
    static qint64 readVariableLength(const QByteArray& data, int& pos)
    {
        qint64 value = 0;
        for (int i = 0; i < 4 && pos < data.size(); ++i) {
            quint8 byte = static_cast<quint8>(data[pos]);
            pos++;
            value = (value << 7) | (byte & 0x7F);
            if (!(byte & 0x80)) break;
        }
        return value;
    }

    /// MIDIファイルのティックをプロジェクトのPPQNに変換
    static qint64 scaleTick(qint64 midiTick, int midiPPQN, int projectPPQN)
    {
        if (midiPPQN == projectPPQN) return midiTick;
        return static_cast<qint64>(std::round(
            static_cast<double>(midiTick) * projectPPQN / midiPPQN));
    }
};
