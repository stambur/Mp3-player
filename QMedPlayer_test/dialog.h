#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QtWidgets>
#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QDir>
#include <QDebug>
#include <QTimer>
#include <QSplashScreen>
#include "lirc.h"
#include "getpath.h"
#include <taglib/tag.h>
#include <taglib/fileref.h>

#include <QAudioBuffer>
#include <QAudioProbe>
#include "fftcalc.h"

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

signals:
    void hw_btn_clicked(int);

public slots:
    void handleKey(const QString&);
    void onDurationChanged(qint64);
    void onSongChanged(QMediaContent);
    void onPositionChanged(qint64);
    void onHwBtnClicked(int);
    void onPlayerStateChanged(QMediaPlayer::State);
    void onEverySecond();

    void processBuffer(QAudioBuffer);
    void loadSamples(QVector<double>);

private:
    Ui::Dialog *ui;
    QMediaPlayer *myPlayer;
    int lcd_h;
    QString currentSong;
    int scrollCounter;
    uchar lcdMode;
    int previousIndex;
    QHBoxLayout* myHLayout;
    QString color;
    Lirc* myLirc;

    int barsCount;
    int octaves;
    int sampleRate;
    QVector<double> sample;
    QVector<QProgressBar*> arr;
    QAudioProbe* probe;
    FFTCalc* calculator;

    void updateStyleSheets();
};

#endif // DIALOG_H
