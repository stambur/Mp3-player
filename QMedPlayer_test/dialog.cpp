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
    color = tr("blue");

	sample.resize(SPECSIZE);
    calculator = new FFTCalc(this);
    probe = new QAudioProbe(this);
	qRegisterMetaType< QVector<double> >("QVector<double>");

    myHLayout = new QHBoxLayout();
    barsCount = 0;
    octaves = 3;
    arr.resize(barsCount);
    ui->gridLayout_2->addLayout(myHLayout,1,0,Qt::AlignCenter);

	connect(probe, SIGNAL(audioBufferProbed(QAudioBuffer)),
            this, SLOT(processBuffer(QAudioBuffer)));
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
	QStringList files = dir.entryList(QStringList() << tr("*.mp3"),QDir::Files);
	QList<QMediaContent> content;
	for(const QString& f:files)
    {
		content.push_back(QUrl::fromLocalFile(dir.path()+ '/' + f));
        TagLib::FileRef fil((dir.path()+ '/' + f).toLatin1().data());
		ui->tableWidget->insertRow(count);
        ui->tableWidget->setItem(count,0,new QTableWidgetItem(QString::number(count+1) + '.'));
        ui->tableWidget->setItem(count,1,new QTableWidgetItem(f));
        ui->tableWidget->setItem(count,2,new QTableWidgetItem(QString::number(fil.audioProperties()->length() / 60) + ':' +
                    QString::number(fil.audioProperties()->length() % 60).rightJustified(2,'0')));
        count++;

	}

    ui->label->setText(tr("0:00"));
    ui->label_2->setText(tr("0:00"));
    ui->label_4->setFont(QFont("Courier New",14));
    ui->label->setFont(QFont("Courier New",14));
    ui->label_2->setFont(QFont("Courier New",14));
    ui->progressBar->setValue(100);

    ui->listWidget->addItem(tr("Play stopped"));
    ui->listWidget->setCurrentRow(0);
    ui->listWidget->setFont(QFont("Courier New",14));
    ui->listWidget->horizontalScrollBar()->setStyleSheet(tr("width:0px;"));
    ui->listWidget->setFixedHeight(ui->listWidget->sizeHintForRow(0));//+2*ui->listWidget->lineWidth());

    ui->tableWidget_2->setRowCount(6);
    ui->tableWidget_2->setColumnCount(2);
    ui->tableWidget_2->setItem(0,0,new QTableWidgetItem(tr("Naziv:")));
    ui->tableWidget_2->setItem(1,0,new QTableWidgetItem(tr("Izvođač:")));
    ui->tableWidget_2->setItem(2,0,new QTableWidgetItem(tr("Žanr:")));
    ui->tableWidget_2->setItem(3,0,new QTableWidgetItem(tr("Godina:")));
    ui->tableWidget_2->setItem(4,0,new QTableWidgetItem(tr("Kbps:")));
    ui->tableWidget_2->setItem(5,0,new QTableWidgetItem(tr("Sample rate:")));
    for(int i=0; i<6; i++) {
        ui->tableWidget_2->setRowHeight(i,16);
        ui->tableWidget_2->setItem(i,1,new QTableWidgetItem(tr("N/A")));
    }
    ui->tableWidget_2->setFixedHeight(6*ui->tableWidget_2->rowHeight(0));
    ui->tableWidget_2->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
    ui->tableWidget_2->horizontalHeader()->setSectionResizeMode(0,QHeaderView::Fixed);
    ui->tableWidget_2->horizontalHeader()->setSectionResizeMode(1,QHeaderView::Stretch);
    ui->tableWidget_2->setAlternatingRowColors(true);

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
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(0,QHeaderView::Fixed);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(1,QHeaderView::Stretch);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(2,QHeaderView::Fixed);
    ui->tableWidget->setCurrentCell(0,1);
    connect(myPlayer,SIGNAL(volumeChanged(int)),ui->progressBar,SLOT(setValue(int)));

	myPlaylist->addMedia(content);
    myPlayer->setPlaylist(myPlaylist);
    myPlayer->playlist()->setPlaybackMode(QMediaPlaylist::Loop);
    myPlayer->playlist()->setCurrentIndex(0);
    connect(myPlayer,SIGNAL(durationChanged(qint64)),this,SLOT(onDurationChanged(qint64)));
	connect(myPlayer,SIGNAL(currentMediaChanged(QMediaContent)),this,SLOT(onSongChanged(QMediaContent)));
    connect(myPlayer,SIGNAL(positionChanged(qint64)),this,SLOT(onPositionChanged(qint64)));
    connect(this,SIGNAL(hw_btn_clicked(int)),this,SLOT(onHwBtnClicked(int)));
    connect(myPlayer,SIGNAL(stateChanged(QMediaPlayer::State)),this,SLOT(onPlayerStateChanged(QMediaPlayer::State)));
	lcdClear(lcd_h);
	lcdPosition(lcd_h,0,0);
	lcdPutchar(lcd_h,0xFF);
	lcdPutchar(lcd_h,0xFF);
	lcdPrintf(lcd_h,"Play stopped");
	for(int i=0; i<18; i++) {
		lcdPutchar(lcd_h,0xFF);
    }

    probe->setSource(myPlayer);
    this->updateStyleSheets();
}

