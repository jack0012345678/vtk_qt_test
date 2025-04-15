#pragma execution_character_set("utf-8")

// MainWindow.cpp
#include "MainWindow.h"
#include "DICOMThread.h"
#include "SliceInteract.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QVTKWidget.h>
#include <QFileDialog>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRendererCollection.h>
#include <vtkVolumeProperty.h>
#include <vtkGPUVolumeRayCastMapper.h>
#include <vtkImageData.h>
#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>
#include <vtkMarchingCubes.h>
#include <vtkCamera.h>
#include <vtkCallbackCommand.h>
#include <vtkStringArray.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <QDebug>
#include <QString>
#include <QThread>
#include <QKeyEvent>
#include <QCoreApplication>
#include <fstream>
#include <QtEndian>
#include <string>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setupUI();
    setupStyle();
    setupButtonConnections();
    setMinimumSize(1920, 1080);
}

void MainWindow::setupUI() {
    QWidget* central = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(central);

    // Control bar
    QHBoxLayout* controlBar = new QHBoxLayout;
    loadButton = new QPushButton("Load DICOM");
    resetViewButton = new QPushButton("reset view");
    addSphereButton = new QPushButton("add sphere");
    patientNameLabel = new QLabel("Patient: ");
    patientIDLabel = new QLabel("ID: ");

    editWindowMin = new QLineEdit("0");
    editWindowMax = new QLineEdit("1000");
    editWindowMin->setFixedWidth(50);
    editWindowMax->setFixedWidth(50);
    slider2D = new QSlider(Qt::Horizontal);
    slider2D->setRange(0, 1000);
    slider2D->setValue(500);
    slider2D->setFixedWidth(150);
    slider2D->setFixedWidth(120);

    slider3DWindow = new QSlider(Qt::Horizontal);
    slider3DWindow->setFixedWidth(120);
    slider3DWindow->setRange(1, 2000);
    slider3DWindow->setValue(500);

    slider3DLevel = new QSlider(Qt::Horizontal);
    slider3DLevel->setFixedWidth(120);
    slider3DLevel->setRange(-1000, 1000);
    slider3DLevel->setValue(0);

    edit3DWindow = new QLineEdit("500");
    edit3DWindow->setFixedWidth(50);
    edit3DLevel = new QLineEdit("0");
    edit3DLevel->setFixedWidth(50);
    

    controlBar->addWidget(loadButton);
    controlBar->addWidget(resetViewButton);
    controlBar->addWidget(addSphereButton);
    controlBar->addWidget(new QLabel("2D W Min:"));
    controlBar->addWidget(editWindowMin);
    controlBar->addWidget(new QLabel("Level:"));
    controlBar->addWidget(slider2D);
    controlBar->addWidget(new QLabel("2D W Max:"));
    controlBar->addWidget(editWindowMax);
    controlBar->addWidget(new QLabel("3D Window:"));
    controlBar->addWidget(slider3DWindow);
    controlBar->addWidget(edit3DWindow);
    controlBar->addWidget(new QLabel("3D Level:"));
    controlBar->addWidget(slider3DLevel);
    controlBar->addWidget(edit3DLevel);
    controlBar->addStretch();
    controlBar->addWidget(patientNameLabel);
    controlBar->addWidget(patientIDLabel);

    // Views
    axialView = new QVTKWidget;
    coronalView = new QVTKWidget;
    sagittalView = new QVTKWidget;
    volumeView = new QVTKWidget;
    polydataView = new QVTKWidget;

    auto axialRender = vtkSmartPointer<vtkRenderer>::New();
    axialRender->SetBackground(0, 0, 0);
    axialView->GetRenderWindow()->AddRenderer(axialRender);

    auto coronalRender = vtkSmartPointer<vtkRenderer>::New();
    coronalRender->SetBackground(0, 0, 0);
    coronalView->GetRenderWindow()->AddRenderer(coronalRender);

    auto sagittalRender = vtkSmartPointer<vtkRenderer>::New();
    sagittalRender->SetBackground(0, 0, 0);
    sagittalView->GetRenderWindow()->AddRenderer(sagittalRender);

    auto volumeRender = vtkSmartPointer<vtkRenderer>::New();
    volumeRender->SetBackground(0, 0, 0);
    volumeView->GetRenderWindow()->AddRenderer(volumeRender);

    auto polyRender = vtkSmartPointer<vtkRenderer>::New();
    polyRender->SetBackground(0, 0, 0);
    polydataView->GetRenderWindow()->AddRenderer(polyRender);
    QGridLayout* grid = new QGridLayout;
    grid->addWidget(axialView, 0, 0, 1, 2);
    grid->addWidget(volumeView, 0, 2, 2, 1);
    grid->addWidget(polydataView, 0, 3, 2, 1);
    grid->addWidget(coronalView, 1, 0);
    grid->addWidget(sagittalView, 1, 1);

    grid->setColumnStretch(0, 1);
    grid->setColumnStretch(1, 1);
    grid->setColumnStretch(2, 1);
    grid->setColumnStretch(3, 1);
    grid->setRowStretch(0, 2);
    grid->setRowStretch(1, 1);

    mainLayout->addLayout(controlBar);
    mainLayout->addLayout(grid);
    setCentralWidget(central);
    setWindowTitle("DICOM Viewer");

    polydataView->installEventFilter(this);
    volumeView->installEventFilter(this);

}


