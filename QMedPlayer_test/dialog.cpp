#include "dialog.h"
#include "ui_dialog.h"

#define MINBAR -60
#define MAXBAR 0

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
	lcdCharDef(lcd_h, 1, pse);

	scrollCounter = 0;
    lcdMode = 0;
    previousIndex = 0;
    sampleRate = 44100;

	sample.resize(SPECSIZE);
    calculator = new FFTCalc(this);
    probe = new QAudioProbe(this);
	qRegisterMetaType< QVector<double> >("QVector<double>");

	barsCount = 9;
	octaves = 1;
	arr.resize(barsCount);
	for(int i=0; i<barsCount; i++) {
		arr[i] = new QProgressBar(this);
		arr[i]->setOrientation(Qt::Vertical);
		arr[i]->setTextVisible(false);
		arr[i]->resize(10,95);
		arr[i]->move(300+10*i,300);
        arr[i]->setMinimum(MINBAR);
        arr[i]->setMaximum(MAXBAR);
        arr[i]->setValue(MINBAR);
	}

	connect(probe, SIGNAL(audioBufferProbed(QAudioBuffer)),
            this, SLOT(processBuffer(QAudioBuffer)));
//	connect(this,  SIGNAL(levels(double,double)),
//			this,SLOT(loadLevels(double,double)));
	connect(calculator, SIGNAL(calculatedSpectrum(QVector<double>)),
			this, SLOT(loadSamples(QVector<double>)));

	myPlayer = new QMediaPlayer(this);
	QMediaPlaylist *myPlaylist = new QMediaPlaylist(this);

	Lirc *myLirc = new Lirc(this);
	connect(myLirc,SIGNAL(key_event(QString)),this,SLOT(handleKey(QString)));

	QTimer *myTimer = new QTimer(this);
    connect(myTimer, SIGNAL(timeout()), this, SLOT(onEverySecond()));
	myTimer->start(1000);

    ui->tableWidget->setColumnCount(3);
	int count = 0;

	QString directory = QString::fromUtf8(usbPath());
	QDir dir(directory);
    //QTime time(0,0);
	QStringList files = dir.entryList(QStringList() << tr("*.mp3"),QDir::Files);
	QList<QMediaContent> content;
	for(const QString& f:files)
	{
        //time.setHMS(0,0,0);
		content.push_back(QUrl::fromLocalFile(dir.path()+ '/' + f));
		TagLib::FileRef fil((dir.path()+ '/' + f).toLatin1().data());
        //qDebug()<<QString::fromStdString(fil.tag()->title().to8Bit());
        //qDebug()<<QString::fromStdString(fil.tag()->artist().to8Bit());
		ui->tableWidget->insertRow(count);
        ui->tableWidget->setItem(count,0,new QTableWidgetItem(QString::number(count+1) + '.'));
        ui->tableWidget->setItem(count,1,new QTableWidgetItem(f));
        ui->tableWidget->setItem(count,2,new QTableWidgetItem(QString::number(fil.audioProperties()->length() / 60) + ':' +
                    QString::number(fil.audioProperties()->length() % 60).rightJustified(2,'0')));
        //ui->tableWidget->setItem(count,2,new QTableWidgetItem(time.addSecs(fil.audioProperties()->length()).toString(tr("mm:ss"))));
        count++;

	}

    ui->label->setText(tr("0:00"));
    ui->label_2->setText(tr("0:00"));
    //ui->horizontalSlider_2->setValue(100);
    ui->progressBar->setValue(100);

    ui->listWidget->addItem(tr("Mi smo iskra u smrtnu prasinu, mi smo luca tamom obuzeta"));
    ui->listWidget->setCurrentRow(0);
    //ui->listWidget->horizontalScrollBar()->setVisible(false);
    ui->listWidget->horizontalScrollBar()->setStyleSheet(tr("width:0px;"));

    ui->listWidget_2->addItem(tr("Naziv:"));
    ui->listWidget_2->addItem(tr("Izvođač:"));
    ui->listWidget_2->addItem(tr("Žanr:"));
    ui->listWidget_2->addItem(tr("Godina:"));
    ui->listWidget_2->addItem(tr("Kbps:"));
    ui->listWidget_2->addItem(tr("Sample rate:"));
    //ui->listWidget_2->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    //ui->listWidget_2->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    ui->listWidget_2->setFixedHeight(6*ui->listWidget_2->sizeHintForRow(0)+2*ui->listWidget_2->lineWidth());
    ui->listWidget_2->setAlternatingRowColors(true);

    for (int i=0; i<6; i++) {
        ui->listWidget_3->addItem(tr("N/A"));
    }
    //ui->listWidget_3->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    ui->listWidget_3->setFixedHeight(ui->listWidget_2->height());
    ui->listWidget_3->setAlternatingRowColors(true);
    //ui->listWidget_3->horizontalScrollBar()->setStyleSheet(tr("width:0px;"));
    //ui->listWidget->horizontalScrollBar()->hide();
    //ui->tableWidget->horizontalHeader()->setVisible(false);
    //ui->tableWidget->setShowGrid(false);
    //connect(ui->tableWidget, SIGNAL(itemSelectionChanged()), this, SLOT(mySlot()));
    //ui->tableWidget->setRowHeight(0,5);
    ui->tableWidget_2->setRowCount(6);
    ui->tableWidget_2->setColumnCount(2);
    ui->tableWidget_2->setItem(0,0,new QTableWidgetItem(tr("Naziv:")));
    ui->tableWidget_2->setItem(1,0,new QTableWidgetItem(tr("Izvođač:")));
    ui->tableWidget_2->setItem(2,0,new QTableWidgetItem(tr("Žanr:")));
    ui->tableWidget_2->setItem(3,0,new QTableWidgetItem(tr("Godina:")));
    ui->tableWidget_2->setItem(4,0,new QTableWidgetItem(tr("Kbps:")));
    ui->tableWidget_2->setItem(5,0,new QTableWidgetItem(tr("Sample rate:")));
    for(int i=0; i<6; i++) {
        ui->tableWidget_2->setRowHeight(i,ui->listWidget->sizeHintForRow(0));
        ui->tableWidget_2->setItem(i,1,new QTableWidgetItem(tr("N/A")));
    }
    ui->tableWidget_2->setFixedHeight(6*ui->tableWidget_2->rowHeight(0) + 2*ui->tableWidget_2->lineWidth());
    ui->tableWidget_2->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
    ui->tableWidget_2->horizontalHeader()->setSectionResizeMode(0,QHeaderView::Fixed);
    ui->tableWidget_2->horizontalHeader()->setSectionResizeMode(1,QHeaderView::Stretch);
    ui->tableWidget_2->setAlternatingRowColors(true);
    //ui->tableWidget_2->setCurrentCell(0,0);
