#include "dialog.h"
#include <QApplication>

//za tastere sa DVK
#define DEBOUNCE_TIME 100
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
uchar toggle_var_btn1,toggle_var_btn2,toggle_var_btn3,toggle_var_btn4 = 0;
int time1_btn1,time1_btn2,time1_btn3,time1_btn4 = 0;
int time2_btn1,time2_btn2,time2_btn3,time2_btn4 = DEBOUNCE_TIME;

Dialog *w;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QMessageBox meinBox(QMessageBox::Critical,"Greška","Ubacite USB Flash",QMessageBox::NoButton);
//    if(usbPath()==NULL) {
//        meinBox.show();
//        while(usbPath() == NULL) {
//            a.processEvents();
//        }
//    }
//    meinBox.accept();
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

    //registruj funkcije koje se izvrsavaju na pritisak tastera
    wiringPiISR(BTN_1,INT_EDGE_RISING,btn_int1);
    wiringPiISR(BTN_2,INT_EDGE_RISING,btn_int2);
    wiringPiISR(BTN_3,INT_EDGE_RISING,btn_int3);
    wiringPiISR(BTN_4,INT_EDGE_RISING,btn_int4);

    if(usbPath()==NULL) {
        meinBox.show();
        while(usbPath() == NULL) {
            a.processEvents();
        }
    }
    meinBox.accept();

    //konstruisi glavni GUI dijalog
    //moralo je ovako dinamicki posto mi treba globalni pokazivac na njega zbog funkcija za tastere
    w = new Dialog;

    w->show();
    splash->finish(w);
    delete splash;

    retVal = a.exec();

    delete w;
    return retVal;
}


//funkcije za debouncing tastera i slanje signala sa brojem tastera koji je pritisnut
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
