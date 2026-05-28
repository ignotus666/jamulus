#include "pluginbrowserwidget.h"
#include "carla_adapter.h"
#include "settings.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QSplitter>
#include <QGroupBox>
#include <QSlider>
#include <QMessageBox>
#include <QDebug>
#include <QFormLayout>
#include <QDialog>
#include <QFileDialog>
#include <QComboBox>
#include <QListWidget>
#include <QTreeWidget>
#include <QDialogButtonBox>
#include <QLabel>
#include <QDir>
#include <QMenu>
#include <QAction>
#include <QPalette>
#include <QStyledItemDelegate>
#include <QPainter>

class BrowserItemDelegate : public QStyledItemDelegate
{
public:
    explicit BrowserItemDelegate ( QObject* parent = nullptr ) : QStyledItemDelegate ( parent ) {}

    void paint ( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const override
    {
        QStyleOptionViewItem opt ( option );
        initStyleOption ( &opt, index );

        const bool isDarkTheme = opt.palette.color ( QPalette::Window ).lightness() < 128;
        if ( isDarkTheme )
        {
            const bool isSelected  = opt.state & QStyle::State_Selected;
            const bool isAlternate = opt.features.testFlag ( QStyleOptionViewItem::Alternate );

            const QColor baseColor =
                isSelected ? QColor ( 0x4f, 0x59, 0x64 ) : ( isAlternate ? QColor ( 0x3a, 0x3a, 0x3a ) : QColor ( 0x2b, 0x2b, 0x2b ) );
            const QColor textColor = Qt::white;

            painter->save();
            painter->fillRect ( opt.rect, baseColor );
            opt.palette.setColor ( QPalette::Text, textColor );
            opt.palette.setColor ( QPalette::WindowText, textColor );
            opt.palette.setColor ( QPalette::HighlightedText, textColor );
            QStyledItemDelegate::paint ( painter, opt, index );
            painter->restore();
            return;
        }

        QStyledItemDelegate::paint ( painter, opt, index );
    }
};

namespace
{

QString encodeField ( const QString& value ) { return QString::fromLatin1 ( value.toUtf8().toBase64() ); }

QString decodeField ( const QString& value ) { return QString::fromUtf8 ( QByteArray::fromBase64 ( value.toLatin1() ) ); }

QString formatNameForType ( int pluginType )
{
    return pluginType == 4
               ? QStringLiteral ( "LV2" )
               : ( pluginType == 5 ? QStringLiteral ( "VST2" ) : ( pluginType == 6 ? QStringLiteral ( "VST3" ) : QStringLiteral ( "CLAP" ) ) );
}

QString formatItemText ( const PluginDiscoveryResult& result )
{
    return result.maker.isEmpty() ? result.name : QStringLiteral ( "%1 - %2" ).arg ( result.name, result.maker );
}

QString formatItemToolTip ( const PluginDiscoveryResult& result )
{
    return QStringLiteral ( "%1\n%2\n%3\n%4" )
        .arg ( result.name )
        .arg ( result.maker.isEmpty() ? QObject::tr ( "Unknown maker" ) : result.maker )
        .arg ( result.filename )
        .arg ( formatNameForType ( result.pluginType ) );
}

} // namespace

// ---------------------------------------------------------------------------
// CScanPathsDlg - Plugin scan path configuration dialog
// ---------------------------------------------------------------------------
class CScanPathsDlg : public QDialog
{
public:
    CScanPathsDlg ( CCarlaDiscovery* discovery, QWidget* parent = nullptr ) : QDialog ( parent ), discovery ( discovery )
    {
        setWindowTitle ( tr ( "Configure Plugin Scan Paths" ) );
        setMinimumSize ( 500, 350 );

        QVBoxLayout* mainLayout = new QVBoxLayout ( this );
        mainLayout->setContentsMargins ( 12, 12, 12, 12 );
        mainLayout->setSpacing ( 10 );

        QLabel* infoLabel = new QLabel ( tr ( "Configure search directories for each plugin format. "
                                              "Use ~ to represent your home directory (e.g. ~/.lv2)." ),
                                         this );
        infoLabel->setWordWrap ( true );
        mainLayout->addWidget ( infoLabel );

        QHBoxLayout* formatLayout = new QHBoxLayout();
        formatLayout->addWidget ( new QLabel ( tr ( "Plugin Format:" ), this ) );

        formatCombo = new QComboBox ( this );
        formatCombo->addItem ( "LV2", 4 );
        formatCombo->addItem ( "VST2", 5 );
        formatCombo->addItem ( "VST3", 6 );
        formatCombo->addItem ( "CLAP", 14 );
        formatLayout->addWidget ( formatCombo, 1 );
        mainLayout->addLayout ( formatLayout );

        QHBoxLayout* listLayout = new QHBoxLayout();
        pathsListWidget         = new QListWidget ( this );
        pathsListWidget->setAlternatingRowColors ( true );
        listLayout->addWidget ( pathsListWidget, 1 );

        QVBoxLayout* buttonsLayout = new QVBoxLayout();
        buttonsLayout->setSpacing ( 6 );

        addButton    = new QPushButton ( tr ( "Add Path..." ), this );
        removeButton = new QPushButton ( tr ( "Remove" ), this );
        resetButton  = new QPushButton ( tr ( "Reset Defaults" ), this );

        buttonsLayout->addWidget ( addButton );
        buttonsLayout->addWidget ( removeButton );
        buttonsLayout->addWidget ( resetButton );
        buttonsLayout->addStretch ( 1 );
        listLayout->addLayout ( buttonsLayout );
        mainLayout->addLayout ( listLayout );

        QDialogButtonBox* buttonBox = new QDialogButtonBox ( QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this );
        mainLayout->addWidget ( buttonBox );

        // Connections
        connect ( formatCombo, QOverload<int>::of ( &QComboBox::currentIndexChanged ), this, &CScanPathsDlg::onFormatChanged );
        connect ( addButton, &QPushButton::clicked, this, &CScanPathsDlg::onAddClicked );
        connect ( removeButton, &QPushButton::clicked, this, &CScanPathsDlg::onRemoveClicked );
        connect ( resetButton, &QPushButton::clicked, this, &CScanPathsDlg::onResetClicked );
        connect ( buttonBox, &QDialogButtonBox::accepted, this, &CScanPathsDlg::onAccepted );
        connect ( buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject );
        connect ( pathsListWidget, &QListWidget::itemSelectionChanged, this, &CScanPathsDlg::updateButtonStates );

        // Load initial paths for all formats
        QList<int> formats = { 4, 5, 6, 14 };
        for ( int fmt : formats )
        {
            currentPaths[fmt] = discovery->getScanPaths ( fmt );
        }

        // Show LV2 initially
        onFormatChanged ( 0 );
    }

private:
    void onFormatChanged ( int index )
    {
        pathsListWidget->clear();
        int         fmt   = formatCombo->itemData ( index ).toInt();
        QStringList paths = currentPaths.value ( fmt );
        for ( const QString& path : paths )
        {
            pathsListWidget->addItem ( path );
        }
        updateButtonStates();
    }

