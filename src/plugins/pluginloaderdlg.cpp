#include "pluginloaderdlg.h"
#include <QComboBox>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QApplication>
#include <QDirIterator>
#include <QMap>
#include <QMessageBox>
#include <QFileInfo>
#include <QDialog>
#include <QCloseEvent>
#include <QTimer>
#include <QSettings>
#include <QMenu>
#include <QAction>
#include <QDir>
#include <QSet>
#include <QVBoxLayout>
#include <QResizeEvent>
#include <QPointer>
#include <QMetaObject>
#include <functional>

#include "client.h"

namespace
{
class PluginEditorWindow final : public QDialog
{
public:
    explicit PluginEditorWindow ( QWidget* parent = nullptr ) : QDialog ( parent, Qt::Window ) {}

    void SetPluginContext ( CClient* client, int pluginId )
    {
        m_client = client;
        m_pluginId = pluginId;
    }

    void ResizeFromPlugin ( int width, int height )
    {
        m_resizeFromPlugin = true;
        resize ( width, height );
        if ( m_client && m_pluginId >= 0 )
            m_client->ResizePluginEditorFromPlugin ( m_pluginId, width, height );
        m_resizeFromPlugin = false;
    }

    void SetCloseCallback ( std::function<void()> callback ) { m_closeCallback = std::move ( callback ); }

protected:
    void closeEvent ( QCloseEvent* event ) override
    {
        if ( m_closeCallback )
        {
            // Schedule the close callback to run after the close event completes and
            // the event loop resumes. This avoids running plugin teardown while the
            // window is still being destroyed.
            std::function<void()> callback = m_closeCallback;
            QTimer::singleShot ( 0, this, [callback] () {
                try
                {
                    callback();
                }
                catch ( const std::exception& e )
                {
                    qWarning() << "PluginEditorWindow: exception in scheduled closeCallback:" << e.what();
                }
                catch ( ... )
                {
                    qWarning() << "PluginEditorWindow: unknown exception in scheduled closeCallback";
                }
            } );
        }
        QDialog::closeEvent ( event );
    }

    void resizeEvent ( QResizeEvent* event ) override
    {
        QDialog::resizeEvent ( event );

        if ( m_resizeFromPlugin )
            return;

        if ( m_client && m_pluginId >= 0 )
            m_client->ResizePluginEditor ( m_pluginId, event->size().width(), event->size().height() );
    }

private:
    std::function<void()> m_closeCallback;
    CClient* m_client { nullptr };
    int m_pluginId { -1 };
    bool m_resizeFromPlugin { false };
};

static const char* kPluginLoaderPathsKey = "PluginLoader/Paths";
static const char* kFavoritesKey = "PluginLoader/Favorites";
static const char* kScannedPluginsKey = "PluginLoader/ScannedPlugins";
static constexpr int kPluginPathRole = Qt::UserRole;
static constexpr int kPluginBaseLabelRole = Qt::UserRole + 1;
static constexpr int kMaxFavorites = 15;

static QString PluginDisplayNameFromPath ( const QString& path )
{
    QString normalizedPath = QDir::fromNativeSeparators ( path );
    while ( normalizedPath.size() > 1 && normalizedPath.endsWith ( '/' ) )
        normalizedPath.chop ( 1 );

    const QString marker = ".vst3";
    const int vst3Pos = normalizedPath.lastIndexOf ( marker, -1, Qt::CaseInsensitive );
    if ( vst3Pos > 0 )
    {
        const int slashBefore = normalizedPath.lastIndexOf ( '/', vst3Pos );
        const int start = ( slashBefore >= 0 ) ? ( slashBefore + 1 ) : 0;
        if ( start < vst3Pos && start >= 0 )
        {
            const QString bundleName = normalizedPath.mid ( start, ( vst3Pos - start ) + marker.size() );
            if ( !bundleName.isEmpty() && bundleName != marker )
                return bundleName;
        }
    }

    QFileInfo info ( normalizedPath );
    const QString name = info.fileName();
    return name.isEmpty() ? normalizedPath : name;
}

static void SetPluginItemData ( QListWidgetItem* item, const QString& path )
{
    item->setData ( kPluginPathRole, path );
    item->setData ( kPluginBaseLabelRole, PluginDisplayNameFromPath ( path ) );
}

static QStringList LoadSavedPluginPaths()
{
    QSettings settings;
    return settings.value ( kPluginLoaderPathsKey ).toStringList();
}

static void SavePluginPaths ( QListWidget* pList )
{
    QStringList paths;
    if ( pList )
    {
        for ( int i = 0; i < pList->count(); ++i )
        {
            const QString path = pList->item ( i )->text();
            if ( !path.isEmpty() )
                paths.append ( path );
        }
    }

    paths.removeDuplicates();
    QSettings settings;
    settings.setValue ( kPluginLoaderPathsKey, paths );
}
} // namespace