Dialog::~Dialog()
{
	lcdClear(lcd_h);
	delete ui;
}

void Dialog::onDurationChanged(qint64 duration) {
    ui->horizontalSlider->setMaximum(duration);
}

void Dialog::onPositionChanged(qint64 pos) {
    ui->horizontalSlider->setValue(pos);
    pos /= 1000;
    ui->label_2->setText((QString::number(pos / 60) + ':' +
                          QString::number(pos % 60).rightJustified(2,'0')));
}

void Dialog::onPlayerStateChanged(QMediaPlayer::State newState) {
    QFont f;
    if(newState == QMediaPlayer::StoppedState) {
        for(int i=0; i<barsCount; i++) {
            arr[i]->setValue(MINBAR);
        }

        f = ui->tableWidget->currentItem()->font();
        f.setItalic(false);
        f.setBold(false);

        if(myPlayer->playlist()->currentIndex() != -1)
        for(int i=0; i<ui->tableWidget->columnCount(); i++) {
            ui->tableWidget->item(myPlayer->playlist()->currentIndex(),i)->setFont(f);
        }

        ui->label->setText(tr("0:00"));
        ui->label_2->setText(tr("0:00"));
        ui->listWidget->currentItem()->setText(tr("Play stopped"));

        for (int i=0; i<6; i++) {
            ui->tableWidget_2->item(i,1)->setText(tr("N/A"));
        }
    }
}

void Dialog::onSongChanged(QMediaContent song) {
    QFont f;
    if(myPlayer->playlist()->currentIndex() >= 0) {
        int current = myPlayer->playlist()->currentIndex();
        f = ui->tableWidget->item(current,1)->font();
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

        ui->listWidget->currentItem()->setText(currentSong);
        ui->listWidget->horizontalScrollBar()->setValue(0);
        ui->tableWidget_2->item(0,1)->setText(QString::fromStdString(file.tag()->title().to8Bit()));
        ui->tableWidget_2->item(1,1)->setText(QString::fromStdString(file.tag()->artist().to8Bit()));
        ui->tableWidget_2->item(2,1)->setText(QString::fromStdString(file.tag()->genre().to8Bit()));
        ui->tableWidget_2->item(3,1)->setText(QString::number(file.tag()->year()));
        ui->tableWidget_2->item(4,1)->setText(QString::number(file.audioProperties()->bitrate()));
        ui->tableWidget_2->item(5,1)->setText(QString::number(sampleRate));
        for(int i=0; i<6; i++) {
            if((ui->tableWidget_2->item(i,1)->text().isEmpty()) || (ui->tableWidget_2->item(i,1)->text()==QString::number(0))) {
                ui->tableWidget_2->item(i,1)->setText(tr("N/A"));
            }
        }
    }
    else {
        f = ui->tableWidget->item(0,1)->font();
        f.setBold(false);
        f.setItalic(false);
        for(int i=0; i<ui->tableWidget->columnCount(); i++) {
            ui->tableWidget->item(previousIndex,i)->setFont(f);
        }
        myPlayer->stop();
    }
}

