
#include "DICOMThread.h"
#include <QDir>
#include <QFileInfoList>
#include <fstream>
#include <vtkSmartPointer.h>
#include <vtkImageData.h>
#include <vtkDICOMImageReader.h>
#include <QDebug>

QString getDICOMFileName(const QString& folderPath) {
    QDir dir(folderPath);
    QFileInfoList fileList = dir.entryInfoList(QDir::Files);
    if (fileList.isEmpty()) return QString();
    return fileList.first().absoluteFilePath();
}

QString getPIDFromDICOM(const QString& dicomFilePath) {
    std::ifstream file(dicomFilePath.toStdString(), std::ios::binary);
    if (!file.is_open()) return "Null";

    file.seekg(132);
    while (file && !file.eof()) {
        uint16_t group = 0, element = 0;
        file.read(reinterpret_cast<char*>(&group), 2);
        file.read(reinterpret_cast<char*>(&element), 2);
        if (file.eof()) break;
        char vr[3] = {0};
        file.read(vr, 2);
        uint32_t length = 0;
        if (QString(vr) == "OB" || QString(vr) == "OW" || QString(vr) == "SQ" ||
            QString(vr) == "UN" || QString(vr) == "UT") {
            file.seekg(2, std::ios::cur);
            file.read(reinterpret_cast<char*>(&length), 4);
        } else {
            uint16_t len16 = 0;
            file.read(reinterpret_cast<char*>(&len16), 2);
            length = len16;
        }

        if (group == 0x0010 && element == 0x0020) {
            std::string value(length, '\0');
            file.read(&value[0], length);
            return QString::fromStdString(value).trimmed();
        } else {
            file.seekg(length, std::ios::cur);
        }
    }

    return "Null";
}

DICOMThread::DICOMThread(const QString& folderPath) : m_folderPath(folderPath) {}

void DICOMThread::process() {
    vtkSmartPointer<vtkDICOMImageReader> reader = vtkSmartPointer<vtkDICOMImageReader>::New();
    reader->SetDirectoryName(m_folderPath.toStdString().c_str());
    reader->Update();
    vtkImageData* imageCopy = vtkImageData::New();
    imageCopy->DeepCopy(reader->GetOutput());

    QString patientName = QString::fromStdString(reader->GetPatientName());
    QString firstFile = getDICOMFileName(m_folderPath);
    QString patientID = getPIDFromDICOM(firstFile);

    qDebug() << "Patient Name:" << patientName;
    qDebug() << "Patient ID:" << patientID;
    int extent[6];
    imageCopy->GetExtent(extent);
    
    int sagittalSlices = extent[1] - extent[0] + 1;
    int coronalSlices = extent[3] - extent[2] + 1;
    int axialSlices = extent[5] - extent[4] + 1;
    
    qDebug() << "Sagittal Total Slices:" << sagittalSlices;
    qDebug() << "Coronal Total Slices:" << coronalSlices;
    qDebug() << "Axial Total Slices:" << axialSlices;
    emit finished(imageCopy, patientName, patientID, axialSlices, coronalSlices, sagittalSlices);
}