//    ui->tableWidget_2->setItem(0,0,new QTableWidgetItem(tr("Bla")));
//    //ui->tableWidget_2->currentItem()->setText(tr("Bla"));
//    ui->tableWidget_2->verticalHeader()->resizeSections(QHeaderView::ResizeToContents);
//    ui->tableWidget_2->horizontalHeader()->setStretchLastSection(true);
    QFont tempFont = ui->tableWidget->item(0,0)->font();
    tempFont.setBold(true);
    tempFont.setItalic(true);
    for(int i=0; i<ui->tableWidget->rowCount(); i++) {
        ui->tableWidget->item(i,2)->setFont(tempFont);
    }
    ui->tableWidget->item(ui->tableWidget->rowCount()-1,0)->setFont(tempFont);

    ui->tableWidget->verticalHeader()->resizeSections(QHeaderView::ResizeToContents);
    ui->tableWidget->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);

    tempFont.setBold(false);
    tempFont.setItalic(false);
    for(int i=0; i<ui->tableWidget->rowCount(); i++) {
        ui->tableWidget->item(i,2)->setFont(tempFont);
    }
    ui->tableWidget->item(ui->tableWidget->rowCount()-1,0)->setFont(tempFont);
    //ui->tableWidget->setColumnWidth(0,30);
    //ui->tableWidget->setColumnWidth(2,60);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(0,QHeaderView::Fixed);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(1,QHeaderView::Stretch);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(2,QHeaderView::Fixed);
    //ui->tableWidget->horizontalHeader()->resizeSection(0,QHeaderView::ResizeToContents);
    //ui->tableWidget->horizontalHeader()->resizeSection(1,200);
    ui->tableWidget->setCurrentCell(0,1);
    ui->tableWidget->setAutoScroll(true);
