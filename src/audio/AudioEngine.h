#pragma once

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <atomic>
#include <functional>
#include <vector>

// 前方宣言（Windows COM インターフェース）
#ifdef Q_OS_WIN
struct IMMDeviceEnumerator;
struct IMMDevice;
struct IAudioClient;
struct IAudioRenderClient;
typedef void* HANDLE;
#endif

/**
 * @brief WASAPI共有モードによるオーディオ出力エンジン
 *
 * 専用スレッドでオーディオバッファを処理し、
 * コールバック関数でバッファの内容を充填する。
 */
class AudioEngine : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief オーディオレンダリングコールバック
     * @param outputBuffer インターリーブされたfloat出力バッファ（書き込み先）
     * @param numFrames フレーム数
     * @param numChannels チャンネル数
     * @param sampleRate サンプルレート
     */
    using RenderCallback = std::function<void(float* outputBuffer, int numFrames, int numChannels, double sampleRate)>;

    explicit AudioEngine(QObject* parent = nullptr);
    ~AudioEngine() override;

    /** WASAPIデバイスを初期化 */
    bool initialize();

    /** オーディオレンダリングを開始 */
    bool start();

    /** オーディオレンダリングを停止 */
    void stop();

    /** エンジンが実行中か */
    bool isRunning() const { return m_running.load(); }

    /** 現在のサンプルレート */
    double sampleRate() const { return m_sampleRate; }

    /** 現在のチャンネル数 */
    int numChannels() const { return m_numChannels; }

    /** バッファサイズ（フレーム単位） */
    int bufferSize() const { return m_bufferSize; }

    /** レンダリングコールバックを設定 */
    void setRenderCallback(RenderCallback callback);

signals:
    void errorOccurred(const QString& message);

private:
    void renderThread();
    void cleanup();

    RenderCallback m_renderCallback;
    QMutex m_mutex;

    // WASAPI
#ifdef Q_OS_WIN
    IMMDeviceEnumerator* m_enumerator = nullptr;
    IMMDevice* m_device = nullptr;
    IAudioClient* m_audioClient = nullptr;
    IAudioRenderClient* m_renderClient = nullptr;
    HANDLE m_eventHandle = nullptr;
#endif

    // Audio format
    double m_sampleRate = 44100.0;
    int m_numChannels = 2;
    int m_bufferSize = 0;

    // Thread
    std::atomic<bool> m_running{false};
    QThread* m_thread = nullptr;

    bool m_initialized = false;
};
