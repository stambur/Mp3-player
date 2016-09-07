#include "dialog.h"
#include "ui_dialog.h"

#define NUM_BANDS 8
#define MINBAR -70
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

	sample.resize(SPECSIZE);
    calculator = new FFTCalc(this);
    probe = new QAudioProbe(this);
	qRegisterMetaType< QVector<double> >("QVector<double>");

	//QProgressBar *arr[9];
	//QVector<QProgressBar*> arr;
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
	}

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

    ui->tableWidget->setColumnCount(3);
	int count = 0;

	QString directory = QString::fromUtf8(usbPath());
	QDir dir(directory);
	QStringList files = dir.entryList(QStringList() << tr("*.mp3"),QDir::Files);
	QList<QMediaContent> content;
	for(const QString& f:files)
	{
		content.push_back(QUrl::fromLocalFile(dir.path()+ '/' + f));
		TagLib::FileRef fil((dir.path()+ '/' + f).toLatin1().data());
        //qDebug()<<QString::fromStdString(fil.tag()->title().to8Bit());
        //qDebug()<<QString::fromStdString(fil.tag()->artist().to8Bit());
//		ui->listWidget->addItem(f); //+ "\t" + QString::number(fil.audioProperties()->length() / 60) + ':' + QString::number(fil.audioProperties()->length() % 60).rightJustified(2,'0'));
//		ui->listWidget_2->addItem(QString::number(fil.audioProperties()->length() / 60) + ':' +
//				QString::number(fil.audioProperties()->length() % 60).rightJustified(2,'0'));
		ui->tableWidget->insertRow(count);
        ui->tableWidget->setItem(count,0,new QTableWidgetItem(QString::number(count+1) + '.'));
        ui->tableWidget->setItem(count,1,new QTableWidgetItem(f));
        ui->tableWidget->setItem(count,2,new QTableWidgetItem(QString::number(fil.audioProperties()->length() / 60) + ':' +
					QString::number(fil.audioProperties()->length() % 60).rightJustified(2,'0')));
        count++;

	}
    //ui->listWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
//    ui->listWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
//    ui->listWidget->setTextElideMode(Qt::ElideRight);
//    ui->listWidget->setAutoScroll(true);
//    ui->listWidget_2->setAutoScroll(true);
//    ui->listWidget->setCurrentRow(0);
//    ui->listWidget_2->setCurrentRow(0);
//    ui->listWidget->horizontalScrollBar()->setVisible(false);
    //ui->listWidget_2->adjustSize();
//    connect(ui->listWidget, SIGNAL(currentRowChanged(int)), this, SLOT(mySlot(int)));

    //ui->tableWidget->horizontalHeader()->setVisible(false);
    //ui->tableWidget->setShowGrid(false);
    //connect(ui->tableWidget, SIGNAL(itemSelectionChanged()), this, SLOT(mySlot()));
	//ui->tableWidget->setRowHeight(0,5);
    ui->tableWidget->verticalHeader()->resizeSections(QHeaderView::ResizeToContents);
    //ui->tableWidget->horizontalHeader()->resizeSection(0,QHeaderView::ResizeToContents);
    ui->tableWidget->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
    ui->tableWidget->horizontalHeader()->resizeSection(1,200);
    ui->tableWidget->setCurrentCell(0,1);
    ui->tableWidget->setAutoScroll(true);
    //ui->tableWidget->setColumnWidth(0,10);
    //ui->tableWidget->verticalHeader()->setHighlightSections(true);
    //ui->tableWidget->verticalHeader()->setFrameShape(QFrame::NoFrame);
    //ui->tableWidget->verticalHeader()->clearMask();
    //ui->tableWidget->adjustSize();
    connect(ui->tableWidget,SIGNAL(currentCellChanged(int,int,int,int)),this,SLOT(onCurrentCellChanged(int,int,int,int)));

	myPlaylist->addMedia(content);
    myPlayer->setPlaylist(myPlaylist);
    myPlayer->playlist()->setPlaybackMode(QMediaPlaylist::Loop);
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

void Dialog::mySlot(int index) {
    //qDebug() << "Usao u slot, red " << bla;
    //ui->listWidget_2->setCurrentRow(index);
}

