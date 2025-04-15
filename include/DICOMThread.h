#pragma once

#include <QObject>
#include <QString>
#include <vtkSmartPointer.h>
#include <vtkImageData.h>
#include <vtkDICOMImageReader.h>

class DICOMThread : public QObject {
    Q_OBJECT
public:
    explicit DICOMThread(const QString& folderPath);

public slots:
    void process();

signals:
    void finished(vtkImageData* image, QString patientName, QString patientID, int axialSlices, int coronalSlices, int sagittalSlices);


private:
    QString m_folderPath;
};
