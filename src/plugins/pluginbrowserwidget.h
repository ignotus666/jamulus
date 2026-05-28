#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QTreeView>
#include <QTreeWidget>
#include <QListWidget>
#include <QPushButton>
#include <QProgressBar>
#include <QLabel>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QHash>
#include <QSplitter>
#include "carla_discovery.h"

class CClientSettings;

class CPluginBrowserWidget : public QWidget
{
    Q_OBJECT
public:
    CPluginBrowserWidget ( void* carlaAdapter, CClientSettings* settings, QWidget* parent = nullptr );
    ~CPluginBrowserWidget();

    void refreshLoadedPlugins();
    void setCarlaAdapter ( void* newHandle );

private slots:
    void onSearchTextChanged ( const QString& text );
    void onCategoryChanged ( int index );
    void onScanButtonClicked();
    void onLoadButtonClicked();
    void onPluginDoubleClicked ( const QModelIndex& index );
    void onPluginContextMenuRequested ( const QPoint& pos );
    void onConfigurePathsClicked();
    void onFavoritesSelectionChanged();
    void onFavoriteDoubleClicked ( const QModelIndex& index );
    void onFavoriteContextMenuRequested ( const QPoint& pos );

    // Loaded list controls
    void onRemovePluginClicked();
    void onShowUiClicked();
    void onDryWetChanged ( int value );
    void onLoadedSelectionChanged();
    void onLoadedItemChanged ( QTreeWidgetItem* item, int column );

    // Discovery signals
    void onPluginFound ( const PluginDiscoveryResult& result );
    void onScanProgress ( int percent );
    void onScanFinished ( const QList<PluginDiscoveryResult>& results );

private:
    void*            carlaAdapter;
    CClientSettings* pSettings;
    CCarlaDiscovery* discovery;
    QSplitter*       pMainSplitter{ nullptr };

    // UI Layout elements
    QLineEdit* searchEdit;
    QComboBox* categoryCombo;
    QTreeView* pluginTreeView;

    QPushButton*  scanButton;
    QPushButton*  loadButton;
    QProgressBar* progressBar;

    QTreeWidget* loadedTreeWidget;
    QListWidget* favoritesListWidget;
    QPushButton* removeButton;
    QPushButton* showUiButton;
    QSlider*     dryWetSlider;
    QLabel*      dryWetLabel;

    // Models
    QStandardItemModel*    pluginModel;
    QSortFilterProxyModel* filterModel;
    PluginDiscoveryResult  activeSelectedPlugin;
    bool                   activeSelectedPluginValid{ false };
    bool                   bRefreshingLoadedPlugins{ false };
    QHash<int, bool>       mapPluginBypassed;

    void                         setupUi();
    void                         populatePluginTree();
    void                         populateFavoritesList();
    void                         syncFavoritesToSettings();
    void                         addFavorite ( const PluginDiscoveryResult& result );
    void                         removeFavoriteAtRow ( int row );
    bool                         loadPluginResult ( const PluginDiscoveryResult& result );
    QList<PluginDiscoveryResult> getFilteredPlugins() const;
    PluginDiscoveryResult        getSelectedDiscoveryResult() const;
    PluginDiscoveryResult        getDiscoveryResultFromIndex ( const QModelIndex& index ) const;
    void                         setActiveSelectedPlugin ( const PluginDiscoveryResult& result, bool valid );
    QString                      pluginDetailsText ( const PluginDiscoveryResult& result ) const;
    QString                      serializeFavoriteResult ( const PluginDiscoveryResult& result ) const;
    bool                         deserializeFavoriteResult ( const QString& encoded, PluginDiscoveryResult& result ) const;
    QString                      favoriteKey ( const PluginDiscoveryResult& result ) const;
    void                         applyThemeAwareItemViewStyle ( QWidget* widget ) const;
    bool                         favoriteExists ( const QString& key ) const;
    void                         restoreLayoutState();
    void                         saveLayoutState();
};