    void onAddClicked()
    {
        QString dir = QFileDialog::getExistingDirectory ( this, tr ( "Select Plugin Directory" ), QDir::homePath() );
        if ( !dir.isEmpty() )
        {
            // Check if we should shorten it to tilde
            QString home = QDir::homePath();
            if ( dir.startsWith ( home ) )
            {
                dir = "~" + dir.mid ( home.length() );
            }

            int          fmt  = formatCombo->itemData ( formatCombo->currentIndex() ).toInt();
            QStringList& list = currentPaths[fmt];
            if ( !list.contains ( dir ) )
            {
                list.append ( dir );
                pathsListWidget->addItem ( dir );
            }
            updateButtonStates();
        }
    }

    void onRemoveClicked()
    {
        QListWidgetItem* item = pathsListWidget->currentItem();
        if ( item )
        {
            int          fmt  = formatCombo->itemData ( formatCombo->currentIndex() ).toInt();
            QStringList& list = currentPaths[fmt];
            list.removeAll ( item->text() );
            delete item;
            updateButtonStates();
        }
    }

    void onResetClicked()
    {
        int fmt           = formatCombo->itemData ( formatCombo->currentIndex() ).toInt();
        currentPaths[fmt] = discovery->getDefaultPaths ( fmt );
        onFormatChanged ( formatCombo->currentIndex() );
    }

    void onAccepted()
    {
        // Save back to discovery / settings
        for ( auto it = currentPaths.begin(); it != currentPaths.end(); ++it )
        {
            discovery->setScanPaths ( it.key(), it.value() );
        }
        accept();
    }

    void updateButtonStates() { removeButton->setEnabled ( pathsListWidget->currentItem() != nullptr ); }

    CCarlaDiscovery* discovery;
    QComboBox*       formatCombo;
    QListWidget*     pathsListWidget;
    QPushButton*     addButton;
    QPushButton*     removeButton;
    QPushButton*     resetButton;

    QMap<int, QStringList> currentPaths;
};

CPluginBrowserWidget::CPluginBrowserWidget ( void* carlaAdapter, CClientSettings* settings, QWidget* parent ) :
    QWidget ( parent ),
    carlaAdapter ( carlaAdapter ),
    pSettings ( settings )
{
    discovery = new CCarlaDiscovery ( this );

    setupUi();
    populatePluginTree();
    populateFavoritesList();
    refreshLoadedPlugins();

    // Connect discovery signals
    connect ( discovery, &CCarlaDiscovery::pluginFound, this, &CPluginBrowserWidget::onPluginFound );
    connect ( discovery, &CCarlaDiscovery::progressUpdated, this, &CPluginBrowserWidget::onScanProgress );
    connect ( discovery, &CCarlaDiscovery::scanFinished, this, &CPluginBrowserWidget::onScanFinished );
}

CPluginBrowserWidget::~CPluginBrowserWidget()
{
    saveLayoutState();
    discovery->cancelScan();
}

void CPluginBrowserWidget::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout ( this );
    mainLayout->setContentsMargins ( 10, 10, 10, 10 );
    mainLayout->setSpacing ( 8 );

    // -----------------------------------------------------------------------
    // Top Bar (Filters and Search)
    // -----------------------------------------------------------------------
    QGroupBox*   filterGroup = new QGroupBox ( tr ( "Plugin Browser" ), this );
    QHBoxLayout* topLayout   = new QHBoxLayout ( filterGroup );
    topLayout->setContentsMargins ( 8, 8, 8, 8 );
    topLayout->setSpacing ( 10 );

    searchEdit = new QLineEdit ( this );
    searchEdit->setPlaceholderText ( tr ( "Search plugins..." ) );
    searchEdit->setClearButtonEnabled ( true );
    connect ( searchEdit, &QLineEdit::textChanged, this, &CPluginBrowserWidget::onSearchTextChanged );
    topLayout->addWidget ( new QLabel ( tr ( "Search:" ), this ) );
    topLayout->addWidget ( searchEdit, 2 );

