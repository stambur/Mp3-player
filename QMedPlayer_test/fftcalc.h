#ifndef FFTCALC_H
#define FFTCALC_H

#include <QThread>
#include <QWaitCondition>
#include <QMutex>
#include <QVector>
#include <QDebug>
#include <QTimer>
#include <QObject>
#include "fft.h"

//broj tacaka u kojim trazimo fft
#define SPECSIZE 2048

class BufferProcessor: public QObject
{
    Q_OBJECT
  QVector<double> array;
  QVector<double> window;
  QVector<double> spectrum;
  QTimer *timer;
  bool running, iscalc;
  int chunks, interval, pass, octaves;
  int sampleRate;
  CArray complexFrame;
public slots:
    void processBuffer(QVector<double> _array, int duration, int _octaves, int _sampleRate);
signals:
    void calculatedSpectrum(QVector<double> spectrum);
    void allDone(void);
protected slots:
    void run();
public:
    explicit BufferProcessor(QObject *parent=0);
    ~BufferProcessor();
};

// fftcalc runs in a separate thread
class FFTCalc : public QObject{
    Q_OBJECT
private:
  bool isBusy;
  BufferProcessor processor;
  QThread processorThread;

public:
  explicit FFTCalc(QObject *parent = 0);
  ~FFTCalc();
  void calc(QVector<double> &_array, int duration, int _octaves, int _sampleRate);
public slots:
  void setSpectrum(QVector<double> spectrum);
  void freeCalc();
signals:
  void calculatedSpectrum(QVector<double> spectrum);
};

#endif // FFTCALC_H
