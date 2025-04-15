#include "SliceInteract.h"
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkObjectFactory.h>
#include <vtkCamera.h>
#include <vtkImageData.h>

vtkStandardNewMacro(SliceInteract);

void SliceInteract::SetImageViewer(vtkImageViewer2* viewer) {
    this->viewer = viewer;
}

void SliceInteract::SetOrientation(QString viewName) {
    this->viewType = viewName;
}


void SliceInteract::OnMouseMove() {
    // override

}
void SliceInteract::OnMouseWheelForward() {
    if (!viewer) return;
    if (this->GetInteractor()->GetControlKey()) {
        vtkRenderer* renderer = this->GetDefaultRenderer();
        if (renderer) {
            renderer->GetActiveCamera()->Zoom(1.1); 
            this->GetInteractor()->Render();
        }
        return;
    }
    int slice = viewer->GetSlice();
    if (slice < viewer->GetSliceMax()) {
        viewer->SetSlice(slice + 1);
        viewer->Render();
    }
}

void SliceInteract::OnMouseWheelBackward() {
    if (!viewer) return;

    if (this->GetInteractor()->GetControlKey()) {
        vtkRenderer* renderer = this->GetDefaultRenderer();
        if (renderer) {
            renderer->GetActiveCamera()->Zoom(0.9); 
            this->GetInteractor()->Render();
        }
        return;
    }

    int slice = viewer->GetSlice();
    if (slice > viewer->GetSliceMin()) {
        viewer->SetSlice(slice - 1);
        viewer->Render();
    }
}

void SliceInteract::OnLeftButtonDown() {
    if (!viewer) return;

    int* clickPos = this->GetInteractor()->GetEventPosition();

    auto picker = vtkSmartPointer<vtkCellPicker>::New();
    picker->SetTolerance(0.001);

    vtkRenderer* renderer = this->GetDefaultRenderer();
    if (!renderer) {
        qDebug("No default renderer set!");
        return;
    }

    picker->Pick(clickPos[0], clickPos[1], 0, renderer);

    if (picker->GetCellId() < 0) {
        qDebug("Nothing pick");
        return;
    }

    double* worldPos = picker->GetPickPosition();

    vtkImageData* image = viewer->GetInput();
    double origin[3];
    double spacing[3];
    image->GetOrigin(origin);
    image->GetSpacing(spacing);

    int x = static_cast<int>((worldPos[0] - origin[0]) / spacing[0] + 0.5);
    int y = static_cast<int>((worldPos[1] - origin[1]) / spacing[1] + 0.5);
    int z = static_cast<int>((worldPos[2] - origin[2]) / spacing[2] + 0.5);

    if (viewType == "Axial") {
        emit crosshairMoved(x, y, viewer->GetSlice());
    } else if (viewType == "Coronal") {
        emit crosshairMoved(x, viewer->GetSlice(), z);
    } else if (viewType == "Sagittal") {
        emit crosshairMoved(viewer->GetSlice(), y, z);
    }

    this->vtkInteractorStyleImage::OnLeftButtonDown();
}