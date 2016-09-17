#include "dialog.h"
#include "ui_dialog.h"

#define MINBAR -60
#define MAXBAR 0

Dialog::Dialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::Dialog)
{
	ui->setupUi(this);

    //inicijalizacija LCD
	lcd_h = lcdInit(2, 16, 4, RS, EN, D0, D1, D2, D3, D0, D1, D2, D3);

	if (lcd_h < 0) {
		fprintf (stderr, "lcdInit failed\n") ;
	}

    //karakteri za LCD
    uchar vol[8] = {0b00001, 0b00011, 0b11011, 0b11011, 0b11011, 0b00011, 0b00001}; //karakter za jacinu zvuka
    uchar pse[8] = {0b11011, 0b11011, 0b11011, 0b11011, 0b11011, 0b11011, 0b11011}; //karakter za pauzu
	lcdCharDef(lcd_h, 0, vol);
    lcdCharDef(lcd_h, 1, pse);

    //podesavanje pocetnog prikaza na LCD
    lcdClear(lcd_h);
    lcdPosition(lcd_h,0,0);
    lcdPutchar(lcd_h,0xFF);
    lcdPutchar(lcd_h,0xFF);
    lcdPrintf(lcd_h,"Zaustavljeno");
    for(int i=0; i<18; i++) {
        lcdPutchar(lcd_h,0xFF);
    }

    //inicijalizacija clanova klase
	scrollCounter = 0;
    lcdMode = 0;
    usbFlag = 1;
    refreshFlag = 0;
    previousIndex = 0;
    sampleRate = 44100;
    color = tr("blue");
	sample.resize(SPECSIZE);
    calculator = new FFTCalc(this);
    probe = new QAudioProbe(this);
	qRegisterMetaType< QVector<double> >("QVector<double>");
    myHLayout = new QHBoxLayout();
    myBox = new QMessageBox(QMessageBox::Critical, tr("Greška"), tr("Ubacite USB Flash"), QMessageBox::NoButton, this);
    myBox2 = new QMessageBox(QMessageBox::Critical, tr("Greška"), tr("Nema mp3 fajlova na USB Flash-u"), QMessageBox::NoButton, this);
    //myBox->show();
    barsCount = 0;
    octaves = 3;
    myPlayer = new QMediaPlayer(this);
    myLirc = new Lirc(this); //klasa Lirc implementirana u lirc.cpp

    //dodavanje layout-a u GUI, gridLayout_2 postaje parent layout-u clanu klase
    ui->gridLayout_2->addLayout(myHLayout,1,0,Qt::AlignCenter);

    //povezivanje odgovarajucih signala i slotova
    connect(probe, SIGNAL(audioBufferProbed(QAudioBuffer)),this, SLOT(processBuffer(QAudioBuffer)));
    connect(calculator, SIGNAL(calculatedSpectrum(QVector<double>)),this, SLOT(loadSamples(QVector<double>)));
    connect(myLirc,SIGNAL(key_event(QString)),this,SLOT(handleKey(QString)));
    connect(this,SIGNAL(hw_btn_clicked(int)),this,SLOT(onHwBtnClicked(int)));

    //deklarisanje i inicijalizacija tajmera na 1 sekundu, za azuriranje LCD
	QTimer *myTimer = new QTimer(this);
    connect(myTimer, SIGNAL(timeout()), this, SLOT(onEverySecond()));
	myTimer->start(1000);

    //ucitavanje plejliste
    ui->tableWidget->setColumnCount(3);
    this->loadPlaylist();

    //povezivanje odgovarajucih signala i slotova za plejer
    connect(myPlayer,SIGNAL(volumeChanged(int)),ui->progressBar,SLOT(setValue(int)));
    connect(myPlayer,SIGNAL(durationChanged(qint64)),this,SLOT(onDurationChanged(qint64)));
    connect(myPlayer,SIGNAL(currentMediaChanged(QMediaContent)),this,SLOT(onSongChanged(QMediaContent)));
    connect(myPlayer,SIGNAL(positionChanged(qint64)),this,SLOT(onPositionChanged(qint64)));
    connect(myPlayer,SIGNAL(stateChanged(QMediaPlayer::State)),this,SLOT(onPlayerStateChanged(QMediaPlayer::State)));
    probe->setSource(myPlayer);

    //inicijalizacija elemenata GUI-a
    ui->label->setText(tr("0:00"));
    ui->label_2->setText(tr("0:00"));
    ui->label_4->setFont(QFont("Courier New",14));
    ui->label->setFont(QFont("Courier New",14));
    ui->label_2->setFont(QFont("Courier New",14));
    ui->progressBar->setValue(100);

    ui->listWidget->addItem(tr("Plejer je zaustavljen"));
    ui->listWidget->setCurrentRow(0);
    ui->listWidget->setFont(QFont("Courier New",14));
    ui->listWidget->horizontalScrollBar()->setStyleSheet(tr("width:0px;"));
    ui->listWidget->setFixedHeight(ui->listWidget->sizeHintForRow(0));

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

    //podesi pocetnu temu
    this->updateStyleSheets();

    // Tajmer za provjeru da li je USB Flash dostupan
    QTimer *myTimer2 = new QTimer(this);
    connect(myTimer2,SIGNAL(timeout()),this,SLOT(timerSlot()));
    myTimer2->start(100);
}

