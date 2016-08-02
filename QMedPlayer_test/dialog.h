#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QDir>
#include <QDebug>
#include "lirc.h"
#include "getpath.h"
#include <taglib/tag.h>
#include <taglib/fileref.h>

#include <stdio.h>
#include <errno.h>
#include <wiringPi.h>
#include <lcd.h>

#define RS  3
#define EN  14
#define D0  4
#define D1  12
#define D2  13
#define D3  6

#define BTN_1   21
#define BTN_2   22
#define BTN_3   23
#define BTN_4   24

namespace Ui {
class Dialog;
}

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = 0);
    ~Dialog();
    QMediaPlayer *myPlayer;
    int lcd_h;
    void bla();

signals:
    void hw_btn_clicked(int);

public slots:
    void handleKey(const QString&);
    void myDurationSlot(qint64);
    void on_hw_btn_clicked(int);

private:
    Ui::Dialog *ui;
};

#endif // DIALOG_H
