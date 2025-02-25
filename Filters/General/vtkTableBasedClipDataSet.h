/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTableBasedClipDataSet.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*****************************************************************************
 *
 * Copyright (c) 2000 - 2009, Lawrence Livermore National Security, LLC
 * Produced at the Lawrence Livermore National Laboratory
 * LLNL-CODE-400124
 * All rights reserved.
 *
 * This file was adapted from the VisIt clipper (vtkVisItClipper). For  details,
 * see https://visit.llnl.gov/.  The full copyright notice is contained in the
 * file COPYRIGHT located at the root of the VisIt distribution or at
 * http://www.llnl.gov/visit/copyright.html.
 *
 *****************************************************************************/

/**
 * @class   vtkTableBasedClipDataSet
 * @brief   Clip any dataset with a user-specified
 *  implicit function or an input scalar point data array.
 *
 *
 *  vtkTableBasedClipDataSet is a filter that clips any type of dataset using
 *  either any subclass of vtkImplicitFunction or an input scalar point data
 *  array. Clipping means that it actually "cuts" through the cells of the
 *  dataset, returning everything outside the specified implicit function (or
 *  greater than the scalar value) including "pieces" of a cell (Note to compare
 *  this with vtkExtractGeometry, which pulls out entire, uncut cells). The
 *  output of this filter is a vtkUnstructuredGrid data.
 *
 *  To use this filter, you need to decide whether an implicit function or an
 *  input scalar point data array is used for clipping. For the former case,
 *  1) define an implicit function
 *  2) provide it to this filter via SetClipFunction()
 *  If a clipping function is not specified, or GenerateClipScalars is off( the
 *  default), the input scalar point data array is then employed for clipping.
 *
 *  You can also specify a scalar (iso-)value, which is used to decide what is
 *  inside and outside the implicit function. You can also reverse the sense of
 *  what inside/outside is by setting IVAR InsideOut. The clipping algorithm
 *  proceeds by computing an implicit function value or using the input scalar
 *  point data value for each point in the dataset. This is compared against the
 *  scalar (iso-)value to determine the inside/outside status.
 *
 *  Although this filter sometimes (but rarely) may resort to the sibling class
 *  vtkClipDataSet for handling some special grids (such as cylinders or cones
 *  with capping faces in the form of a vtkPolyData), it itself is able to deal
 *  with most grids. It is worth mentioning that vtkTableBasedClipDataSet is
 *  capable of addressing the artifacts that may occur with vtkClipDataSet due
 *  to the possibly inconsistent triangulation modes between neighboring cells.
 *  In addition, the former is much faster than the latter. Furthermore, the
 *  former produces less cells (with ratio usually being 5~6) than by the latter
 *  in the output. In other words, this filter retains the original cells (i.e.,
 *  without triangulation / tetrahedralization) wherever possible. All these
 *  advantages are gained by adopting the unique clipping and triangulation tables
 *  proposed by VisIt.
 *
 * @warning
 *  vtkTableBasedClipDataSet makes use of a hash table (that is provided by class
 *  maintained by internal class vtkTableBasedClipperDataSetFromVolume) to achieve
 *  rapid removal of duplicate points. The hash-based mechanism simply compares the
 *  point Ids, without considering the actual inter-point distance (vtkClipDataSet
 *  adopts vtkMergePoints that though considers the inter-point distance for robust
 *  points merging ). As a result, some duplicate points may be present in the output.
 *  This problem occurs when some boundary (cut-through cells) happen to have faces
 *  EXACTLY aligned with the clipping plane (such as Plane, Box, or other implicit
 *  functions with planar shapes). The occurrence (though very rare) of duplicate
 *  points produces degenerate cells, which can be fixed by post-processing the
 *  output with a filter like vtkCleanGrid.
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @par Thanks:
 *  This filter was adapted from the VisIt clipper (vtkVisItClipper).
 *
 * @sa
 *  vtkClipDataSet vtkClipVolume vtkClipPolyData vtkCutter vtkImplicitFunction
 */

#ifndef vtkTableBasedClipDataSet_h
#define vtkTableBasedClipDataSet_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkUnstructuredGridAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkCallbackCommand;
class vtkDoubleArray;
class vtkImplicitFunction;