Dialog::~Dialog()
{
	lcdClear(lcd_h);
	delete ui;
}

// ovde se provjerava da li je USB Flash tu, ako nije prikazuje se message box
void Dialog::timerSlot() {
    if(usbPath() == NULL) {
        if(usbFlag) {
            if(myBox2->isVisible()) {
                myBox2->accept();
            }
            usbFlag = 0;
            myPlayer->stop();
            refreshFlag = 1;
            lcdClear(lcd_h);
            lcdPosition(lcd_h,0,0);
            lcdPutchar(lcd_h,0xFF);
            lcdPrintf(lcd_h,"Nedostupan USB");
            for(int i=0; i<17; i++) {
                lcdPutchar(lcd_h,0xFF);
            }
            myBox->show();
            QCoreApplication::processEvents();
        }
    }
    else {
        if(!usbFlag) {
            this->loadPlaylist();
            myBox->accept();
            usbFlag = 1;
            myPlayer->setPosition(0);
            previousIndex = 0;
            refreshFlag = 0;
        }
    }
}

//preuzimanje putanja fajlova sa USB-a i popunjavanje plejliste u GUI-u
void Dialog::loadPlaylist() {
    int count = 0;
    if(ui->tableWidget->rowCount()) {
        ui->tableWidget->setRowCount(0);
    }
    QString directory = QString::fromUtf8(usbPath());//usbPath() je funkcija iz getpath.cpp
    QDir dir(directory);
    QDirIterator it(dir.path(), QStringList() << "*.mp3", QDir::Files, QDirIterator::Subdirectories);
    QList<QMediaContent> content;
    QString f;
    if(it.hasNext()) {
        if(myBox2->isVisible()) {
            myBox2->accept();
        }

        while(it.hasNext())
        {
            f = it.next();
            content.push_back(QUrl::fromLocalFile(f));
            TagLib::FileRef fil(f.toLatin1().data());
            ui->tableWidget->insertRow(count);
            ui->tableWidget->setItem(count,0,new QTableWidgetItem(QString::number(count+1) + '.'));
            ui->tableWidget->setItem(count,1,new QTableWidgetItem(it.fileName()));
            ui->tableWidget->setItem(count,2,new QTableWidgetItem(QString::number(fil.audioProperties()->length() / 60) + ':' +
                        QString::number(fil.audioProperties()->length() % 60).rightJustified(2,'0')));
            count++;
        }

        //postavljanje plejliste za plejer i povezivanje odgovarajucih signala i slotova
        QMediaPlaylist *myPlaylist = new QMediaPlaylist(this);
        myPlaylist->addMedia(content);
        myPlayer->setPlaylist(myPlaylist);

        //ovde se postavlja bold italic font(pa se kasnije vraca u standardni), posto ce u tom slucaju
        //biti najsire kolone, pa da se prema njemu automatski podesi sirina
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

        // inicijalizacija plejliste
        myPlayer->playlist()->setCurrentIndex(0);
    }
    else {
        //ako nema fajlova, izbaci gresku
        lcdClear(lcd_h);
        lcdPosition(lcd_h,0,0);
        lcdPutchar(lcd_h,0xFF);
        lcdPutchar(lcd_h,0xFF);
        lcdPrintf(lcd_h,"Nema fajlova");
        for(int i=0; i<18; i++) {
            lcdPutchar(lcd_h,0xFF);
        }
        myBox2->show();
        QCoreApplication::processEvents();
    }
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(0,QHeaderView::Fixed);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(1,QHeaderView::Stretch);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(2,QHeaderView::Fixed);
    ui->tableWidget->setCurrentCell(0,1);

}

