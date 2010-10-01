#ifndef MEMORYMAPWINDOW_H
#define MEMORYMAPWINDOW_H

#include <QtGui/QMainWindow>
#include "ui_memorymapwindow.h"

class MemoryMapWidget;
class MemoryMap;
class QLabel;
class QProgressBar;

class MemoryMapWindow : public QMainWindow
{
    Q_OBJECT

public:
    MemoryMapWindow(QWidget *parent = 0);
    ~MemoryMapWindow();

//    void setMap(const MemoryMap* map);
    MemoryMapWidget* mapWidget();

private slots:
    void virtualAddressChanged(quint64 address);
    void memMapBuildingStarted();
    void memMapBuildingStopped();

private:
    Ui::MemoryMapWindowClass ui;
    MemoryMapWidget* _memMapWidget;
    QLabel* _sbCursorPosition;
    QLabel* _sbBuildingMsg;
    QProgressBar* _sbBuildingProgBar;
};

extern MemoryMapWindow* memMapWindow;

#endif // MEMORYMAPWINDOW_H