void Dialog::onCurrentCellChanged(int currRow, int currCol, int prevRow, int prevCol) {
    if(ui->tableWidget->item(currRow,currCol)->textColor() == Qt::blue) {
        ui->tableWidget->setStyleSheet("selection-color: turquoise");
    }
    else if(ui->tableWidget->item(prevRow,prevCol)->textColor() == Qt::blue) {
        ui->tableWidget->setStyleSheet("selection-color: white");
    }
}

void Dialog::onMetaDataChanged() {
    //TagLib::FileRef file((dir.path()+ '/' + f).toLatin1().data());
    //qDebug()<<myPlayer->currentMedia().canonicalUrl().path();
    TagLib::FileRef file(myPlayer->currentMedia().canonicalUrl().path().toLatin1().data());
	if(myPlayer->isMetaDataAvailable()) {
        //ui->label->setText(myPlayer->metaData("SampleRate").toString());
        ui->label->setText(QString::number(file.audioProperties()->sampleRate()));
	}
}

void Dialog::onDurationChanged(qint64 duration) {

}

void Dialog::onPositionChanged(qint64 pos) {
	//
}

void Dialog::onSongChanged(QMediaContent song) {
    //ui->listWidget->setCurrentRow(myPlayer->playlist()->currentIndex() != -1 ? myPlayer->playlist()->currentIndex():0);
    //ui->listWidget_2->setCurrentRow(myPlayer->playlist()->currentIndex() != -1 ? myPlayer->playlist()->currentIndex():0);
    int current = myPlayer->playlist()->currentIndex();
    //int previous = myPlayer->playlist()->previousIndex();
    for(int i=0; i<ui->tableWidget->columnCount(); i++) {
        ui->tableWidget->item(previousIndex,i)->setTextColor(Qt::black);
        ui->tableWidget->item(current,i)->setTextColor(Qt::blue);
    }
//    ui->listWidget->item(previousIndex)->setTextColor(Qt::black);
//    ui->listWidget_2->item(previousIndex)->setTextColor(Qt::black);
//    ui->listWidget->item(current)->setTextColor(Qt::blue);
//    ui->listWidget_2->item(current)->setTextColor(Qt::blue);
    previousIndex = current;
	//lcdClear(lcd_h);
	//lcdPosition(lcd_h,0,0);
	for(int i=0; i<barsCount; i++) {
		arr[i]->setValue(arr[i]->minimum());
    }
	currentSong = song.canonicalUrl().fileName();
}

