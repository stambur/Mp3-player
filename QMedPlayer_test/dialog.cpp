#include "dialog.h"
#include "ui_dialog.h"

#define NUM_BANDS 8

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
//    connect(this,  SIGNAL(spectrumChanged(QVector<double>&)),
//            this,SLOT(loadSamples(QVector<double>&)));
    connect(this,  SIGNAL(levels(double,double)),
            this,SLOT(loadLevels(double,double)));
//    connect(calculator, SIGNAL(calculatedSpectrum(QVector<double>)),
//            this, SLOT(spectrumAvailable(QVector<double>)));
    connect(calculator, SIGNAL(calculatedSpectrum(QVector<double>)),
            this, SLOT(loadSamples(QVector<double>)));

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
    //bla->addWidget(ui->horizontalLayout);

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
    ui->progressBar->setValue(ui->progressBar->minimum());
    ui->progressBar_2->setValue(ui->progressBar->minimum());
    ui->progressBar_3->setValue(ui->progressBar->minimum());
    ui->progressBar_4->setValue(ui->progressBar->minimum());
    ui->progressBar_5->setValue(ui->progressBar->minimum());
    ui->progressBar_6->setValue(ui->progressBar->minimum());
    ui->progressBar_7->setValue(ui->progressBar->minimum());
    ui->progressBar_8->setValue(ui->progressBar->minimum());
    ui->progressBar_9->setValue(ui->progressBar->minimum());
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
      sample[i] = (data[i].left + data[i].right)/2/peakValue;
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
      sample[i] = (data[i].left + data[i].right)/2/peakValue;
      levelLeft+= abs(data[i].left)/peakValue;
      levelRight+= abs(data[i].right)/peakValue;
    }
  }

  // audio is float type
  else if(buffer.format().sampleType() == QAudioFormat::Float){
    QAudioBuffer::S32F *data = buffer.data<QAudioBuffer::S32F>();
    //peakValue = 1.00003;
    peakValue = 1.0;
    for(int i=0; i<buffer.frameCount(); i++){
      sample[i] = (data[i].left + data[i].right)/2/peakValue;
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
  // emit spectrumChanged(spectrum);
}

void Dialog::loadSamples(QVector<double> samples) {
    //QVector<double> samples = spectrum;
    int increment = 1;
    int min = -70;
    int max = 0;
    QString bla;
    for(int i=0; i<samples.size(); i++) {
        bla = bla + QString::number(samples[i],'f',2) + "_";
    }
    ui->label_2->setText(bla);
    //increment = samples.size()/NUM_BANDS;
    ui->progressBar->setMinimum(min);
    ui->progressBar_2->setMinimum(min);
    ui->progressBar_3->setMinimum(min);
    ui->progressBar_4->setMinimum(min);
    ui->progressBar_5->setMinimum(min);
    ui->progressBar_6->setMinimum(min);
    ui->progressBar_7->setMinimum(min);
    ui->progressBar_8->setMinimum(min);
    ui->progressBar_9->setMinimum(min);

    ui->progressBar->setMaximum(max);
    ui->progressBar_2->setMaximum(max);
    ui->progressBar_3->setMaximum(max);
    ui->progressBar_4->setMaximum(max);
    ui->progressBar_5->setMaximum(max);
    ui->progressBar_6->setMaximum(max);
    ui->progressBar_7->setMaximum(max);
    ui->progressBar_8->setMaximum(max);
    ui->progressBar_9->setMaximum(max);

    ui->progressBar->setValue(samples.at(0*increment));
    ui->progressBar_2->setValue(samples.at(1*increment));
    ui->progressBar_3->setValue(samples.at(2*increment));
    ui->progressBar_4->setValue(samples.at(3*increment));
    ui->progressBar_5->setValue(samples.at(4*increment));
    ui->progressBar_6->setValue(samples.at(5*increment));
    ui->progressBar_7->setValue(samples.at(6*increment));
    ui->progressBar_8->setValue(samples.at(7*increment));
    ui->progressBar_9->setValue(samples.at(8*increment));
    //qDebug() << "Max: " + QString::number(max_val) << "Min: " + QString::number(min_val);
}

void Dialog::loadLevels(double left, double right) {
    //qDebug() << "left" + QString::number(left) << "right:" + QString::number(right);
}