//    QPalette bla = ui->tableWidget->palette();
//    bla.setColor(QPalette::Base,Qt::cyan);
//    ui->tableWidget->setPalette(bla);
    //ui->tableWidget->setStyleSheet("background-color:skyblue");
    ui->tableWidget->setProperty("myProperty","bla");
    //ui->tableWidget->setStyleSheet("QTableWidget[myProperty=bla]{background:skyblue}");
    //ui->tableWidget->horizontalHeader()->resizeSections(QHeaderView::Fixed);
    //ui->tableWidget->setColumnWidth(0,10);
    //ui->tableWidget->adjustSize();
    connect(ui->tableWidget,SIGNAL(currentCellChanged(int,int,int,int)),this,SLOT(onCurrentCellChanged(int,int,int,int)));
    connect(myPlayer,SIGNAL(volumeChanged(int)),ui->progressBar,SLOT(setValue(int)));

	myPlaylist->addMedia(content);
    myPlayer->setPlaylist(myPlaylist);
    myPlayer->playlist()->setPlaybackMode(QMediaPlaylist::Loop);
    myPlayer->playlist()->setCurrentIndex(0);
    connect(myPlayer,SIGNAL(durationChanged(qint64)),this,SLOT(onDurationChanged(qint64)));
	connect(myPlayer,SIGNAL(currentMediaChanged(QMediaContent)),this,SLOT(onSongChanged(QMediaContent)));
    connect(myPlayer,SIGNAL(positionChanged(qint64)),this,SLOT(onPositionChanged(qint64)));
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

    probe->setSource(myPlayer);
    //emit myPlayer->durationChanged(300000);
    //emit myPlayer->currentMediaChanged(myPlayer->playlist()->currentMedia());
}

Dialog::~Dialog()
{
	lcdClear(lcd_h);
	delete ui;
}

void Dialog::mySlot(int index) {
}

void Dialog::onCurrentCellChanged(int currRow, int currCol, int prevRow, int prevCol) {
}

void Dialog::onMetaDataChanged() {
    TagLib::FileRef file(myPlayer->currentMedia().canonicalUrl().path().toLatin1().data());
	if(myPlayer->isMetaDataAvailable()) {
        //ui->label->setText(myPlayer->metaData("SampleRate").toString());
        //ui->label->setText(QString::number(file.audioProperties()->sampleRate()));
	}
}

void Dialog::onDurationChanged(qint64 duration) {
    //ui->label->setText(QString::number(duration));
    ui->horizontalSlider->setMaximum(duration);
}

void Dialog::onPositionChanged(qint64 pos) {
    ui->horizontalSlider->setValue(pos);
    pos /= 1000;
    ui->label_2->setText((QString::number(pos / 60) + ':' +
                          QString::number(pos % 60).rightJustified(2,'0')));
    //ui->label_2->setText(QString::number(pos));
	//
}

void Dialog::onSongChanged(QMediaContent song) {
    int current = myPlayer->playlist()->currentIndex();
    QFont f = ui->tableWidget->item(current,1)->font();
    for(int i=0; i<ui->tableWidget->columnCount(); i++) {
        ui->tableWidget->item(previousIndex,i)->setFont(f);
        f.setItalic(true);
        f.setBold(true);
        ui->tableWidget->item(current,i)->setFont(f);
        f.setItalic(false);
        f.setBold(false);
    }
    ui->label->setText(ui->tableWidget->item(current,2)->text());
    previousIndex = current;
	for(int i=0; i<barsCount; i++) {
		arr[i]->setValue(arr[i]->minimum());
    }

    TagLib::FileRef file(song.canonicalUrl().path().toLatin1().data());
    sampleRate = file.audioProperties()->sampleRate();

    currentSong = song.canonicalUrl().fileName();
    //ui->tableWidget_2->currentItem()->setText(currentSong);
    //ui->tableWidget_2->item(0,0)->setText(currentSong);
    ui->listWidget->currentItem()->setText(currentSong);
    ui->listWidget->horizontalScrollBar()->setValue(0);
    //ui->listWidget_3->horizontalScrollBar()->setValue(0);

    ui->listWidget_3->item(0)->setText(QString::fromStdString(file.tag()->title().to8Bit()));
    ui->listWidget_3->item(1)->setText(QString::fromStdString(file.tag()->artist().to8Bit()));
    ui->listWidget_3->item(2)->setText(QString::fromStdString(file.tag()->genre().to8Bit()));
    ui->listWidget_3->item(3)->setText(QString::number(file.tag()->year()));
    ui->listWidget_3->item(4)->setText(QString::number(file.audioProperties()->bitrate()));
    ui->listWidget_3->item(5)->setText(QString::number(sampleRate));
    for(int i=0; i<6; i++) {
        if((ui->listWidget_3->item(i)->text().isEmpty()) || (ui->listWidget_3->item(i)->text()==QString::number(0))) {
            ui->listWidget_3->item(i)->setText(tr("N/A"));
        }
        if(ui->listWidget_3->item(i)->text().isNull()) {
            qDebug() << currentSong << "item no "<<i;
        }
    }
}