CPluginLoaderDlg::CPluginLoaderDlg ( CClient* pClientP, QWidget* parent ) : QDialog ( parent ), pClient ( pClientP )
{
    setupUi ( this );

    Qt::WindowFlags flags = windowFlags();
    flags &= ~Qt::WindowStaysOnTopHint;
    flags &= ~Qt::Tool;
    flags &= ~Qt::Dialog;
    flags |= Qt::Window;
    flags |= Qt::WindowMinimizeButtonHint;
    setWindowFlags ( flags );

    setWindowTitle ( tr ( "Plugins" ) );
    setMinimumSize ( 600, 400 );
    setWindowModality ( Qt::NonModal );

    m_loadingBanner = new QLabel ( this );
    m_loadingBanner->setVisible ( false );
    m_loadingBanner->setAlignment ( Qt::AlignCenter );
    m_loadingBanner->setWordWrap ( true );
    m_loadingBanner->setStyleSheet ( "QLabel { color: rgb(75, 213, 248); background: #353535; "
                                     "border: 1px solid rgb(75, 213, 248); padding: 6px 10px; border-radius: 4px; }" );
    if ( verticalLayoutMain )
        verticalLayoutMain->insertWidget ( 0, m_loadingBanner );

    cmbFavorites->setSizeAdjustPolicy ( QComboBox::AdjustToContentsOnFirstShow );
    cmbFavorites->setMinimumWidth ( 320 );
    cmbFavorites->setContextMenuPolicy ( Qt::CustomContextMenu );

    connect ( cmbFavorites, &QComboBox::customContextMenuRequested,
              this, &CPluginLoaderDlg::OnFavoritesContextMenu );
    connect ( cmbFavorites->view(), &QWidget::customContextMenuRequested,
              this, &CPluginLoaderDlg::OnFavoritesContextMenu );
    cmbFavorites->view()->setContextMenuPolicy ( Qt::CustomContextMenu );
    connect ( cmbFavorites, QOverload<int>::of ( &QComboBox::activated ),
              this, &CPluginLoaderDlg::OnFavoriteActivated );

    // Path controls
    lstPaths->setSelectionMode ( QListWidget::SingleSelection );

    // Plugins list
    lstPlugins->setSelectionMode ( QListWidget::SingleSelection );
    lstPlugins->setContextMenuPolicy ( Qt::CustomContextMenu );

    connect ( lstPlugins, &QListWidget::customContextMenuRequested,
              this, &CPluginLoaderDlg::OnPluginListContextMenu );

    // Load/Close buttons
    connect ( butAddPath, &QPushButton::clicked, this, &CPluginLoaderDlg::OnAddPath );
    connect ( butRemovePath, &QPushButton::clicked, this, &CPluginLoaderDlg::OnRemovePath );
    connect ( butScan, &QPushButton::clicked, this, &CPluginLoaderDlg::OnScanPaths );
    connect ( butLoad, &QPushButton::clicked, this, &CPluginLoaderDlg::OnLoadSelectedPlugin );
    connect ( butUnload, &QPushButton::clicked, this, &CPluginLoaderDlg::OnUnloadSelectedPlugin );
    connect ( butShowUI, &QPushButton::clicked, this, &CPluginLoaderDlg::OnShowSelectedPluginUI );
    connect ( butClose, &QPushButton::clicked, this, &QDialog::accept );
    
    // Connect double-click to load plugin
    connect ( lstPlugins, &QListWidget::itemDoubleClicked, this, [this] ( QListWidgetItem* item ) {
        lstPlugins->setCurrentItem ( item );
        OnLoadSelectedPlugin();
    } );
    
    // Make dialog modeless (non-blocking)
    setWindowModality ( Qt::NonModal );

    // Save favorites when dialog closes
    connect ( this, &QDialog::finished, this, &CPluginLoaderDlg::OnDialogClosed );

    // Restore search paths from previous sessions.
    const QStringList savedPaths = LoadSavedPluginPaths();
    for ( const QString& path : savedPaths )
    {
        if ( !path.isEmpty() )
            lstPaths->addItem ( path );
    }
    
    // Load and display favorites
    LoadFavorites();
    // Restore last scanned plugins list
    LoadScannedPlugins();
    RefreshLoadedPluginsFromHost();
}

