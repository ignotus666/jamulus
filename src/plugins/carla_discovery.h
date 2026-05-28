#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>
#include <QThread>
#include <QMutex>

#include <QMap>

struct PluginDiscoveryResult {
    QString name;
    QString maker;
    int     category;
    int     hints;
    int     binaryType;
    int     pluginType;
    QString filename;
    QString label;
    uint64_t uniqueId;
    int     audioIns;
    int     audioOuts;
    int     midiIns;
    int     midiOuts;
};

class CCarlaDiscoveryWorker : public QObject {
    Q_OBJECT
public:
    CCarlaDiscoveryWorker(const QMap<int, QStringList>& paths, QObject* parent = nullptr);
    
public slots:
    void doScan();
    void cancelScan();
    void pluginFoundSlot(const PluginDiscoveryResult& result);

signals:
    void pluginFound(const PluginDiscoveryResult& result);
    void progressUpdated(int percent);
    void scanFinished(const QList<PluginDiscoveryResult>& results);

private:
    QMap<int, QStringList> pathsToScan;
    bool bCancel;
    QMutex mutex;
};

class CCarlaDiscovery : public QObject {
    Q_OBJECT
public:
    CCarlaDiscovery(QObject* parent = nullptr);
    ~CCarlaDiscovery();

    void startScan();
    void cancelScan();
    bool isScanning() const { return bScanning; }

    QList<PluginDiscoveryResult> getCachedPlugins() const { return cachedResults; }
    void saveCache();
    void loadCache();

    QStringList getScanPaths(int pluginType) const;
    void setScanPaths(int pluginType, const QStringList& paths);
    QStringList getDefaultPaths(int pluginType) const;

signals:
    void pluginFound(const PluginDiscoveryResult& result);
    void progressUpdated(int percent);
    void scanFinished(const QList<PluginDiscoveryResult>& results);

private slots:
    void onPluginFound(const PluginDiscoveryResult& result);
    void onWorkerFinished(const QList<PluginDiscoveryResult>& results);

private:
    bool bScanning;
    QThread* workerThread;
    CCarlaDiscoveryWorker* worker;
    QList<PluginDiscoveryResult> cachedResults;
    
    QString getCacheFilePath() const;
};
