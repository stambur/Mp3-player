#include "fftcalc.h"

// fftcalc class is designed to treat with fft calculations
FFTCalc::FFTCalc(QObject *parent)
	:QObject(parent){

        //inicijalizuj thread za fft i povezi odgovarajuce signale
		processor.moveToThread(&processorThread);

        //ovo mora da se uradi da bi QVector<double> moglo da se koristi kao parametar za signale i slotove
        qRegisterMetaType< QVector<double> >("QVector<double>");

        connect(&processor, SIGNAL(calculatedSpectrum(QVector<double>)),this, SLOT(setSpectrum(QVector<double>)));
        connect(&processor, SIGNAL(allDone()), this, SLOT(freeCalc()));
		processorThread.start(QThread::LowestPriority);

		isBusy = false;
	}

FFTCalc::~FFTCalc(){
	processorThread.quit();
    //cekanje da se zavrsi thread, 10ms je dovoljno
	processorThread.wait(10000);
}

//ova funkcija se poziva iz dialog.cpp
void FFTCalc::calc(QVector<double> &_array, int duration, int _octaves, int _sampleRate){
    //ako je procesor zauzet, vrati se
	if(isBusy)
		return;

	isBusy = true;

    //pozovi metodu processBuffer objekta processor i proslijedi joj sve argumente
	QMetaObject::invokeMethod(&processor, "processBuffer",
			Qt::QueuedConnection, Q_ARG(QVector<double>, _array),
            Q_ARG(int, duration), Q_ARG(int, _octaves),
            Q_ARG(int, _sampleRate));
}

void FFTCalc::setSpectrum(QVector<double> spectrum){
    //posalji spektar u dialog.cpp
	emit calculatedSpectrum(spectrum);
}

void FFTCalc::freeCalc()
{
    //procesor je slobodan za dalje racunanje
	isBusy = false;
}

//konstruktor BufferProcessor-a
BufferProcessor::BufferProcessor(QObject *parent){
    Q_UNUSED(parent);

    //napravi tajmer i povezi ga sa odgovarajucim slotom
	timer = new QTimer(this);
	connect(timer,SIGNAL(timeout()),this,SLOT(run()));

    //podesi duzine odgovarajucih vektora, SPECSIZE je broj tacaka u kojima se racuna fft
	window.resize(SPECSIZE);
    complexFrame.resize(SPECSIZE);

    //zbog simetrije ce duzina spektra biti duplo manja
	spectrum.resize(SPECSIZE/2);

    // prozorska funkcija
	for(int i=0; i<SPECSIZE;i++){
        window[i] = 0.5 * (1 - cos((2*PI*i)/(SPECSIZE-1))); //HANN
//        window[i] = 1 - 1.93 * cos((2*PI*i)/(SPECSIZE-1)) + 1.29 * cos((4*PI*i)/(SPECSIZE-1)) -
//                0.388 * cos((6*PI*i)/(SPECSIZE-1)) + 0.028 * cos((8*PI*i)/(SPECSIZE-1)); //FLAT TOP
	}


    running = false;
}

BufferProcessor::~BufferProcessor(){
	timer->stop();

}

//pripremi sve za racunanje fft
void BufferProcessor::processBuffer(QVector<double> _array, int duration, int _octaves, int _sampleRate){
    if(array.size() != _array.size()){
        if(_array.size() >= SPECSIZE) {
            //ako pristigli niz odbiraka ima duzinu vecu od SPECSIZE, racunacemo kasnije fft iz nekoliko dijelova
            //prepisi pristigli niz u lokalni i nadji broj dijelova(chunk-ova) iz kojih ce se racunati fft
            chunks = _array.size()/SPECSIZE;
            array.resize(_array.size());
            array = _array;
        }
        else {
            //ako pristigli niz odbiraka ima manju duzinu od SPECSIZE, napuni ga nulama do SPECSIZE
            //ovako se dobija spektar sa manje curenja
            chunks = 1;
            array.resize(SPECSIZE);
            array.fill(0);
            for(int i=0; i<_array.size(); i++) {
                array[i] = _array[i];
            }
        }
	}
    else {
        chunks = _array.size()/SPECSIZE;
        array = _array;
    }

    //interval vremena za koji se racuna jedan dio fft
	interval = duration/chunks;

    // kopiraj pristigle parametre
    octaves = _octaves;
    sampleRate = _sampleRate;

    // inicijalizuj brojac na nulu
	pass = 0;

	if(interval < 1)
		interval = 1;

    //zapocni tajmer, metoda run() ce se izvrsavati svakih interval sekundi
    timer->start(interval);
}