void CPluginLoaderDlg::OnAddPath()
{
    QString dir = QFileDialog::getExistingDirectory ( this, tr ( "Select plugin directory" ), QString(), QFileDialog::ShowDirsOnly );
    if ( !dir.isEmpty() )
    {
        // avoid duplicates
        for ( int i = 0; i < lstPaths->count(); ++i )
        {
            if ( lstPaths->item ( i )->text() == dir )
                return;
        }

        lstPaths->addItem ( dir );
        SavePluginPaths ( lstPaths );
    }
}

void CPluginLoaderDlg::OnRemovePath()
{
    QListWidgetItem* it = lstPaths->currentItem();
    if ( it )
    {
        delete it;
        SavePluginPaths ( lstPaths );
    }
}

void CPluginLoaderDlg::OnScanPaths()
{
    lstPlugins->clear();
    lblStatus->setText ( tr ( "Scanning..." ) );
    QApplication::processEvents();

    for ( int i = 0; i < lstPaths->count(); ++i )
    {
        QString path = lstPaths->item ( i )->text();
        ScanPath ( path );
    }

    lblStatus->setText ( tr ( "Scan complete" ) );
    UpdateLoadedPluginsDisplay();
    SaveScannedPlugins();
}

void CPluginLoaderDlg::OnPluginListContextMenu ( const QPoint& pos )
{
    QListWidgetItem* item = lstPlugins->itemAt ( pos );
    if ( !item )
        return;

    const QString path = item->data ( kPluginPathRole ).toString();
    if ( path.isEmpty() )
        return;

    QMenu menu ( this );
    QAction* addFavorite = menu.addAction ( tr ( "Add to favorites" ) );

    const QAction* selected = menu.exec ( lstPlugins->viewport()->mapToGlobal ( pos ) );
    if ( selected == addFavorite )
        AddFavoritePath ( path );
}

void CPluginLoaderDlg::OnFavoritesContextMenu ( const QPoint& pos )
{
    int iIndex = -1;
    if ( sender() == cmbFavorites->view() )
    {
        const QModelIndex idx = cmbFavorites->view()->indexAt ( pos );
        if ( idx.isValid() )
            iIndex = idx.row();
    }
    else
    {
        iIndex = cmbFavorites->currentIndex();
    }

    if ( iIndex < 0 || iIndex >= cmbFavorites->count() )
        return;

    const QString path = cmbFavorites->itemData ( iIndex, kPluginPathRole ).toString();
    if ( path.isEmpty() )
        return;

    QMenu menu ( this );
    QAction* removeFavorite = menu.addAction ( tr ( "Remove from favorites" ) );

    const QPoint globalPos = ( sender() == cmbFavorites->view() )
        ? cmbFavorites->view()->mapToGlobal ( pos )
        : cmbFavorites->mapToGlobal ( pos );
    const QAction* selected = menu.exec ( globalPos );
    if ( selected == removeFavorite )
        RemoveFavoritePath ( path );
}