//duration je trajanje pjesme u milisekundama
void Dialog::onDurationChanged(qint64 duration) {
    ui->horizontalSlider->setMaximum(duration);
}

//pos je vrijeme proteklo od pocetka pjesme u milisekundama
void Dialog::onPositionChanged(qint64 pos) {
    ui->horizontalSlider->setValue(pos);
    pos /= 1000;
    ui->label_2->setText((QString::number(pos / 60) + ':' +
                          QString::number(pos % 60).rightJustified(2,'0')));
}

//kada je stanje stopped, vrati sve na pocetne vrijednosti
void Dialog::onPlayerStateChanged(QMediaPlayer::State newState) {
    QFont f;
    if(newState == QMediaPlayer::StoppedState) {
        for(int i=0; i<barsCount; i++) {
            arr[i]->setValue(MINBAR);
        }

        f = ui->tableWidget->currentItem()->font();
        f.setItalic(false);
        f.setBold(false);

        if(myPlayer->playlist()->currentIndex() != -1) {
            for(int i=0; i<ui->tableWidget->columnCount(); i++) {
                ui->tableWidget->item(myPlayer->playlist()->currentIndex(),i)->setFont(f);
            }
        }

        ui->label->setText(tr("0:00"));
        ui->label_2->setText(tr("0:00"));
        ui->listWidget->currentItem()->setText(tr("Plejer je zaustavljen"));

        for (int i=0; i<6; i++) {
            ui->tableWidget_2->item(i,1)->setText(tr("N/A"));
        }
    }
}

//kada se pjesma promijeni, azuriraj sve potrebne elemente interfejsa
void Dialog::onSongChanged(QMediaContent song) {
    QFont f;
    //kod nekih rezima rada currentIndex bude -1 u odredjenim situacijama, pa samo zaustavi sve u tom slucaju

    if(!refreshFlag) {
        if(myPlayer->playlist()->currentIndex() >= 0) {
            int current = myPlayer->playlist()->currentIndex();

            //podesi font bold italic za onu pjesmu koja trenutno ide, a vrati font na standardni za prethodnu
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

            //progres barove koji prikazuju spektar vrati na minimum
            for(int i=0; i<barsCount; i++) {
                arr[i]->setValue(arr[i]->minimum());
            }

            //popuni tabelu sa informacijama o tekucoj pjesmi
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
            //vrati font i stopiraj plejer, slot onStateChanged ce onda odraditi ostalo
            f = ui->tableWidget->item(0,1)->font();
            f.setBold(false);
            f.setItalic(false);
            for(int i=0; i<ui->tableWidget->columnCount(); i++) {
                ui->tableWidget->item(previousIndex,i)->setFont(f);
            }
            myPlayer->stop();
        }
    }
}

