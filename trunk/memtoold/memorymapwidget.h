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
    ptPartlyUsed,
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

    quint64 totalAddrSpace() const;
    quint64 visibleAddrSpaceStart() const;
    quint64 visibleAddrSpaceEnd() const;
    quint64 visibleAddrSpaceLength() const;

    const MemoryMap* map() const;
    void setMap(const MemoryMap* map);
    bool antiAliasing() const;
    bool isPainting() const;
    bool isBuilding() const;
    bool showOnlyKernelSpace() const;

public slots:
    void setAntiAliasing(bool value);
    void forceMapRecreaction();
    void setShowOnlyKernelSpace(bool value);

protected:
    void closeEvent(QCloseEvent* e);
    void paintEvent(QPaintEvent* e);
    void mouseMoveEvent(QMouseEvent* e);
    void resizeEvent(QResizeEvent* event);

private slots:
    void buildVisMemMap();

private:
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
    bool _antialiasing;
    bool _isPainting;
    bool _isBuilding;
    bool _showOnlyKernelSpace;
    quint64 _shownAddrSpaceOffset;

signals:
    void addressChanged(quint64 address);
    void buildingStarted();
    void buildingStopped();
    void buildingProgress(int);
};

#endif /* MEMORYMAPWIDGET_H_ */