void CPluginLoaderDlg::OnFavoriteActivated ( int index )
{
    if ( index < 0 || index >= cmbFavorites->count() )
        return;

    const QString path = cmbFavorites->itemData ( index, kPluginPathRole ).toString();
    if ( path.isEmpty() )
        return;

    EnsurePluginItemVisible ( path );

    for ( int i = 0; i < lstPlugins->count(); ++i )
    {
        QListWidgetItem* item = lstPlugins->item ( i );
        if ( item->data ( kPluginPathRole ).toString() == path )
        {
            lstPlugins->setCurrentItem ( item );
            break;
        }
    }

    lblStatus->setText ( tr ( "Selected favorite plugin: %1" ).arg ( path ) );
}

void CPluginLoaderDlg::ScanPath ( const QString& strPath )
{
    QSet<QString> addedPaths;

    // If the path itself is a VST3 bundle (a directory with .vst3), include it
    QFileInfo rootInfo ( strPath );
    if ( rootInfo.isDir() && strPath.endsWith ( ".vst3", Qt::CaseInsensitive ) )
    {
        const QString displayName = PluginDisplayNameFromPath ( rootInfo.absoluteFilePath() );
        if ( !displayName.isEmpty() && displayName != ".vst3" )
        {
            QListWidgetItem* it = new QListWidgetItem ( displayName );
            SetPluginItemData ( it, rootInfo.absoluteFilePath() );
            lstPlugins->addItem ( it );
            addedPaths.insert ( rootInfo.absoluteFilePath() );
        }
    }

    // Recurse into subdirectories and files to find plugin files (.so/.dll/.dylib) and .vst3 bundles
    QDirIterator iter ( strPath, QDir::Files | QDir::Dirs | QDir::NoSymLinks, QDirIterator::Subdirectories );
    while ( iter.hasNext() )
    {
        QString file = iter.next();
        QFileInfo fi ( file );
        QString suffix = fi.suffix().toLower();

        bool ok = false;
        bool isBundle = false;
        if ( fi.isDir() )
        {
            if ( file.endsWith ( ".vst3", Qt::CaseInsensitive ) )
            {
                ok = true;
                isBundle = true;
            }
        }
        else
        {
#if defined( Q_OS_WIN )
            ok = ( suffix == "dll" );
#elif defined( Q_OS_MACOS )
            ok = ( suffix == "dylib" );
#else
            ok = ( suffix == "so" );
#endif
        }

        if ( !ok )
            continue;

        // Skip if already added (e.g., we added the .vst3 bundle, skip the .so inside it)
        if ( addedPaths.contains ( fi.absoluteFilePath() ) )
            continue;

        // For .so files, check if they are inside a .vst3 bundle we already added
        if ( !isBundle && suffix == "so" )
        {
            bool isInsideBundle = false;
            for ( const QString& addedPath : addedPaths )
            {
                if ( file.contains ( addedPath + "/" ) )
                {
                    isInsideBundle = true;
                    break;
                }
            }
            if ( isInsideBundle )
                continue;
        }

        QString displayName = PluginDisplayNameFromPath ( fi.absoluteFilePath() );
        if ( displayName.isEmpty() || displayName == ".vst3" )
            continue;

        QListWidgetItem* it = new QListWidgetItem ( displayName );
        SetPluginItemData ( it, fi.absoluteFilePath() );
        lstPlugins->addItem ( it );
        addedPaths.insert ( fi.absoluteFilePath() );
    }
}

void CPluginLoaderDlg::EnsurePluginItemVisible ( const QString& path )
{
    if ( path.isEmpty() )
        return;

    for ( int i = 0; i < lstPlugins->count(); ++i )
    {
        if ( lstPlugins->item ( i )->data ( Qt::UserRole ).toString() == path )
            return;
    }

    QFileInfo info ( path );
    QListWidgetItem* item = new QListWidgetItem ( PluginDisplayNameFromPath ( info.absoluteFilePath() ) );
    SetPluginItemData ( item, info.absoluteFilePath() );
    lstPlugins->insertItem ( 0, item );
}

void CPluginLoaderDlg::EnsureFavoriteVisible ( const QString& path )
{
    if ( path.isEmpty() )
        return;

    for ( int i = 0; i < cmbFavorites->count(); ++i )
    {
        if ( cmbFavorites->itemData ( i, kPluginPathRole ).toString() == path )
            return;
    }

    cmbFavorites->addItem ( PluginDisplayNameFromPath ( path ) );
    const int iNewIndex = cmbFavorites->count() - 1;
    cmbFavorites->setItemData ( iNewIndex, path, kPluginPathRole );
}

