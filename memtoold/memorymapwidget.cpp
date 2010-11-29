/*
 * memorymapwidget.cpp
 *
 *  Created on: 01.09.2010
 *      Author: chrschn
 */

#include "memorymapwidget.h"
#include <QApplication>
#include <QEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QMultiMap>
#include <QLinkedList>
#include <QTime>
#include <QTimer>
#include <QHelpEvent>
#include <QToolTip>
#include <QTextDocument>
#include <QTextCursor>
#include <QTextTable>
#include <math.h>
#include "memorymap.h"
#include "virtualmemory.h"
#include "varsetter.h"
#include "debug.h"

static const int margin = 3;
static const int cellSize = 16;
static const unsigned char MAX_PROB = 16;

static const QColor probColor[MAX_PROB+1] = {
        QColor(255,   0, 0),  //  0
        QColor(255,  16, 0),  //  1
        QColor(255,  32, 0),  //  2
        QColor(255,  48, 0),  //  3
        QColor(255,  64, 0),  //  4
        QColor(255,  80, 0),  //  5
        QColor(255,  96, 0),  //  6
        QColor(255, 112, 0),  //  7
        QColor(255, 128, 0),  //  8
        QColor(224, 128, 0),  //  9
        QColor(192, 128, 0),  // 10
        QColor(160, 128, 0),  // 11
        QColor(128, 128, 0),  // 12
        QColor( 96, 128, 0),  // 13
        QColor( 64, 128, 0),  // 14
        QColor( 32, 128, 0),  // 15
        QColor(  0, 128, 0)   // 16
};

//------------------------------------------------------------------------------

VisMapPiece::VisMapPiece(quint64 start, quint64 length, int type,
        unsigned char probability)
    : address(start), length(length), type(type), probability(probability)
{
}

//------------------------------------------------------------------------------

MemoryMapWidget::MemoryMapWidget(const MemoryMap* map, QWidget *parent)
    : QWidget(parent), _map(map), _visMapValid(false), _address(-1),
      /*_bytesPerPixelF(0),*/ _cols(0), _rows(0),
      _antialiasing(false), _isPainting(false),_isBuilding(false),
      _showOnlyKernelSpace(false), _shownAddrSpaceOffset(0)
{
//    setWindowTitle(tr("Difference view"));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setContentsMargins(margin, margin, margin, margin);
    setMouseTracking(true);
}


MemoryMapWidget::~MemoryMapWidget()
{
}


quint64 MemoryMapWidget::visibleAddrSpaceStart() const
{
    return _showOnlyKernelSpace ?
            _map->vmem()->memSpecs().pageOffset :
            0;
}


quint64 MemoryMapWidget::visibleAddrSpaceEnd() const
{
    return (_map && _map->vmem() &&
            _map->vmem()->memSpecs().arch == MemSpecs::x86_64) ?
            0xFFFFFFFFFFFFFFFFUL : 0xFFFFFFFFUL;
}


quint64 MemoryMapWidget::visibleAddrSpaceLength() const
{
    quint64 length = visibleAddrSpaceEnd() - visibleAddrSpaceStart();
    // Correct the size, if below the 64 bit boundary
    if (length < 0xFFFFFFFFFFFFFFFF)
        ++length;
    return length;
}


quint64 MemoryMapWidget::totalAddrSpace() const
{
    return (_map && _map->vmem() &&
            _map->vmem()->memSpecs().arch == MemSpecs::x86_64) ?
            0xFFFFFFFFFFFFFFFFUL : (1UL << 32);
}


quint64 MemoryMapWidget::totalAddrSpaceEnd() const
{
    return (_map && _map->vmem() &&
            _map->vmem()->memSpecs().arch == MemSpecs::x86_64) ?
            0xFFFFFFFFFFFFFFFFUL : 0xFFFFFFFFUL;
}


template<class T>
void insertSorted(T& list, const typename T::value_type value,
        bool eliminateDoubles = true)
{
    typename T::iterator it = list.begin();
    while ((it != list.end()) && (*it <= value)) {
        if (eliminateDoubles && (*it == value))
            return;
        ++it;
    }
    list.insert(it, value);
}