void Dialog::handleKey(const QString& key) {
    QFont f;
	if(key == tr("KEY_PLAY")) {
		switch(myPlayer->state()) {
			case QMediaPlayer::StoppedState:
//                if(myPlayer->playlist()->currentIndex() == ui->tableWidget->currentRow()) {
//                    f = ui->tableWidget->currentItem()->font();
//                    f.setItalic(true);
//                    f.setBold(true);
//                    for(int i=0; i<ui->tableWidget->columnCount(); i++) {
//                        ui->tableWidget->item(ui->tableWidget->currentRow(),i)->setFont(f);
//                    }
//                }
                myPlayer->playlist()->setCurrentIndex(ui->tableWidget->currentRow());
//                ui->label->setText(ui->tableWidget->item(ui->tableWidget->currentRow(),2)->text());
//                ui->horizontalSlider->setMaximum(myPlayer->duration());
                //ui->tableWidget->setStyleSheet("selection-color: turquoise");
				myPlayer->play();
				lcdClear(lcd_h);
//				currentSong = myPlayer->currentMedia().canonicalUrl().fileName();
                emit myPlayer->currentMediaChanged(myPlayer->currentMedia());
				break;
			case QMediaPlayer::PausedState:
                if(myPlayer->playlist()->currentIndex() == ui->tableWidget->currentRow()) {
                    myPlayer->play();
                }
                else {
                    myPlayer->playlist()->setCurrentIndex(ui->tableWidget->currentRow());
                    //ui->tableWidget->setStyleSheet("selection-color: turquoise");
                    myPlayer->play();
                }
				break;
			case QMediaPlayer::PlayingState:
                if(myPlayer->playlist()->currentIndex() == ui->tableWidget->currentRow()) {
                    myPlayer->pause();
                }
                else {
                    myPlayer->playlist()->setCurrentIndex(ui->tableWidget->currentRow());
                    //ui->tableWidget->setStyleSheet("selection-color: turquoise");
                    myPlayer->play();
                }
				break;
		}
	}
	else if(key == tr("KEY_CH")) {
		myPlayer->stop();

        for(int i=0; i<barsCount; i++) {
            arr[i]->setValue(MINBAR);
        }

        f = ui->tableWidget->currentItem()->font();
        f.setItalic(false);
        f.setBold(false);

        for(int i=0; i<ui->tableWidget->columnCount(); i++) {
            ui->tableWidget->item(myPlayer->playlist()->currentIndex(),i)->setFont(f);
        }

        ui->label->setText(tr("0:00"));
        ui->label_2->setText(tr("0:00"));

        for (int i=0; i<6; i++) {
            ui->listWidget_3->item(i)->setText(tr("N/A"));
        }
	}
	else if(key == tr("KEY_NEXT")) {
		scrollCounter = 0;
		lcdClear(lcd_h);
        if(myPlayer->playlist()->currentIndex() == myPlayer->playlist()->mediaCount()-1) {
            myPlayer->playlist()->setCurrentIndex(0);
        }
        else {
            myPlayer->playlist()->next();
        }
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
        //ui->listWidget->horizontalScrollBar()->setValue(ui->listWidget->horizontalScrollBar()->value()-20);
	}
    else if(key == tr("KEY_CH+")) {
        //ui->listWidget->horizontalScrollBar()->setValue(ui->listWidget->horizontalScrollBar()->value()+20);
	}
	else if(key == tr("KEY_UP")) {
		myPlayer->setVolume(myPlayer->volume() + 5);
	}
	else if(key == tr("KEY_DOWN")) {
		myPlayer->setVolume(myPlayer->volume() - 5);
	}
	else if(key == tr("KEY_EQ")) {
		if(ui->radioButton->isChecked()) {
			ui->radioButton_2->setChecked(true);
			octaves = 2;
		}
		else if(ui->radioButton_2->isChecked()) {
			ui->radioButton_3->setChecked(true);
			octaves = 3;
		}
		else {
			ui->radioButton->setChecked(true);
			octaves = 1;
		}
	}
    else if(key == tr("KEY_5")) {
        ui->tableWidget->setCurrentCell((ui->tableWidget->currentRow()+1)%ui->tableWidget->rowCount(),1);
    }
    else if(key == tr("KEY_2")) {
        if(ui->tableWidget->currentRow()) {
            ui->tableWidget->setCurrentCell(ui->tableWidget->currentRow()-1,1);
        }
        else {
            ui->tableWidget->setCurrentCell(ui->tableWidget->rowCount()-1,1);
        }
    }
    else if(key == tr("KEY_3")) {
        ui->tableWidget->setProperty("myProperty","kek");
        ui->tableWidget->style()->unpolish(ui->tableWidget);
        ui->tableWidget->style()->polish(ui->tableWidget);
    }
    else if(key == tr("KEY_1")) {
        ui->tableWidget->setProperty("myProperty","bla");
        ui->tableWidget->style()->unpolish(ui->tableWidget);
        ui->tableWidget->style()->polish(ui->tableWidget);
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
                        emit myPlayer->currentMediaChanged(myPlayer->currentMedia());
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
                if(myPlayer->playlist()->currentIndex() == myPlayer->playlist()->mediaCount()-1) {
                    myPlayer->playlist()->setCurrentIndex(0);
                }
                else {
                    myPlayer->playlist()->next();
                }
				myPlayer->play();
			}
			break;
	}
}

