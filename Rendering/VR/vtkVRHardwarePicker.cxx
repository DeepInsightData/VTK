/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkVRHardwarePicker.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkVRHardwarePicker.h"

#include "vtkCommand.h"
#include "vtkHardwareSelector.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkSelection.h"
#include "vtkTransform.h"
#include "vtkVRCamera.h"
#include "vtkVRRenderWindow.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkVRHardwarePicker);

vtkSelection* vtkVRHardwarePicker::GetSelection()
{
  return this->Selection;
}

// set up for a pick
void vtkVRHardwarePicker::Initialize()
{
  this->Superclass::Initialize();
}

// Pick from the given collection
int vtkVRHardwarePicker::PickProp(
  double p0[3], double wxyz[4], vtkRenderer* renderer, vtkPropCollection*, bool actorPassOnly)
{
  //  Initialize picking process
  this->Initialize();
  this->Renderer = renderer;

  // Invoke start pick method if defined
  this->InvokeEvent(vtkCommand::StartPickEvent, nullptr);

  vtkVRRenderWindow* renWin = vtkVRRenderWindow::SafeDownCast(renderer->GetRenderWindow());
  if (!renWin)
  {
    return 0;
  }

  vtkNew<vtkHardwareSelector> sel;
  sel->SetFieldAssociation(vtkDataObject::FIELD_ASSOCIATION_CELLS);
  sel->SetRenderer(renderer);
  sel->SetActorPassOnly(actorPassOnly);
  vtkVRCamera* activeCam = vtkVRCamera::SafeDownCast(renderer->GetActiveCamera());
  if (!activeCam)
  {
    return 0;
  }
  bool lastTrackHMD = activeCam->GetTrackHMD();
  activeCam->SetTrackHMD(false);

  vtkNew<vtkTransform> tran;
  tran->RotateWXYZ(wxyz[0], wxyz[1], wxyz[2], wxyz[3]);
  double pin[4] = { 0.0, 0.0, -1.0, 1.0 };
  double dop[4];
  tran->MultiplyPoint(pin, dop);
  double distance = activeCam->GetDistance();
  activeCam->SetPosition(p0);
  activeCam->SetFocalPoint(
    p0[0] + dop[0] * distance, p0[1] + dop[1] * distance, p0[2] + dop[2] * distance);
  activeCam->OrthogonalizeViewUp();

  const int* size = renderer->GetSize();

  sel->SetArea(size[0] / 2 - 5, size[1] / 2 - 5, size[0] / 2 + 5, size[1] / 2 + 5);

  this->Selection = nullptr;
  if (sel->CaptureBuffers())
  {
    unsigned int outPos[2];
    unsigned int inPos[2] = { static_cast<unsigned int>(size[0] / 2),
      static_cast<unsigned int>(size[1] / 2) };
    // find the data closest to the center
    vtkHardwareSelector::PixelInformation pinfo = sel->GetPixelInformation(inPos, 5, outPos);
    if (pinfo.Valid)
    {
      this->Selection.TakeReference(
        sel->GenerateSelection(outPos[0], outPos[1], outPos[0], outPos[1]));
    }
  }

  // this->Selection = sel->Select();
  // sel->SetArea(0, 0, size[0]-1, size[1]-1);

  activeCam->SetTrackHMD(lastTrackHMD);

  this->InvokeEvent(vtkCommand::EndPickEvent, this->Selection);

  if (this->Selection && this->Selection->GetNode(0))
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

//------------------------------------------------------------------------------
void vtkVRHardwarePicker::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->Selection)
  {
    this->Selection->PrintSelf(os, indent);
  }
}
VTK_ABI_NAMESPACE_END