void CPluginLoaderDlg::AddFavoritePath ( const QString& path )
{
    if ( path.isEmpty() )
        return;

    m_favoritePlugins.removeAll ( path );
    m_favoritePlugins.prepend ( path );
    while ( m_favoritePlugins.size() > kMaxFavorites )
        m_favoritePlugins.removeLast();

    RefreshFavoritesDisplay();
    SaveFavorites();
    UpdateLoadedPluginsDisplay();
    lblStatus->setText ( tr ( "Added to favorites: %1" ).arg ( PluginDisplayNameFromPath ( path ) ) );
}

void CPluginLoaderDlg::RemoveFavoritePath ( const QString& path )
{
    if ( path.isEmpty() )
        return;

    m_favoritePlugins.removeAll ( path );
    RefreshFavoritesDisplay();
    SaveFavorites();
    UpdateLoadedPluginsDisplay();
    lblStatus->setText ( tr ( "Removed from favorites: %1" ).arg ( PluginDisplayNameFromPath ( path ) ) );
}

void CPluginLoaderDlg::RefreshLoadedPluginsFromHost()
{
    m_loadedPlugins.clear();

    const auto snapshot = pClient->GetLoadedPluginsSnapshot();
    for ( const auto& plugin : snapshot )
    {
        const QString path = QString::fromStdString ( plugin.path );
        if ( !path.isEmpty() )
        {
            m_loadedPlugins.insert ( path, plugin.id );
            EnsurePluginItemVisible ( path );
        }
    }

    UpdateLoadedPluginsDisplay();
}

void CPluginLoaderDlg::OnLoadSelectedPlugin()
{
    QListWidgetItem* it = lstPlugins->currentItem();
    if ( !it )
    {
        QMessageBox::information ( this, tr ( "Load Plugin" ), tr ( "Please select a plugin from the list." ) );
        return;
    }

    QString path = it->data ( kPluginPathRole ).toString();
    if ( path.isEmpty() )
    {
        QMessageBox::warning ( this, tr ( "Load Plugin" ), tr ( "Invalid plugin path." ) );
        return;
    }

    if ( m_loadingBanner )
    {
        m_loadingBanner->setText ( tr ( "Loading %1..." ).arg ( PluginDisplayNameFromPath ( path ) ) );
        m_loadingBanner->setVisible ( true );
    }
    QApplication::processEvents();

    int iId = pClient->LoadPlugin ( path.toStdString() );
    if ( m_loadingBanner )
        m_loadingBanner->setVisible ( false );
    if ( iId >= 0 )
    {
        lblStatus->setText ( tr ( "Loaded: %1" ).arg ( PluginDisplayNameFromPath ( path ) ) );
        m_loadedPlugins.insert ( path, iId );
        
        // Update UI to show loaded plugins
        UpdateLoadedPluginsDisplay();
    }
    else
    {
        QMessageBox::critical ( this, tr ( "Load Plugin" ), tr ( "Failed to load plugin: %1" ).arg ( path ) );
        lblStatus->setText ( tr ( "Load failed" ) );
    }
}

void CPluginLoaderDlg::OnUnloadSelectedPlugin()
{
    QListWidgetItem* it = lstPlugins->currentItem();
    if ( !it )
    {
        QMessageBox::information ( this, tr ( "Unload Plugin" ), tr ( "Please select a plugin from the list." ) );
        return;
    }

    QString path = it->data ( kPluginPathRole ).toString();
    if ( path.isEmpty() )
    {
        QMessageBox::warning ( this, tr ( "Unload Plugin" ), tr ( "Invalid plugin path." ) );
        return;
    }

    if ( !m_loadedPlugins.contains ( path ) )
    {
        QMessageBox::information ( this, tr ( "Unload Plugin" ), tr ( "Plugin not loaded." ) );
        lblStatus->setText ( tr ( "Plugin not loaded" ) );
        return;
    }

    int iId = m_loadedPlugins.value ( path );
    CloseEditorWindowsForPlugin ( iId );
    bool bOk = pClient->UnloadPlugin ( iId );
    if ( bOk )
    {
        lblStatus->setText ( tr ( "Unloaded: %1" ).arg ( PluginDisplayNameFromPath ( path ) ) );
        m_loadedPlugins.remove ( path );
        UpdateLoadedPluginsDisplay();
    }
    else
    {
        QMessageBox::critical ( this, tr ( "Unload Plugin" ), tr ( "Failed to unload plugin id: %1" ).arg ( iId ) );
        lblStatus->setText ( tr ( "Unload failed" ) );
    }
}

