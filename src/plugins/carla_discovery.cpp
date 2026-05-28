#include "carla_discovery.h"
#include <CarlaUtils.h>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>
#include <QSettings>
#include <vector>

#if defined(__x86_64__) || defined(__aarch64__)
# define BINARY_NATIVE CarlaBackend::BINARY_POSIX64
#else
# define BINARY_NATIVE CarlaBackend::BINARY_POSIX32
#endif

// ---------------------------------------------------------------------------
// C Discovery Callback
// ---------------------------------------------------------------------------
static void discoveryCallback(void* ptr, const CarlaPluginDiscoveryInfo* info, const char*) {
    auto worker = static_cast<CCarlaDiscoveryWorker*>(ptr);
    if (info) {
        PluginDiscoveryResult res;
        res.name = QString::fromUtf8(info->metadata.name);
        res.maker = QString::fromUtf8(info->metadata.maker);
        res.category = (int)info->metadata.category;
        res.hints = (int)info->metadata.hints;
        res.binaryType = (int)info->btype;
        res.pluginType = (int)info->ptype;
        res.filename = QString::fromUtf8(info->filename);
        res.label = QString::fromUtf8(info->label);
        res.uniqueId = info->uniqueId;
        res.audioIns = (int)info->io.audioIns;
        res.audioOuts = (int)info->io.audioOuts;
        res.midiIns = (int)info->io.midiIns;
        res.midiOuts = (int)info->io.midiOuts;
        
        emit worker->pluginFound(res);
    }
}

// ---------------------------------------------------------------------------
// CCarlaDiscoveryWorker Implementation
// ---------------------------------------------------------------------------
CCarlaDiscoveryWorker::CCarlaDiscoveryWorker(const QMap<int, QStringList>& paths, QObject* parent)
    : QObject(parent), pathsToScan(paths), bCancel(false) {}

void CCarlaDiscoveryWorker::cancelScan() {
    QMutexLocker locker(&mutex);
    bCancel = true;
}

// Extra declaration to allow slot collection in worker
namespace {
    struct FormatScan {
        CarlaBackend::PluginType type;
        QString                  pathEnvName;
        QString                  defaultPaths;
    };
}

void CCarlaDiscoveryWorker::doScan() {
    bCancel = false;
    
    std::vector<FormatScan> formats = {
        { CarlaBackend::PLUGIN_LV2, "LV2_PATH", "/usr/lib/lv2:/usr/local/lib/lv2:~/.lv2" },
        { CarlaBackend::PLUGIN_VST3, "VST3_PATH", "/usr/lib/vst3:/usr/local/lib/vst3:~/.vst3" },
        { CarlaBackend::PLUGIN_VST2, "VST2_PATH", "/usr/lib/vst:/usr/local/lib/vst:~/.vst:/usr/lib/lxvst" },
        { CarlaBackend::PLUGIN_CLAP, "CLAP_PATH", "/usr/lib/clap:/usr/local/lib/clap:~/.clap" }
    };
    
    const char* tool = "/usr/lib/carla/carla-discovery-native";
    
    // Check if tool actually exists
    if (!QFile::exists(tool)) {
        qWarning() << "carla_discovery: scanning tool NOT found at" << tool;
        emit progressUpdated(100);
        emit scanFinished(QList<PluginDiscoveryResult>());
        return;
    }
    
    int totalFormats = formats.size();
    for (int i = 0; i < totalFormats; ++i) {
        {
            QMutexLocker locker(&mutex);
            if (bCancel) break;
        }
        
        emit progressUpdated((i * 100) / totalFormats);
        
        FormatScan& fmt = formats[i];
        
        // Retrieve paths from the QMap
        QStringList listToScan = pathsToScan.value(fmt.type);
        if (listToScan.isEmpty()) {
            // Fallback to default
            listToScan = fmt.defaultPaths.split(':', QString::SkipEmptyParts);
        }
        
        // Resolve ~ to QDir::homePath()
        QStringList resolvedPaths;
        for (QString p : listToScan) {
            p = p.trimmed();
            if (p.startsWith("~/")) {
                p = QDir::homePath() + p.mid(1);
            } else if (p == "~") {
                p = QDir::homePath();
            }
            if (!p.isEmpty()) {
                resolvedPaths.append(p);
            }
        }
        
        QString scanPath = resolvedPaths.join(":");
        qDebug() << "carla_discovery: scanning format" << fmt.type << "with paths:" << scanPath;
        
        CarlaPluginDiscoveryHandle handle = carla_plugin_discovery_start(
            tool,
            BINARY_NATIVE,
            fmt.type,
            scanPath.toUtf8().constData(),
            discoveryCallback,
            nullptr, // checkCacheCb
            this
        );
        
        if (handle) {
            while (true) {
                {
                    QMutexLocker locker(&mutex);
                    if (bCancel) {
                        carla_plugin_discovery_stop(handle);
                        break;
                    }
                }
                
                bool running = carla_plugin_discovery_idle(handle);
                if (!running) {
                    carla_plugin_discovery_stop(handle);
                    break;
                }
                
                QThread::msleep(5);
            }
        }
    }
    
    emit progressUpdated(100);
    emit scanFinished(QList<PluginDiscoveryResult>());
}