//prepoznavanje koda sa daljinskog
void Dialog::handleKey(const QString& key) {
    if(!myBox->isVisible() && !myBox2->isVisible()) {
        if(key == tr("KEY_PLAY")) {
            //ovo dugme ima funkciju play/pause
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
            //dugme stop
            myPlayer->stop();
        }
        else if(key == tr("KEY_NEXT")) {
            //dugme next
            scrollCounter = 0;
            lcdClear(lcd_h);
            if((myPlayer->playlist()->playbackMode()==QMediaPlaylist::CurrentItemOnce) ||
            (myPlayer->playlist()->playbackMode()==QMediaPlaylist::CurrentItemInLoop)) {
                //za odredjene rezime rada po default-u
                //se ne ide uobicajeno na sledecu pjesmu, pa ispravi to
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
            //dugme previous
            scrollCounter = 0;
            lcdClear(lcd_h);
            if((myPlayer->playlist()->playbackMode()==QMediaPlaylist::CurrentItemOnce) ||
            (myPlayer->playlist()->playbackMode()==QMediaPlaylist::CurrentItemInLoop)) {
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
            //dugme za biranje rezima rada, povezi sa button-ima na GUI-u i setuj sta treba
            if(ui->radioButton_4->isChecked()) {
                ui->radioButton_5->setChecked(true);
                myPlayer->playlist()->setPlaybackMode(QMediaPlaylist::Loop);
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
                myPlayer->playlist()->setPlaybackMode(QMediaPlaylist::Sequential);
            }
        }
        else if(key == tr("KEY_CH+")) {
            //dugme za biranje kolor seme, povezi sa GUI-em i pozovi odgovarajuce funkcije
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
            //dugme volume up
            myPlayer->setVolume(myPlayer->volume() + 5);
        }
        else if(key == tr("KEY_DOWN")) {
            //dugme volume down
            myPlayer->setVolume(myPlayer->volume() - 5);
        }
        else if(key == tr("KEY_EQ")) {
            //dugme za biranje prikaza spektra, setuj promjenljivu, thread ce odraditi svoje
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
            //dugme up, za kretanje po plejlisti
            ui->tableWidget->setCurrentCell((ui->tableWidget->currentRow()+1)%ui->tableWidget->rowCount(),1);
        }
        else if(key == tr("KEY_2")) {
            //dugme down, za kretanje po plejlisti
            if(ui->tableWidget->currentRow()) {
                ui->tableWidget->setCurrentCell(ui->tableWidget->currentRow()-1,1);
            }
            else {
                ui->tableWidget->setCurrentCell(ui->tableWidget->rowCount()-1,1);
            }
        }
        else if(key == tr("KEY_100+")) {
            //dugme za izlazak iz programa
            myPlayer->stop();
            lcdClear(lcd_h);
            delete ui;
            exit(0);
        }
    }
}

//funkcija za azuriranje boja
void Dialog::updateStyleSheets() {
    //ovi sa set property koriste dynamic properties opciju u qt-u(brza varijanta)
    //stylesheet-ovi su definisani u main-u u zavisnosti od property-ja za citavu aplikaciju
    this->setProperty("colorScheme",color);
    ui->groupBox->setProperty("colorScheme",color);
    ui->groupBox_2->setProperty("colorScheme",color);
    ui->groupBox_3->setProperty("colorScheme",color);
    ui->tableWidget->setProperty("colorScheme",color);

    //moraju da se refresh-uju
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

    //oni sto nisu mogli preko dynamic property idu rucno, podesavanjem stylesheet-a ovde(sporija varijanta)
    if(color == tr("blue")) {
        for(int i=0; i<ui->tableWidget->rowCount(); i++) {
            for(int j=0; j<ui->tableWidget->columnCount(); j++) {
                ui->tableWidget->item(i,j)->setBackgroundColor(QColor(193, 224, 242));
            }
        }
        for(int i=0; i<ui->tableWidget_2->rowCount(); i++) {
            for(int j=0; j<ui->tableWidget_2->columnCount(); j++) {
                ui->tableWidget_2->item(i,j)->setBackgroundColor(QColor(193, 224, 242));
            }
        }
        for(int i=0; i<barsCount; i++) {
            arr[i]->setStyleSheet(tr("background-color: rgb(193, 224, 242); selection-background-color: rgb(51, 151, 213);"));
        }
        ui->progressBar->setStyleSheet(tr("background-color: skyblue; selection-background-color: rgb(51, 151, 213);"));
        ui->horizontalSlider->setStyleSheet(tr("selection-background-color: rgb(51, 151, 213);"));
        ui->listWidget->setStyleSheet(tr("selection-background-color:blue"));
    }
    else if(color == tr("green")) {
        for(int i=0; i<ui->tableWidget->rowCount(); i++) {
            for(int j=0; j<ui->tableWidget->columnCount(); j++) {
                ui->tableWidget->item(i,j)->setBackgroundColor(QColor(201, 255, 152));
            }
        }
        for(int i=0; i<ui->tableWidget_2->rowCount(); i++) {
            for(int j=0; j<ui->tableWidget_2->columnCount(); j++) {
                ui->tableWidget_2->item(i,j)->setBackgroundColor(QColor(201, 255, 152));
            }
        }
        for(int i=0; i<barsCount; i++) {
            arr[i]->setStyleSheet(tr("background-color: rgb(201, 255, 152); selection-background-color: rgb(0, 170, 43);"));
        }
        ui->progressBar->setStyleSheet(tr("background-color: rgb(201, 255, 152); selection-background-color: rgb(0, 170, 43);"));
        ui->horizontalSlider->setStyleSheet(tr("selection-background-color: rgb(0, 170, 43);"));
        ui->listWidget->setStyleSheet(tr("selection-background-color:green"));
    }
    else {
        for(int i=0; i<ui->tableWidget->rowCount(); i++) {
            for(int j=0; j<ui->tableWidget->columnCount(); j++) {
                ui->tableWidget->item(i,j)->setBackgroundColor(QColor(252, 167, 170));
            }
        }
        for(int i=0; i<ui->tableWidget_2->rowCount(); i++) {
            for(int j=0; j<ui->tableWidget_2->columnCount(); j++) {
                ui->tableWidget_2->item(i,j)->setBackgroundColor(QColor(252, 167, 170));
            }
        }
        for(int i=0; i<barsCount; i++) {
            arr[i]->setStyleSheet(tr("background-color: rgb(252, 167, 170); selection-background-color: rgb(255, 52, 48);"));
        }
        ui->progressBar->setStyleSheet(tr("background-color: rgb(252, 167, 170); selection-background-color: rgb(255, 52, 48);"));
        ui->horizontalSlider->setStyleSheet(tr("selection-background-color: rgb(255, 52, 48)"));
        ui->listWidget->setStyleSheet(tr("selection-background-color:red"));
    }
}

//prepoznavanje tastera sa DVK i zadavanje odgovarajucih komandi plejeru
void Dialog::onHwBtnClicked(int btn) {
    if(!myBox->isVisible() && !myBox2->isVisible()) {
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
                    if((myPlayer->playlist()->playbackMode()==QMediaPlaylist::CurrentItemOnce) ||
                    (myPlayer->playlist()->playbackMode()==QMediaPlaylist::CurrentItemInLoop)) {
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
                break;
            case BTN_4: //next(mode=0) / vol_up(mode=1)
                if(lcdMode) {
                    myPlayer->setVolume(myPlayer->volume() + 5);
                }
                else {
                    scrollCounter = 0;
                    lcdClear(lcd_h);
                    if((myPlayer->playlist()->playbackMode()==QMediaPlaylist::CurrentItemOnce) ||
                    (myPlayer->playlist()->playbackMode()==QMediaPlaylist::CurrentItemInLoop)) {
                        //za odredjene rezime rada po default-u
                        //se ne ide uobicajeno na sledecu pjesmu, pa ispravi to
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
                break;
        }
    }
}

//funkcija koja se izvrsava na svaki tik tajmera
//regulise prikaz na LCD, kao i automatsko skrolovanje teksta tekuce pjesme u GUI-u
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
            lcdPrintf(lcd_h,"Podesi zvuk");
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
        if(!myBox->isVisible() && !myBox2->isVisible()) {
            lcdClear(lcd_h);
            lcdPosition(lcd_h,0,0);
            lcdPutchar(lcd_h,0xFF);
            lcdPutchar(lcd_h,0xFF);
            lcdPrintf(lcd_h,"Zaustavljeno");
            for(int i=0; i<18; i++) {
                lcdPutchar(lcd_h,0xFF);
            }
        }
	}
}

//obradi bafer koji qaudioprobe posalje sa stream-a
//ovo su dekodovani semplovi
void Dialog::processBuffer(QAudioBuffer buffer){
	qreal peakValue;
	int duration;

	if(buffer.frameCount() < 512)
		return;

    //ako nije stereo, onda nista
	if(buffer.format().channelCount() != 2)
		return;

	sample.resize(buffer.frameCount());

    //u zavisnosti od tipa podataka (signed int, unsigned int ili float), podesi peak value,
    //nadji aritmeticku sredinu oba kanala i normalizuj na [0,1]
	if(buffer.format().sampleType() == QAudioFormat::SignedInt){
		QAudioBuffer::S16S *data = buffer.data<QAudioBuffer::S16S>();
		// peak value changes according to sample size.
		if (buffer.format().sampleSize() == 32)
			peakValue=INT_MAX;
		else if (buffer.format().sampleSize() == 16)
			peakValue=SHRT_MAX;
		else
			peakValue=CHAR_MAX;

		for(int i=0; i<buffer.frameCount(); i++){
            sample[i] = (data[i].left + data[i].right)/2/peakValue;
		}
    }
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
	else if(buffer.format().sampleType() == QAudioFormat::Float){
		QAudioBuffer::S32F *data = buffer.data<QAudioBuffer::S32F>();
		//peakValue = 1.00003;
		peakValue = 1.0;
		for(int i=0; i<buffer.frameCount(); i++){
			sample[i] = (data[i].left + data[i].right)/2/peakValue;
            //nekad se desi da bude beskonacno, ovako moze da se provjeri
			if(sample[i] != sample[i]){
				sample[i] = 0;
            }
		}
	}

    //ako je probe jos aktivan, racunaj fft
    if(probe->isActive()){
		duration = buffer.format().durationForBytes(buffer.frameCount())/1000;
        calculator->calc(sample, duration, octaves, sampleRate); //posalji podatke u thread gdje ce se racunati fft
        //pogledati fftcalc.cpp za detalje
    }
}

//obrada spektra dobijenog iz thread-a
//glavni dio je tamo odradjen, ovde se samo azuriraju progres bar-ovi za GUI
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

