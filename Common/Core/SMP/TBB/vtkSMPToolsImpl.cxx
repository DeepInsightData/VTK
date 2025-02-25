/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMPToolsImpl.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "SMP/Common/vtkSMPToolsImpl.h"
#include "SMP/TBB/vtkSMPToolsImpl.txx"

#include <cstdlib> // For std::getenv()
#include <mutex>   // For std::mutex
#include <stack>   // For std::stack

#ifdef _MSC_VER
#pragma push_macro("__TBB_NO_IMPLICIT_LINKAGE")
#define __TBB_NO_IMPLICIT_LINKAGE 1
#endif

#include <tbb/task_arena.h> // For tbb:task_arena

#ifdef _MSC_VER
#pragma pop_macro("__TBB_NO_IMPLICIT_LINKAGE")
#endif

namespace vtk
{
namespace detail
{
namespace smp
{
VTK_ABI_NAMESPACE_BEGIN

static tbb::task_arena* taskArena;
static std::mutex* vtkSMPToolsCS;
static std::stack<int>* threadIdStack;
static std::mutex* threadIdStackLock;

//------------------------------------------------------------------------------
// Must NOT be initialized. Default initialization to zero is necessary.
unsigned int vtkSMPToolsImplTBBInitializeCount;

//------------------------------------------------------------------------------
vtkSMPToolsImplTBBInitialize::vtkSMPToolsImplTBBInitialize()
{
  if (++vtkSMPToolsImplTBBInitializeCount == 1)
  {
    taskArena = new tbb::task_arena;
    vtkSMPToolsCS = new std::mutex;
    threadIdStack = new std::stack<int>;
    threadIdStackLock = new std::mutex;
  }
}

//------------------------------------------------------------------------------
vtkSMPToolsImplTBBInitialize::~vtkSMPToolsImplTBBInitialize()
{
  if (--vtkSMPToolsImplTBBInitializeCount == 0)
  {
    delete taskArena;
    taskArena = nullptr;

    delete vtkSMPToolsCS;
    vtkSMPToolsCS = nullptr;

    delete threadIdStack;
    threadIdStack = nullptr;

    delete threadIdStackLock;
    threadIdStackLock = nullptr;
  }
}

//------------------------------------------------------------------------------
template <>
vtkSMPToolsImpl<BackendType::TBB>::vtkSMPToolsImpl()
  : NestedActivated(true)
{
}

//------------------------------------------------------------------------------
template <>
void vtkSMPToolsImpl<BackendType::TBB>::Initialize(int numThreads)
{
  vtkSMPToolsCS->lock();

  if (numThreads == 0)
  {
    const char* vtkSmpNumThreads = std::getenv("VTK_SMP_MAX_THREADS");
    if (vtkSmpNumThreads)
    {
      numThreads = std::atoi(vtkSmpNumThreads);
    }
    else if (taskArena->is_active())
    {
      taskArena->terminate();
    }
  }
  if (numThreads > 0 && numThreads != taskArena->max_concurrency())
  {
    if (taskArena->is_active())
    {
      taskArena->terminate();
    }
    taskArena->initialize(numThreads);
  }

  vtkSMPToolsCS->unlock();
}

//------------------------------------------------------------------------------
template <>
int vtkSMPToolsImpl<BackendType::TBB>::GetEstimatedNumberOfThreads()
{
  return taskArena->max_concurrency();
}

//------------------------------------------------------------------------------
template <>
bool vtkSMPToolsImpl<BackendType::TBB>::GetSingleThread()
{
  return threadIdStack->top() == tbb::this_task_arena::current_thread_index();
}

//------------------------------------------------------------------------------
void vtkSMPToolsImplForTBB(vtkIdType first, vtkIdType last, vtkIdType grain,
  ExecuteFunctorPtrType functorExecuter, void* functor)
{
  threadIdStackLock->lock();
  threadIdStack->emplace(tbb::this_task_arena::current_thread_index());
  threadIdStackLock->unlock();

  if (taskArena->is_active())
  {
    taskArena->execute([&] { functorExecuter(functor, first, last, grain); });
  }
  else
  {
    functorExecuter(functor, first, last, grain);
  }

  threadIdStackLock->lock();
  threadIdStack->pop();
  threadIdStackLock->unlock();
}

VTK_ABI_NAMESPACE_END
} // namespace smp
} // namespace detail
} // namespace vtk
