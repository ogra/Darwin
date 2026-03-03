#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>

/**
 * @brief VST3プラグイン情報
 */
struct VST3PluginInfo {
    QString name;           // プラグイン名
    QString path;           // ファイルパス
    QString vendor;         // ベンダー名
    QString version;        // バージョン
    bool isInstrument;      // インストゥルメントかどうか
    bool isEffect;          // エフェクトかどうか
    QString category;       // カテゴリ (Instrument, Fx, etc.)
    int usageCount;         // 使用回数
    
    VST3PluginInfo() : isInstrument(false), isEffect(false), usageCount(0) {}
};

/**
 * @brief VST3プラグインをスキャンするクラス
 */
class VST3Scanner : public QObject
{
    Q_OBJECT

public:
    explicit VST3Scanner(QObject* parent = nullptr);
    
    /**
     * @brief VST3プラグインをスキャン（並列）
     * @param instrumentsOnly trueの場合、インストゥルメントのみを返す
     * @return 見つかったプラグインのリスト
     */
    QVector<VST3PluginInfo> scan(bool instrumentsOnly = true);
    
    /**
     * @brief スキャンパスを追加
     */
    void addScanPath(const QString& path);
    
    /**
     * @brief スキャンパスを取得
     */
    QStringList scanPaths() const;
    
    /**
     * @brief スキャンパスを設定
     */
    void setScanPaths(const QStringList& paths);
    
    /**
     * @brief デフォルトのスキャンパスを取得
     */
    static QStringList defaultScanPaths();

signals:
    void scanProgress(int current, int total);
    void pluginFound(const VST3PluginInfo& info);
    void scanComplete(int count);

private:
    QVector<VST3PluginInfo> scanDirectory(const QString& dir);
    VST3PluginInfo parseVST3Bundle(const QString& path);
    bool parseModuleInfo(const QString& bundlePath, VST3PluginInfo& info);
    
    QStringList m_scanPaths;
};