void MemoryMapWidget::buildVisMemMap()
{
    if (_visMapValid || !_buildMutex.tryLock())
        return;

    VarSetter<bool> building(&_isBuilding, true, false);

    emit buildingStarted();

    _mappings.clear();
    _maxIntensity = 0;

    if (_map) {
        int progress = 0;
        // Holds the currently active nodes, ordered by their end address
        typedef QMultiMap<quint64, const MemoryMapNode*> NodeMap;
        NodeMap nodes;
        // Holds the sorted (!!!) addresses at which we have to stop
        QLinkedList<quint64> addresses;

        quint64 lastAddr = 0;
        bool firstRound = true;
        unsigned char lastProbability = 0;
        int piecesMerged = 0;
        qint64 processed = 0, total = _map->vmemMap().size();
        bool nodesWasEmpty = false;

        for (PointerNodeMap::const_iterator vmap_it = _map->vmemMap().constBegin();
                vmap_it != _map->vmemMap().constEnd() && processed < total;
                ++vmap_it)
        {
            // The map contains multiple nodes with the same start address,
            // so curAddr == lastAddr is very likely
            quint64 curAddr = vmap_it.key();
            const MemoryMapNode* node = vmap_it.value();

            // Init everything in the first round
            if (firstRound) {
                firstRound = false;
                lastAddr = curAddr;
                lastProbability = (unsigned char)(MAX_PROB * node->probability());
                insertSorted(addresses, curAddr);
            }

            // Add all nodes with the same address into a map ordered by their
            // end address
            if (curAddr == lastAddr) {
                quint64 endAddr = curAddr + node->size();
                nodes.insert(endAddr, node);
#ifdef DEBUG
                if (endAddr < curAddr)
                    assert(endAddr >= curAddr);
#endif
                insertSorted(addresses, endAddr);
            }
            // We've moved on to the next address
            else {
                insertSorted(addresses, curAddr);

                // Go through all addresses which are <= curAddr
                do {
#ifdef DEBUG
                    // Make sure the list is sorted
//                    quint64 __prevAddr = addresses.front();
//                    QLinkedList<quint64>::iterator it = addresses.begin();
//                    while (++it != addresses.end()) {
//                        assert(__prevAddr < *it);
//                        __prevAddr = *it;
//                    }
#endif
                    quint64 addr = addresses.takeFirst();
#ifdef DEBUG
//                    if (lastAddr >= addr) {
//                        debugerr(QString("lastAddr = 0x%1 >= addr = 0x%2")
//                                .arg(lastAddr, 8, 16, QChar('0'))
//                                .arg(addr, 8, 16, QChar('0')));
//                    }
//                    assert(lastAddr < addr);
                    assert(addr <= curAddr);
#endif
                    // Remove all nodes that are out of scope
                    NodeMap::iterator nit = nodes.begin();
                    while (nit != nodes.end() && nit.key() <= addr)
                        nit = nodes.erase(nit);
                    // Length of MapPiece object
                    quint64 length = addr - lastAddr;

                    // Can we merge this with the previous piece?
                    if (!nodesWasEmpty) {
                        if (!_mappings.isEmpty() &&
                                _mappings.last().address + _mappings.last().length == lastAddr &&
                                _mappings.last().probability == lastProbability)
                        {
                            _mappings.last().length += length;
                            ++piecesMerged;
                        }
                        // No, create a new one
                        else {
                            _mappings.append(VisMapPiece(lastAddr, length, ptUsed,
                                    lastProbability));
//                            if (lastProbability > _maxIntensity)
//                                _maxIntensity = lastProbability;
                        }
#ifdef DEBUG
                        if (_mappings.last().length > 0xFFFFFF)
                            debugmsg(QString("[%1] addr = 0x%2, intensity = %3, len = %4")
                                                    .arg(_mappings.size() - 1)
                                                    .arg(_mappings.last().address, 8, 16, QChar('0'))
                                                    .arg(_mappings.last().probability, 3)
                                                    .arg(_mappings.last().length));
#endif
                    }

                    // Update the max. probability
                    lastProbability = 0;
                    for (NodeMap::const_iterator it = nodes.constBegin();
                            it != nodes.constEnd(); ++it)
                    {
                        unsigned char p =
                                (unsigned char)(MAX_PROB * it.value()->probability());
                        if (lastProbability < p)
                            lastProbability = p;
                    }
                    nodesWasEmpty = nodes.isEmpty(); // any "active" nodes?
                    lastAddr = addr;

                    // Stop if we've reached the current address
                } while (!addresses.isEmpty() &&  addresses.front() <= curAddr);
            }

            // Emit a signal of the building progress
            int i = (int) (++processed / (float)total * 100.0);
            if (i != progress) {
                progress = i;
                emit buildingProgress(progress);
            }

            QApplication::processEvents();
        }

#ifdef DEBUG
        debugmsg(__PRETTY_FUNCTION__ << ": piecesMerged = " << piecesMerged
                << ", remaining pieces = " << _mappings.count());

        if (_mappings.count() < 20) {
            for (int i = 0; i < _mappings.size(); ++i) {
                debugmsg(QString("[%1] addr = 0x%2, intensity = %3, len = %4")
                        .arg(i)
                        .arg(_mappings[i].address, 8, 16, QChar('0'))
                        .arg(_mappings[i].probability, 3)
                        .arg(_mappings[i].length));
            }
        }
#endif

    }

    _visMapValid = true;
    emit buildingStopped();

    _buildMutex.unlock();

    // Repaint the widget
    update();
}