// Map the C callback to QMetaObject invocation
extern "C" {
    void CCarlaDiscoveryWorker_pluginFoundSlot_internal(CCarlaDiscoveryWorker* worker, const PluginDiscoveryResult& res) {
        emit worker->pluginFound(res);
    }
}

void CCarlaDiscoveryWorker::pluginFoundSlot(const PluginDiscoveryResult& result) {
    emit pluginFound(result);
}

// Standard Qt QObject compilation via qmake

// ---------------------------------------------------------------------------
// CCarlaDiscovery Implementation
// ---------------------------------------------------------------------------
CCarlaDiscovery::CCarlaDiscovery(QObject* parent)
    : QObject(parent), bScanning(false), workerThread(nullptr), worker(nullptr) {
    qRegisterMetaType<PluginDiscoveryResult>("PluginDiscoveryResult");
    qRegisterMetaType<QList<PluginDiscoveryResult>>("QList<PluginDiscoveryResult>");
    loadCache();
}

CCarlaDiscovery::~CCarlaDiscovery() {
    cancelScan();
}

QString CCarlaDiscovery::getCacheFilePath() const {
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(path);
    return path + "/carla_plugins_cache.json";
}

void CCarlaDiscovery::startScan() {
    if (bScanning) return;
    
    bScanning = true;
    cachedResults.clear();
    
    // Build path mapping to pass to worker
    QMap<int, QStringList> paths;
    paths[CarlaBackend::PLUGIN_LV2] = getScanPaths(CarlaBackend::PLUGIN_LV2);
    paths[CarlaBackend::PLUGIN_VST2] = getScanPaths(CarlaBackend::PLUGIN_VST2);
    paths[CarlaBackend::PLUGIN_VST3] = getScanPaths(CarlaBackend::PLUGIN_VST3);
    paths[CarlaBackend::PLUGIN_CLAP] = getScanPaths(CarlaBackend::PLUGIN_CLAP);
    
    workerThread = new QThread();
    worker = new CCarlaDiscoveryWorker(paths);
    worker->moveToThread(workerThread);
    
    // Wire up slots and signals
    connect(workerThread, &QThread::started, worker, &CCarlaDiscoveryWorker::doScan);
    connect(worker, &CCarlaDiscoveryWorker::pluginFound, this, &CCarlaDiscovery::onPluginFound);
    connect(worker, &CCarlaDiscoveryWorker::progressUpdated, this, &CCarlaDiscovery::progressUpdated);
    connect(worker, &CCarlaDiscoveryWorker::scanFinished, this, &CCarlaDiscovery::onWorkerFinished);
    
    workerThread->start();
}

void CCarlaDiscovery::cancelScan() {
    if (!bScanning) return;
    
    if (worker) {
        worker->cancelScan();
    }
    
    if (workerThread) {
        workerThread->quit();
        workerThread->wait();
        delete workerThread;
        workerThread = nullptr;
    }
    
    if (worker) {
        delete worker;
        worker = nullptr;
    }
    
    bScanning = false;
}

void CCarlaDiscovery::onPluginFound(const PluginDiscoveryResult& result) {
    cachedResults.append(result);
    emit pluginFound(result);
}

void CCarlaDiscovery::onWorkerFinished(const QList<PluginDiscoveryResult>&) {
    saveCache();
    cancelScan(); // cleanup thread and worker
    emit scanFinished(cachedResults);
}

