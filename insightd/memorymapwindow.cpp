#include "memorymapwindow.h"

#include "memorymapwidget.h"
#include <QStatusBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QLabel>
//#include <QProgressBar>

MemoryMapWindow* memMapWindow = 0;

const char* LBL_MOVE_CURSOR_POS = "Position: 0x%1";
const char* LBL_BUILDING = "Processing map";


MemoryMapWindow::MemoryMapWindow(QWidget *parent)
    : QMainWindow(parent), _memMapWidget(0)
{
	ui.setupUi(this);

	setWindowTitle(tr("MemoryTool - Differences View"));

	// Setup main window area
	_memMapWidget = new MemoryMapWidget(0, this);

	QCheckBox* cbAntiAlias = new QCheckBox(this);
	cbAntiAlias->setText(tr("Enable anti-aliasing"));
	cbAntiAlias->setChecked(false);

	QCheckBox* cbKernelSpaceOnly = new QCheckBox(this);
	cbKernelSpaceOnly->setText(tr("Show only kernel space"));
	cbKernelSpaceOnly->setChecked(false);

	QHBoxLayout* topLayout = new QHBoxLayout();
	topLayout->addWidget(cbAntiAlias);
	topLayout->addWidget(cbKernelSpaceOnly);

	centralWidget()->layout()->addItem(topLayout);
    centralWidget()->layout()->addWidget(_memMapWidget);

	connect(_memMapWidget, SIGNAL(addressChanged(quint64)),
	        SLOT(virtualAddressChanged(quint64)));
	connect(cbAntiAlias, SIGNAL(toggled(bool)),
	        _memMapWidget, SLOT(setAntiAliasing(bool)));
	connect(cbKernelSpaceOnly, SIGNAL(toggled(bool)),
	        _memMapWidget, SLOT(setShowOnlyKernelSpace(bool)));



	// Setup status bar
//	_sbCursorPosition = new QLabel(this);
//	statusBar()->addPermanentWidget(_sbCursorPosition);

//	_sbBuildingMsg = new QLabel(this);
//	_sbBuildingMsg->setText(LBL_BUILDING);
//	_sbBuildingMsg->hide();
//	statusBar()->addPermanentWidget(_sbBuildingMsg);

//	_sbBuildingProgBar = new QProgressBar(this);
//	_sbBuildingProgBar->setMinimum(0);
//	_sbBuildingProgBar->setMaximum(100);
//	_sbBuildingProgBar->setValue(0);
//	_sbBuildingProgBar->hide();
//    connect(_memMapWidget, SIGNAL(buildingProgress(int)),
//            _sbBuildingProgBar, SLOT(setValue(int)));
//    statusBar()->addPermanentWidget(_sbBuildingProgBar);
//
//    connect(_memMapWidget, SIGNAL(buildingStarted()),
//            SLOT(memMapBuildingStarted()));
//    connect(_memMapWidget, SIGNAL(buildingStopped()),
//            SLOT(memMapBuildingStopped()));
}


MemoryMapWindow::~MemoryMapWindow()
{
}


void MemoryMapWindow::virtualAddressChanged(quint64 address)
{
    int width = _memMapWidget->totalAddrSpaceEnd() >= (1UL << 32) ? 16 : 8;
    QString s = QString(LBL_MOVE_CURSOR_POS)
                    .arg(address, width, 16, QChar('0'));
//    _sbCursorPosition->setText(s);
    statusBar()->showMessage(s);
}


//void MemoryMapWindow::memMapBuildingStarted()
//{
//    _sbBuildingMsg->show();
//    _sbBuildingProgBar->show();
//}


//void MemoryMapWindow::memMapBuildingStopped()
//{
//    _sbBuildingProgBar->hide();
//    _sbBuildingMsg->hide();
//    _memMapWidget->update();
//}


//void MemoryMapWindow::setMap(const MemoryMap* map)
//{
//    _memMapWidget->setMap(map);
//}


MemoryMapWidget* MemoryMapWindow::mapWidget()
{
    return _memMapWidget;
}