void MemoryMapWidget::closeEvent(QCloseEvent* e)
{
    e->accept();
}


void MemoryMapWidget::resizeEvent(QResizeEvent* /*e*/)
{
    // Paint the bytes as squares into the visible widget area
    quint64 addrSpaceLen = visibleAddrSpaceLength();

//    double ratio = width() / (double) height();
//    _cols = (qint64) ceil(sqrt(addrSpaceLen * ratio));
//    _rows = addrSpaceLen / _cols;
//    if (addrSpaceLen % _cols != 0)
//        _rows++;

    _cols = drawWidth() / cellSize;
    _rows = drawHeight() / cellSize;

    // TODO: Fix situations where addrSpaceLen < _cols * _rows
//    _bytesPerPixelX = _cols / (double) drawWidth();
//    _bytesPerPixelY = _rows / (double) drawHeight();
    double bytesPerPixelF = addrSpaceLen / (double)(_cols * _rows);
    _bytesPerPixelL = ceil(bytesPerPixelF);
}


void MemoryMapWidget::mouseMoveEvent(QMouseEvent *e)
{
    // Find out at which address the mouse pointer is
    int x = e->x() - margin, y = e->y() - margin;
    if (x < 0 || y < 0 || x >= drawWidth() || y >= drawHeight())
        return;

    int c = x / cellSize, r = y / cellSize;

    quint64 addr = (r * _cols + c) * _bytesPerPixelL + visibleAddrSpaceStart();

    if (_address != addr && addr < visibleAddrSpaceEnd()) {
        _address = addr;
        emit addressChanged(_address);
    }
}


void MemoryMapWidget::paintEvent(QPaintEvent * e)
{
    // Set _isPainting to true now and to false on exit
    if (_isPainting) {
        debugmsg("Ignoring paint event");
        e->ignore();
        return;
    }

    VarSetter<bool> painting(&_isPainting, true, false);

    QColor unusedColor(255, 255, 255);
    QColor usedColor(0, 0, 0);
    QColor gridColor(240, 240, 240);
    QColor bgColor(255, 255, 255);

    QPainter painter(this);
    if (_antialiasing)
        painter.setRenderHint(QPainter::Antialiasing);

    // Draw in "byte"-coordinates, scale bytes to entire widget
    painter.translate(margin, margin);
//    painter.scale(1.0 / _bytesPerPixelX, 1.0 / _bytesPerPixelY);
    // Clear widget
    painter.setPen(Qt::NoPen);
    painter.setBrush(bgColor);
    painter.drawRect(0, 0, _cols * cellSize, _rows * cellSize);
//    // Draw the whole address space  as "unused"
//    painter.setBrush(unusedColor);
//    quint64 h = totalAddrSpace() / _cols;
//    if (h >= 1)
//        painter.drawRect(0, 0, _cols, h);
//
//    painter.drawRect(0, h, totalAddrSpace() - (_cols * h), 1);

    // Draw all used parts
    painter.setBrush(usedColor);
    int lastColor = -1;

    // State of current memory cell
    struct {
        quint64 relStart, nextRelStart, index;
        int elemCount;
        VisMapPiece::PropType maxProb, minProb;
        bool isFirst;
    } cell;

    cell.isFirst = true;

    for (int i = 0; i < _mappings.size(); ++i) {

        if (_mappings[i].address + _mappings[i].length < visibleAddrSpaceStart() ||
                _mappings[i].address > visibleAddrSpaceEnd())
            continue;

        quint64 len = _mappings[i].length;
        // Find the row and column of the offset
        quint64 relAddr;
        // Beware of overflows when calculating the relative address
        if (visibleAddrSpaceStart() > _mappings[i].address) {
            relAddr = 0;
            // shorten length to the visible part
            len -= visibleAddrSpaceStart() - _mappings[i].address;
        }
        else
            relAddr = _mappings[i].address - visibleAddrSpaceStart();

        while (len > 0) {
            // In which memory cell does current address lie?
            quint64 newIndex = relAddr / _bytesPerPixelL;
            // Does this mapping start in a new cell?
            if (cell.isFirst || cell.index != newIndex) {
                if (!cell.isFirst) {
                    // Draw previous cell
                    quint64 r = cell.index / _cols;
                    quint64 c = cell.index % _cols;

                    if (cellSize == 1) {
                        if (cell.maxProb != lastColor) {
                            lastColor = cell.maxProb;
                            painter.setPen(probColor[lastColor]);
                        }
                        painter.drawPoint(c, r);
                    }
                    else {
//                        if (cell.maxProb != lastColor) {
                            lastColor = cell.maxProb;
                            painter.setBrush(probColor[lastColor]);
//                        }
                        painter.drawRect(c * cellSize, r * cellSize,
                                cellSize - 1, cellSize - 1);
                    }
                }
                // Init cell with new values
                cell.index = newIndex;
                cell.relStart = cell.index * _bytesPerPixelL;
                cell.nextRelStart = (cell.index + 1) * _bytesPerPixelL;
                cell.elemCount = 1;
                cell.minProb = cell.maxProb = _mappings[i].probability;
                cell.isFirst = false;
            }
            else {
                ++cell.elemCount;
                if (cell.maxProb < _mappings[i].probability)
                    cell.maxProb = _mappings[i].probability;
                if (cell.minProb > _mappings[i].probability)
                    cell.minProb = _mappings[i].probability;
            }

            // Handle cases where relAddr + len > cell.nextRelStart
            if (relAddr + len > cell.nextRelStart) {
                len -= cell.nextRelStart - relAddr;
                relAddr = cell.nextRelStart;
            }
            else
                break;
        }

    }

    // Only draw grid lines if we have at least 3 pixel per byte
    if (cellSize > 1 && gridColor != unusedColor) {
        painter.setPen(gridColor);
        for (int i = 1; i < _cols; i++)
            painter.drawLine(i * cellSize - 1, 0, i * cellSize - 1, _rows * cellSize - 1);
        for (int i = 1; i < _rows; i++)
            painter.drawLine(0, i * cellSize - 1, _cols * cellSize - 1, i * cellSize - 1);
    }
}


