#pragma once

#include <QMainWindow>
#include <QVTKWidget.h>
#include <vtkSmartPointer.h>
#include <vtkDICOMImageReader.h>
#include <vtkImageViewer2.h>
#include <vtkCamera.h>
#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>
#include <vtkVolumeProperty.h>
#include <vtkMarchingCubes.h>
#include <QLineEdit>
#include <vtkHandleWidget.h>
#include <vtkSphereHandleRepresentation.h>
#include <vector>
#include <stack>

class QPushButton;
class QSlider;
class QLabel;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);

private:
    void setupUI();
    void setupButtonConnections();
    void setupStyle();
    void onLoadDICOM();

    //Control bar
    QPushButton* loadButton;
    QPushButton* resetViewButton;
    QPushButton* addSphereButton;
    QSlider* slider2D;
    QSlider* slider3DWindow;
    QSlider* slider3DLevel;
    QLineEdit* edit3DWindow;
    QLineEdit* edit3DLevel;    
    QLabel* patientNameLabel;
    QLabel* patientIDLabel;
    QLineEdit *editWindowMin, *editWindowMax;
    int level2D = 0, window2D = 0;
    double imageCenter[3] = {0.0, 0.0, 0.0};
    // VTK widgets
    QVTKWidget* axialView;
    QVTKWidget* coronalView;
    QVTKWidget* sagittalView;
    QVTKWidget* volumeView;
    QVTKWidget* polydataView;

    // DICOM data
    vtkSmartPointer<vtkDICOMImageReader> dicomReader;

    vtkSmartPointer<vtkImageViewer2> axialViewer;
    vtkSmartPointer<vtkImageViewer2> coronalViewer;
    vtkSmartPointer<vtkImageViewer2> sagittalViewer;
    vtkSmartPointer<vtkRenderer> volumeRenderer;
    vtkSmartPointer<vtkRenderer> polydataRenderer;
    vtkSmartPointer<vtkCamera> volumeCameraOriginal;
    vtkSmartPointer<vtkCamera> polydataCameraOriginal;

    vtkSmartPointer<vtkColorTransferFunction> color3D;
    vtkSmartPointer<vtkPiecewiseFunction> opacity3D;
    vtkSmartPointer<vtkVolumeProperty> volumeProperty3D;
    vtkSmartPointer<vtkMarchingCubes> contour;

    std::vector<vtkSmartPointer<vtkHandleWidget>> sphereWidgets;
    std::vector<vtkSmartPointer<vtkSphereHandleRepresentation>> sphereReps;
    int selectedSphereStatus = -1;
    std::stack<vtkSmartPointer<vtkCamera>> undoVolumeStack;
    std::stack<vtkSmartPointer<vtkCamera>> redoVolumeStack;

    std::stack<vtkSmartPointer<vtkCamera>> undoPolyStack;
    std::stack<vtkSmartPointer<vtkCamera>> redoPolyStack;

    enum class ViewType { None, Volume, PolyData };
    ViewType currentActiveView = ViewType::None;
protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
};