    categoryCombo = new QComboBox ( this );
    categoryCombo->addItem ( tr ( "All Formats" ), -1 );
    categoryCombo->addItem ( tr ( "LV2" ), 4 );   // PLUGIN_LV2
    categoryCombo->addItem ( tr ( "VST2" ), 5 );  // PLUGIN_VST2
    categoryCombo->addItem ( tr ( "VST3" ), 6 );  // PLUGIN_VST3
    categoryCombo->addItem ( tr ( "CLAP" ), 14 ); // PLUGIN_CLAP
    connect ( categoryCombo, QOverload<int>::of ( &QComboBox::currentIndexChanged ), this, &CPluginBrowserWidget::onCategoryChanged );
    topLayout->addWidget ( new QLabel ( tr ( "Format:" ), this ) );
    topLayout->addWidget ( categoryCombo, 1 );

    scanButton = new QPushButton ( tr ( "Scan Plugins" ), this );
    connect ( scanButton, &QPushButton::clicked, this, &CPluginBrowserWidget::onScanButtonClicked );
    topLayout->addWidget ( scanButton );

    QPushButton* pathsButton = new QPushButton ( tr ( "Configure Paths..." ), this );
    connect ( pathsButton, &QPushButton::clicked, this, &CPluginBrowserWidget::onConfigurePathsClicked );
    topLayout->addWidget ( pathsButton );

    progressBar = new QProgressBar ( this );
    progressBar->setRange ( 0, 100 );
    progressBar->setValue ( 0 );
    progressBar->setVisible ( false );
    progressBar->setFixedHeight ( 16 );
    topLayout->addWidget ( progressBar, 1 );

    mainLayout->addWidget ( filterGroup );

    // -----------------------------------------------------------------------
    // Middle Splitter (scanned / load / loaded)
    // -----------------------------------------------------------------------
    pMainSplitter = new QSplitter ( Qt::Horizontal, this );

    pluginModel = new QStandardItemModel ( this );
    pluginModel->setHorizontalHeaderLabels ( { tr ( "Name" ), tr ( "Maker" ), tr ( "Format" ) } );

    filterModel = new QSortFilterProxyModel ( this );
    filterModel->setSourceModel ( pluginModel );
    filterModel->setFilterKeyColumn ( -1 ); // search all columns
    filterModel->setFilterCaseSensitivity ( Qt::CaseInsensitive );

    pluginTreeView = new QTreeView ( this );
    pluginTreeView->setModel ( filterModel );
    pluginTreeView->setAlternatingRowColors ( true );
    pluginTreeView->setEditTriggers ( QAbstractItemView::NoEditTriggers );
    pluginTreeView->setSelectionMode ( QAbstractItemView::SingleSelection );
    pluginTreeView->setSelectionBehavior ( QAbstractItemView::SelectRows );
    pluginTreeView->header()->setSectionResizeMode ( QHeaderView::Interactive );
    pluginTreeView->header()->setStretchLastSection ( false );
    pluginTreeView->header()->setMinimumSectionSize ( 80 );
    pluginTreeView->setColumnWidth ( 0, 260 );
    pluginTreeView->setColumnWidth ( 1, 180 );
    pluginTreeView->setColumnWidth ( 2, 100 );
    pluginTreeView->setItemDelegate ( new BrowserItemDelegate ( pluginTreeView ) );
    connect ( pluginTreeView, &QTreeView::doubleClicked, this, &CPluginBrowserWidget::onPluginDoubleClicked );
    connect ( pluginTreeView, &QWidget::customContextMenuRequested, this, &CPluginBrowserWidget::onPluginContextMenuRequested );
    pluginTreeView->setContextMenuPolicy ( Qt::CustomContextMenu );
    applyThemeAwareItemViewStyle ( pluginTreeView );

    QGroupBox*   scannedGroup  = new QGroupBox ( tr ( "Scanned Plugins" ), this );
    QVBoxLayout* scannedLayout = new QVBoxLayout ( scannedGroup );
    scannedLayout->setContentsMargins ( 8, 8, 8, 8 );
    scannedLayout->setSpacing ( 8 );
    scannedLayout->addWidget ( pluginTreeView, 2 );

    QGroupBox*   favoritesGroup  = new QGroupBox ( tr ( "Favourite Plugins" ), this );
    QVBoxLayout* favoritesLayout = new QVBoxLayout ( favoritesGroup );
    favoritesLayout->setContentsMargins ( 8, 8, 8, 8 );

    favoritesListWidget = new QListWidget ( this );
    favoritesListWidget->setAlternatingRowColors ( true );
    favoritesListWidget->setSelectionMode ( QAbstractItemView::SingleSelection );
    favoritesListWidget->setEditTriggers ( QAbstractItemView::NoEditTriggers );
    favoritesListWidget->setContextMenuPolicy ( Qt::CustomContextMenu );
    favoritesListWidget->setItemDelegate ( new BrowserItemDelegate ( favoritesListWidget ) );
    connect ( favoritesListWidget, &QListWidget::itemSelectionChanged, this, &CPluginBrowserWidget::onFavoritesSelectionChanged );
    connect ( favoritesListWidget, &QListWidget::doubleClicked, this, &CPluginBrowserWidget::onFavoriteDoubleClicked );
    connect ( favoritesListWidget, &QWidget::customContextMenuRequested, this, &CPluginBrowserWidget::onFavoriteContextMenuRequested );
    applyThemeAwareItemViewStyle ( favoritesListWidget );
    favoritesLayout->addWidget ( favoritesListWidget );
    scannedLayout->addWidget ( favoritesGroup, 1 );

