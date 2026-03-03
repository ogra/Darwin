#pragma once

#include <QObject>
#include <QString>
#include <QFile>
#include <cstdint>

class Project;
class PlaybackController;

/**
 * @brief オーディオファイルのエクスポートを行うクラス
 *
 * プロジェクトのMIDIデータをオフラインでプラグイン処理し、
 * WAVファイルとして書き出す。
 */
class AudioExporter : public QObject
{
    Q_OBJECT

public:
    explicit AudioExporter(QObject* parent = nullptr);
    ~AudioExporter() override = default;

    /**
     * @brief WAVファイルにエクスポート
     * @param project エクスポート対象のプロジェクト
     * @param filePath 出力先ファイルパス
     * @param sampleRate サンプルレート（デフォルト44100）
     * @param bitDepth ビット深度（16 or 24, デフォルト24）
     * @return 成功時true
     */
    bool exportToWav(Project* project, const QString& filePath,
                     double sampleRate = 44100.0, int bitDepth = 24);

    /**
     * @brief WAVファイルにエクスポート（範囲指定）
     * @param startTick 開始ティック（0=先頭）
     * @param endTick 終了ティック（-1=自動）
     */
    bool exportToWav(Project* project, const QString& filePath,
                     qint64 startTick, qint64 endTick,
                     double sampleRate = 44100.0, int bitDepth = 24);

signals:
    /** エクスポート進捗 (0.0 - 1.0) */
    void progressChanged(double progress);

    /** エクスポート完了 */
    void exportFinished(bool success, const QString& message);

private:
    /** WAVヘッダーを書き込み */
    bool writeWavHeader(QFile& file, int numChannels, double sampleRate,
                        int bitDepth, uint32_t numSamples);

    /** WAVヘッダーのサイズフィールドを更新 */
    bool updateWavHeader(QFile& file, uint32_t dataSize);
};