class VTKFILTERSGENERAL_EXPORT vtkTableBasedClipDataSet : public vtkUnstructuredGridAlgorithm
{
public:
  vtkTypeMacro(vtkTableBasedClipDataSet, vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Create an instance with a user-specified implicit function, turning off
   * IVARs InsideOut and GenerateClipScalars and setting IVAR Value to 0.0.
   */
  static vtkTableBasedClipDataSet* New();

  /**
   * Get the MTime for which the clip function are considered.
   */
  vtkMTimeType GetMTime() override;

  ///@{
  /**
   * Set/Get the InsideOut flag. With this flag off, a vertex is considered
   * inside (the implicit function or the isosurface) if the (function or scalar)
   * value is greater than IVAR Value. With this flag on, a vertex is considered
   * inside (the implicit function or the isosurface) if the (function or scalar)
   * value is less than or equal to IVAR Value. This flag is off by default.
   */
  vtkSetMacro(InsideOut, vtkTypeBool);
  vtkGetMacro(InsideOut, vtkTypeBool);
  vtkBooleanMacro(InsideOut, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/Get the clipping value of the implicit function (if an implicit function
   * is applied) or scalar data array (if a scalar data array is used), with 0.0
   * as the default value. This value is ignored if flag UseValueAsOffset is true
   * AND a clip function is defined.
   */
  vtkSetMacro(Value, double);
  vtkGetMacro(Value, double);
  ///@}

  ///@{
  /**
   * Set/Get flag UseValueAsOffset, with true as the default value. With this flag
   * on, IVAR Value is used as an offset parameter to the implicit function. Value
   * is used only when clipping using a scalar array.
   */
  vtkSetMacro(UseValueAsOffset, bool);
  vtkGetMacro(UseValueAsOffset, bool);
  vtkBooleanMacro(UseValueAsOffset, bool);
  ///@}

  ///@{
  /**
   * Set/Get the implicit function with which to perform the clipping operation.
   * Unless an implicit function is defined, the specified input scalar data will
   * be used for clipping.
   */
  virtual void SetClipFunction(vtkImplicitFunction*);
  vtkGetObjectMacro(ClipFunction, vtkImplicitFunction);
  ///@}

  ///@{
  /**
   * Set/Get flag GenerateClipScalars, with 0 as the default value. With this
   * flag on, the scalar point data values obtained by evaluating the implicit
   * function will be exported to the output. Note that this flag requiries that
   * an implicit function be provided.
   */
  vtkSetMacro(GenerateClipScalars, vtkTypeBool);
  vtkGetMacro(GenerateClipScalars, vtkTypeBool);
  vtkBooleanMacro(GenerateClipScalars, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/Get the tolerance used for merging duplicate points near the clipping
   * intersection cells. This tolerance may prevent the generation of degenerate
   * primitives. Note that only 3D cells actually use this IVAR when vtkClipDataSet
   * is invoked as last resort.
   */
  vtkSetClampMacro(MergeTolerance, double, 0.0001, 0.25);
  vtkGetMacro(MergeTolerance, double);
  ///@}

  ///@{
  /**
   * Set/Get whether a second output is generated. The second output contains the
   * polygonal data that is clipped away by the iso-surface.
   */
  vtkSetMacro(GenerateClippedOutput, vtkTypeBool);
  vtkGetMacro(GenerateClippedOutput, vtkTypeBool);
  vtkBooleanMacro(GenerateClippedOutput, vtkTypeBool);
  ///@}

  /**
   * Return the clipped output.
   */
  vtkUnstructuredGrid* GetClippedOutput();

  ///@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the vtkAlgorithm::Precision enum for an explanation of the available
   * precision settings.
   */
  vtkSetClampMacro(OutputPointsPrecision, int, SINGLE_PRECISION, DEFAULT_PRECISION);
  vtkGetMacro(OutputPointsPrecision, int);
  ///@}

  ///@{
  /**
   * Specify the number of input cells in a batch, where a batch defines
   * a subset of the input cells operated on during threaded
   * execution. Generally this is only used for debugging or performance
   * studies (since batch size affects the thread workload).
   *
   * Default is 1000.
   */
  vtkSetClampMacro(BatchSize, unsigned int, 1, VTK_INT_MAX);
  vtkGetMacro(BatchSize, unsigned int);
  ///@}

protected:
  vtkTableBasedClipDataSet(vtkImplicitFunction* cf = nullptr);
  ~vtkTableBasedClipDataSet() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  /**
   * This function resorts to the sibling class vtkClipDataSet to handle
   * special grids (such as cylinders or cones with capping faces in the
   * form a vtkPolyData).
   */
  void ClipDataSet(vtkDataSet* pDataSet, vtkUnstructuredGrid* outputUG);

  /**
   * This function clips a vtkImageData object based on a specified iso-value
   * (isoValue) using a scalar point data array that is either just an
   * input scalar point data array or the result of evaluating an implicit function
   * (provided via SetClipFunction()). The clipping result is exported to outputUG.
   */
  void ClipImageData(vtkDataSet* inputGrid, vtkImplicitFunction* implicitFunction,
    vtkDoubleArray* scalars, double isoValue, vtkUnstructuredGrid* outputUG);

  /**
   * This function clips a vtkPolyData object based on a specified iso-value
   * (isoValue) using a scalar point data array that is either just an
   * input scalar point data array or the result of evaluating an implicit function
   * (provided via SetClipFunction()). The clipping result is exported to outputUG.
   */
  void ClipPolyData(vtkDataSet* inputGrid, vtkImplicitFunction* implicitFunction,
    vtkDoubleArray* scalars, double isoValue, vtkUnstructuredGrid* outputUG);

  /**
   * This function clips a vtkRectilinear object based on a specified iso-value
   * (isoValue) using a scalar point data array that is either just an
   * input scalar point data array or the result of evaluating an implicit function
   * (provided via SetClipFunction()). The clipping result is exported to outputUG.
   */
  void ClipRectilinearGrid(vtkDataSet* inputGrid, vtkImplicitFunction* implicitFunction,
    vtkDoubleArray* scalars, double isoValue, vtkUnstructuredGrid* outputUG);

  /**
   * This function clips a vtkStructuredGrid object based on a specified iso-value
   * (isoValue) using a scalar point data array that is either just an
   * input scalar point data array or the result of evaluating an implicit function
   * (provided via SetClipFunction()). The clipping result is exported to outputUG.
   */
  void ClipStructuredGrid(vtkDataSet* inputGrid, vtkImplicitFunction* implicitFunction,
    vtkDoubleArray* scalars, double isoValue, vtkUnstructuredGrid* outputUG);

  /**
   * This function clips a vtkUnstructuredGrid based on a specified iso-value
   * (isoValue) using a scalar point data array that is either just an
   * input scalar point data array or the result of evaluating an implicit function
   * (provided via SetClipFunction()). The clipping result is exported to outputUG.
   */
  void ClipUnstructuredGrid(vtkDataSet* inputGrid, vtkImplicitFunction* implicitFunction,
    vtkDoubleArray* scalars, double isoValue, vtkUnstructuredGrid* outputUG);

  /**
   * This function checks if vtkPolyData/vtkUnstructuredGrid have cells that can be handled
   * by this filter or vtkClipDataSet needs to be used internally.
   */
  bool CanFullyProcessUnstructuredData(vtkDataSet* inputGrid);

  /**
   * Register a callback function with the InternalProgressObserver.
   */
  static void InternalProgressCallbackFunction(vtkObject*, unsigned long, void* clientdata, void*);

  /**
   * The actual operation executed by the callback function.
   */
  void InternalProgressCallback(vtkAlgorithm* algorithm);

protected:
  vtkTypeBool InsideOut;
  vtkTypeBool GenerateClipScalars;
  vtkTypeBool GenerateClippedOutput;
  bool UseValueAsOffset;
  double Value;
  double MergeTolerance;
  vtkCallbackCommand* InternalProgressObserver;
  vtkImplicitFunction* ClipFunction;
  unsigned int BatchSize;

  int OutputPointsPrecision;

private:
  vtkTableBasedClipDataSet(const vtkTableBasedClipDataSet&) = delete;
  void operator=(const vtkTableBasedClipDataSet&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
