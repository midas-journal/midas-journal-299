/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: itkQuadEdgeMeshDelaunayConformingFilter.txx,v $
  Language:  C++
  Date:      $Date: 2008-10-09 14:33:44 $
  Version:   $Revision: 1.8 $

  Copyright (c) Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/

#ifndef __itkQuadEdgeMeshDelaunayConformingFilter_txx
#define __itkQuadEdgeMeshDelaunayConformingFilter_txx

#include "itkQuadEdgeMeshDelaunayConformingFilter.h"

namespace itk
{

// ---------------------------------------------------------------------
template< typename TInputMesh, typename TOutputMesh >
QuadEdgeMeshDelaunayConformingFilter< TInputMesh, TOutputMesh >::
  QuadEdgeMeshDelaunayConformingFilter( ) : Superclass( ), m_NumberOfEdgeFlips( 0 )
{
  m_FlipEdge = FlipEdgeFunctionType::New( );
  m_PriorityQueue = PriorityQueueType::New( );
}

// ---------------------------------------------------------------------
template< typename TInputMesh, typename TOutputMesh >
QuadEdgeMeshDelaunayConformingFilter< TInputMesh, TOutputMesh >::
~QuadEdgeMeshDelaunayConformingFilter( )
{
  OutputEdgeCellType* edge;

  while( !m_PriorityQueue->Empty() )
    {
    edge = m_PriorityQueue->Peek( )->m_Element;

    m_PriorityQueue->Pop( );
    delete m_QueueMapper[edge];
    m_QueueMapper.erase( edge );
    }
}
// ---------------------------------------------------------------------
template< typename TInputMesh, typename TOutputMesh >
void QuadEdgeMeshDelaunayConformingFilter< TInputMesh, TOutputMesh >::
InitializePriorityQueue()
{
  OutputMeshType* output = this->GetOutput( );
    
  OutputEdgeCellType* edge = 0;

  CriterionValueType value = 0.; 

  for( OutputCellsContainerIterator
        outCellIterator = output->GetEdgeCells( )->Begin( );
      outCellIterator != output->GetEdgeCells( )->End( );
      ++outCellIterator )
    {
    if( ( edge = dynamic_cast< OutputEdgeCellType* >(
      outCellIterator.Value( ) ) ) )
      {
      value = Dyer07Criterion( output, edge->GetQEGeom( ) );

      if( value > 0.0 )
        {
        PriorityQueueItemType* qi =
          new PriorityQueueItemType( edge, PriorityType( true, value ) );
        m_QueueMapper[ edge ] = qi;
        m_PriorityQueue->Push( qi );
        }
      }
    }

  OutputEdgeCellListIterator const_edge_it = 
    m_ListOfConstrainedEdges.begin();

  while( const_edge_it != m_ListOfConstrainedEdges.end() )
    {
    QueueMapIterator queue_it = m_QueueMapper.find( *const_edge_it );

    if( queue_it == m_QueueMapper.end() )
      {
      queue_it->second->m_Priority = PriorityType( false, value );
      m_PriorityQueue->Update( queue_it->second );
      }
    else
      {
      PriorityQueueItemType* qi = 
        new PriorityQueueItemType( edge, PriorityType( false, 0.0 ) );
      m_QueueMapper[ edge ] = qi;
      m_PriorityQueue->Push( qi );
      }

    ++const_edge_it;
    }
}

// ---------------------------------------------------------------------
template< typename TInputMesh, typename TOutputMesh >
void QuadEdgeMeshDelaunayConformingFilter< TInputMesh, TOutputMesh >::
Process( )
{
  OutputMeshType* output = this->GetOutput( );
  m_FlipEdge->SetInput( output );

  typename std::vector< OutputQEType* > list_qe( 5 );
  typename std::vector< OutputQEType* >::iterator it;

  OutputEdgeCellType* edge;
  OutputQEType* qe;
  OutputQEType* e_it;

  CriterionValueType value;

  while( !m_PriorityQueue->Empty( ) )
    {

    if( !m_PriorityQueue->Peek( )->m_Priority.first )
      {
      break;
      }
          
    edge = m_PriorityQueue->Peek( )->m_Element;
    qe = edge->GetQEGeom( );

    list_qe[0] = qe->GetLnext( );
    list_qe[1] = qe->GetLprev( );
    list_qe[2] = qe->GetRnext( );
    list_qe[3] = qe->GetRprev( );

    m_PriorityQueue->Pop( );
    delete m_QueueMapper[edge];
    m_QueueMapper.erase( edge );

    qe = m_FlipEdge->Evaluate( qe );
    if( qe != 0 )
      {
      ++m_NumberOfEdgeFlips;
      list_qe[4] = qe;

      for( it = list_qe.begin( ); it != list_qe.end( ); ++it )
        {
        e_it = *it;
        if( e_it )
          {
          value = Dyer07Criterion( output, e_it );
          if( value > 0.0 )
            {
            edge = output->FindEdgeCell( e_it->GetOrigin( ),
              e_it->GetDestination( ) );
            QueueMapIterator queue_it = m_QueueMapper.find( edge );
            if( queue_it == m_QueueMapper.end() )
              {
              PriorityQueueItemType* qi =
                new PriorityQueueItemType( edge,
                  PriorityType( true, value ) );
              m_QueueMapper[ edge ] = qi;
              m_PriorityQueue->Push( qi );
              }
            else
              {
              if( queue_it->second->m_Priority.first )
                {
                queue_it->second->m_Priority = PriorityType( true, value );
                m_PriorityQueue->Update( queue_it->second );
                }
              }
            }
          }
        }
      }
    }
}

  
// ---------------------------------------------------------------------
template< typename TInputMesh, typename TOutputMesh >
void QuadEdgeMeshDelaunayConformingFilter< TInputMesh, TOutputMesh >::
GenerateData( )
{
  Superclass::GenerateData( );

  // initialize all required instances
  m_NumberOfEdgeFlips = 0;

  InitializePriorityQueue();
  Process();
}

} // end namespace itk

#endif