void MainWindow::setupButtonConnections() {
    connect(loadButton, &QPushButton::clicked, this, &MainWindow::onLoadDICOM);
    auto applyWindowLevel = [=]() {
        int min = editWindowMin->text().toInt();
        int max = editWindowMax->text().toInt();
        int level = slider2D->value();
        int window = max - min;
    
        for (auto viewer : {axialViewer, coronalViewer, sagittalViewer}) {
            if (viewer) {
                viewer->SetColorWindow(window);
                viewer->SetColorLevel(level);
                viewer->Render();
            }
        }
    };
    
    connect(slider2D, &QSlider::valueChanged, this, applyWindowLevel);
    
    auto updateSliderRange = [=]() {
        int min = editWindowMin->text().toInt();
        int max = editWindowMax->text().toInt();
        slider2D->setRange(min, max);
        slider2D->setValue((min + max) / 2);
        applyWindowLevel();
    };
    
    auto update3DFromText = [=]() {
        int window = edit3DWindow->text().toInt();
        int level = edit3DLevel->text().toInt();
        slider3DWindow->setValue(window);
        slider3DLevel->setValue(level);
    };
    
    connect(edit3DWindow, &QLineEdit::editingFinished, this, update3DFromText);
    connect(edit3DLevel, &QLineEdit::editingFinished, this, update3DFromText);
    

    connect(editWindowMin, &QLineEdit::editingFinished, this, updateSliderRange);
    connect(editWindowMax, &QLineEdit::editingFinished, this, updateSliderRange);
    auto update3DTransfer = [=]() {
        int window = slider3DWindow->value();
        int level = slider3DLevel->value();
        int wMin = level - window / 2;
        int wMax = level + window / 2;
    
        edit3DWindow->setText(QString::number(window));
        edit3DLevel->setText(QString::number(level));
        
    
        if (color3D && opacity3D) {
            color3D->RemoveAllPoints();
            opacity3D->RemoveAllPoints();
            color3D->AddRGBPoint(wMin, 0, 0, 0);
            color3D->AddRGBPoint(wMax, 1, 1, 1);
            opacity3D->AddPoint(wMin, 0.0);
            opacity3D->AddPoint(wMax, 1.0);

            volumeProperty3D->SetColor(color3D);
            volumeProperty3D->SetScalarOpacity(opacity3D);
            volumeView->GetRenderWindow()->Render();
        }
    
        if (contour && polydataView) {
            contour->SetValue(0, level);
            contour->Update();
            polydataView->GetRenderWindow()->Render();
        }
    };
    
    connect(slider3DWindow, &QSlider::valueChanged, this, update3DTransfer);
    connect(slider3DLevel, &QSlider::valueChanged, this, update3DTransfer);
    connect(addSphereButton, &QPushButton::clicked, this, [=]() {

        auto rep = vtkSmartPointer<vtkSphereHandleRepresentation>::New();
        rep->SetHandleSize(10.0);
        rep->SetSphereRadius(10.0);
        rep->GetProperty()->SetColor(1.0, 0.0, 0.0);
        rep->SetWorldPosition(imageCenter);

        auto widget = vtkSmartPointer<vtkHandleWidget>::New();
        widget->SetInteractor(polydataView->GetRenderWindow()->GetInteractor());
        widget->SetRepresentation(rep);
        widget->EnabledOn();

        sphereWidgets.push_back(widget);
        sphereReps.push_back(rep);
    
        vtkSmartPointer<vtkHandleWidget> currentWidget = sphereWidgets.back();
        vtkSmartPointer<vtkSphereHandleRepresentation> currentRep = sphereReps.back();
    
        vtkSmartPointer<vtkCallbackCommand> selectCallback = vtkSmartPointer<vtkCallbackCommand>::New();
        selectCallback->SetCallback([](vtkObject* caller, unsigned long, void* clientData, void*) {
            auto* that = static_cast<MainWindow*>(clientData);
            vtkHandleWidget* widget = static_cast<vtkHandleWidget*>(caller);
    
            for (int i = 0; i < that->sphereWidgets.size(); ++i) {
                if (that->sphereWidgets[i] == widget) {
                    that->selectedSphereStatus = i;
    
                    for (int j = 0; j < that->sphereReps.size(); ++j) {
                        if (j == i)
                            that->sphereReps[j]->GetProperty()->SetColor(0.0, 1.0, 0.0);
                        else
                            that->sphereReps[j]->GetProperty()->SetColor(1.0, 0.0, 0.0);
                    }
    
                    that->polydataView->GetRenderWindow()->Render();
                    break;
                }
            }
        });
        selectCallback->SetClientData(this);
        currentWidget->AddObserver(vtkCommand::StartInteractionEvent, selectCallback);
    
        polydataView->GetRenderWindow()->Render();
    });
    
    
}