    pMainSplitter->addWidget ( scannedGroup );

    QWidget*     loadPanel  = new QWidget ( this );
    QVBoxLayout* loadLayout = new QVBoxLayout ( loadPanel );
    loadLayout->setContentsMargins ( 8, 8, 8, 8 );
    loadLayout->addStretch ( 1 );

    loadButton = new QPushButton ( tr ( "Load" ), this );
    loadButton->setEnabled ( false );
    loadButton->setStyleSheet ( "font-weight: bold; padding: 6px;" );
    connect ( loadButton, &QPushButton::clicked, this, &CPluginBrowserWidget::onLoadButtonClicked );
    loadLayout->addWidget ( loadButton );
    loadLayout->addStretch ( 1 );
    pMainSplitter->addWidget ( loadPanel );

    QGroupBox*   loadedGroup  = new QGroupBox ( tr ( "Loaded Plugins" ), this );
    QVBoxLayout* loadedLayout = new QVBoxLayout ( loadedGroup );
    loadedLayout->setContentsMargins ( 8, 8, 8, 8 );
    loadedLayout->setSpacing ( 8 );

    loadedTreeWidget = new QTreeWidget ( this );
    loadedTreeWidget->setAlternatingRowColors ( true );
    loadedTreeWidget->setRootIsDecorated ( false );
    loadedTreeWidget->setSelectionMode ( QAbstractItemView::SingleSelection );
    loadedTreeWidget->setSelectionBehavior ( QAbstractItemView::SelectRows );
    loadedTreeWidget->setHeaderLabels ( { tr ( "Bypassed" ), tr ( "Plugin" ) } );
    loadedTreeWidget->header()->setSectionResizeMode ( 0, QHeaderView::ResizeToContents );
    loadedTreeWidget->header()->setSectionResizeMode ( 1, QHeaderView::Stretch );
    loadedTreeWidget->setItemDelegate ( new BrowserItemDelegate ( loadedTreeWidget ) );
    connect ( loadedTreeWidget, &QTreeWidget::itemSelectionChanged, this, &CPluginBrowserWidget::onLoadedSelectionChanged );
    connect ( loadedTreeWidget, &QTreeWidget::itemChanged, this, &CPluginBrowserWidget::onLoadedItemChanged );
    applyThemeAwareItemViewStyle ( loadedTreeWidget );
    loadedLayout->addWidget ( loadedTreeWidget, 3 );

    // Loaded Controls
    QGroupBox*   controlsGroup  = new QGroupBox ( tr ( "Plugin Controls" ), this );
    QFormLayout* controlsLayout = new QFormLayout ( controlsGroup );
    controlsLayout->setContentsMargins ( 8, 8, 8, 8 );
    controlsLayout->setSpacing ( 6 );

    showUiButton = new QPushButton ( tr ( "Show Plugin UI" ), this );
    showUiButton->setEnabled ( false );
    connect ( showUiButton, &QPushButton::clicked, this, &CPluginBrowserWidget::onShowUiClicked );
    controlsLayout->addRow ( showUiButton );

    removeButton = new QPushButton ( tr ( "Remove Plugin" ), this );
    removeButton->setEnabled ( false );
    connect ( removeButton, &QPushButton::clicked, this, &CPluginBrowserWidget::onRemovePluginClicked );
    controlsLayout->addRow ( removeButton );

    dryWetSlider = new QSlider ( Qt::Horizontal, this );
    dryWetSlider->setRange ( 0, 100 );
    dryWetSlider->setValue ( 100 );
    dryWetSlider->setEnabled ( false );
    connect ( dryWetSlider, &QSlider::valueChanged, this, &CPluginBrowserWidget::onDryWetChanged );

    dryWetLabel = new QLabel ( "100%", this );
    controlsLayout->addRow ( new QLabel ( tr ( "Dry/Wet Mix:" ), this ), dryWetSlider );
    controlsLayout->addRow ( new QLabel ( tr ( "Mix Value:" ), this ), dryWetLabel );

    loadedLayout->addWidget ( controlsGroup, 2 );
    pMainSplitter->addWidget ( loadedGroup );

    pMainSplitter->setSizes ( { 560, 170, 430 } );
    mainLayout->addWidget ( pMainSplitter, 1 );

    restoreLayoutState();
    connect ( pMainSplitter, &QSplitter::splitterMoved, this, [this] ( int, int ) { saveLayoutState(); } );
    connect ( pluginTreeView->header(), &QHeaderView::sectionResized, this, [this] ( int, int, int ) { saveLayoutState(); } );
    connect ( loadedTreeWidget->header(), &QHeaderView::sectionResized, this, [this] ( int, int, int ) { saveLayoutState(); } );

    // Tree selection changed
    connect ( pluginTreeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, [&]() {
        QModelIndexList selected = pluginTreeView->selectionModel()->selectedRows();
        if ( selected.isEmpty() )
        {
            loadButton->setEnabled ( false );
            activeSelectedPluginValid = false;
        }
        else
        {
            PluginDiscoveryResult res = getSelectedDiscoveryResult();
            setActiveSelectedPlugin ( res, true );
        }
    } );
}

