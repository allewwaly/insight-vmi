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
#include <QSet>
#include <math.h>
#include "memorymap.h"
#include "virtualmemory.h"
#include "debug.h"

MemoryMapWidget* mapWidget = 0;

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
    setWindowTitle(tr("Difference view"));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setContentsMargins(margin, margin, margin, margin);
    setMouseTracking(true);
    resize(400, 300);
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


void MemoryMapWidget::buildVisMemMap()
{
    if (_visMapValid)
        return;

    debugenter();

    _mappings.clear();
    _maxIntensity = 0;

    // Paint the bytes as squares into the area
    double ratio = width() / (double) height();
    _cols = (qint64) ceil(sqrt(virtAddrSpace() * ratio));
    _rows = virtAddrSpace() / _cols;
    if (virtAddrSpace() % _cols != 0)
        _rows++;

    _bytesPerPixelX = _cols / (double) drawWidth();
    _bytesPerPixelY = _rows / (double) drawHeight();

    if (_map) {
        // Holds the currently active nodes, ordered by their end address
        typedef QMultiMap<quint64, const MemoryMapNode*> NodeMap;
        NodeMap nodes;
        // Holds the addresses at which we have to stop
        QSet<quint64> addresses;

        quint64 lastAddr = 0;
        int lastObjCount = 0;
        int piecesMerged = 0;

        for (PointerNodeMap::const_iterator vmap_it = _map->vmemMap().constBegin();
                vmap_it != _map->vmemMap().constEnd(); ++vmap_it)
        {
            quint64 curAddr = vmap_it.key();
            const MemoryMapNode* node = vmap_it.value();

            // Init everything in the first round
            if (!lastAddr) {
                lastAddr = curAddr;
                lastObjCount = _map->vmemMap().count(curAddr);
            }

            // Add all nodes with the same address into a map ordered by their end
            // address
            if (curAddr == lastAddr) {
                nodes.insert(curAddr + node->size(), node);
                addresses.insert(curAddr + node->size());
            }
            else {
                addresses.insert(curAddr);
                // Go through all addresses which are <= curAddr
                QSet<quint64>::iterator addr_it = addresses.begin();
                do {
                    quint64 addr = *addr_it;
                    // Remove all nodes that are out of scope
                    NodeMap::iterator nit = nodes.begin();
                    while (nit != nodes.end() && nit.key() < addr)
                        nit = nodes.erase(nit);
                    // Length of MapPiece object
                    quint64 length = addr - lastAddr;

                    // Can we merge this with the previous piece?
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

                    // Update the last object count and address
                    lastObjCount = nodes.size(); // no. of "active" nodes
                    lastAddr = curAddr;

                    // Remove used address and advance iterator
                    addr_it = addresses.erase(addr_it);
                    // Stop if we've reached the current address
                } while (*addr_it < curAddr && addr_it != addresses.end());
            }

            lastAddr = curAddr;
        }

        debugmsg(__PRETTY_FUNCTION__ << ": piecesMerged = " << piecesMerged);
    }

    _visMapValid = true;
    debugleave();
}


void MemoryMapWidget::closeEvent(QCloseEvent* e)
{
    hide();
    e->ignore();
}


void MemoryMapWidget::changeEvent(QEvent *e)
{
    switch (e->type()) {
    case QEvent::Resize:
        _visMapValid = false;
    default:
        break;
    }
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

    QColor unusedColor(80, 255, 80);
    QColor usedColor(255, 80, 80);
    QColor gridColor(255, 255, 255);
    QColor bgColor(255, 255, 255);
    QPainter painter(this);
//    painter.setRenderHint(QPainter::Antialiasing);

    // Draw in "byte"-coordinates, scale bytes to entire widget
    painter.translate(margin, margin);
    painter.scale(drawWidth() / (double)_cols, drawHeight() / (double)_rows);
    // Clear widget
    painter.setPen(Qt::NoPen);
    painter.setBrush(bgColor);
    painter.drawRect(0, 0, _cols, _rows);
    // Draw the whole address space  as "unused"
    painter.setBrush(unusedColor);
    int h = virtAddrSpace() / _cols;
    if (h >= 1)
        painter.drawRect(0, 0, _cols, h);

    painter.drawRect(0, h, virtAddrSpace() - (_cols * h), 1);
    // Draw all used parts
    painter.setBrush(usedColor);
    for (int i = 0; i < _mappings.size(); ++i) {
        int len = _mappings[i].length;
        // Find the row and column of the offset
        int r = _mappings[i].address / _cols;
        int c = _mappings[i].address % _cols;
        // Draw the fist line
        if (c != 0) {
            int currLen = (_cols - c > len) ? len : _cols - c;
            painter.drawRect(c, r, currLen, 1);
            // Shorten length by drawn bytes
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
            // Next drawing starts in next line
            r += h;
        }
        // Draw the last line
        if (len > 0) {
            painter.drawRect(0, r, len, 1);
        }
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
    repaint();
}

