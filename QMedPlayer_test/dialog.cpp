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

    uchar vol[8] = {0b00001, 0b00011, 0b11011, 0b11011, 0b11011, 0b00011, 0b00001}; //volume icon
    uchar pse[8] = {0b11011, 0b11011, 0b11011, 0b11011, 0b11011, 0b11011, 0b11011}; //pause icon
    lcdCharDef(lcd_h, 0, vol);
    lcdCharDef(lcd_h,1,pse);

    scrollCounter = 0;
    lcdMode = 0;

    myPlayer = new QMediaPlayer(this);
    QMediaPlaylist *myPlaylist = new QMediaPlaylist(this);

    Lirc *myLirc = new Lirc(this);
    connect(myLirc,SIGNAL(key_event(QString)),this,SLOT(handleKey(QString)));

    QTimer *myTimer = new QTimer(this);
    connect(myTimer, SIGNAL(timeout()), this, SLOT(lcdScroll()));
    myTimer->start(1000);

    ui->tableWidget->setColumnCount(2);
    int count = 0;

    QString directory = QString::fromUtf8(usbPath());
    QDir dir(directory);
    QStringList files = dir.entryList(QStringList() << "*.mp3",QDir::Files);
    QList<QMediaContent> content;
    for(const QString& f:files)
    {
        content.push_back(QUrl::fromLocalFile(dir.path()+"/" + f));
        TagLib::FileRef fil((dir.path()+"/" + f).toLatin1().data());
        ui->listWidget->addItem(f); //+ "\t" + QString::number(fil.audioProperties()->length() / 60) + ':' + QString::number(fil.audioProperties()->length() % 60).rightJustified(2,'0'));
        ui->listWidget_2->addItem(QString::number(fil.audioProperties()->length() / 60) + ':' +
                                  QString::number(fil.audioProperties()->length() % 60).rightJustified(2,'0'));
        ui->tableWidget->insertRow(count);
        ui->tableWidget->setItem(count,0,new QTableWidgetItem(f));
        ui->tableWidget->setItem(count++,1,new QTableWidgetItem(QString::number(fil.audioProperties()->length() / 60) + ':' +
                                                                QString::number(fil.audioProperties()->length() % 60).rightJustified(2,'0')));
        //count++;

    }
    ui->tableWidget->adjustSize();
    ui->tableWidget->horizontalHeader()->setVisible(false);
    ui->tableWidget->setShowGrid(false);
    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableWidget->setWordWrap(false);
    myPlaylist->addMedia(content);
    myPlayer->setPlaylist(myPlaylist);
    //connect(myPlayer,SIGNAL(durationChanged(qint64)),this,SLOT(onDurationChanged(qint64)));
    connect(myPlayer,SIGNAL(currentMediaChanged(QMediaContent)),this,SLOT(onSongChanged(QMediaContent)));
    //connect(myPlayer,SIGNAL(positionChanged(qint64)),this,SLOT(onPositionChanged(qint64)));
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

}

void Dialog::onPositionChanged(qint64 pos) {
    //
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
        switch(myPlayer->state()) {
            case QMediaPlayer::StoppedState:
                myPlayer->play();
                lcdClear(lcd_h);
                currentSong = myPlayer->currentMedia().canonicalUrl().fileName();
                break;
            case QMediaPlayer::PausedState:
                myPlayer->play();
                break;
            case QMediaPlayer::PlayingState:
                myPlayer->pause();
                break;
        }
    }
    else if(key == "KEY_CH") {
        myPlayer->stop();
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
    else if(key == "KEY_UP") {
        myPlayer->setVolume(myPlayer->volume() + 5);
    }
    else if(key == "KEY_DOWN") {
        myPlayer->setVolume(myPlayer->volume() - 5);
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
        case BTN_1: //mode
            lcdMode ^= 1;
            lcdClear(lcd_h);
            break;
        case BTN_2: //play/pause(mode=0) / stop(mode=1)
            //qDebug() << "Bla 22";
            if(lcdMode) {
                myPlayer->stop();
            }
            else {
                switch(myPlayer->state()) {
                    case QMediaPlayer::StoppedState:
                        myPlayer->play();
                        lcdClear(lcd_h);
                        currentSong = myPlayer->currentMedia().canonicalUrl().fileName();
                        break;
                    case QMediaPlayer::PausedState:
                        myPlayer->play();
                        break;
                    case QMediaPlayer::PlayingState:
                        myPlayer->pause();
                        break;
                }
            }
            break;
        case BTN_3: //prev(mode=0) / vol_down(mode=1)
            //qDebug() << "Bla 23";
            if(lcdMode) {
                myPlayer->setVolume(myPlayer->volume() - 5);
            }
            else {
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
            break;
        case BTN_4: //next(mode=0) / vol_up(mode=1)
            //qDebug() << "Bla 24";
            if(lcdMode) {
                myPlayer->setVolume(myPlayer->volume() + 5);
            }
            else {
                scrollCounter = 0;
                lcdClear(lcd_h);
                myPlayer->playlist()->next();
                myPlayer->play();
            }
            break;
    }
}

void Dialog::lcdScroll() {
    if(myPlayer->state() != QMediaPlayer::StoppedState) {
        //lcdClear(lcd_h);
        lcdPosition(lcd_h,0,0);     
        lcdPrintf(lcd_h,currentSong.mid(scrollCounter,16).toLatin1().data());
        if(scrollCounter+16 < currentSong.length()) {
            scrollCounter++;
        }
        else {
            scrollCounter = 0;
        }

        lcdPosition(lcd_h,0,1);
        if(!lcdMode) {
            lcdPrintf(lcd_h,(QString::number(myPlayer->position()/1000/60) + ':' +
                             QString::number(myPlayer->position()/1000%60).rightJustified(2,'0') + '/' +
                             QString::number(myPlayer->duration()/1000/60) + ':' +
                             QString::number(myPlayer->duration()/1000%60).rightJustified(2,'0')).toLatin1().data());
        }
        else {
            lcdPrintf(lcd_h,"Set volume:");
        }
        lcdPosition(lcd_h,12,1);
        if(myPlayer->state() != QMediaPlayer::PausedState || lcdMode) {
            lcdPutchar(lcd_h,0);
            lcdPosition(lcd_h,13,1);
            lcdPrintf(lcd_h,QString::number(myPlayer->volume()).rightJustified(3).toLatin1().data());
        }
        else {
            lcdPrintf(lcd_h, "P ");
            lcdPutchar(lcd_h,1);
            lcdPutchar(lcd_h,' ');
        }
    }
    else {
        lcdClear(lcd_h);
        lcdPosition(lcd_h,0,0);
        lcdPutchar(lcd_h,0xFF);
        lcdPutchar(lcd_h,0xFF);
        lcdPrintf(lcd_h,"Play stopped");
        for(int i=0; i<18; i++) {
            lcdPutchar(lcd_h,0xFF);
        }
    }
}