void Dialog::handleKey(const QString& key) {
	if(key == tr("KEY_PLAY")) {
		switch(myPlayer->state()) {
            case QMediaPlayer::StoppedState:
                myPlayer->playlist()->setCurrentIndex(ui->tableWidget->currentRow());
				myPlayer->play();
                lcdClear(lcd_h);
                emit myPlayer->currentMediaChanged(myPlayer->currentMedia());
				break;
			case QMediaPlayer::PausedState:
                if(myPlayer->playlist()->currentIndex() == ui->tableWidget->currentRow()) {
                    myPlayer->play();
                }
                else {
                    myPlayer->playlist()->setCurrentIndex(ui->tableWidget->currentRow());
                    myPlayer->play();
                }
				break;
			case QMediaPlayer::PlayingState:
                if(myPlayer->playlist()->currentIndex() == ui->tableWidget->currentRow()) {
                    myPlayer->pause();
                }
                else {
                    myPlayer->playlist()->setCurrentIndex(ui->tableWidget->currentRow());
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
        if((ui->radioButton_6->isChecked()) || (ui->radioButton_7->isChecked())) {
            myPlayer->playlist()->setCurrentIndex((myPlayer->playlist()->currentIndex()+1)%myPlayer->playlist()->mediaCount());
        }
        else if(myPlayer->playlist()->currentIndex() == myPlayer->playlist()->mediaCount()-1) {
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
        if((ui->radioButton_6->isChecked()) || (ui->radioButton_7->isChecked())) {
            if(myPlayer->playlist()->currentIndex()) {
                myPlayer->playlist()->setCurrentIndex(myPlayer->playlist()->currentIndex()-1);
            }
            else {
                myPlayer->playlist()->setCurrentIndex(myPlayer->playlist()->mediaCount()-1);
            }
        }
        else if(myPlayer->playlist()->currentIndex()==0) {
			myPlayer->playlist()->setCurrentIndex(myPlayer->playlist()->mediaCount()-1);
		}
		else {
			myPlayer->playlist()->previous();
		}
		myPlayer->play();
	}
    else if(key == tr("KEY_CH-")) {
        if(ui->radioButton_4->isChecked()) {
            ui->radioButton_5->setChecked(true);
            myPlayer->playlist()->setPlaybackMode(QMediaPlaylist::Sequential);
        }
        else if(ui->radioButton_5->isChecked()) {
            ui->radioButton_6->setChecked(true);
            myPlayer->playlist()->setPlaybackMode(QMediaPlaylist::CurrentItemOnce);
        }
        else if(ui->radioButton_6->isChecked()) {
            ui->radioButton_7->setChecked(true);
            myPlayer->playlist()->setPlaybackMode(QMediaPlaylist::CurrentItemInLoop);
        }
        else if(ui->radioButton_7->isChecked()) {
            ui->radioButton_8->setChecked(true);
            myPlayer->playlist()->setPlaybackMode(QMediaPlaylist::Random);
        }
        else {
            ui->radioButton_4->setChecked(true);
            myPlayer->playlist()->setPlaybackMode(QMediaPlaylist::Loop);
        }
	}
    else if(key == tr("KEY_CH+")) {
        if(ui->radioButton_9->isChecked()) {
            ui->radioButton_10->setChecked(true);
            ui->label_3->setPixmap(tr(":/imgs/VolumeNormal.png"));
            ui->label_3->setScaledContents(true);
            color = tr("green");
            this->updateStyleSheets();
        }
        else if(ui->radioButton_10->isChecked()) {
            ui->radioButton_11->setChecked(true);
            ui->label_3->setPixmap(tr(":/imgs/VolumeNormalRed.png"));
            ui->label_3->setScaledContents(true);
            color = tr("red");
            this->updateStyleSheets();
        }
        else {
            ui->radioButton_9->setChecked(true);
            ui->label_3->setPixmap(tr(":/imgs/VolumeNormalBlue.png"));
            ui->label_3->setScaledContents(true);
            color = tr("blue");
            this->updateStyleSheets();
        }
	}
	else if(key == tr("KEY_UP")) {
		myPlayer->setVolume(myPlayer->volume() + 5);
	}
	else if(key == tr("KEY_DOWN")) {
		myPlayer->setVolume(myPlayer->volume() - 5);
	}
	else if(key == tr("KEY_EQ")) {
        if(ui->radioButton_3->isChecked()) {
			ui->radioButton_2->setChecked(true);
			octaves = 2;
		}
		else if(ui->radioButton_2->isChecked()) {
            ui->radioButton->setChecked(true);
            octaves = 1;
		}
		else {
            ui->radioButton_3->setChecked(true);
            octaves = 3;
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
    }
    else if(key == tr("KEY_1")) {
        ui->tableWidget->setProperty("myProperty","bla");
    }
	else {
		myPlayer->stop();
		lcdClear(lcd_h);
		delete ui;
		exit(0);
    }
}

void Dialog::updateStyleSheets() {
    this->setProperty("colorScheme",color);
    ui->groupBox->setProperty("colorScheme",color);
    ui->groupBox_2->setProperty("colorScheme",color);
    ui->groupBox_3->setProperty("colorScheme",color);

    ui->tableWidget->setProperty("colorScheme",color);
    if(color == tr("blue")) {
        for(int i=0; i<ui->tableWidget->rowCount(); i++) {
            for(int j=0; j<ui->tableWidget->columnCount(); j++) {
                ui->tableWidget->item(i,j)->setBackgroundColor(QColor(230, 230, 255));
                ui->tableWidget->item(i,j)->setForeground(QBrush("violet"));
            }
        }
        for(int i=0; i<ui->tableWidget_2->rowCount(); i++) {
            for(int j=0; j<ui->tableWidget_2->columnCount(); j++) {
                ui->tableWidget_2->item(i,j)->setBackgroundColor(QColor(255, 230, 230));
                ui->tableWidget_2->item(i,j)->setForeground(QBrush("orange"));
            }
        }
        for(int i=0; i<barsCount; i++) {
            arr[i]->setStyleSheet(tr("background-color: skyblue; selection-background-color: rgb(20, 170, 255)"));
        }
        ui->progressBar->setStyleSheet(tr("background-color: skyblue; selection-background-color: rgb(20, 170, 255)"));
        ui->horizontalSlider->setStyleSheet(tr("selection-background-color: rgb(20, 170, 255)"));
        ui->listWidget->setStyleSheet(tr("selection-background-color:blue"));
    }
    else if(color == tr("green")) {
        for(int i=0; i<ui->tableWidget->rowCount(); i++) {
            for(int j=0; j<ui->tableWidget->columnCount(); j++) {
                ui->tableWidget->item(i,j)->setBackgroundColor(QColor(200, 255, 200));
                ui->tableWidget->item(i,j)->setForeground(QBrush("yellow"));
            }
        }
        for(int i=0; i<ui->tableWidget_2->rowCount(); i++) {
            for(int j=0; j<ui->tableWidget_2->columnCount(); j++) {
                ui->tableWidget_2->item(i,j)->setBackgroundColor(QColor(255, 230, 230));
                ui->tableWidget_2->item(i,j)->setForeground(QBrush("orange"));
            }
        }
        for(int i=0; i<barsCount; i++) {
            arr[i]->setStyleSheet(tr("background-color: rgb(152, 251, 152); selection-background-color: green"));
        }
        ui->progressBar->setStyleSheet(tr("background-color: rgb(152, 251, 152); selection-background-color: green"));
        ui->horizontalSlider->setStyleSheet(tr("selection-background-color: green"));
        ui->listWidget->setStyleSheet(tr("selection-background-color:green"));
    }
    else {
        for(int i=0; i<ui->tableWidget->rowCount(); i++) {
            for(int j=0; j<ui->tableWidget->columnCount(); j++) {
                ui->tableWidget->item(i,j)->setBackgroundColor(QColor(255, 230, 230));
                ui->tableWidget->item(i,j)->setForeground(QBrush("orange"));
            }
        }
        for(int i=0; i<ui->tableWidget_2->rowCount(); i++) {
            for(int j=0; j<ui->tableWidget_2->columnCount(); j++) {
                ui->tableWidget_2->item(i,j)->setBackgroundColor(QColor(255, 230, 230));
                ui->tableWidget_2->item(i,j)->setForeground(QBrush("orange"));
            }
        }
        for(int i=0; i<barsCount; i++) {
            arr[i]->setStyleSheet(tr("background-color: rgb(235, 60, 60); selection-background-color: red"));
        }
        ui->progressBar->setStyleSheet(tr("background-color: rgb(235, 60, 60); selection-background-color: red"));
        ui->horizontalSlider->setStyleSheet(tr("selection-background-color: red"));
        ui->listWidget->setStyleSheet(tr("selection-background-color:red"));
    }

    this->style()->polish(this);
    this->style()->unpolish(this);
    ui->groupBox->style()->polish(ui->groupBox);
    ui->groupBox->style()->unpolish(ui->groupBox);
    ui->groupBox_2->style()->polish(ui->groupBox_2);
    ui->groupBox_2->style()->unpolish(ui->groupBox_2);
    ui->groupBox_3->style()->polish(ui->groupBox_3);
    ui->groupBox_3->style()->unpolish(ui->groupBox_3);
    ui->tableWidget->style()->polish(ui->tableWidget);
    ui->tableWidget->style()->unpolish(ui->tableWidget);
}

void Dialog::onHwBtnClicked(int btn) {
	switch(btn) {
		case BTN_1: //mode
			lcdMode ^= 1;
			lcdClear(lcd_h);
			break;
        case BTN_2: //play/pause(mode=0) / stop(mode=1)
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
		}
	}

	// if the probe is listening to the audio
	// do fft calculations
	// when it is done, calculator will tell us
    if(probe->isActive()){
		duration = buffer.format().durationForBytes(buffer.frameCount())/1000;
        calculator->calc(sample, duration, octaves, sampleRate);
    }
}

void Dialog::loadSamples(QVector<double> samples) {
	if(barsCount != samples.size()) {
        for (int i=0; i<barsCount; i++) {
            myHLayout->removeWidget(arr[i]);
            arr[i]->deleteLater();
		}

        barsCount = samples.size();

		arr.resize(barsCount);
		for(int i=0; i<barsCount; i++) {
            arr[i] = new QProgressBar();
			arr[i]->setOrientation(Qt::Vertical);
            arr[i]->setTextVisible(false);
            arr[i]->setFixedSize(QSize(10,ui->tableWidget_2->height()));
            myHLayout->addWidget(arr[i]);
            myHLayout->setSpacing(0);
            arr[i]->setMinimum(MINBAR);
            arr[i]->setMaximum(MAXBAR);
            arr[i]->setValue(MINBAR);
        }
        this->updateStyleSheets();
    }

    for(int i=0; i<barsCount; i++) {
        if(samples[i] != samples[i]) {
            int j = i+1;
            while(samples[j] != samples[j]) {
                j++;
            }
            samples[i] = (samples[i-1]+samples[j])/2;
        }
        if(myPlayer->state() != QMediaPlayer::StoppedState) {
            arr[i]->setValue(samples[i]);
        }
    }
}

