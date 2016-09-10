#include "fftcalc.h"

#undef CLAMP
#define CLAMP(a,min,max) ((a) < (min) ? (min) : (a) > (max) ? (max) : (a))

// fftcalc class is designed to treat with fft calculations
FFTCalc::FFTCalc(QObject *parent)
	:QObject(parent){

		// fftcalc is done in other thread
		// so it cannot overload the main thread
		processor.moveToThread(&processorThread);

		// qRegisterMetaType is used to register QVector<double> as the typename for QVector<double>
		// it is necessary for signal/slots events treatment.
		qRegisterMetaType< QVector<double> >("QVector<double>");

		// when the spectrum is calculated, setSpectrum function will comunicate
		// such spectrum to Qt as an emitted signal
		connect(&processor, SIGNAL(calculatedSpectrum(QVector<double>)), SLOT(setSpectrum(QVector<double>)));

		// when the processor finishes the calculation, the busy condition is changed.
		connect(&processor, SIGNAL(allDone()),SLOT(freeCalc()));

		// start the processor thread with low priority
		processorThread.start(QThread::LowestPriority);

		// initially, the processor is not occupied
		isBusy = false;
	}

FFTCalc::~FFTCalc(){
	// tells the processor thread to quit
	processorThread.quit();
	// wait a bit until it finishes. I guess 10ms is enough!
	processorThread.wait(10000);
}

void FFTCalc::calc(QVector<double> &_array, int duration, int _octaves, int _sampleRate){
	if(isBusy)
		return;

	// processor is busy performing fft calculations...
	isBusy = true;

	// start the calcs...
	QMetaObject::invokeMethod(&processor, "processBuffer",
			Qt::QueuedConnection, Q_ARG(QVector<double>, _array),
            Q_ARG(int, duration), Q_ARG(int, _octaves),
            Q_ARG(int, _sampleRate));
}

void FFTCalc::setSpectrum(QVector<double> spectrum){
	// tells Qt about that a new spectrum has arrived
	emit calculatedSpectrum(spectrum);
}

void FFTCalc::freeCalc()
{
	// fftcalc is ready for new spectrum calculations
	isBusy = false;
}
/*
 * processes the buffer for fft calculation
 */

BufferProcessor::BufferProcessor(QObject *parent){
	// the pointer is not used here
	Q_UNUSED(parent);

	// this timer is used to send the pieces of spectrum
	// as they are calculated
	timer = new QTimer(this);

	// call run() to send such pieces
	connect(timer,SIGNAL(timeout()),this,SLOT(run()));

	// window functions are used to filter some undesired
	// information for fft calculation.
	window.resize(SPECSIZE);

	// the complex frame that is sent to fft function
	complexFrame.resize(SPECSIZE);

	// only half spectrum is used because of the simetry property
	spectrum.resize(SPECSIZE/2);

	// logscale is used for audio spectrum display
	logscale.resize(SPECSIZE/2+1);

	// by default, spectrum is log scaled (compressed)
    //compressed = false;

	// window function (HANN)
	for(int i=0; i<SPECSIZE;i++){
        window[i] = 0.5 * (1 - cos((2*PI*i)/(SPECSIZE-1))); //HANN
//        window[i] = 1 - 1.93 * cos((2*PI*i)/(SPECSIZE-1)) + 1.29 * cos((4*PI*i)/(SPECSIZE-1)) -
//                0.388 * cos((6*PI*i)/(SPECSIZE-1)) + 0.028 * cos((8*PI*i)/(SPECSIZE-1)); //FLAT TOP
        //qDebug() << "Pendzer od"<<i<<"="<<window[i];
	}

	// the log scale
//	for(int i=0; i<=SPECSIZE/2; i++){
//		logscale[i] = powf (SPECSIZE/2, (float) 2*i / SPECSIZE) - 0.5f;
//	}

	// nothing is running yet
    running = false;
	// process buffer each 100ms (initially, of course)
    //timer->start(100);
}

BufferProcessor::~BufferProcessor(){
	timer->stop();

}

