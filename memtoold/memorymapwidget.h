/*
 * memorymapwidget.h
 *
 *  Created on: 01.09.2010
 *      Author: chrschn
 */

#ifndef MEMORYMAPWIDGET_H_
#define MEMORYMAPWIDGET_H_

#include <QWidget>

class QEvent;
class QPaintEvent;
class QMouseEvent;
class QCloseEvent;
class MemoryMap;

enum VisMapPieceTypes {
    ptUnused,
    ptUsed
};

struct VisMapPiece
{
    VisMapPiece(quint64 start = 0, quint64 length = 0, int type = ptUsed,
            int intensity = 0);
    quint64 address;
    quint64 length;
    int type;
    int intensity;
};

typedef QList<VisMapPiece> MapPieceList;


class MemoryMapWidget: public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY(MemoryMapWidget)

public:
    explicit MemoryMapWidget(const MemoryMap* map = 0, QWidget *parent = 0);
    virtual ~MemoryMapWidget();

    quint64 virtAddrSpace() const;

    const MemoryMap* map() const;
    void setMap(const MemoryMap* map);

protected:
    void closeEvent(QCloseEvent* e);
    void changeEvent(QEvent* e);
    void paintEvent(QPaintEvent* e);
    void mouseMoveEvent(QMouseEvent* e);

private:
    void buildVisMemMap();
    int drawWidth() const;
    int drawHeight() const;

    const MemoryMap* _map;
    MapPieceList _mappings;
    bool _visMapValid;
    quint64 _address;
    double _bytesPerPixelX;
    double _bytesPerPixelY;
    qint64 _cols;
    qint64 _rows;
    int _maxIntensity;

signals:
    void addressChanged(quint64 address);
};

extern MemoryMapWidget* mapWidget;

#endif /* MEMORYMAPWIDGET_H_ */