void CPluginBrowserWidget::restoreLayoutState()
{
    if ( !pSettings )
        return;

    if ( !pSettings->vecPluginBrowserSplitter.isEmpty() )
    {
        pMainSplitter->restoreState ( pSettings->vecPluginBrowserSplitter );
    }

    if ( !pSettings->vecPluginBrowserScannedHeader.isEmpty() )
    {
        pluginTreeView->header()->restoreState ( pSettings->vecPluginBrowserScannedHeader );
    }

    if ( !pSettings->vecPluginBrowserLoadedHeader.isEmpty() )
    {
        loadedTreeWidget->header()->restoreState ( pSettings->vecPluginBrowserLoadedHeader );
    }
}

void CPluginBrowserWidget::saveLayoutState()
{
    if ( !pSettings || !pMainSplitter )
        return;

    pSettings->vecPluginBrowserSplitter      = pMainSplitter->saveState();
    pSettings->vecPluginBrowserScannedHeader = pluginTreeView->header()->saveState();
    pSettings->vecPluginBrowserLoadedHeader  = loadedTreeWidget->header()->saveState();
}

void CPluginBrowserWidget::populatePluginTree()
{
    pluginModel->removeRows ( 0, pluginModel->rowCount() );

    QList<PluginDiscoveryResult> plugins = discovery->getCachedPlugins();

    // Sort plugins by name
    std::sort ( plugins.begin(), plugins.end(), [] ( const PluginDiscoveryResult& a, const PluginDiscoveryResult& b ) {
        return a.name.compare ( b.name, Qt::CaseInsensitive ) < 0;
    } );

    for ( const auto& res : plugins )
    {
        QStandardItem* nameItem  = new QStandardItem ( res.name );
        QStandardItem* makerItem = new QStandardItem ( res.maker.isEmpty() ? tr ( "Unknown" ) : res.maker );

        QString        formatName = res.pluginType == 4 ? "LV2" : ( res.pluginType == 5 ? "VST2" : ( res.pluginType == 6 ? "VST3" : "CLAP" ) );
        QStandardItem* formatItem = new QStandardItem ( formatName );

        // Save indices or metadata to recover the selected item
        nameItem->setData ( res.name, Qt::UserRole + 0 );
        nameItem->setData ( res.maker, Qt::UserRole + 1 );
        nameItem->setData ( res.category, Qt::UserRole + 2 );
        nameItem->setData ( res.hints, Qt::UserRole + 3 );
        nameItem->setData ( res.binaryType, Qt::UserRole + 4 );
        nameItem->setData ( res.pluginType, Qt::UserRole + 5 );
        nameItem->setData ( res.filename, Qt::UserRole + 6 );
        nameItem->setData ( res.label, Qt::UserRole + 7 );
        nameItem->setData ( (double) res.uniqueId, Qt::UserRole + 8 );
        nameItem->setData ( res.audioIns, Qt::UserRole + 9 );
        nameItem->setData ( res.audioOuts, Qt::UserRole + 10 );
        nameItem->setData ( res.midiIns, Qt::UserRole + 11 );
        nameItem->setData ( res.midiOuts, Qt::UserRole + 12 );

        pluginModel->appendRow ( { nameItem, makerItem, formatItem } );
    }

}

PluginDiscoveryResult CPluginBrowserWidget::getSelectedDiscoveryResult() const
{
    PluginDiscoveryResult res{};
    QModelIndexList       selected = pluginTreeView->selectionModel()->selectedRows();
    if ( selected.isEmpty() )
        return res;

    QModelIndex sourceIdx = filterModel->mapToSource ( selected.first() );
    return getDiscoveryResultFromIndex ( sourceIdx );
}

PluginDiscoveryResult CPluginBrowserWidget::getDiscoveryResultFromIndex ( const QModelIndex& index ) const
{
    PluginDiscoveryResult res{};
    QStandardItem*        item = pluginModel->item ( index.row(), 0 );
    if ( !item )
        return res;

    res.name       = item->data ( Qt::UserRole + 0 ).toString();
    res.maker      = item->data ( Qt::UserRole + 1 ).toString();
    res.category   = item->data ( Qt::UserRole + 2 ).toInt();
    res.hints      = item->data ( Qt::UserRole + 3 ).toInt();
    res.binaryType = item->data ( Qt::UserRole + 4 ).toInt();
    res.pluginType = item->data ( Qt::UserRole + 5 ).toInt();
    res.filename   = item->data ( Qt::UserRole + 6 ).toString();
    res.label      = item->data ( Qt::UserRole + 7 ).toString();
    res.uniqueId   = static_cast<uint64_t> ( item->data ( Qt::UserRole + 8 ).toULongLong() );
    res.audioIns   = item->data ( Qt::UserRole + 9 ).toInt();
    res.audioOuts  = item->data ( Qt::UserRole + 10 ).toInt();
    res.midiIns    = item->data ( Qt::UserRole + 11 ).toInt();
    res.midiOuts   = item->data ( Qt::UserRole + 12 ).toInt();
    return res;
}

QString CPluginBrowserWidget::favoriteKey ( const PluginDiscoveryResult& result ) const
{
    return QStringLiteral ( "%1|%2|%3|%4|%5" )
        .arg ( result.binaryType )
        .arg ( result.pluginType )
        .arg ( QString::number ( result.uniqueId ) )
        .arg ( encodeField ( result.filename ) )
        .arg ( encodeField ( result.label ) );
}

QString CPluginBrowserWidget::serializeFavoriteResult ( const PluginDiscoveryResult& result ) const
{
    return ( QStringList() << encodeField ( result.name ) << encodeField ( result.maker ) << QString::number ( result.category )
                           << QString::number ( result.hints ) << QString::number ( result.binaryType ) << QString::number ( result.pluginType )
                           << encodeField ( result.filename ) << encodeField ( result.label ) << QString::number ( result.uniqueId )
                           << QString::number ( result.audioIns ) << QString::number ( result.audioOuts ) << QString::number ( result.midiIns )
                           << QString::number ( result.midiOuts ) )
        .join ( '|' );
}