void CPluginLoaderDlg::OnShowSelectedPluginUI()
{
    QListWidgetItem* it = lstPlugins->currentItem();
    if ( !it )
    {
        QMessageBox::information ( this, tr ( "Show Plugin UI" ), tr ( "Please select a plugin from the list." ) );
        return;
    }

    QString path = it->data ( kPluginPathRole ).toString();
    if ( path.isEmpty() )
    {
        QMessageBox::warning ( this, tr ( "Show Plugin UI" ), tr ( "Invalid plugin path." ) );
        return;
    }

    if ( !m_loadedPlugins.contains ( path ) )
    {
        QMessageBox::information ( this, tr ( "Show Plugin UI" ), tr ( "Load the plugin first." ) );
        return;
    }

    int iId = m_loadedPlugins.value ( path );

    auto* pEditorWindow = new PluginEditorWindow ( nullptr );  // Use a top-level window for the plugin editor
    pEditorWindow->setAttribute ( Qt::WA_DeleteOnClose, true );
    pEditorWindow->setWindowTitle ( tr ( "Plugin UI - %1" ).arg ( PluginDisplayNameFromPath ( path ) ) );
    pEditorWindow->setWindowModality ( Qt::NonModal );  // Non-modal so main window stays accessible
    pEditorWindow->resize ( 900, 650 );

    QPointer<PluginEditorWindow> editorWindowPtr ( pEditorWindow );
    pClient->SetPluginEditorHostResizeCallback ( iId, [editorWindowPtr] ( int width, int height ) {
        if ( !editorWindowPtr )
            return;

        QMetaObject::invokeMethod ( editorWindowPtr, [editorWindowPtr, width, height] {
            if ( editorWindowPtr )
                editorWindowPtr->ResizeFromPlugin ( width, height );
        }, Qt::QueuedConnection );
    } );

    pEditorWindow->show();

    // Ensure a native window exists before passing handle to VST3 view.
    pEditorWindow->winId();
    const bool bShown = pClient->ShowPluginEditor ( iId, reinterpret_cast<void*> ( pEditorWindow->winId() ) );
    if ( !bShown )
    {
        pEditorWindow->close();
        QMessageBox::warning ( this,
                               tr ( "Show Plugin UI" ),
                               tr ( "Failed to open plugin UI for this plugin." ) );
        lblStatus->setText ( tr ( "Show UI failed" ) );
        return;
    }

    int editorWidth = 0;
    int editorHeight = 0;
    if ( pClient->GetPluginEditorSize ( iId, editorWidth, editorHeight ) && editorWidth > 0 && editorHeight > 0 )
    {
        pEditorWindow->resize ( editorWidth, editorHeight );
    }

    pEditorWindow->SetPluginContext ( pClient, iId );

    pEditorWindow->SetCloseCallback ( [this, iId] {
        pClient->ClosePluginEditor ( iId );
    } );

    connect ( pEditorWindow, &QObject::destroyed, this, [this, pEditorWindow] {
        m_editorWindows.removeAll ( pEditorWindow );
        m_editorWindowPluginIds.remove ( pEditorWindow );
    } );

    m_editorWindows.push_back ( pEditorWindow );
    m_editorWindowPluginIds.insert ( pEditorWindow, iId );
    lblStatus->setText ( tr ( "UI open: %1" ).arg ( path ) );
}

