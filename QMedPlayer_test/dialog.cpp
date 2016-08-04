#include "dialog.h"
#include "ui_dialog.h"

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);
    lcd_h = lcdInit(2, 16, 4, RS, EN, D0, D1, D2, D3, D0, D1, D2, D3);

    if (lcd_h < 0) {
        fprintf (stderr, "lcdInit failed\n") ;
    }

    scrollCounter = 0;

    myPlayer = new QMediaPlayer(this);
    QMediaPlaylist *myPlaylist = new QMediaPlaylist(this);

    Lirc *myLirc = new Lirc(this);
    connect(myLirc,SIGNAL(key_event(QString)),this,SLOT(handleKey(QString)));

    QTimer *myTimer = new QTimer(this);
    connect(myTimer, SIGNAL(timeout()), this, SLOT(lcdScroll()));
    myTimer->start(1000);

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
    connect(myPlayer,SIGNAL(durationChanged(qint64)),this,SLOT(onDurationChanged(qint64)));
    connect(myPlayer,SIGNAL(currentMediaChanged(QMediaContent)),this,SLOT(onSongChanged(QMediaContent)));
    connect(myPlayer,SIGNAL(positionChanged(qint64)),this,SLOT(onPositionChanged(qint64)));
    connect(this,SIGNAL(hw_btn_clicked(int)),this,SLOT(onHwBtnClicked(int)));
    lcdClear(lcd_h);
    lcdPosition(lcd_h,0,0);
    lcdPutchar(lcd_h,0xFF);
    lcdPutchar(lcd_h,0xFF);
    lcdPrintf(lcd_h,"Play stopped");
    for(int i=0; i<18; i++) {
        lcdPutchar(lcd_h,0xFF);
    }

    //ui->listWidget->setCurrentRow(0);

}

Dialog::~Dialog()
{
    lcdClear(lcd_h);
    delete ui;
}

void Dialog::onDurationChanged(qint64 duration) {
    //ui->listWidget->setCurrentRow(myPlayer->playlist()->currentIndex() != -1 ? myPlayer->playlist()->currentIndex():0);
    if(myPlayer->state() != QMediaPlayer::StoppedState) {
        lcdPosition(lcd_h,5,1);
        lcdPrintf(lcd_h,(QString::number(duration/1000/60) + ':' + QString::number(duration/1000%60).rightJustified(2,'0')).toLatin1().data());
    }
}

void Dialog::onPositionChanged(qint64 pos) {
    if(myPlayer->state() != QMediaPlayer::StoppedState) {
    lcdPosition(lcd_h,0,1);
    lcdPrintf(lcd_h,(QString::number(pos/1000/60) + ':' + QString::number(pos/1000%60).rightJustified(2,'0')).toLatin1().data());
    }
}

void Dialog::onSongChanged(QMediaContent song) {
    ui->listWidget->setCurrentRow(myPlayer->playlist()->currentIndex() != -1 ? myPlayer->playlist()->currentIndex():0);
    //lcdClear(lcd_h);
    //lcdPosition(lcd_h,0,0);
    currentSong = song.canonicalUrl().fileName();
    //lcdPad(song.canonicalUrl().fileName().toLatin1().data());
    //lcdPrintf(lcd_h,song.canonicalUrl().fileName().toLatin1().data());
}

void Dialog::handleKey(const QString& key) {
    if(key == "KEY_PLAY") {
        if(myPlayer->state() == QMediaPlayer::StoppedState) {
            myPlayer->play();
            lcdClear(lcd_h);
            currentSong = myPlayer->currentMedia().canonicalUrl().fileName();

            lcdPosition(lcd_h,5,1);
            lcdPrintf(lcd_h,(QString::number(myPlayer->duration()/1000/60) + ':' + QString::number(myPlayer->duration()/1000%60).rightJustified(2,'0')).toLatin1().data());

            //lcdPrintf(lcd_h,myPlayer->currentMedia().canonicalUrl().fileName().toLatin1().data());
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
        lcdClear(lcd_h);
        lcdPosition(lcd_h,0,0);
        lcdPutchar(lcd_h,0xFF);
        lcdPutchar(lcd_h,0xFF);
        lcdPrintf(lcd_h,"Play stopped");
        for(int i=0; i<18; i++) {
            lcdPutchar(lcd_h,0xFF);
        }
    }
    else if(key == "KEY_NEXT") {
        scrollCounter = 0;
        lcdClear(lcd_h);
        myPlayer->playlist()->next();
        myPlayer->play();
    }
    else if(key == "KEY_PREVIOUS") {
        scrollCounter = 0;
        lcdClear(lcd_h);
        if(myPlayer->playlist()->currentIndex()==0) {
            myPlayer->playlist()->setCurrentIndex(myPlayer->playlist()->mediaCount()-1);
        }
        else {
            myPlayer->playlist()->previous();
        }
        myPlayer->play();
    }
    else if(key == "KEY_CH-") {
        qDebug() << "Position = " + QString::number(myPlayer->position());
        if(myPlayer->position()>5000) {
            myPlayer->setPosition(myPlayer->position() - 5000);
        }
        else {
            myPlayer->setPosition(0);
        }
        qDebug() << "New position = " + QString::number(myPlayer->position());
    }
    else if(key == "KEY_CH+") {
        qDebug() << "Position = " + QString::number(myPlayer->position());
        myPlayer->setPosition(myPlayer->position() + 10000);
        qDebug() << "New position = " + QString::number(myPlayer->position());
    }
    else {
        myPlayer->stop();
        lcdClear(lcd_h);
        delete ui;
        exit(0);
    }
}

void Dialog::onHwBtnClicked(int btn) {
    switch(btn) {
        case BTN_1:
            qDebug() << "Bla 21";
            break;
        case BTN_2:
            qDebug() << "Bla 22";
            break;
        case BTN_3:
            qDebug() << "Bla 23";
            break;
        case BTN_4:
            qDebug() << "Bla 24";
            break;
    }
}

void Dialog::lcdScroll() {
    if(myPlayer->state() != QMediaPlayer::StoppedState) {
        lcdPosition(lcd_h,0,0);     
        lcdPrintf(lcd_h,currentSong.mid(scrollCounter,16).toLatin1().data());
        if(scrollCounter+16 < currentSong.length()) {
            scrollCounter++;
        }
        else {
            scrollCounter = 0;
        }
    }
}
