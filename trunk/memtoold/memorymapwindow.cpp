#include "memorymapwindow.h"

#include "memorymapwidget.h"
#include <QStatusBar>
#include <QVBoxLayout>

MemoryMapWindow* memMapWindow = 0;


MemoryMapWindow::MemoryMapWindow(QWidget *parent)
    : QMainWindow(parent), _memMapWidget(0)
{
	ui.setupUi(this);
	setWindowTitle(tr("MemoryTool - Differences View"));
	_memMapWidget = new MemoryMapWidget(0, this);
	setCentralWidget(_memMapWidget);

	connect(_memMapWidget, SIGNAL(addressChanged(quint64)),
	        SLOT(virtualAddressChanged(quint64)));
}


MemoryMapWindow::~MemoryMapWindow()
{
}


void MemoryMapWindow::virtualAddressChanged(quint64 address)
{
    int width = _memMapWidget->virtAddrSpace() > 0xFFFFFFFFUL ? 16 : 8;
    QString s = QString("Position: 0x%1").arg(address, width, 16, QChar('0'));
    statusBar()->showMessage(s);
}


void MemoryMapWindow::setMap(const MemoryMap* map)
{
    _memMapWidget->setMap(map);
}