bool MainWindow::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Delete) {
            if (selectedSphereStatus >= 0 && selectedSphereStatus < sphereWidgets.size()) {
                sphereWidgets[selectedSphereStatus]->EnabledOff();
                sphereWidgets.erase(sphereWidgets.begin() + selectedSphereStatus);
                sphereReps.erase(sphereReps.begin() + selectedSphereStatus);
                selectedSphereStatus = -1;
                polydataView->GetRenderWindow()->Render();
                qDebug() << "Sphere deleted.";
            }
            return true;
        }
    }
    if (event->type() == QEvent::Enter) {
        if (obj == volumeView)
            currentActiveView = ViewType::Volume;
        else if (obj == polydataView)
            currentActiveView = ViewType::PolyData;
    }

    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

        // Undo
        if (keyEvent->key() == Qt::Key_Z && keyEvent->modifiers() & Qt::ControlModifier) {
            if (currentActiveView == ViewType::Volume && undoVolumeStack.size() > 1) {
                auto current = undoVolumeStack.top(); 
                undoVolumeStack.pop();
                redoVolumeStack.push(current);

                auto cam = undoVolumeStack.top();
                volumeRenderer->GetActiveCamera()->DeepCopy(cam);
                volumeRenderer->ResetCameraClippingRange();
                volumeView->GetRenderWindow()->Render();
            }

            if (currentActiveView == ViewType::PolyData && undoPolyStack.size() > 1) {
                auto current = undoPolyStack.top(); 
                undoPolyStack.pop();
                redoPolyStack.push(current);

                auto cam = undoPolyStack.top();
                polydataRenderer->GetActiveCamera()->DeepCopy(cam);
                polydataRenderer->ResetCameraClippingRange();
                polydataView->GetRenderWindow()->Render();
            }

            return true;
        }

        // Redo
        if (keyEvent->key() == Qt::Key_Y && keyEvent->modifiers() & Qt::ControlModifier) {
            if (currentActiveView == ViewType::Volume && !redoVolumeStack.empty()) {
                auto cam = redoVolumeStack.top(); redoVolumeStack.pop();
                undoVolumeStack.push(cam);
                volumeRenderer->GetActiveCamera()->DeepCopy(cam);
                volumeRenderer->ResetCameraClippingRange();
                volumeView->GetRenderWindow()->Render();
            }

            if (currentActiveView == ViewType::PolyData && !redoPolyStack.empty()) {
                auto cam = redoPolyStack.top(); redoPolyStack.pop();
                undoPolyStack.push(cam);
                polydataRenderer->GetActiveCamera()->DeepCopy(cam);
                polydataRenderer->ResetCameraClippingRange();
                polydataView->GetRenderWindow()->Render();
            }

            return true;
        }
    }
    
    return QMainWindow::eventFilter(obj, event);
}