bool CPluginBrowserWidget::deserializeFavoriteResult ( const QString& encoded, PluginDiscoveryResult& result ) const
{
    const QStringList parts = encoded.split ( '|' );
    if ( parts.size() != 13 )
        return false;

    result.name       = decodeField ( parts[0] );
    result.maker      = decodeField ( parts[1] );
    result.category   = parts[2].toInt();
    result.hints      = parts[3].toInt();
    result.binaryType = parts[4].toInt();
    result.pluginType = parts[5].toInt();
    result.filename   = decodeField ( parts[6] );
    result.label      = decodeField ( parts[7] );
    result.uniqueId   = parts[8].toULongLong();
    result.audioIns   = parts[9].toInt();
    result.audioOuts  = parts[10].toInt();
    result.midiIns    = parts[11].toInt();
    result.midiOuts   = parts[12].toInt();
    return true;
}

bool CPluginBrowserWidget::favoriteExists ( const QString& key ) const
{
    for ( int i = 0; i < favoritesListWidget->count(); ++i )
    {
        if ( favoritesListWidget->item ( i )->data ( Qt::UserRole ).toString() == key )
            return true;
    }
    return false;
}

void CPluginBrowserWidget::applyThemeAwareItemViewStyle ( QWidget* widget ) const
{
    if ( !widget )
        return;

    const QColor windowColor = widget->palette().color ( QPalette::Window );
    if ( windowColor.lightness() >= 128 )
        return;

    QPalette pal = widget->palette();
    pal.setColor ( QPalette::Base, QColor ( 0x2b, 0x2b, 0x2b ) );
    pal.setColor ( QPalette::AlternateBase, QColor ( 0x3a, 0x3a, 0x3a ) );
    pal.setColor ( QPalette::Text, Qt::white );
    pal.setColor ( QPalette::WindowText, Qt::white );
    pal.setColor ( QPalette::ButtonText, Qt::white );
    pal.setColor ( QPalette::HighlightedText, Qt::white );
    widget->setPalette ( pal );
    widget->setStyleSheet ( "QTreeView, QListWidget {"
                            " background: #2b2b2b;"
                            " alternate-background-color: #3a3a3a;"
                            " color: #ffffff;"
                            " selection-background-color: #4f5964;"
                            " selection-color: #ffffff;"
                            " }"
                            "QTreeView::item, QListWidget::item { color: #ffffff; }"
                            "QTreeView::item:alternate, QListWidget::item:alternate { background: #3a3a3a; color: #ffffff; }"
                            "QTreeView::item:selected, QListWidget::item:selected { background: #4f5964; color: #ffffff; }" );
}

void CPluginBrowserWidget::setActiveSelectedPlugin ( const PluginDiscoveryResult& result, bool valid )
{
    activeSelectedPlugin      = result;
    activeSelectedPluginValid = valid;
    loadButton->setEnabled ( valid );
}

QString CPluginBrowserWidget::pluginDetailsText ( const PluginDiscoveryResult& result ) const
{
    return QStringLiteral ( "Name: %1\n"
                            "Maker: %2\n"
                            "Format: %3\n"
                            "Audio Inputs: %4\n"
                            "Audio Outputs: %5\n"
                            "MIDI Inputs: %6\n"
                            "MIDI Outputs: %7\n"
                            "File: %8" )
        .arg ( result.name )
        .arg ( result.maker.isEmpty() ? tr ( "Unknown" ) : result.maker )
        .arg ( formatNameForType ( result.pluginType ) )
        .arg ( result.audioIns )
        .arg ( result.audioOuts )
        .arg ( result.midiIns )
        .arg ( result.midiOuts )
        .arg ( result.filename );
}

bool CPluginBrowserWidget::loadPluginResult ( const PluginDiscoveryResult& res )
{
    if ( res.name.isEmpty() )
        return false;

    if ( !carlaAdapter )
    {
        QMessageBox::critical ( this,
                                tr ( "Error" ),
                                tr ( "Carla audio host is not initialized. Make sure you are connected or the audio engine is running." ) );
        return false;
    }

    int newId = carla_adapter_add_plugin ( carlaAdapter,
                                           res.binaryType,
                                           res.pluginType,
                                           res.filename.toUtf8().constData(),
                                           res.name.toUtf8().constData(),
                                           res.label.toUtf8().constData(),
                                           res.uniqueId );

    if ( newId == -1 )
    {
        QMessageBox::warning ( this, tr ( "Load Failed" ), tr ( "Could not instantiate the plugin inside Carla." ) );
        return false;
    }

    refreshLoadedPlugins();
    for ( int i = 0; i < loadedTreeWidget->topLevelItemCount(); ++i )
    {
        QTreeWidgetItem* item = loadedTreeWidget->topLevelItem ( i );
        if ( item && item->data ( 1, Qt::UserRole ).toInt() == newId )
        {
            loadedTreeWidget->setCurrentItem ( item );
            break;
        }
    }

    return true;
}

