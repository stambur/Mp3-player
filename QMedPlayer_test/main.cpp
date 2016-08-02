#include "dialog.h"
#include <QApplication>

//void on_btn1(void);
//void on_btn2(void);
//void on_btn3(void);
//void on_btn4(void);
void btn_int1(void);
void btn_int2(void);
void btn_int3(void);
void btn_int4(void);

Dialog *w;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    int retVal;

    if (wiringPiSetup() < 0) {
        fprintf (stderr, "Unable to open serial device: %s\n", strerror (errno));
        return 1;
    }

    for(int i=21;i<25;i++) { //tasteri su 21,22,23,24
        pinMode (i,INPUT) ;
        pullUpDnControl(i, PUD_UP);
    }

    wiringPiISR(21,INT_EDGE_RISING,btn_int1);
    wiringPiISR(22,INT_EDGE_RISING,btn_int2);
    wiringPiISR(23,INT_EDGE_RISING,btn_int3);
    wiringPiISR(24,INT_EDGE_RISING,btn_int4);

    //Dialog w;
    //pt = &w;
    w = new Dialog;

    w->show();
    retVal = a.exec();

    delete w;
    return retVal;
}

void btn_int1() {
    //w->bla();
    emit w->hw_btn_clicked(BTN_1);
}

void btn_int2() {
    //w->bla();
    emit w->hw_btn_clicked(BTN_2);
}

void btn_int3() {
    //w->bla();
    emit w->hw_btn_clicked(BTN_3);
}

void btn_int4() {
    //w->bla();
    emit w->hw_btn_clicked(BTN_4);
}
