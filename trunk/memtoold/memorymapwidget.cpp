/*
 * memorymapwidget.cpp
 *
 *  Created on: 01.09.2010
 *      Author: chrschn
 */

#include "memorymapwidget.h"
#include <QEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QMultiMap>
#include <QLinkedList>
#include <QTime>
#include <math.h>
#include "memorymap.h"
#include "virtualmemory.h"
#include "debug.h"

static const int margin = 3;

//------------------------------------------------------------------------------

VisMapPiece::VisMapPiece(quint64 start, quint64 length, int type,
        int intensity)
    : address(start), length(length), type(type), intensity(intensity)
{
}

//------------------------------------------------------------------------------

MemoryMapWidget::MemoryMapWidget(const MemoryMap* map, QWidget *parent)
    : QWidget(parent), _map(map), _visMapValid(false), _address(-1),
      _bytesPerPixelX(0), _bytesPerPixelY(0), _cols(0), _rows(0)
{
//    setWindowTitle(tr("Difference view"));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setContentsMargins(margin, margin, margin, margin);
    setMouseTracking(true);
}


MemoryMapWidget::~MemoryMapWidget()
{
}


quint64 MemoryMapWidget::virtAddrSpace() const
{
    return _map && _map->vmem() &&
            _map->vmem()->memSpecs().arch == MemSpecs::x86_64 ?
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
    if (_visMapValid)
        return;

    debugenter();

    _mappings.clear();
    _maxIntensity = 0;

    if (_map) {
        // Holds the currently active nodes, ordered by their end address
        typedef QMultiMap<quint64, const MemoryMapNode*> NodeMap;
        NodeMap nodes;
        // Holds the sorted (!!!) addresses at which we have to stop
        QLinkedList<quint64> addresses;

        quint64 lastAddr = 0;
        int lastObjCount = 0;
        int piecesMerged = 0;

        for (PointerNodeMap::const_iterator vmap_it = _map->vmemMap().constBegin();
                vmap_it != _map->vmemMap().constEnd(); ++vmap_it)
        {
            // The map contains multiple nodes with the same start address,
            // so curAddr == lastAddr is very likely
            quint64 curAddr = vmap_it.key();
            const MemoryMapNode* node = vmap_it.value();

            // Init everything in the first round
            if (!lastAddr) {
                lastAddr = curAddr;
                lastObjCount = _map->vmemMap().count(curAddr);
            }

            // Add all nodes with the same address into a map ordered by their
            // end address
            if (curAddr == lastAddr) {
                nodes.insert(curAddr + node->size(), node);
                quint64 endAddr = curAddr + node->size();
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
                    quint64 addr = addresses.front();
                    addresses.pop_front();
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
                    if (lastObjCount > 0) {
                        if (!_mappings.isEmpty() &&
                                _mappings.last().address + _mappings.last().length == lastAddr &&
                                _mappings.last().intensity == lastObjCount)
                        {
                            _mappings.last().length += length;
                            ++piecesMerged;
                        }
                        // No, create a new one
                        else {
                            _mappings.append(VisMapPiece(lastAddr, length, ptUsed,
                                    lastObjCount));
                            if (lastObjCount > _maxIntensity)
                                _maxIntensity = lastObjCount;
                        }
#ifdef DEBUG
                        if (_mappings.last().length > 0xFFFFFF)
                            debugmsg(QString("[%1] addr = 0x%2, intensity = %3, len = %4")
                                                    .arg(_mappings.size() - 1)
                                                    .arg(_mappings.last().address, 8, 16, QChar('0'))
                                                    .arg(_mappings.last().intensity, 3)
                                                    .arg(_mappings.last().length));
#endif
                    }

                    // Update the last object count and address
                    lastObjCount = nodes.size(); // no. of "active" nodes
                    lastAddr = addr;

                    // Stop if we've reached the current address
                } while (!addresses.isEmpty() &&  addresses.front() <= curAddr);
            }

//            lastAddr = curAddr;
        }

#ifdef DEBUG
        debugmsg(__PRETTY_FUNCTION__ << ": piecesMerged = " << piecesMerged
                << ", remaining pieces = " << _mappings.count());

        if (_mappings.count() < 20) {
            for (int i = 0; i < _mappings.size(); ++i) {
                debugmsg(QString("[%1] addr = 0x%2, intensity = %3, len = %4")
                        .arg(i)
                        .arg(_mappings[i].address, 8, 16, QChar('0'))
                        .arg(_mappings[i].intensity, 3)
                        .arg(_mappings[i].length));
            }
        }
#endif

    }

    _visMapValid = true;
    debugleave();
}


