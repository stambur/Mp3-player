#include "dialog.h"
#include <QApplication>

#define DEBOUNCE_TIME 70

//Stylesheets
#define TABLESS "QTableWidget[colorScheme=blue]{background:skyblue} \
                QTableWidget[colorScheme=green]{background:green} \
                QTableWidget[colorScheme=red]{background:red}"
#define DIALOGSS "Dialog[colorScheme=blue]{background:skyblue} \
                Dialog[colorScheme=green]{background:green} \
                Dialog[colorScheme=red]{background:red}"
#define GROUPBOXSS "QGroupBox[colorScheme=blue] {background-color: rgb(230, 255, 255);} \
                    QGroupBox[colorScheme=green] {background-color: rgb(152, 251, 152);} \
                    QGroupBox[colorScheme=red] {background-color: rgb(235, 60, 60);}"
void btn_int1(void);
void btn_int2(void);
void btn_int3(void);
void btn_int4(void);
uchar toggle_var_btn1,toggle_var_btn2,toggle_var_btn3,toggle_var_btn4 = 0;
int time1_btn1,time1_btn2,time1_btn3,time1_btn4 = 0;
int time2_btn1,time2_btn2,time2_btn3,time2_btn4 = DEBOUNCE_TIME;

Dialog *w;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setStyleSheet(QObject::tr(DIALOGSS TABLESS GROUPBOXSS));
    int retVal;

    QSplashScreen *splash = new QSplashScreen;
    splash->setPixmap(QPixmap(":/imgs/pi_notes.jpg"));
    splash->show();
    //splash->showMessage(QObject::tr("Setting up wiringPi..."), Qt::AlignBottom, Qt::magenta);

    if (wiringPiSetup() < 0) {
        fprintf (stderr, "Unable to open serial device: %s\n", strerror (errno));
        return 1;
    }

    for(int i=21;i<25;i++) { //tasteri su 21,22,23,24
        pinMode (i,INPUT) ;
        pullUpDnControl(i, PUD_UP);
    }

    wiringPiISR(BTN_1,INT_EDGE_RISING,btn_int1);
    wiringPiISR(BTN_2,INT_EDGE_RISING,btn_int2);
    wiringPiISR(BTN_3,INT_EDGE_RISING,btn_int3);
    wiringPiISR(BTN_4,INT_EDGE_RISING,btn_int4);

    w = new Dialog;

    w->show();
    splash->finish(w);
    delete splash;

    retVal = a.exec();

    delete w;
    return retVal;
}

void btn_int1() {
    if(!toggle_var_btn1) {
        time1_btn1 = millis();
        toggle_var_btn1 = 1;
    }
    else {
        time2_btn1 = millis();
        toggle_var_btn1 = 0;
    }

    if((time2_btn1-time1_btn1 > DEBOUNCE_TIME) || (time1_btn1-time2_btn1 > DEBOUNCE_TIME)) {
        emit w->hw_btn_clicked(BTN_1);
    }
}

void btn_int2() {
    if(!toggle_var_btn2) {
        time1_btn2 = millis();
        toggle_var_btn2 = 1;
    }
    else {
        time2_btn2 = millis();
        toggle_var_btn2 = 0;
    }

    if((time2_btn2-time1_btn2 > DEBOUNCE_TIME) || (time1_btn2-time2_btn2 > DEBOUNCE_TIME)) {
        emit w->hw_btn_clicked(BTN_2);
    }
}

void btn_int3() {
    if(!toggle_var_btn3) {
        time1_btn3 = millis();
        toggle_var_btn3 = 1;
    }
    else {
        time2_btn3 = millis();
        toggle_var_btn3 = 0;
    }

    if((time2_btn3-time1_btn3 > DEBOUNCE_TIME) || (time1_btn3-time2_btn3 > DEBOUNCE_TIME)) {
        emit w->hw_btn_clicked(BTN_3);
    }
}

void btn_int4() {
    if(!toggle_var_btn4) {
        time1_btn4 = millis();
        toggle_var_btn4 = 1;
    }
    else {
        time2_btn4 = millis();
        toggle_var_btn4 = 0;
    }

    if((time2_btn4-time1_btn4 > DEBOUNCE_TIME) || (time1_btn4-time2_btn4 > DEBOUNCE_TIME)) {
        emit w->hw_btn_clicked(BTN_4);
    }
}
