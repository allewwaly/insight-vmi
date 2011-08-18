/*
 * memorymapwidget.h
 *
 *  Created on: 01.09.2010
 *      Author: chrschn
 */

#ifndef MEMORYMAPWIDGET_H_
#define MEMORYMAPWIDGET_H_

#include <QWidget>
#include <QMutex>

class QEvent;
class QPaintEvent;
class QMouseEvent;
class QCloseEvent;

#include "memorymaprangetree.h"
#include "memorydifftree.h"


class MemoryMapWidget: public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY(MemoryMapWidget)

public:
    explicit MemoryMapWidget(const MemoryMapRangeTree* map = 0, QWidget *parent = 0);
    virtual ~MemoryMapWidget();

    quint64 totalAddrSpaceEnd() const;
    quint64 visibleAddrSpaceStart() const;
    quint64 visibleAddrSpaceEnd() const;
    quint64 visibleAddrSpaceLength() const;

    const MemoryMapRangeTree* map() const;
    void setMap(const MemoryMapRangeTree* map);
    const MemoryDiffTree* diff() const;
    void setDiff(const MemoryDiffTree* diff);
    bool antiAliasing() const;
    bool isPainting() const;
    bool showOnlyKernelSpace() const;

public slots:
    void setAntiAliasing(bool value);
    void setShowOnlyKernelSpace(bool value);

protected:
    void closeEvent(QCloseEvent* e);
    void paintEvent(QPaintEvent* e);
    void mouseMoveEvent(QMouseEvent* e);
    void resizeEvent(QResizeEvent* event);
    bool event(QEvent *event);

private:
    int drawWidth() const;
    int drawHeight() const;

    const MemoryMapRangeTree* _map;
    const MemoryDiffTree* _diff;
    bool _visMapValid;
    quint64 _address;
    quint64 _bytesPerPixel;
    qint64 _cols;
    qint64 _rows;
    int _maxIntensity;
    bool _antialiasing;
    bool _isPainting;
    bool _showOnlyKernelSpace;
    quint64 _shownAddrSpaceOffset;
    QMutex _buildMutex;

signals:
    void addressChanged(quint64 address);
};

#endif /* MEMORYMAPWIDGET_H_ */