void MemoryMapWidget::closeEvent(QCloseEvent* e)
{
    e->accept();
}


void MemoryMapWidget::resizeEvent(QResizeEvent* e)
{
    // Paint the bytes as squares into the visible widget area
    double ratio = width() / (double) height();
    _cols = (qint64) ceil(sqrt(virtAddrSpace() * ratio));
    _rows = virtAddrSpace() / _cols;
    if (virtAddrSpace() % _cols != 0)
        _rows++;

    _bytesPerPixelX = _cols / (double) drawWidth();
    _bytesPerPixelY = _rows / (double) drawHeight();

    e->accept();
}


void MemoryMapWidget::mouseMoveEvent(QMouseEvent *e)
{
    int drawWidth = width() - 2*margin;
    int drawHeight = height() - 2*margin;

    // Find out at which address the mouse pointer is
    int x = e->x() - margin, y = e->y() - margin;
    if (x < 0 || y < 0 || x >= drawWidth || y >= drawHeight) return;
    quint64 col = (quint64)(_bytesPerPixelX * x);
    quint64 row = (quint64)(_bytesPerPixelY * y);
    quint64 addr = row * _cols + col;

    if (_address != addr && addr < virtAddrSpace()) {
        _address = addr;
        emit addressChanged(_address);
    }
}


void MemoryMapWidget::paintEvent(QPaintEvent * /*e*/)
{
    buildVisMemMap();

    QColor unusedColor(255, 255, 255);
    QColor usedColor(0, 0, 0);
    QColor gridColor(255, 255, 255);
    QColor bgColor(255, 255, 255);
    QPainter painter(this);
    if (_antialiasing)
        painter.setRenderHint(QPainter::Antialiasing);

    // Draw in "byte"-coordinates, scale bytes to entire widget
    painter.translate(margin, margin);
    painter.scale(1.0 / _bytesPerPixelX, 1.0 / _bytesPerPixelY);
    // Clear widget
    painter.setPen(Qt::NoPen);
    painter.setBrush(bgColor);
    painter.drawRect(0, 0, _cols, _rows);
    // Draw the whole address space  as "unused"
    painter.setBrush(unusedColor);
    quint64 h = virtAddrSpace() / _cols;
    if (h >= 1)
        painter.drawRect(0, 0, _cols, h);

    painter.drawRect(0, h, virtAddrSpace() - (_cols * h), 1);
//    painter.end();

    // Draw all used parts
    painter.setBrush(usedColor);
//    QTime timer;
//    timer.start();
    int lastIntensity = 255;
    for (int i = 0; i < _mappings.size(); ++i) {

//        if (!painter.isActive()) {
//            painter.begin(this);
//            painter.translate(margin, margin);
//            painter.scale(1.0 / _bytesPerPixelX, 1.0 / _bytesPerPixelY);
//            painter.setPen(Qt::NoPen);
//        }

        if (_mappings[i].intensity != lastIntensity) {
//            usedColor.setRedF(_mappings[i].intensity / (float)_maxIntensity);
            painter.setBrush(usedColor);
        }

        quint64 len = _mappings[i].length;
        // Find the row and column of the offset
        quint64 r = _mappings[i].address / _cols;
        quint64 c = _mappings[i].address % _cols;

        // The memory region is drawn in three operations: the first line, the
        // middle (rectangular) block, and finally the list line.

        // Draw the first line
        if (c != 0) {
            quint64 currLen = (_cols - c > len) ? len : _cols - c;
            painter.drawRect(c, r, currLen, 1);
            // Shorten remaining length by no. of bytes drawn
            len -= currLen;
            // Next drawing starts in next line
            r++;
        }
        // Draw the largest possible block
        h = len / _cols;
        if (h > 0) {
            painter.drawRect(0, r, _cols, h);
            // Shorten length by drawn bytes
            len %= _cols;
            // Next drawing starts h lines below
            r += h;
        }
        // Draw the last line
        if (len > 0) {
            painter.drawRect(0, r, len, 1);
        }

//        // Flush the painter from time to time
//        if (timer.elapsed() > 500) {
//            painter.end();
//            timer.restart();
//        }
    }

    // Only draw grid lines if we have at least 3 pixel per byte
    if (_bytesPerPixelX <= 1.0/3 && _bytesPerPixelY <= 1.0/3) {
        painter.setPen(gridColor);
        for (int i = 0; i <= _cols; i++)
            painter.drawLine(i, 0, i, _rows);
        for (int i = 0; i <= _rows; i++)
            painter.drawLine(0, i, _cols, i);
    }
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
    _visMapValid = false;
    update();
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



