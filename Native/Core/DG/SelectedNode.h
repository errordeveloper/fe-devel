/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_DG_SELECTED_NODE_H
#define _FABRIC_DG_SELECTED_NODE_H

#include <Fabric/Base/RC/Handle.h>
#include <Fabric/Base/RC/ConstHandle.h>
#include <Fabric/Core/RT/Desc.h>
#include <Fabric/Core/DG/Node.h>

#include <vector>
#include <list>

namespace Fabric
{
  namespace DG
  {
    class Node;
    
    struct SelectedNode
    {
      RC::Handle<Node> node;
      std::vector<uint8_t> data;
      
      SelectedNode()
      {
      }
      
      SelectedNode( SelectedNode const &that )
        : node( that.node )
        , data( that.data )
      {
      }
      
      SelectedNode( RC::Handle<Node> const &_node, RC::ConstHandle<RT::Desc> desc )
        : node( _node )
      {
        data.resize( desc->getAllocSize() );
      }
      
      SelectedNode &operator =( SelectedNode const &that )
      {
        node = that.node;
        data = that.data;
        return *this;
      }
      
      ~SelectedNode()
      {
      }
    };

    typedef std::list<SelectedNode> SelectedNodeList;
  };
};

#endif //_FABRIC_DG_SELECTED_NODE_H