//ovde se racuna fft i obradjuje se spektar
void BufferProcessor::run(){
    qreal amplitude;

    //ako je izracunato za sve dijelove, salji signal da je zavrseno
	if(pass == chunks){
		emit allDone();
		return;
	}

    //izvedi "prozoriranje" i pripremi odgovarajucu strukturu za poziv fft
	for(uint i=0; i<SPECSIZE; i++){
		complexFrame[i] = Complex(window[i]*array[i+pass*SPECSIZE],0);
	}

    //uradi fft
	fft(complexFrame);

    //skaliraj dobijene rezultate, sacuvaj dobijeno u realni dio complexFrame-a da ne bismo pravili novi niz za to
    for(uint i=0; i<SPECSIZE/2;i++){
        amplitude = 2*std::abs(complexFrame[i])/SPECSIZE;
		complexFrame[i] = amplitude;
	}

    //priprema za obradu spektra, racunanje frekvencijske rezolucije
    //odbirci ce da se podijele u odredjene opsege, siroke 1/3, 1/2 ili 1 oktavu, pa se trazi
    //aritmeticka sredina tih odbiraka, a citav opseg se predstavlja nekom centralnom frekvencijom
    //ovaj princip sam vidio da se koristi cesto
    spectrum.resize(50);
    //qDebug() << "Sample rate ="<<sampleRate;
    double frequencies[SPECSIZE/2];
    double resolution = (sampleRate/2)/(double)(SPECSIZE/2);
    double sum = 0;
    int cnt = 0;
    int i = 0;
    double central = 15.625;
    double lower,upper;

    //napravi niz cije vrijednosti odgovaraju frekvencijama na i-tom odbirku spektra
    //qDebug() << "Resolution ="<<resolution;
    for(int i=0; i<SPECSIZE/2; i++) {
        sum = i*resolution;
        frequencies[i] = sum;
        //qDebug() << "Frequency "<< QString::number(i) << ": "<< QString::number(sum,'f',2);
    }

    //nadji pocetne granice za opsege u kojima cemo sabirati odbirke
    upper = central*powf(2,(float)1/(2*octaves));
    lower = central/powf(2,(float)1/(2*octaves));

    while((upper < frequencies[1]) || (upper<20)) {
        central = central*powf(2,(float)1/octaves);
        upper = central*powf(2,(float)1/(2*octaves));
        lower = central/powf(2,(float)1/(2*octaves));
    }

    while(lower < 20000) {
        sum = 0;
        cnt = 0;
        //skupi odbirke koji upadaju u opseg i nadji aritmeticku sredinu
        //qDebug() << "Lower: " << QString::number(lower) << " and upper: " << QString::number(upper);
        for(int j=0; j<SPECSIZE/2; j++) {
            if((frequencies[j] >= lower) && (frequencies[j] <= upper)) {
//                qDebug() << "Sample "<< j << "=" << QString::number(complexFrame[j].real()) << "is between " <<
//                        QString::number(lower) << " and " << QString::number(upper);
                sum += complexFrame[j].real();
                cnt++;
            }
        }
        sum /= cnt;

        //nadji vrijednosti za sledeci opseg
        //qDebug() << "Central: " << QString::number(central) << "Lower: " << QString::number(lower);
        central = central*powf(2,(float)1/octaves);
        upper = central*powf(2,(float)1/(2*octaves));
        lower = central/powf(2,(float)1/(2*octaves));

        //prebaci u decibele, ovo bude otprilike od -70 do 0 db
        spectrum[i] = 10*log10f(sum);

        //qDebug() << "Sample " << QString::number(i) << " in db: " << QString::number(spectrum[i]);
        i++;
    }

    //spektar ce sada imati tacno onoliko clanova koliko ima opsega, i u GUI-u ce biti toliki broj progress bar-ova
    spectrum.resize(i);
		//qDebug() << "Max = " << QString::number(max);


    //posalji spektar
	emit calculatedSpectrum(spectrum);

    //qDebug() << "Emitovao novi spektar, octaves = "<<octaves;
	pass++;
	}


