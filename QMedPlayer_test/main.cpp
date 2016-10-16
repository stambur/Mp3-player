#include "dialog.h"
#include <QApplication>

//za tastere sa DVK
#define DEBOUNCE_TIME_1 50
#define DEBOUNCE_TIME_2 50
#define DEBOUNCE_TIME_3 50
#define DEBOUNCE_TIME_4 30
//Stylesheets za boje
#define TABLESS "QTableWidget[colorScheme=blue]::item{selection-background-color:rgb(51, 151, 213);} \
                QTableWidget[colorScheme=green]::item{selection-background-color:rgb(0, 170, 43);} \
                QTableWidget[colorScheme=red]::item{selection-background-color:rgb(255, 52, 48);}"
#define DIALOGSS "Dialog[colorScheme=blue]{background:rgb(51, 151, 213);} \
                Dialog[colorScheme=green]{background:rgb(0, 170, 43);} \
                Dialog[colorScheme=red]{background:rgb(255, 52, 48);}"
#define GROUPBOXSS "QGroupBox[colorScheme=blue] {background-color: rgb(109, 180, 208);} \
                    QGroupBox[colorScheme=green] {background-color: rgb(125, 240, 113);} \
                    QGroupBox[colorScheme=red] {background-color: rgb(238, 114, 102);}"

void btn_int1(void);
void btn_int2(void);
void btn_int3(void);
void btn_int4(void);
void timer_btn1(void);
void timer_btn2(void);
void timer_btn3(void);
void timer_btn4(void);
uchar edge_flag1,edge_flag2,edge_flag3,edge_flag4 = 0;
QTimer *timer1;
QTimer *timer2;
QTimer *timer3;
QTimer *timer4;

Dialog *w;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    //setuj styleSheet za citavu aplikaciju, pa ce on moci da se mijenja preko dynamic properties
    //unutar aplikacije
    a.setStyleSheet(QObject::tr(DIALOGSS TABLESS GROUPBOXSS));
    int retVal;

    //splashscreen - prikazuje se par sekundi dok se ne pokrene glavni GUI
    QSplashScreen *splash = new QSplashScreen;
    splash->setPixmap(QPixmap(":/imgs/pi_notes.jpg"));
    splash->show();
    //a.processEvents();
    splash->showMessage(QObject::tr("Setting up..."), Qt::AlignBottom, Qt::magenta);
    a.processEvents();

    if (wiringPiSetup() < 0) {
        fprintf (stderr, "Unable to open serial device: %s\n", strerror (errno));
        return 1;
    }

    //setuj pinove za tastere kao ulazne
    for(int i=21;i<25;i++) { //tasteri su 21,22,23,24
        pinMode (i,INPUT) ;
        pullUpDnControl(i, PUD_UP);
    }

    timer1 = new QTimer;
    timer2 = new QTimer;
    timer3 = new QTimer;
    timer4 = new QTimer;
    QObject::connect(timer1, &QTimer::timeout, timer_btn1);
    QObject::connect(timer2, &QTimer::timeout, timer_btn2);
    QObject::connect(timer3, &QTimer::timeout, timer_btn3);
    QObject::connect(timer4, &QTimer::timeout, timer_btn4);
    timer1->setSingleShot(true);
    timer2->setSingleShot(true);
    timer3->setSingleShot(true);
    timer4->setSingleShot(true);

    //registruj funkcije koje se izvrsavaju na pritisak tastera
    wiringPiISR(BTN_1,INT_EDGE_BOTH,btn_int1);
    wiringPiISR(BTN_2,INT_EDGE_BOTH,btn_int2);
    wiringPiISR(BTN_3,INT_EDGE_BOTH,btn_int3);
    wiringPiISR(BTN_4,INT_EDGE_BOTH,btn_int4);

    //konstruisi glavni GUI dijalog
    //moralo je ovako dinamicki posto mi treba globalni pokazivac na njega zbog funkcija za tastere
    w = new Dialog;

    w->show();
    splash->finish(w);
    delete splash;

    retVal = a.exec();

    delete timer1;
    delete timer2;
    delete timer3;
    delete timer4;
    delete w;
    return retVal;
}


//funkcije za debouncing tastera i slanje signala sa brojem tastera koji je pritisnut
void btn_int1() {
    if(!edge_flag1) {
        edge_flag1 = 1;
        timer1->start(DEBOUNCE_TIME_1);
    }
}

void btn_int2() {
    if(!edge_flag2) {
        edge_flag2 = 1;
        timer2->start(DEBOUNCE_TIME_2);
    }
}

void btn_int3() {
    if(!edge_flag3) {
        edge_flag3 = 1;
        timer3->start(DEBOUNCE_TIME_3);
    }
}

void btn_int4() {
    if(!edge_flag4) {
        edge_flag4 = 1;
        timer4->start(DEBOUNCE_TIME_4);
    }
}

void timer_btn1() {
    edge_flag1 = 0;
    if(!digitalRead(BTN_1)) {
        emit w->hw_btn_clicked(BTN_1);
    }
}

void timer_btn2() {
    edge_flag2 = 0;
    if(!digitalRead(BTN_2)) {
        emit w->hw_btn_clicked(BTN_2);
    }
}

void timer_btn3() {
    edge_flag3 = 0;
    if(!digitalRead(BTN_3)) {
        emit w->hw_btn_clicked(BTN_3);
    }
}

void timer_btn4() {
    edge_flag4 = 0;
    if(!digitalRead(BTN_4)) {
        emit w->hw_btn_clicked(BTN_4);
    }
}
