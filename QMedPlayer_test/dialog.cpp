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

    sample.resize(SPECSIZE);
    calculator = new FFTCalc();
    probe = new QAudioProbe();
    qRegisterMetaType< QVector<double> >("QVector<double>");

    connect(probe, SIGNAL(audioBufferProbed(QAudioBuffer)),
              this, SLOT(processBuffer(QAudioBuffer)));
    connect(this,  SIGNAL(spectrumChanged(QVector<double>&)),
              this,SLOT(loadSamples(QVector<double>&)));
    connect(this,  SIGNAL(levels(double,double)),
            this,SLOT(loadLevels(double,double)));
    connect(calculator, SIGNAL(calculatedSpectrum(QVector<double>)),
            this, SLOT(spectrumAvailable(QVector<double>)));

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
    QStringList files = dir.entryList(QStringList() << tr("*.mp3"),QDir::Files);
    QList<QMediaContent> content;
    for(const QString& f:files)
    {
        content.push_back(QUrl::fromLocalFile(dir.path()+ '/' + f));
        TagLib::FileRef fil((dir.path()+ '/' + f).toLatin1().data());
        ui->listWidget->addItem(f); //+ "\t" + QString::number(fil.audioProperties()->length() / 60) + ':' + QString::number(fil.audioProperties()->length() % 60).rightJustified(2,'0'));
        ui->listWidget_2->addItem(QString::number(fil.audioProperties()->length() / 60) + ':' +
                                  QString::number(fil.audioProperties()->length() % 60).rightJustified(2,'0'));
        ui->tableWidget->insertRow(count);
        ui->tableWidget->setItem(count,0,new QTableWidgetItem(f));
        ui->tableWidget->setItem(count++,1,new QTableWidgetItem(QString::number(fil.audioProperties()->length() / 60) + ':' +
                                                                QString::number(fil.audioProperties()->length() % 60).rightJustified(2,'0')));
        //count++;

    }
    ui->listWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->listWidget->setTextElideMode(Qt::ElideRight);
    ui->tableWidget->horizontalHeader()->setVisible(false);
    ui->tableWidget->setShowGrid(false);
    //ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    //ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    //ui->tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(ui->tableWidget, SIGNAL(itemSelectionChanged()), this, SLOT(mySlot()));
    //ui->tableWidget->setRowHeight(0,5);
    ui->tableWidget->verticalHeader()->resizeSections(QHeaderView::ResizeToContents);
    ui->tableWidget->setColumnWidth(1,50);
    //ui->tableWidget->horizontalHeader()->
    ui->tableWidget->adjustSize();

    myPlaylist->addMedia(content);
    myPlayer->setPlaylist(myPlaylist);
    //connect(myPlayer,SIGNAL(durationChanged(qint64)),this,SLOT(onDurationChanged(qint64)));
    connect(myPlayer,SIGNAL(currentMediaChanged(QMediaContent)),this,SLOT(onSongChanged(QMediaContent)));
    //connect(myPlayer,SIGNAL(positionChanged(qint64)),this,SLOT(onPositionChanged(qint64)));
    connect(this,SIGNAL(hw_btn_clicked(int)),this,SLOT(onHwBtnClicked(int)));
    connect(myPlayer,SIGNAL(metaDataChanged()),this,SLOT(onMetaDataChanged()));
    lcdClear(lcd_h);
    lcdPosition(lcd_h,0,0);
    lcdPutchar(lcd_h,0xFF);
    lcdPutchar(lcd_h,0xFF);
    lcdPrintf(lcd_h,"Play stopped");
    for(int i=0; i<18; i++) {
        lcdPutchar(lcd_h,0xFF);
    }

    //ui->listWidget->setCurrentRow(0);
    probe->setSource(myPlayer);

}

Dialog::~Dialog()
{
    lcdClear(lcd_h);
    delete ui;
}

void Dialog::mySlot() {
    //qDebug() << "Usao u slot";
}

void Dialog::onMetaDataChanged() {
    if(myPlayer->isMetaDataAvailable()) {
        ui->label->setText(myPlayer->metaData("AudioBitRate").toString());
    }
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
}