void CPluginBrowserWidget::populateFavoritesList()
{
    favoritesListWidget->clear();

    if ( !pSettings )
        return;

    for ( const QString& encoded : pSettings->vstrFavoritePlugins )
    {
        PluginDiscoveryResult result;
        if ( !deserializeFavoriteResult ( encoded, result ) )
            continue;

        QListWidgetItem* item = new QListWidgetItem ( formatItemText ( result ) );
        item->setData ( Qt::UserRole, favoriteKey ( result ) );
        item->setData ( Qt::UserRole + 1, encoded );
        item->setToolTip ( formatItemToolTip ( result ) );
        favoritesListWidget->addItem ( item );
    }
}

void CPluginBrowserWidget::syncFavoritesToSettings()
{
    if ( !pSettings )
        return;

    pSettings->vstrFavoritePlugins.clear();
    pSettings->vstrFavoritePlugins.reserve ( favoritesListWidget->count() );
    for ( int i = 0; i < favoritesListWidget->count(); ++i )
    {
        const QListWidgetItem* item = favoritesListWidget->item ( i );
        pSettings->vstrFavoritePlugins.append ( item->data ( Qt::UserRole + 1 ).toString() );
    }
}

void CPluginBrowserWidget::addFavorite ( const PluginDiscoveryResult& result )
{
    const QString key = favoriteKey ( result );
    if ( favoriteExists ( key ) )
        return;

    QListWidgetItem* item = new QListWidgetItem ( formatItemText ( result ) );
    item->setData ( Qt::UserRole, key );
    const QString encoded = serializeFavoriteResult ( result );
    item->setData ( Qt::UserRole + 1, encoded );
    item->setToolTip ( formatItemToolTip ( result ) );
    favoritesListWidget->addItem ( item );
    syncFavoritesToSettings();
}

void CPluginBrowserWidget::removeFavoriteAtRow ( int row )
{
    if ( row < 0 || row >= favoritesListWidget->count() )
        return;

    delete favoritesListWidget->takeItem ( row );
    syncFavoritesToSettings();
}

void CPluginBrowserWidget::onSearchTextChanged ( const QString& text ) { filterModel->setFilterFixedString ( text ); }

void CPluginBrowserWidget::onCategoryChanged ( int index )
{
    int format = categoryCombo->itemData ( index ).toInt();
    if ( format == -1 )
    {
        filterModel->setFilterKeyColumn ( -1 );
        filterModel->setFilterRegExp ( QString() );
    }
    else
    {
        QString formatStr = format == 4 ? "LV2" : ( format == 5 ? "VST2" : ( format == 6 ? "VST3" : "CLAP" ) );
        filterModel->setFilterKeyColumn ( 2 ); // Format column
        filterModel->setFilterFixedString ( formatStr );
    }
}

void CPluginBrowserWidget::onScanButtonClicked()
{
    scanButton->setEnabled ( false );
    progressBar->setVisible ( true );
    progressBar->setValue ( 0 );

    discovery->startScan();
}

void CPluginBrowserWidget::onPluginFound ( const PluginDiscoveryResult& )
{
    // Populate live results during scan
    populatePluginTree();
}

void CPluginBrowserWidget::onScanProgress ( int percent ) { progressBar->setValue ( percent ); }

void CPluginBrowserWidget::onScanFinished ( const QList<PluginDiscoveryResult>& )
{
    scanButton->setEnabled ( true );
    progressBar->setVisible ( false );
    populatePluginTree();
    QMessageBox::information ( this, tr ( "Scan Finished" ), tr ( "Plugin scanning completed successfully." ) );
}

void CPluginBrowserWidget::onLoadButtonClicked()
{
    if ( favoritesListWidget && favoritesListWidget->currentItem() )
    {
        PluginDiscoveryResult favoriteResult;
        if ( deserializeFavoriteResult ( favoritesListWidget->currentItem()->data ( Qt::UserRole + 1 ).toString(), favoriteResult ) )
        {
            loadPluginResult ( favoriteResult );
            return;
        }
    }

    if ( activeSelectedPluginValid )
    {
        loadPluginResult ( activeSelectedPlugin );
        return;
    }

    loadPluginResult ( getSelectedDiscoveryResult() );
}

void CPluginBrowserWidget::onPluginDoubleClicked ( const QModelIndex& )
{
    const PluginDiscoveryResult res = getSelectedDiscoveryResult();
    if ( !res.name.isEmpty() )
        loadPluginResult ( res );
}

void CPluginBrowserWidget::onPluginContextMenuRequested ( const QPoint& pos )
{
    const QModelIndex viewIndex = pluginTreeView->indexAt ( pos );
    if ( !viewIndex.isValid() )
        return;

    const QModelIndex           sourceIndex = filterModel->mapToSource ( viewIndex );
    const PluginDiscoveryResult result      = getDiscoveryResultFromIndex ( sourceIndex );
    if ( result.name.isEmpty() )
        return;

    QMenu    menu ( this );
    QAction* addAction = menu.addAction ( tr ( "Add to favourites" ) );
    QAction* detailsAction = menu.addAction ( tr ( "Plugin details" ) );
    connect ( addAction, &QAction::triggered, this, [this, result]() { addFavorite ( result ); } );
    connect ( detailsAction, &QAction::triggered, this, [this, result]() {
        QMessageBox::information ( this, tr ( "Plugin details" ), pluginDetailsText ( result ) );
    } );
    menu.exec ( pluginTreeView->viewport()->mapToGlobal ( pos ) );
}

void CPluginBrowserWidget::onFavoriteDoubleClicked ( const QModelIndex& index )
{
    if ( !index.isValid() )
        return;

    const QListWidgetItem* item = favoritesListWidget->item ( index.row() );
    if ( !item )
        return;

    PluginDiscoveryResult result;
    if ( !deserializeFavoriteResult ( item->data ( Qt::UserRole + 1 ).toString(), result ) )
        return;

    setActiveSelectedPlugin ( result, true );
    loadPluginResult ( result );
}

