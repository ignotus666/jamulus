#pragma once

#include <QDialog>
#include <QString>
#include <QMap>
#include <QVector>
#include <QStringList>

class CClient;

class QListWidget;
class QComboBox;
class QPushButton;
class QLabel;
class QDialog;
class QPoint;

class CPluginLoaderDlg : public QDialog
{
    Q_OBJECT

public:
    explicit CPluginLoaderDlg ( CClient* pClient, QWidget* parent = nullptr );

private slots:
    void OnAddPath();
    void OnRemovePath();
    void OnScanPaths();
    void OnPluginListContextMenu ( const QPoint& pos );
    void OnFavoritesContextMenu ( const QPoint& pos );
    void OnFavoriteActivated ( int index );
    void OnLoadSelectedPlugin();
    void OnUnloadSelectedPlugin();
    void OnShowSelectedPluginUI();
    void OnDialogClosed();

private:
    void ScanPath ( const QString& strPath );
    void EnsurePluginItemVisible ( const QString& path );
    void RefreshLoadedPluginsFromHost();
    void UpdateLoadedPluginsDisplay();
    void EnsureFavoriteVisible ( const QString& path );
    void RefreshFavoritesDisplay();
    void AddFavoritePath ( const QString& path );
    void RemoveFavoritePath ( const QString& path );
    void CloseEditorWindowsForPlugin ( int iPluginId );
    void LoadFavorites();
    void SaveFavorites();

    CClient* pClient;

    QListWidget* lstPaths;
    QPushButton* butAddPath;
    QPushButton* butRemovePath;
    QPushButton* butScan;

    QLabel* lblFavorites;
    QComboBox* cmbFavorites;

    QListWidget* lstPlugins;
    QPushButton* butLoad;
    QPushButton* butUnload;
    QPushButton* butShowUI;
    QPushButton* butClose;

    QLabel* lblStatus;

    QMap<QString,int> m_loadedPlugins; // path -> plugin id
    QVector<QDialog*> m_editorWindows;
    QMap<QDialog*, int> m_editorWindowPluginIds;
    QStringList m_favoritePlugins; // ordered list of favorite plugin paths (up to 15)
};