void CPluginLoaderDlg::UpdateLoadedPluginsDisplay()
{
    // Add visual indicator for loaded plugins
    for ( int i = 0; i < lstPlugins->count(); ++i )
    {
        QListWidgetItem* item = lstPlugins->item ( i );
        QString path = item->data ( Qt::UserRole ).toString();
        const bool bIsFavorite = m_favoritePlugins.contains ( path );

        if ( m_loadedPlugins.contains ( path ) )
        {
            int iId = m_loadedPlugins.value ( path );
            const QString displayName = item->data ( kPluginBaseLabelRole ).toString().isEmpty()
                ? PluginDisplayNameFromPath ( path )
                : item->data ( kPluginBaseLabelRole ).toString();
            QString loadedName = displayName;
            if ( bIsFavorite )
                loadedName.append ( " [FAV]" );
            loadedName.append ( " [LOADED #" + QString::number ( iId ) + "]" );
            item->setText ( loadedName );
        }
        else
        {
            const QString displayName = item->data ( kPluginBaseLabelRole ).toString().isEmpty()
                ? PluginDisplayNameFromPath ( path )
                : item->data ( kPluginBaseLabelRole ).toString();
            QString normalName = displayName;
            if ( bIsFavorite )
                normalName.append ( " [FAV]" );
            item->setText ( normalName );
        }
    }
}

void CPluginLoaderDlg::LoadFavorites()
{
    QSettings settings;
    m_favoritePlugins = settings.value ( kFavoritesKey ).toStringList();

    while ( m_favoritePlugins.size() > kMaxFavorites )
        m_favoritePlugins.removeLast();

    RefreshFavoritesDisplay();
    UpdateLoadedPluginsDisplay();
}

void CPluginLoaderDlg::RefreshFavoritesDisplay()
{
    cmbFavorites->clear();
    for ( const QString& path : m_favoritePlugins )
        EnsureFavoriteVisible ( path );

    if ( cmbFavorites->count() > 0 )
        cmbFavorites->setCurrentIndex ( 0 );
}

void CPluginLoaderDlg::CloseEditorWindowsForPlugin ( int iPluginId )
{
    QVector<QDialog*> windowsToClose;
    for ( auto it = m_editorWindowPluginIds.cbegin(); it != m_editorWindowPluginIds.cend(); ++it )
    {
        if ( it.value() == iPluginId && it.key() )
            windowsToClose.push_back ( it.key() );
    }

    for ( QDialog* dlg : windowsToClose )
    {
        if ( dlg->isVisible() )
            dlg->close();
    }
}

void CPluginLoaderDlg::SaveFavorites()
{
    QSettings settings;
    settings.setValue ( kFavoritesKey, m_favoritePlugins );
}

void CPluginLoaderDlg::LoadScannedPlugins()
{
    QSettings settings;
    const QStringList savedPaths = settings.value ( kScannedPluginsKey ).toStringList();
    QSet<QString> seen;

    for ( const QString& path : savedPaths )
    {
        if ( path.isEmpty() )
            continue;

        QFileInfo info ( path );
        if ( !info.exists() )
            continue;

        const QString normalized = info.absoluteFilePath();
        if ( seen.contains ( normalized ) )
            continue;

        QListWidgetItem* item = new QListWidgetItem ( PluginDisplayNameFromPath ( normalized ) );
        SetPluginItemData ( item, normalized );
        lstPlugins->addItem ( item );
        seen.insert ( normalized );
    }

    if ( lstPlugins->count() > 0 )
        lblStatus->setText ( tr ( "Loaded %1 saved plugins" ).arg ( lstPlugins->count() ) );
}

void CPluginLoaderDlg::SaveScannedPlugins()
{
    QStringList paths;
    for ( int i = 0; i < lstPlugins->count(); ++i )
    {
        QListWidgetItem* item = lstPlugins->item ( i );
        const QString path = item ? item->data ( kPluginPathRole ).toString() : QString();
        if ( !path.isEmpty() )
            paths.append ( path );
    }

    paths.removeDuplicates();
    QSettings settings;
    settings.setValue ( kScannedPluginsKey, paths );
}

void CPluginLoaderDlg::OnDialogClosed()
{
    SaveFavorites();
}