void CPluginBrowserWidget::onFavoritesSelectionChanged()
{
    QListWidgetItem* item = favoritesListWidget->currentItem();
    if ( !item )
    {
        if ( pluginTreeView->selectionModel()->selectedRows().isEmpty() )
        {
            setActiveSelectedPlugin ( PluginDiscoveryResult{}, false );
        }
        return;
    }

    PluginDiscoveryResult result;
    if ( !deserializeFavoriteResult ( item->data ( Qt::UserRole + 1 ).toString(), result ) )
        return;

    setActiveSelectedPlugin ( result, true );
}

void CPluginBrowserWidget::onFavoriteContextMenuRequested ( const QPoint& pos )
{
    const QListWidgetItem* item = favoritesListWidget->item ( favoritesListWidget->indexAt ( pos ).row() );
    if ( !item )
        return;

    QMenu    menu ( this );
    QAction* removeAction = menu.addAction ( tr ( "Remove from favourites" ) );
    connect ( removeAction, &QAction::triggered, this, [this, item]() {
        const int row = favoritesListWidget->row ( const_cast<QListWidgetItem*> ( item ) );
        removeFavoriteAtRow ( row );
    } );
    menu.exec ( favoritesListWidget->viewport()->mapToGlobal ( pos ) );
}

void CPluginBrowserWidget::refreshLoadedPlugins()
{
    bRefreshingLoadedPlugins = true;

    QHash<int, bool> previousBypassed = mapPluginBypassed;
    mapPluginBypassed.clear();
    loadedTreeWidget->clear();

    if ( !carlaAdapter )
    {
        bRefreshingLoadedPlugins = false;
        onLoadedSelectionChanged();
        return;
    }

    int count = carla_adapter_get_plugin_count ( carlaAdapter );
    for ( int i = 0; i < count; ++i )
    {
        const char* name = carla_adapter_get_plugin_name ( carlaAdapter, i );
        if ( name )
        {
            bool bypassed = previousBypassed.value ( i, false );
            if ( carlaAdapter )
            {
                bypassed = !carla_adapter_get_active ( carlaAdapter, i );
            }
            mapPluginBypassed.insert ( i, bypassed );

            QTreeWidgetItem* item = new QTreeWidgetItem ( loadedTreeWidget );
            item->setText ( 1, QString::fromUtf8 ( name ) );
            item->setData ( 1, Qt::UserRole, i );
            item->setFlags ( item->flags() | Qt::ItemIsUserCheckable );
            item->setCheckState ( 0, bypassed ? Qt::Checked : Qt::Unchecked );
            loadedTreeWidget->addTopLevelItem ( item );
        }
    }

    bRefreshingLoadedPlugins = false;

    onLoadedSelectionChanged();
}

void CPluginBrowserWidget::setCarlaAdapter ( void* newHandle )
{
    carlaAdapter = newHandle;
    refreshLoadedPlugins();
}

void CPluginBrowserWidget::onLoadedSelectionChanged()
{
    QTreeWidgetItem* item = loadedTreeWidget->currentItem();
    if ( !item )
    {
        showUiButton->setEnabled ( false );
        removeButton->setEnabled ( false );
        dryWetSlider->setEnabled ( false );
        dryWetLabel->setText ( "100%" );
    }
    else
    {
        showUiButton->setEnabled ( true );
        removeButton->setEnabled ( true );
        dryWetSlider->setEnabled ( true );
    }
}

void CPluginBrowserWidget::onLoadedItemChanged ( QTreeWidgetItem* item, int column )
{
    if ( bRefreshingLoadedPlugins || !item || column != 0 || !carlaAdapter )
        return;

    const int  pluginId = item->data ( 1, Qt::UserRole ).toInt();
    const bool bypassed = item->checkState ( 0 ) == Qt::Checked;

    mapPluginBypassed[pluginId] = bypassed;
    carla_adapter_set_active ( carlaAdapter, pluginId, !bypassed );
}

void CPluginBrowserWidget::onRemovePluginClicked()
{
    QTreeWidgetItem* item = loadedTreeWidget->currentItem();
    if ( !item )
        return;

    int pluginId = item->data ( 1, Qt::UserRole ).toInt();
    if ( carla_adapter_remove_plugin ( carlaAdapter, pluginId ) )
    {
        mapPluginBypassed.remove ( pluginId );
        refreshLoadedPlugins();
    }
}

void CPluginBrowserWidget::onShowUiClicked()
{
    QTreeWidgetItem* item = loadedTreeWidget->currentItem();
    if ( !item )
        return;

    int pluginId = item->data ( 1, Qt::UserRole ).toInt();
    carla_adapter_show_plugin_ui ( carlaAdapter, pluginId, true );
}

void CPluginBrowserWidget::onDryWetChanged ( int value )
{
    QTreeWidgetItem* item = loadedTreeWidget->currentItem();
    if ( !item )
        return;

    int   pluginId = item->data ( 1, Qt::UserRole ).toInt();
    float val      = value / 100.0f;
    carla_adapter_set_drywet ( carlaAdapter, pluginId, val );
    dryWetLabel->setText ( QString ( "%1%" ).arg ( value ) );
}

void CPluginBrowserWidget::onConfigurePathsClicked()
{
    CScanPathsDlg dlg ( discovery, this );
    dlg.exec();
}
    
