#pragma once

#include <vtkInteractorStyleImage.h>
#include <vtkSmartPointer.h>
#include <vtkImageViewer2.h>
#include <vtkCellPicker.h>
#include <QObject>

class SliceInteract : public QObject, public vtkInteractorStyleImage {
    Q_OBJECT
public:
    static SliceInteract* New();
    vtkTypeMacro(SliceInteract, vtkInteractorStyleImage);

    void SetImageViewer(vtkImageViewer2* viewer);
    void SetOrientation(QString viewName);  // "Axial", "Coronal", "Sagittal"

    void OnMouseWheelForward() override;
    void OnMouseWheelBackward() override;
    void OnLeftButtonDown() override;
    void OnMouseMove() override;
signals:
    void crosshairMoved(int x, int y, int z);

private:
    vtkImageViewer2* viewer = nullptr;
    QString viewType = "Axial";
};