void CCarlaDiscovery::saveCache() {
    QJsonArray array;
    for (const auto& res : cachedResults) {
        QJsonObject obj;
        obj["name"] = res.name;
        obj["maker"] = res.maker;
        obj["category"] = res.category;
        obj["hints"] = res.hints;
        obj["binaryType"] = res.binaryType;
        obj["pluginType"] = res.pluginType;
        obj["filename"] = res.filename;
        obj["label"] = res.label;
        obj["uniqueId"] = (double)res.uniqueId;
        obj["audioIns"] = res.audioIns;
        obj["audioOuts"] = res.audioOuts;
        obj["midiIns"] = res.midiIns;
        obj["midiOuts"] = res.midiOuts;
        array.append(obj);
    }
    
    QFile file(getCacheFilePath());
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(array);
        file.write(doc.toJson());
        file.close();
    }
}

void CCarlaDiscovery::loadCache() {
    cachedResults.clear();
    QFile file(getCacheFilePath());
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        file.close();
        if (doc.isArray()) {
            QJsonArray array = doc.array();
            for (int i = 0; i < array.size(); ++i) {
                QJsonObject obj = array[i].toObject();
                PluginDiscoveryResult res;
                res.name = obj["name"].toString();
                res.maker = obj["maker"].toString();
                res.category = obj["category"].toInt();
                res.hints = obj["hints"].toInt();
                res.binaryType = obj["binaryType"].toInt();
                res.pluginType = obj["pluginType"].toInt();
                res.filename = obj["filename"].toString();
                res.label = obj["label"].toString();
                res.uniqueId = (uint64_t)obj["uniqueId"].toDouble();
                res.audioIns = obj["audioIns"].toInt();
                res.audioOuts = obj["audioOuts"].toInt();
                res.midiIns = obj["midiIns"].toInt();
                res.midiOuts = obj["midiOuts"].toInt();
                cachedResults.append(res);
            }
        }
    }
}

QStringList CCarlaDiscovery::getDefaultPaths(int pluginType) const {
    QStringList paths;
    QString envVarName;
    
    switch (pluginType) {
        case 4: // CarlaBackend::PLUGIN_LV2
            envVarName = "LV2_PATH";
            break;
        case 5: // CarlaBackend::PLUGIN_VST2
            envVarName = "VST2_PATH";
            break;
        case 6: // CarlaBackend::PLUGIN_VST3
            envVarName = "VST3_PATH";
            break;
        case 14: // CarlaBackend::PLUGIN_CLAP
            envVarName = "CLAP_PATH";
            break;
    }
    
    if (!envVarName.isEmpty()) {
        char* envVal = getenv(envVarName.toStdString().c_str());
        if (envVal && strlen(envVal) > 0) {
            QStringList rawParts = QString::fromUtf8(envVal).split(':');
            for (const QString& part : rawParts) {
                QString trimmed = part.trimmed();
                if (!trimmed.isEmpty()) {
                    paths.append(trimmed);
                }
            }
        }
    }
    
    if (paths.isEmpty()) {
        switch (pluginType) {
            case 4: // CarlaBackend::PLUGIN_LV2
                paths << "/usr/lib/lv2" << "/usr/local/lib/lv2" << "~/.lv2" << "/usr/lib/x86_64-linux-gnu/lv2";
                break;
            case 5: // CarlaBackend::PLUGIN_VST2
                paths << "/usr/lib/vst" << "/usr/local/lib/vst" << "~/.vst" << "/usr/lib/lxvst" << "/usr/lib/x86_64-linux-gnu/vst";
                break;
            case 6: // CarlaBackend::PLUGIN_VST3
                paths << "/usr/lib/vst3" << "/usr/local/lib/vst3" << "~/.vst3" << "/usr/lib/x86_64-linux-gnu/vst3";
                break;
            case 14: // CarlaBackend::PLUGIN_CLAP
                paths << "/usr/lib/clap" << "/usr/local/lib/clap" << "~/.clap" << "/usr/lib/x86_64-linux-gnu/clap";
                break;
        }
    }
    
    return paths;
}

QStringList CCarlaDiscovery::getScanPaths(int pluginType) const {
    QSettings settings("Jamulus", "CarlaDiscovery");
    QString key = QString("ScanPaths/%1").arg(pluginType);
    if (settings.contains(key)) {
        return settings.value(key).toStringList();
    }
    return getDefaultPaths(pluginType);
}

void CCarlaDiscovery::setScanPaths(int pluginType, const QStringList& paths) {
    QSettings settings("Jamulus", "CarlaDiscovery");
    QString key = QString("ScanPaths/%1").arg(pluginType);
    settings.setValue(key, paths);
}