void BufferProcessor::processBuffer(QVector<double> _array, int duration, int _octaves, int _sampleRate){
	// if the music is new, array size may change
    //qDebug() << "Poceo da procesira bafer";
    //qDebug() << "Velicina bafera: " << _array.size();
	if(array.size() != _array.size()){
        if(_array.size() >= SPECSIZE) {
            //array is splitted into a set of small chunks
            chunks = _array.size()/SPECSIZE;

            // resize the array to the new array size
            array.resize(_array.size());
            array = _array;
        }
        else {
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
	// interval of notification depends on the duration of the sample
	interval = duration/chunks;

	// copy the array
    //array = _array;
    octaves = _octaves;
    sampleRate = _sampleRate;

	// count the number of fft calculations until the chunk size
	// is reached
	pass = 0;

	// it cannot be so small
	if(interval < 1)
		interval = 1;

	// redefines the timer interval
	timer->start(interval);
    //qDebug() << "Zavrsio procesiranje bafera, octave = "<<octaves;
}

void BufferProcessor::run(){
	// spectrum amplitude
	qreal amplitude;
    //qreal SpectrumAnalyserMultiplier = 1e-2;

	// tells when all chunks has been processed
	if(pass == chunks){
		emit allDone();
		return;
	}

	// we do not calc spectra when array is too small
	if(array.size() < SPECSIZE){
		return;
	}

	// prepare complex frame for fft calculations
	for(uint i=0; i<SPECSIZE; i++){
		complexFrame[i] = Complex(window[i]*array[i+pass*SPECSIZE],0);
	}

	// do the magic
	fft(complexFrame);

	// some scaling/windowing is needed for displaying the fourier spectrum somewhere
	for(uint i=0; i<SPECSIZE/2;i++){
		//    amplitude = SpectrumAnalyserMultiplier*std::abs(complexFrame[i]);
		//    amplitude = qMax(qreal(0.0), amplitude);
		//    amplitude = qMin(qreal(1.0), amplitude);
		//    complexFrame[i] = amplitude;
		amplitude = 2*std::abs(complexFrame[i])/SPECSIZE;
        //qDebug() << "Sample " << i << " after fft: "<< amplitude;
		complexFrame[i] = amplitude;
	}

	// audio spectrum is usually compressed for better displaying

    // if not compressed, just copy the real part clamped between 0 and 1
    //    for(int i=0; i<SPECSIZE/2; i++){
    //      spectrum[i] = CLAMP(complexFrame[i].real()*100,0,1);
    //
    spectrum.resize(50);
    //qDebug() << "Sample rate ="<<sampleRate;
    double frequencies[SPECSIZE/2];
    double resolution = (sampleRate/2)/(double)(SPECSIZE/2);
    double sum = 0;
    int cnt = 0;
    int i = 0;
    double central = 15.625;
    double lower,upper;
    //static double max = 300;
    //qDebug() << "Resolution ="<<resolution;
    for(int i=0; i<SPECSIZE/2; i++) {
        sum = i*resolution;
        //qDebug() << "Sum of"<<i<<"is"<<sum;
        frequencies[i] = sum;
        //qDebug() << "Frequency "<< QString::number(i) << ": "<< QString::number(sum,'f',2);
    }

    upper = central*powf(2,(float)1/(2*octaves));
    lower = central/powf(2,(float)1/(2*octaves));

    //qDebug() << lower;
    //qDebug() << upper;

    while((upper < frequencies[1]) || (upper<20)) {// || (upper<20)
        central = central*powf(2,(float)1/octaves);
        upper = central*powf(2,(float)1/(2*octaves));
        lower = central/powf(2,(float)1/(2*octaves));
    }

    //for(int i=8; i>=0; i--) {
    while(lower < 20000) {
        sum = 0;
        cnt = 0;
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
        //qDebug() << "Central: " << QString::number(central) << "Lower: " << QString::number(lower);
        central = central*powf(2,(float)1/octaves);
        upper = central*powf(2,(float)1/(2*octaves));
        lower = central/powf(2,(float)1/(2*octaves));

        spectrum[i] = 10*log10f(sum);

        //qDebug() << "Sample " << QString::number(i) << " in db: " << QString::number(spectrum[i]);
        i++;
    }
    spectrum.resize(i);
		//qDebug() << "Max = " << QString::number(max);


	// emit the spectrum
	emit calculatedSpectrum(spectrum);

    //qDebug() << "Emitovao novi spektar, octaves = "<<octaves;
	// count the pass
	pass++;
	}