bool MemoryMapWidget::event(QEvent *event)
{
    if (event->type() == QEvent::ToolTip) {
        QHelpEvent* helpEvent = static_cast<QHelpEvent *>(event);

        // Find out at which address the mouse pointer is
        int x = helpEvent->x() - margin, y = helpEvent->y() - margin;
        if (x < 0 || y < 0 || x >= drawWidth() || y >= drawHeight()) {
            QToolTip::hideText();
            event->ignore();
        }
        else {
            int c = x / cellSize, r = y / cellSize;
            quint64 addrStart = (r * _cols + c) * _bytesPerPixelL + visibleAddrSpaceStart();
            quint64 addrEnd = addrStart + _bytesPerPixelL - 1;
            // Limit addrEnd to acutal end of address space. There are two cases:
            // 32-bit: addrEnd is beyond totalAddrSpaceEnd()
            // 64-bit: addrEnd is close to zero, because an overflow occured
            if (addrEnd > totalAddrSpaceEnd() ||
                totalAddrSpaceEnd() - addrStart < totalAddrSpaceEnd() - addrEnd)
            {
                addrEnd = totalAddrSpaceEnd();
            }
            int width = _map->vmem()->memSpecs().sizeofUnsignedLong << 1;

            QTextDocument doc;
            QTextCursor cur(&doc);

            QTextBlockFormat hdrBlkFmt;
            hdrBlkFmt.setAlignment(Qt::AlignHCenter);
//            QTextCharFormat hdrChrFmt;
//            hdrChrFmt.setFontPointSize(doc.defaultFont().pointSizeF());
//            hdrChrFmt.setFontWeight(QFont::Bold);
//            hdrChrFmt.setFontFixedPitch(true);

            QFont font; // = doc.defaultFont();
            font.setPointSizeF(8);
            doc.setDefaultFont(font);

            const QTextBlockFormat defBlkFmt = cur.blockFormat();
            const QTextCharFormat defCharFmt = cur.charFormat();
            QTextBlockFormat cellProbBlkFmt = cur.blockFormat();
            cellProbBlkFmt.setAlignment(Qt::AlignRight);
            QTextCharFormat cellAddrCharFmt = cur.charFormat();
            cellAddrCharFmt.setFontFixedPitch(true);
            QTextCharFormat cellTypeCharFmt = cur.charFormat();
            cellTypeCharFmt.setFontWeight(QFont::Bold);
            QTextCharFormat cellNameCharFmt = cur.charFormat();
//            cellNameCharFmt.setFontWeight(QFont::Bold);
            QTextCharFormat cellProbCharFmt = cur.charFormat();

            cur.insertHtml(QString("<h3><tt>0x%1</tt>&nbsp;&ndash;&nbsp;<tt>0x%2</tt></h3>")
                            .arg(addrStart, width, 16, QChar('0'))
                            .arg(addrEnd, width, 16, QChar('0')));
            cur.mergeBlockFormat(hdrBlkFmt);
//            cur.setCharFormat(hdrChrFmt);


            QString elem;
            ConstNodeList nodes = _map->vmemMapsInRange(addrStart, addrEnd);
            qSort(nodes.begin(), nodes.end(), NodeProbabilityGreaterThan);

            if (!nodes.isEmpty()) {
                const int maxTables = 1;
                const int maxRowsPerTable = 30;
                int rows = nodes.size() > maxTables * maxRowsPerTable ?
                        maxTables * maxRowsPerTable : nodes.size();

                QTextTableFormat tabFmt;
                tabFmt.setCellSpacing(2);
                tabFmt.setBorderStyle(QTextFrameFormat::BorderStyle_None);

                cur.insertBlock(defBlkFmt, defCharFmt);
                cur.insertHtml(QString("<b>%1 object(s)</b>").arg(nodes.size()));

                cur.insertBlock(defBlkFmt, defCharFmt);

                int rowCnt = 0;
                for (ConstNodeList::iterator it = nodes.begin();
                        it != nodes.end() && rowCnt < rows;
                        ++it, ++rowCnt)
                {
                    if (rowCnt % maxRowsPerTable == 0) {
                        int r = rows - rowCnt >= maxRowsPerTable ?
                                maxRowsPerTable : rows - rowCnt;
                        cur.movePosition(QTextCursor::End);
                        cur.insertTable(r, 4, tabFmt);
                    }

                    const MemoryMapNode* node = *it;
                    int colorIdx = (unsigned char)(MAX_PROB * node->probability());
                    cellProbCharFmt.setForeground(QBrush(probColor[colorIdx]));

                    cur.setBlockFormat(defBlkFmt);
                    cur.setCharFormat(cellAddrCharFmt);
                    cur.insertText(QString("0x%1")
                            .arg(node->address(), width, 16, QChar('0')));
                    cur.movePosition(QTextCursor::NextCell);

                    cur.setCharFormat(cellTypeCharFmt);
                    cur.insertText(node->type()->prettyName());
                    cur.movePosition(QTextCursor::NextCell);

                    cur.setCharFormat(cellNameCharFmt);
                    QString name = node->fullName();
                    if (name.length() > 100)
                        name.replace(49, name.length() - 97, "...");
                    cur.insertText(name);
                    cur.movePosition(QTextCursor::NextCell);

                    cur.setBlockFormat(cellProbBlkFmt);
                    cur.setCharFormat(cellProbCharFmt);
                    cur.insertText(QString("%1%")
                            .arg(node->probability() * 100.0, 0, 'f', 1));
                    cur.movePosition(QTextCursor::NextCell);
                }

                if (rows < nodes.size()) {
                    cur.movePosition(QTextCursor::End);
                    cur.insertBlock(defBlkFmt, defCharFmt);
                    cur.insertText(QString("(%1 more)")
                            .arg(nodes.size() - rows));
                }
            }

            QToolTip::showText(helpEvent->globalPos(), doc.toHtml());
        }

        return true;
     }
     return QWidget::event(event);
}


