#include "dialog.h"
#include "ui_dialog.h"

void myFun(void);

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);
    lcd_h = lcdInit(2, 16, 4, RS, EN, D0, D1, D2, D3, D0, D1, D2, D3);

    if (lcd_h < 0) {
        fprintf (stderr, "lcdInit failed\n") ;
    }

    myPlayer = new QMediaPlayer(this);
    QMediaPlaylist *myPlaylist = new QMediaPlaylist(this);

    Lirc *myLirc = new Lirc(this);
    connect(myLirc,SIGNAL(key_event(QString)),this,SLOT(handleKey(QString)));

    QString directory = QString::fromUtf8(usbPath());
    QDir dir(directory);
    QStringList files = dir.entryList(QStringList() << "*.mp3",QDir::Files);
    QList<QMediaContent> content;
    for(const QString& f:files)
    {
        content.push_back(QUrl::fromLocalFile(dir.path()+"/" + f));
        TagLib::FileRef fil((dir.path()+"/" + f).toLatin1().data());
        ui->listWidget->addItem(f + "\t" + QString::number(fil.audioProperties()->length() / 60) + ':' + QString::number(fil.audioProperties()->length() % 60).rightJustified(2,'0'));
    }

    myPlaylist->addMedia(content);
    myPlayer->setPlaylist(myPlaylist);
    //myPlayer->playlist()->addMedia(content);
    connect(myPlayer,SIGNAL(durationChanged(qint64)),this,SLOT(myDurationSlot(qint64)));
    connect(this, SIGNAL(hw_btn_clicked(int)), this, SLOT(on_hw_btn_clicked(int)));
    //lcdPrintf(lcd_h,"Hallo Welt");
    //wiringPiISR(21,INT_EDGE_RISING,myFun);

}

Dialog::~Dialog()
{
    delete ui;
}

void Dialog::myDurationSlot(qint64 duration) {
    ui->listWidget->setCurrentRow(myPlayer->playlist()->currentIndex() != -1 ? myPlayer->playlist()->currentIndex():0);
}

void Dialog::bla() {
    qDebug() << "Usao u metodu";
}

void Dialog::handleKey(const QString& key) {
    //QProcess* myProcess = new QProcess(this);
    //QString cmd;
    if(key == "KEY_PLAY") {
        if(myPlayer->state() == QMediaPlayer::StoppedState) {
            myPlayer->play();
        }
        else if(myPlayer->state() == QMediaPlayer::PausedState) {
            myPlayer->play();
        }
        else {
            myPlayer->pause();
        }
    }
    else if(key == "KEY_CH") {
        myPlayer->stop();
    }
    else if(key == "KEY_NEXT") {
        myPlayer->playlist()->next();
        myPlayer->play();
    }
    else if(key == "KEY_PREVIOUS") {
        if(myPlayer->playlist()->currentIndex()==0) {
            myPlayer->playlist()->setCurrentIndex(myPlayer->playlist()->mediaCount()-1);
        }
        else {
            myPlayer->playlist()->previous();
        }
        myPlayer->play();
    }
    else if(key == "KEY_CH-") {
        if(myPlayer->position()>5000) {
            myPlayer->setPosition(myPlayer->position() - 5000);
        }
        else {
            myPlayer->setPosition(0);
        }
    }
    else if(key == "KEY_CH+") {
        myPlayer->setPosition(myPlayer->position() + 5000);
    }
    else {
        myPlayer->stop();
        delete ui;
        exit(0);
    }
    //ui->label->setText(QString::number(v.duration[list_iterator-1]));
}

void Dialog::on_hw_btn_clicked(int btn) {
    switch(btn) {
        case BTN_1:
            qDebug() << "Bla 21" << endl;
            break;
        case BTN_2:
            qDebug() << "Bla 22" << endl;
            break;
        case BTN_3:
            qDebug() << "Bla 23" << endl;
            break;
        case BTN_4:
            qDebug() << "Bla 24" << endl;
            break;
    }
}

void myFun(void) {
    printf("Bla\n");
}