void Dialog::onEverySecond() {
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

        if(ui->listWidget->horizontalScrollBar()->value() < ui->listWidget->horizontalScrollBar()->maximum()) {
            ui->listWidget->horizontalScrollBar()->setValue(ui->listWidget->horizontalScrollBar()->value()+7);
        }
        else {
            ui->listWidget->horizontalScrollBar()->setValue(0);
        }

//        if(ui->listWidget_3->horizontalScrollBar()->value() < ui->listWidget_3->horizontalScrollBar()->maximum()) {
//            ui->listWidget_3->horizontalScrollBar()->setValue(ui->listWidget_3->horizontalScrollBar()->value()+7);
//        }
//        else {
//            ui->listWidget_3->horizontalScrollBar()->setValue(0);
//        }
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
			sample[i] = (data[i].left + data[i].right)/2/peakValue;
//			levelLeft+= abs(data[i].left)/peakValue;
//			levelRight+= abs(data[i].right)/peakValue;
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
//			levelLeft+= abs(data[i].left)/peakValue;
//			levelRight+= abs(data[i].right)/peakValue;
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
		//qDebug() << "Pozivam calculator";
		duration = buffer.format().durationForBytes(buffer.frameCount())/1000;
        calculator->calc(sample, duration, octaves, sampleRate);
	}
	// tells anyone interested about left and right mean levels
//	emit levels(levelLeft/buffer.frameCount(),levelRight/buffer.frameCount());
}

void Dialog::loadSamples(QVector<double> samples) {
	//QVector<double> samples = spectrum;
	if(barsCount != samples.size()) {
		for (int i=0; i<barsCount; i++) {
			arr[i]->deleteLater();
		}

        //qDebug() << "Obrisani barovi";
		barsCount = samples.size();
        //qDebug() << "Sada barova treba da ima: " << barsCount;

		arr.resize(barsCount);
		for(int i=0; i<barsCount; i++) {
			arr[i] = new QProgressBar(this);
			arr[i]->setOrientation(Qt::Vertical);
			arr[i]->setTextVisible(false);
			arr[i]->resize(10,95);
			arr[i]->move(300+10*i,300);
            arr[i]->setMinimum(MINBAR);
            arr[i]->setMaximum(MAXBAR);
            arr[i]->setValue(MINBAR);
			arr[i]->show();
		}
        //qDebug() << "Postavljeni novi barovi";
    }
	//QString bla;
	//    for(int i=0; i<samples.size(); i++) {
	//        bla = bla + QString::number(samples[i],'f',2) + "_";
	//    }
    //int bla;

    for(int i=0; i<barsCount; i++) {
        //budz
        //qDebug() << "Prije ulaska u budz, octaves="<<octaves<<",barsCount="<<barsCount<<",samplesSize="<<samples.size();
        if(samples[i] != samples[i]) {
            //samples[i] = -34;
            //qDebug() << "Usao u budz region, i="<<i;
            int j = i+1;
            while(samples[j] != samples[j]) {
                //qDebug() << "Usao u while u budz regionu";
                j++;
                //qDebug() << "j = "<< j << " barsCount = " << barsCount;
            }
            samples[i] = (samples[i-1]+samples[j])/2;
        }
        if(myPlayer->state() != QMediaPlayer::StoppedState) {
            arr[i]->setValue(samples[i]);
        }
    }
}

