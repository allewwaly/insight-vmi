#ifndef MEMORYMAPWINDOW_H
#define MEMORYMAPWINDOW_H

#include <QtGui/QMainWindow>
#include "ui_memorymapwindow.h"

class MemoryMapWidget;
class MemoryMap;

class MemoryMapWindow : public QMainWindow
{
    Q_OBJECT

public:
    MemoryMapWindow(QWidget *parent = 0);
    ~MemoryMapWindow();

    void setMap(const MemoryMap* map);

private slots:
    void virtualAddressChanged(quint64 address);

private:
    Ui::MemoryMapWindowClass ui;
    MemoryMapWidget* _memMapWidget;
};

extern MemoryMapWindow* memMapWindow;

#endif // MEMORYMAPWINDOW_H