void Dialog::handleKey(const QString& key) {
    if(key == tr("KEY_PLAY")) {
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
    else if(key == tr("KEY_CH")) {
        myPlayer->stop();
    }
    else if(key == tr("KEY_NEXT")) {
        scrollCounter = 0;
        lcdClear(lcd_h);
        myPlayer->playlist()->next();
        myPlayer->play();
    }
    else if(key == tr("KEY_PREVIOUS")) {
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
    else if(key == tr("KEY_CH-")) {
        //qDebug() << "Position = " + QString::number(myPlayer->position());
        if(myPlayer->position()>5000) {
            myPlayer->setPosition(myPlayer->position() - 5000);
        }
        else {
            myPlayer->setPosition(0);
        }
        //qDebug() << "New position = " + QString::number(myPlayer->position());
    }
    else if(key == tr("KEY_CH+")) {
        //qDebug() << "Position = " + QString::number(myPlayer->position());
        myPlayer->setPosition(myPlayer->position() + 5000);
        //qDebug() << "New position = " + QString::number(myPlayer->position());
    }
    else if(key == tr("KEY_UP")) {
        myPlayer->setVolume(myPlayer->volume() + 5);
    }
    else if(key == tr("KEY_DOWN")) {
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


void Dialog::processBuffer(QAudioBuffer buffer){
  qreal peakValue;
  int duration;

  if(buffer.frameCount() < 512)
    return;

  // return left and right audio mean levels
  levelLeft = levelRight = 0;
  // It only knows how to process stereo audio frames
  // mono frames = :P
  if(buffer.format().channelCount() != 2)
    return;

  sample.resize(buffer.frameCount());
  // audio is signed int
  if(buffer.format().sampleType() == QAudioFormat::SignedInt){
    QAudioBuffer::S16S *data = buffer.data<QAudioBuffer::S16S>();
    // peak value changes according to sample size.
    if (buffer.format().sampleSize() == 32)
      peakValue=INT_MAX;
    else if (buffer.format().sampleSize() == 16)
      peakValue=SHRT_MAX;
    else
      peakValue=CHAR_MAX;

    // scale everything to [0,1]
    for(int i=0; i<buffer.frameCount(); i++){
      // for visualization purposes, we only need one of the
      // left/right channels
      sample[i] = data[i].left/peakValue;
      levelLeft+= abs(data[i].left)/peakValue;
      levelRight+= abs(data[i].right)/peakValue;
    }
  }

  // audio is unsigned int
  else if(buffer.format().sampleType() == QAudioFormat::UnSignedInt){
    QAudioBuffer::S16U *data = buffer.data<QAudioBuffer::S16U>();
    if (buffer.format().sampleSize() == 32)
      peakValue=UINT_MAX;
    else if (buffer.format().sampleSize() == 16)
      peakValue=USHRT_MAX;
    else
      peakValue=UCHAR_MAX;
    for(int i=0; i<buffer.frameCount(); i++){
      sample[i] = data[i].left/peakValue;
      levelLeft+= abs(data[i].left)/peakValue;
      levelRight+= abs(data[i].right)/peakValue;
    }
  }

  // audio is float type
  else if(buffer.format().sampleType() == QAudioFormat::Float){
    QAudioBuffer::S32F *data = buffer.data<QAudioBuffer::S32F>();
    peakValue = 1.00003;
    for(int i=0; i<buffer.frameCount(); i++){
      sample[i] = data[i].left/peakValue;
      // test if sample[i] is infinity (it works)
      // some tests produced infinity values :p
      if(sample[i] != sample[i]){
        sample[i] = 0;
      }
      else{
        levelLeft+= abs(data[i].left)/peakValue;
        levelRight+= abs(data[i].right)/peakValue;
      }
    }
  }
  // if the probe is listening to the audio
  // do fft calculations
  // when it is done, calculator will tell us
  if(probe->isActive()){
    duration = buffer.format().durationForBytes(buffer.frameCount())/1000;
    calculator->calc(sample, duration);
  }
  // tells anyone interested about left and right mean levels
  emit levels(levelLeft/buffer.frameCount(),levelRight/buffer.frameCount());
}

// what to do when fft spectrum is available
void Dialog::spectrumAvailable(QVector<double> spectrum){
  // just tell the spectrum
  // the visualization widget will catch the signal...
  emit spectrumChanged(spectrum);
}

void Dialog::loadSamples(QVector<double>& samples) {
    qDebug() << "loadSamples";
}

void Dialog::loadLevels(double left, double right) {
    qDebug() << "loadLevels";
}