void MainWindow::onLoadDICOM() {
    QString folder = QFileDialog::getExistingDirectory(this, "Select DICOM Folder");
    if (folder.isEmpty()) {
        qDebug() << "No folder selected!";
        return;
    }

    // Create Thread
    QThread* thread = new QThread;
    DICOMThread* worker = new DICOMThread(folder);
    worker->moveToThread(thread);

    connect(thread, &QThread::started, worker, &DICOMThread::process);

    connect(worker, &DICOMThread::finished, this, [=](vtkImageData* imageRawData, QString patientName, QString patientID, int axialSlices, int coronalSlices, int sagittalSlices) {
        // Patient info
        vtkSmartPointer<vtkImageData> image = imageRawData;
        double range[2];
        image->GetScalarRange(range);
        image->GetCenter(imageCenter);
        qDebug() << "Image center:" << imageCenter[0] << imageCenter[1] << imageCenter[2];

        editWindowMin->setText(QString::number(int(range[0])));
        editWindowMax->setText(QString::number(int(range[1])));
        slider2D->setRange(int(range[0]), int(range[1]));
        slider2D->setValue(int((range[0] + range[1]) / 2));
        int window = (int(range[1]) - int(range[0]));
        int level = int((range[0] + range[1]) / 2);
        slider3DWindow->setRange(1, window * 2);
        slider3DWindow->setValue(window);
        slider3DLevel->setRange(int(range[0]) - window, int(range[1]) + window);
        slider3DLevel->setValue(level);
        edit3DWindow->setText(QString::number(window));
        edit3DLevel->setText(QString::number(level));
        color3D = vtkSmartPointer<vtkColorTransferFunction>::New();
        opacity3D = vtkSmartPointer<vtkPiecewiseFunction>::New();
        volumeProperty3D = vtkSmartPointer<vtkVolumeProperty>::New();
        int wMin = level - window / 2;
        int wMax = level + window / 2;
        color3D->AddRGBPoint(wMin, 0, 0, 0);
        color3D->AddRGBPoint(wMax, 1, 1, 1);
        opacity3D->AddPoint(wMin, 0.0);
        opacity3D->AddPoint(wMax, 1.0);

        patientNameLabel->setText("Patient: " + patientName);
        patientIDLabel->setText("ID: " + patientID);
        QCoreApplication::processEvents();
        axialViewer = vtkSmartPointer<vtkImageViewer2>::New();
        axialViewer->SetInputData(image);
        axialViewer->SetSliceOrientationToXY();
        axialViewer->SetRenderWindow(axialView->GetRenderWindow());
        axialViewer->SetupInteractor(axialView->GetRenderWindow()->GetInteractor());

        coronalViewer = vtkSmartPointer<vtkImageViewer2>::New();
        coronalViewer->SetInputData(image);
        coronalViewer->SetSliceOrientationToXZ();
        coronalViewer->SetRenderWindow(coronalView->GetRenderWindow());
        coronalViewer->SetupInteractor(coronalView->GetRenderWindow()->GetInteractor());

        sagittalViewer = vtkSmartPointer<vtkImageViewer2>::New();
        sagittalViewer->SetInputData(image);
        sagittalViewer->SetSliceOrientationToYZ();
        sagittalViewer->SetRenderWindow(sagittalView->GetRenderWindow());
        sagittalViewer->SetupInteractor(sagittalView->GetRenderWindow()->GetInteractor());

        auto axialInteract = vtkSmartPointer<SliceInteract>::New();
        axialInteract->SetImageViewer(axialViewer);
        axialInteract->SetOrientation("Axial");
        axialView->GetRenderWindow()->GetInteractor()->SetInteractorStyle(axialInteract);
        axialInteract->SetDefaultRenderer(axialViewer->GetRenderer());

        auto coronalInteract = vtkSmartPointer<SliceInteract>::New();
        coronalInteract->SetImageViewer(coronalViewer);
        coronalInteract->SetOrientation("Coronal");
        coronalView->GetRenderWindow()->GetInteractor()->SetInteractorStyle(coronalInteract);
        coronalInteract->SetDefaultRenderer(coronalViewer->GetRenderer());

        auto sagittalInteract = vtkSmartPointer<SliceInteract>::New();
        sagittalInteract->SetImageViewer(sagittalViewer);
        sagittalInteract->SetOrientation("Sagittal");
        sagittalView->GetRenderWindow()->GetInteractor()->SetInteractorStyle(sagittalInteract);
        sagittalInteract->SetDefaultRenderer(sagittalViewer->GetRenderer());

        auto volumeMapper = vtkSmartPointer<vtkGPUVolumeRayCastMapper>::New();
        volumeMapper->SetInputData(image);
        auto volume = vtkSmartPointer<vtkVolume>::New();
        volume->SetMapper(volumeMapper);
        volume->SetProperty(volumeProperty3D);
        
        auto volRen = vtkSmartPointer<vtkRenderer>::New();
        volRen->AddVolume(volume);
        volRen->ResetCamera();
        volumeView->GetRenderWindow()->GetRenderers()->RemoveAllItems();
        volumeView->GetRenderWindow()->AddRenderer(volRen);
        volumeRenderer = volRen; 

        contour = vtkSmartPointer<vtkMarchingCubes>::New();
        contour->SetInputData(image);
        contour->ComputeNormalsOn();
        contour->SetValue(0, level);
        auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(contour->GetOutputPort());
        mapper->ScalarVisibilityOff();
        auto actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        actor->GetProperty()->SetColor(0, 0, 1);
        actor->GetProperty()->SetOpacity(0.6);
        auto polyRen = vtkSmartPointer<vtkRenderer>::New();
        polyRen->AddActor(actor);
        polyRen->ResetCamera();
        polydataView->GetRenderWindow()->GetRenderers()->RemoveAllItems();
        polydataView->GetRenderWindow()->AddRenderer(polyRen);
        polydataRenderer = polyRen;

        volumeCameraOriginal = vtkSmartPointer<vtkCamera>::New();
        volumeCameraOriginal->DeepCopy(volumeRenderer->GetActiveCamera());
        polydataCameraOriginal = vtkSmartPointer<vtkCamera>::New();
        polydataCameraOriginal->DeepCopy(polydataRenderer->GetActiveCamera());

        axialViewer->Render();
        coronalViewer->Render();
        sagittalViewer->Render();
        volumeView->GetRenderWindow()->Render();
        polydataView->GetRenderWindow()->Render();
        //init status
        undoVolumeStack.push(volumeCameraOriginal);
        undoPolyStack.push(polydataCameraOriginal);
        auto addCameraObserver = [=](vtkRenderWindowInteractor* interactor,
                                    vtkSmartPointer<vtkRenderer> renderer,
                                    std::stack<vtkSmartPointer<vtkCamera>>& undoStack,
                                    std::stack<vtkSmartPointer<vtkCamera>>& redoStack) {
            vtkSmartPointer<vtkCallbackCommand> callback = vtkSmartPointer<vtkCallbackCommand>::New();
            callback->SetCallback([](vtkObject*, unsigned long, void* clientData, void*) {
                auto* data = static_cast<std::tuple<vtkRenderer*, std::stack<vtkSmartPointer<vtkCamera>>*, std::stack<vtkSmartPointer<vtkCamera>>*>*>(clientData);
                vtkRenderer* renderer = std::get<0>(*data);
                auto* undoStack = std::get<1>(*data);
                auto* redoStack = std::get<2>(*data);
                auto currentCamera = vtkSmartPointer<vtkCamera>::New();
                currentCamera->DeepCopy(renderer->GetActiveCamera());
                undoStack->push(currentCamera);
                while (!redoStack->empty()) redoStack->pop();
            });
            auto* data = new std::tuple<vtkRenderer*, std::stack<vtkSmartPointer<vtkCamera>>*, std::stack<vtkSmartPointer<vtkCamera>>*>(renderer, &undoStack, &redoStack);
            callback->SetClientData(data);
            interactor->AddObserver(vtkCommand::EndInteractionEvent, callback);
        };
        addCameraObserver(volumeView->GetRenderWindow()->GetInteractor(), volumeRenderer, undoVolumeStack, redoVolumeStack);
        addCameraObserver(polydataView->GetRenderWindow()->GetInteractor(), polydataRenderer, undoPolyStack, redoPolyStack);
        

        connect(axialInteract, &SliceInteract::crosshairMoved, this, [=](int x, int y, int z){
            coronalViewer->SetSlice(y); 
            qDebug() << "coronal:" << y;
            coronalViewer->Render();
            sagittalViewer->SetSlice(x); 
            sagittalViewer->Render();
        });
        connect(coronalInteract, &SliceInteract::crosshairMoved, this, [=](int x, int y, int z){
            axialViewer->SetSlice(z); 
            axialViewer->Render();
            sagittalViewer->SetSlice(x); 
            sagittalViewer->Render();
        });
        connect(sagittalInteract, &SliceInteract::crosshairMoved, this, [=](int x, int y, int z){
            axialViewer->SetSlice(z); 
            axialViewer->Render();
            coronalViewer->SetSlice(y); 
            coronalViewer->Render();
        });

        thread->quit();
        thread->wait();
        worker->deleteLater();
        thread->deleteLater();
    });
    vtkRenderer* volumeReset = volumeView->GetRenderWindow()->GetRenderers()->GetFirstRenderer();
    vtkRenderer* polyReset = polydataView->GetRenderWindow()->GetRenderers()->GetFirstRenderer(); 
    connect(resetViewButton, &QPushButton::clicked, this, [=]() {
        if (volumeRenderer && volumeCameraOriginal) {
            volumeRenderer->GetActiveCamera()->DeepCopy(volumeCameraOriginal);
            volumeRenderer->ResetCameraClippingRange();
            volumeView->GetRenderWindow()->Render();
        }
        if (polydataRenderer && polydataCameraOriginal) {
            polydataRenderer->GetActiveCamera()->DeepCopy(polydataCameraOriginal);
            polydataRenderer->ResetCameraClippingRange();
            polydataView->GetRenderWindow()->Render();
        }
    });


    thread->start();
}

void MainWindow::setupStyle() {
    this->setStyleSheet(R"(
        QWidget {
            background-color:rgb(43, 43, 43);
            color: #e0e0e0;
            font-size: 14px;
        }

        QPushButton {
            background-color:rgb(63, 63, 63);
            color: #e0e0e0;
            border: 1px solid #555;
            border-radius: 6px;
            padding: 6px 12px;
        }

        QPushButton:hover {
            background-color:rgb(66, 66, 66);
            border: 1px solidrgb(90, 165, 250);
        }

        QLabel {
            font-weight: bold;
        }

        QSlider::groove:horizontal {
            height: 6px;
            background: #444;
            border-radius: 3px;
        }

        QSlider::handle:horizontal {
            background:rgb(85, 151, 225);
            width: 16px;
            margin: -5px 0;
            border-radius: 8px;
        }
    )");
}