void Dialog::handleKey(const QString& key) {
	if(key == tr("KEY_PLAY")) {
		switch(myPlayer->state()) {
			case QMediaPlayer::StoppedState:
                myPlayer->playlist()->setCurrentIndex(ui->tableWidget->currentRow());
//                ui->listWidget->setStyleSheet(QString("selection-color: turquoise"));
//                ui->listWidget_2->setStyleSheet(QString("selection-color: turquoise"));
                ui->tableWidget->setStyleSheet("selection-color: turquoise");
				myPlayer->play();
				lcdClear(lcd_h);
				currentSong = myPlayer->currentMedia().canonicalUrl().fileName();
				break;
			case QMediaPlayer::PausedState:
                if(myPlayer->playlist()->currentIndex() == ui->tableWidget->currentRow()) {
                    myPlayer->play();
                }
                else {
                    myPlayer->playlist()->setCurrentIndex(ui->tableWidget->currentRow());
//                    ui->listWidget->setStyleSheet(QString("selection-color: turquoise"));
//                    ui->listWidget_2->setStyleSheet(QString("selection-color: turquoise"));
                    ui->tableWidget->setStyleSheet("selection-color: turquoise");
                    myPlayer->play();
                }
				break;
			case QMediaPlayer::PlayingState:
                if(myPlayer->playlist()->currentIndex() == ui->tableWidget->currentRow()) {
                    myPlayer->pause();
                }
                else {
                    myPlayer->playlist()->setCurrentIndex(ui->tableWidget->currentRow());
//                    ui->listWidget->setStyleSheet(QString("selection-color: turquoise"));
//                    ui->listWidget_2->setStyleSheet(QString("selection-color: turquoise"));
                    ui->tableWidget->setStyleSheet("selection-color: turquoise");
                    myPlayer->play();
                }
				break;
		}
	}
	else if(key == tr("KEY_CH")) {
		myPlayer->stop();
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
//        ui->listWidget->setStyleSheet(QString("selection-color: white"));
//        ui->listWidget_2->setStyleSheet(QString("selection-color: white"));
//        ui->listWidget->setCurrentRow((ui->listWidget->currentRow()+1)%ui->listWidget->count());
        ui->tableWidget->setCurrentCell((ui->tableWidget->currentRow()+1)%ui->tableWidget->rowCount(),1);
//        if(ui->listWidget->item(ui->listWidget->currentRow())->textColor() == Qt::blue) {
//            ui->listWidget->setStyleSheet(QString("selection-color: turquoise"));
//            ui->listWidget_2->setStyleSheet(QString("selection-color: turquoise"));
//        }
        //ui->listWidget->horizontalScrollBar()->setValue(ui->listWidget->horizontalScrollBar()->minimum());
    }
    else if(key == tr("KEY_2")) {
//        ui->listWidget->setStyleSheet(QString("selection-color: white"));
//        ui->listWidget_2->setStyleSheet(QString("selection-color: white"));
//        if(ui->listWidget->currentRow()) {
//            ui->listWidget->setCurrentRow((ui->listWidget->currentRow()-1));
//        }
//        else {
//            ui->listWidget->setCurrentRow(ui->listWidget->count()-1);
//        }

        if(ui->tableWidget->currentRow()) {
            ui->tableWidget->setCurrentCell(ui->tableWidget->currentRow()-1,1);
        }
        else {
            ui->tableWidget->setCurrentCell(ui->tableWidget->rowCount()-1,1);
        }

//        if(ui->listWidget->selectedItems()[0]->textColor() == Qt::blue) {
//            ui->listWidget->setStyleSheet(QString("selection-color: turquoise"));
//            ui->listWidget_2->setStyleSheet(QString("selection-color: turquoise"));
//        }
        //ui->listWidget->horizontalScrollBar()->setValue(ui->listWidget->horizontalScrollBar()->minimum());
    }
    else if(key == tr("KEY_3")) {
        //ui->listWidget->horizontalScrollBar()->setValue(ui->listWidget->horizontalScrollBar()->value()+20);
    }
    else if(key == tr("KEY_1")) {
        //ui->listWidget->horizontalScrollBar()->setValue(ui->listWidget->horizontalScrollBar()->value()-20);
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
		//qDebug() << "Pozivam calculator";
		duration = buffer.format().durationForBytes(buffer.frameCount())/1000;
		calculator->calc(sample, duration, octaves);
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
			arr[i]->show();
		}
		//qDebug() << "Postavljeni novi barovi";
    }
	//QString bla;
	//    for(int i=0; i<samples.size(); i++) {
	//        bla = bla + QString::number(samples[i],'f',2) + "_";
	//    }
    int bla;

    for(int i=0; i<barsCount; i++) {
        if(samples[i] != samples[i]) {
            //samples[i] = -34;
        }

        arr[i]->setValue(samples[i]);

	}
	//ui->label_2->setText(bla);
	//increment = samples.size()/NUM_BANDS;

	//    ui->progressBar->setValue(samples.at(0*increment));
	//    ui->progressBar_2->setValue(samples.at(1*increment));
	//    ui->progressBar_3->setValue(samples.at(2*increment));
	//    ui->progressBar_4->setValue(samples.at(3*increment));
	//    ui->progressBar_5->setValue(samples.at(4*increment));
	//    ui->progressBar_6->setValue(samples.at(5*increment));
	//    ui->progressBar_7->setValue(samples.at(6*increment));
	//    ui->progressBar_8->setValue(samples.at(7*increment));
	//    ui->progressBar_9->setValue(samples.at(8*increment));
	//qDebug() << "Max: " + QString::number(max_val) << "Min: " + QString::number(min_val);
}

void Dialog::loadLevels(double left, double right) {
    //qDebug() << "left" + QString::number(left) << "right:" + QString::number(right);
    ui->progressBar->setValue(left*100);
    ui->progressBar_2->setValue(right*100);
}