int MemoryMapWidget::drawWidth() const
{
    return width() - 2*margin;
}


int MemoryMapWidget::drawHeight() const
{
    return height() - 2*margin;
}


const MemoryMap* MemoryMapWidget::map() const
{
    return _map;
}


void MemoryMapWidget::setMap(const MemoryMap* map)
{
    if (_map == map)
        return;
    _map = map;
    forceMapRecreaction();
}


bool MemoryMapWidget::antiAliasing() const
{
    return _antialiasing;
}


void MemoryMapWidget::setAntiAliasing(bool value)
{
    _antialiasing = value;
    update();
}


bool MemoryMapWidget::isPainting() const
{
    return _isPainting;
}


bool MemoryMapWidget::isBuilding() const
{
    return _isBuilding;
}


void MemoryMapWidget::forceMapRecreaction()
{
    _visMapValid = false;
    if (!QMetaObject::invokeMethod(this, "buildVisMemMap"))
        debugerr("Cannot invoke buildVisMemMap() on this widget!");
}


void MemoryMapWidget::setShowOnlyKernelSpace(bool value)
{
    if (_showOnlyKernelSpace == value)
        return;
    _showOnlyKernelSpace = value;
    resizeEvent(0);
    update();
}


bool MemoryMapWidget::showOnlyKernelSpace() const
{
    return _showOnlyKernelSpace;
}
