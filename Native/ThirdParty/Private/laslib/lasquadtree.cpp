/*
===============================================================================

  FILE:  lasquadtree.cpp
  
  CONTENTS:
  
    see corresponding header file
  
  PROGRAMMERS:
  
    martin.isenburg@gmail.com
  
  COPYRIGHT:
  
    (c) 2011, Martin Isenburg, LASSO - tools to catch reality

    This software is distributed WITHOUT ANY WARRANTY and without even the
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  
  CHANGE HISTORY:
  
    see corresponding header file
  
===============================================================================
*/
#include "lasquadtree.hpp"

#include "bytestreamin.hpp"
#include "bytestreamout.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <vector.h>

typedef vector<I32> my_cell_vector;

/*

class LAScell
{
public:
  LAScell* parent;
  LAScell* child[4];
  U32 num_children :  3;
  U32 level        :  5;
  U32 idx          : 24;
  F32 mid_x;
  F32 mid_y;
  F32 half_size;
  F32 get_min_x() const { return mid_x - half_size; };
  F32 get_min_y() const { return mid_y - half_size; };
  F32 get_max_x() const { return mid_x + half_size; };
  F32 get_max_y() const { return mid_y + half_size; };
  LAScell();
};

LAScell::LAScell()
{
  memset(this, 0, sizeof(LAScell));
}

  LAScell* get_cell(const F64 x, const F64 y);
LAScell* LASquadtree::get_cell(const F64 x, const F64 y)
{
  LAScell* cell = &root;
  while (cell && cell->num_children)
  {
    int child = 0;
    if (x < cell->mid_x) // only if strictly less
    {
      child |= 1;
    }
    if (!(y < cell->mid_y)) // only if not strictly less
    {
      child |= 2;
    }
    cell = cell->child[child];
  }
  return cell;
}

static bool intersect_point(LAScell* cell, const float* p_pos)
{
  if (cell->num_children) // check if cell has children
  {
    int idx = 0;
    if (p_pos[0] < cell->r_mid[0]) idx |= 1;
    if (!(p_pos[1] < cell->r_mid[1])) idx |= 2;
    if (cell->child[idx] && intersect_point(cell->child[idx], p_pos)) return true;
    return false;
  }
  return true;
}
*/

// returns the bounding box of the cell that x & y fall into at the specified level
void LASquadtree::get_cell_bounding_box(const F64 x, const F64 y, U32 level, F32* min, F32* max) const
{
  volatile float cell_mid_x;
  volatile float cell_mid_y;
  float cell_min_x, cell_max_x;
  float cell_min_y, cell_max_y;
  
  cell_min_x = min_x;
  cell_max_x = max_x;
  cell_min_y = min_y;
  cell_max_y = max_y;

  while (level)
  {
    cell_mid_x = (cell_min_x + cell_max_x)/2;
    cell_mid_y = (cell_min_y + cell_max_y)/2;
    if (x < cell_mid_x)
    {
      cell_max_x = cell_mid_x;
    }
    else
    {
      cell_min_x = cell_mid_x;
    }
    if (y < cell_mid_y)
    {
      cell_max_y = cell_mid_y;
    }
    else
    {
      cell_min_y = cell_mid_y;
    }
    level--;
  }
  if (min)
  {
    min[0] = cell_min_x;
    min[1] = cell_min_y;
  }
  if (max)
  {
    max[0] = cell_max_x;
    max[1] = cell_max_y;
  }
}

// returns the bounding box of the cell that x & y fall into
void LASquadtree::get_cell_bounding_box(const F64 x, const F64 y, F32* min, F32* max) const
{
  get_cell_bounding_box(x, y, levels, min, max);
}

// returns the bounding box of the cell with the spedified level_index at the specified level
void LASquadtree::get_cell_bounding_box(U32 level_index, U32 level, F32* min, F32* max) const
{
  volatile float cell_mid_x;
  volatile float cell_mid_y;
  float cell_min_x, cell_max_x;
  float cell_min_y, cell_max_y;
  
  cell_min_x = min_x;
  cell_max_x = max_x;
  cell_min_y = min_y;
  cell_max_y = max_y;

  U32 index;
  while (level)
  {
    index = (level_index >>(2*(level-1)))&3;
    cell_mid_x = (cell_min_x + cell_max_x)/2;
    cell_mid_y = (cell_min_y + cell_max_y)/2;
    if (index & 1)
    {
      cell_min_x = cell_mid_x;
    }
    else
    {
      cell_max_x = cell_mid_x;
    }
    if (index & 2)
    {
      cell_min_y = cell_mid_y;
    }
    else
    {
      cell_max_y = cell_mid_y;
    }
    level--;
  }
  if (min)
  {
    min[0] = cell_min_x;
    min[1] = cell_min_y;
  }
  if (max)
  {
    max[0] = cell_max_x;
    max[1] = cell_max_y;
  }
}

// returns the bounding box of the cell with the spedified level_index
void LASquadtree::get_cell_bounding_box(U32 level_index, F32* min, F32* max) const
{
  get_cell_bounding_box(level_index, levels, min, max);
}

// returns the bounding box of the cell that x & y fall into
void LASquadtree::get_cell_bounding_box(const I32 cell_index, F32* min, F32* max) const
{
  U32 level = get_level((U32)cell_index);
  U32 level_index = get_level_index((U32)cell_index, level);
  get_cell_bounding_box(level_index, level, min, max);
}

// returns the (sub-)level index of the cell that x & y fall into at the specified level
U32 LASquadtree::get_level_index(const F64 x, const F64 y, U32 level) const
{
  volatile float cell_mid_x;
  volatile float cell_mid_y;
  float cell_min_x, cell_max_x;
  float cell_min_y, cell_max_y;
  
  cell_min_x = min_x;
  cell_max_x = max_x;
  cell_min_y = min_y;
  cell_max_y = max_y;

  U32 level_index = 0;

  while (level)
  {
    level_index <<= 2;

    cell_mid_x = (cell_min_x + cell_max_x)/2;
    cell_mid_y = (cell_min_y + cell_max_y)/2;

    if (x < cell_mid_x)
    {
      cell_max_x = cell_mid_x;
    }
    else
    {
      cell_min_x = cell_mid_x;
      level_index |= 1;
    }
    if (y < cell_mid_y)
    {
      cell_max_y = cell_mid_y;
    }
    else
    {
      cell_min_y = cell_mid_y;
      level_index |= 2;
    }
    level--;
  }

  return level_index;
}

// returns the (sub-)level index of the cell that x & y fall into
U32 LASquadtree::get_level_index(const F64 x, const F64 y) const
{
  return get_level_index(x, y, levels);
}

// returns the index of the cell that x & y fall into at the specified level
U32 LASquadtree::get_cell_index(const F64 x, const F64 y, U32 level) const
{
  if (sub_level)
  {
    return level_offset[sub_level+level] + (sub_level_index << (level*2)) + get_level_index(x, y, level);
  }
  else
  {
    return level_offset[level]+get_level_index(x, y, level);
  }
}

// returns the index of the cell that x & y fall into
U32 LASquadtree::get_cell_index(const F64 x, const F64 y) const
{
  return get_cell_index(x, y, levels);
}

// returns the indices of parent and siblings for the specified cell index
BOOL LASquadtree::coarsen(const I32 cell_index, I32* coarser_cell_index, U32* num_cell_indices, I32** cell_indices) const
{
  if (cell_index < 0) return FALSE;
  U32 level = get_level((U32)cell_index);
  if (level == 0) return FALSE;
  U32 level_index = get_level_index((U32)cell_index, level);
  level_index = level_index >> 2;
  if (coarser_cell_index) (*coarser_cell_index) = get_cell_index(level_index, level-1);
  if (num_cell_indices && cell_indices)
  {
    (*num_cell_indices) = 4;
    (*cell_indices) = (I32*)coarser_indices;
    level_index = level_index << 2;
    (*cell_indices)[0] = get_cell_index(level_index + 0, level);
    (*cell_indices)[1] = get_cell_index(level_index + 1, level);
    (*cell_indices)[2] = get_cell_index(level_index + 2, level);
    (*cell_indices)[3] = get_cell_index(level_index + 3, level);
  }
  return TRUE;
}

// returns the level index of the cell index at the specified level
U32 LASquadtree::get_level_index(U32 cell_index, U32 level) const
{
  if (sub_level)
  {
    return cell_index - (sub_level_index << (level*2)) - level_offset[sub_level+level];
  }
  else
  {
    return cell_index - level_offset[level];
  }
}

// returns the level index of the cell index
U32 LASquadtree::get_level_index(U32 cell_index) const
{
  return get_level_index(cell_index, levels);
}

// returns the level the cell index
U32 LASquadtree::get_level(U32 cell_index) const
{
  int level = 0;
  while (cell_index >= level_offset[level+1]) level++;
  return level;
}

// returns the cell index of the level index at the specified level
U32 LASquadtree::get_cell_index(U32 level_index, U32 level) const
{
  if (sub_level)
  {
    return level_index + (sub_level_index << (level*2)) + level_offset[sub_level+level];
  }
  else
  {
    return level_index + level_offset[level];
  }
}

// returns the cell index of the level index
U32 LASquadtree::get_cell_index(U32 level_index) const
{
  return get_cell_index(level_index, levels);
}

// returns the maximal level index at the specified level
U32 LASquadtree::get_max_level_index(U32 level) const
{
  return (1<<level)*(1<<level);
}

// returns the maximal level index
U32 LASquadtree::get_max_level_index() const
{
  return get_max_level_index(levels);
}

// returns the maximal cell index at the specified level
U32 LASquadtree::get_max_cell_index(U32 level) const
{
  return level_offset[level+1]-1;
}

// returns the maximal cell index
U32 LASquadtree::get_max_cell_index() const
{
  return get_max_cell_index(levels);
}

// read from file
BOOL LASquadtree::read(ByteStreamIn* stream)
{
  // read data in the following order
  //     U32  levels          4 bytes 
  //     U32  level_index     4 bytes (default 0)
  //     U32  implicit_levels 4 bytes (only used when level_index != 0))
  //     F32  min_x           4 bytes 
  //     F32  max_x           4 bytes 
  //     F32  min_y           4 bytes 
  //     F32  max_y           4 bytes 
  // which totals 28 bytes

  if (!stream->get32bitsLE((U8*)&levels))
  {
    fprintf(stderr,"ERROR (LASquadtree): reading levels\n");
    return FALSE;
  }
  U32 level_index;
  if (!stream->get32bitsLE((U8*)&level_index))
  {
    fprintf(stderr,"ERROR (LASquadtree): reading level_index\n");
    return FALSE;
  }
  U32 implicit_levels;
  if (!stream->get32bitsLE((U8*)&implicit_levels))
  {
    fprintf(stderr,"ERROR (LASquadtree): reading implicit_levels\n");
    return FALSE;
  }
  if (!stream->get32bitsLE((U8*)&min_x))
  {
    fprintf(stderr,"ERROR (LASquadtree): reading min_x\n");
    return FALSE;
  }
  if (!stream->get32bitsLE((U8*)&max_x))
  {
    fprintf(stderr,"ERROR (LASquadtree): reading max_x\n");
    return FALSE;
  }
  if (!stream->get32bitsLE((U8*)&min_y))
  {
    fprintf(stderr,"ERROR (LASquadtree): reading min_y\n");
    return FALSE;
  }
  if (!stream->get32bitsLE((U8*)&max_y))
  {
    fprintf(stderr,"ERROR (LASquadtree): reading max_y\n");
    return FALSE;
  }
  return TRUE;
}

BOOL LASquadtree::write(ByteStreamOut* stream) const
{
  // which totals 20 bytes
  //     U32  levels          4 bytes 
  //     U32  level_index     4 bytes (default 0)
  //     U32  implicit_levels 4 bytes (only used when level_index != 0))
  //     F32  min_x           4 bytes 
  //     F32  max_x           4 bytes 
  //     F32  min_y           4 bytes 
  //     F32  max_y           4 bytes 
  // which totals 28 bytes

  if (!stream->put32bitsLE((U8*)&levels))
  {
    fprintf(stderr,"ERROR (LASquadtree): writing levels %u\n", levels);
    return FALSE;
  }
  U32 level_index = 0;
  if (!stream->put32bitsLE((U8*)&level_index))
  {
    fprintf(stderr,"ERROR (LASquadtree): writing level_index %u\n", level_index);
    return FALSE;
  }
  U32 implicit_levels = 0;
  if (!stream->put32bitsLE((U8*)&implicit_levels))
  {
    fprintf(stderr,"ERROR (LASquadtree): writing implicit_levels %u\n", implicit_levels);
    return FALSE;
  }
  if (!stream->put32bitsLE((U8*)&min_x))
  {
    fprintf(stderr,"ERROR (LASquadtree): writing min_x %g\n", min_x);
    return FALSE;
  }
  if (!stream->put32bitsLE((U8*)&max_x))
  {
    fprintf(stderr,"ERROR (LASquadtree): writing max_x %g\n", max_x);
    return FALSE;
  }
  if (!stream->put32bitsLE((U8*)&min_y))
  {
    fprintf(stderr,"ERROR (LASquadtree): writing min_y %g\n", min_y);
    return FALSE;
  }
  if (!stream->put32bitsLE((U8*)&max_y))
  {
    fprintf(stderr,"ERROR (LASquadtree): writing max_y %g\n", max_y);
    return FALSE;
  }
  return TRUE;
}

// create or finalize the cell (in the spatial hierarchy) 
BOOL LASquadtree::manage_cell(const U32 cell_index, const BOOL finalize)
{
  U32 adaptive_pos = cell_index/32;
  U32 adaptive_bit = ((U32)1) << (cell_index%32);
  if (adaptive_pos >= adaptive_alloc)
  {
    if (adaptive)
    {
      adaptive = (U32*)realloc(adaptive, adaptive_pos*2*sizeof(U32));
      for (U32 i = adaptive_alloc; i < adaptive_pos*2; i++) adaptive[i] = 0;
      adaptive_alloc = adaptive_pos*2;
    }
    else
    {
      adaptive = (U32*)malloc((adaptive_pos+1)*sizeof(U32));
      for (U32 i = adaptive_alloc; i <= adaptive_pos; i++) adaptive[i] = 0;
      adaptive_alloc = adaptive_pos+1;
    }
  }
  adaptive[adaptive_pos] &= ~adaptive_bit;
  U32 index;
  U32 level = get_level(cell_index);
  U32 level_index = get_level_index(cell_index, level);
  while (level)
  {
    level--;
    level_index = level_index >> 2;
    index = get_cell_index(level_index, level);
    adaptive_pos = index/32;
    adaptive_bit = ((U32)1) << (index%32);
    if (adaptive[adaptive_pos] & adaptive_bit) break;
    adaptive[adaptive_pos] |= adaptive_bit;
  }
  return TRUE;
}

// check whether the x & y coordinates fall into the tiling
BOOL LASquadtree::inside(const F64 x, const F64 y) const
{
  if (sub_level)
  {
    return (min_x <= x && x < max_x && min_y <= y && y < max_y);
  }
  else
  {
    return (min_x <= x && x <= max_x && min_y <= y && y <= max_y);
  }
}

U32 LASquadtree::intersect_rectangle(const F64 r_min_x, const F64 r_min_y, const F64 r_max_x, const F64 r_max_y, U32 level)
{
  if (intersected_cells == 0)
  {
    intersected_cells = (void*) new my_cell_vector;
  }
  else
  {
    ((my_cell_vector*)intersected_cells)->clear();
  }

  if (r_max_x < min_x || !(r_min_x < max_x) || r_max_y < min_y || !(r_min_y < max_y))
  {
    return 0;
  }

  if (adaptive)
  {
    intersect_rectangle_with_cells_adaptive(r_min_x, r_min_y, r_max_x, r_max_y, min_x, max_x, min_y, max_y, 0, 0);
  }
  else
  {
    intersect_rectangle_with_cells(r_min_x, r_min_y, r_max_x, r_max_y, min_x, max_x, min_y, max_y, level, 0);
  }

  return (U32)(((my_cell_vector*)intersected_cells)->size());
}

U32 LASquadtree::intersect_rectangle(const F64 r_min_x, const F64 r_min_y, const F64 r_max_x, const F64 r_max_y)
{
  return intersect_rectangle(r_min_x, r_min_y, r_max_x, r_max_y, levels);
}

U32 LASquadtree::intersect_tile(const F32 ll_x, const F32 ll_y, const F32 size, U32 level)
{
  if (intersected_cells == 0)
  {
    intersected_cells = (void*) new my_cell_vector;
  }
  else
  {
    ((my_cell_vector*)intersected_cells)->clear();
  }

  volatile F32 ur_x = ll_x + size;
  volatile F32 ur_y = ll_y + size;

  if (ur_x <= min_x || !(ll_x <= max_x) || ur_y <= min_y || !(ll_y <= max_y))
  {
    return 0;
  }

  if (adaptive)
  {
    intersect_tile_with_cells_adaptive(ll_x, ll_y, ur_x, ur_y, min_x, max_x, min_y, max_y, 0, 0);
  }
  else
  {
    intersect_tile_with_cells(ll_x, ll_y, ur_x, ur_y, min_x, max_x, min_y, max_y, level, 0);
  }

  return (U32)(((my_cell_vector*)intersected_cells)->size());
}

U32 LASquadtree::intersect_tile(const F32 ll_x, const F32 ll_y, const F32 size)
{
  return intersect_tile(ll_x, ll_y, size, levels);
}

U32 LASquadtree::intersect_circle(const F64 center_x, const F64 center_y, const F64 radius, U32 level)
{
  if (intersected_cells == 0)
  {
    intersected_cells = (void*) new my_cell_vector;
  }
  else
  {
    ((my_cell_vector*)intersected_cells)->clear();
  }

  F64 r_min_x = center_x - radius; 
  F64 r_min_y = center_y - radius; 
  F64 r_max_x = center_x + radius;
  F64 r_max_y = center_y + radius;

  if (r_max_x < min_x || !(r_min_x < max_x) || r_max_y < min_y || !(r_min_y < max_y))
  {
    return 0;
  }

  if (adaptive)
  {
    intersect_circle_with_cells_adaptive(center_x, center_y, radius, r_min_x, r_min_y, r_max_x, r_max_y, min_x, max_x, min_y, max_y, 0, 0);
  }
  else
  {
    intersect_circle_with_cells(center_x, center_y, radius, r_min_x, r_min_y, r_max_x, r_max_y, min_x, max_x, min_y, max_y, level, 0);
  }

  return (U32)(((my_cell_vector*)intersected_cells)->size());
}

U32 LASquadtree::intersect_circle(const F64 center_x, const F64 center_y, const F64 radius)
{
  return intersect_circle(center_x, center_y, radius, levels);
}

void LASquadtree::intersect_rectangle_with_cells(const F64 r_min_x, const F64 r_min_y, const F64 r_max_x, const F64 r_max_y, const F32 cell_min_x, const F32 cell_max_x, const F32 cell_min_y, const F32 cell_max_y, U32 level, U32 level_index)
{
  volatile float cell_mid_x;
  volatile float cell_mid_y;
  if (level)
  {
    level--;
    level_index <<= 2;

    cell_mid_x = (cell_min_x + cell_max_x)/2;
    cell_mid_y = (cell_min_y + cell_max_y)/2;

    if (r_max_x < cell_mid_x)
    {
      // cell_max_x = cell_mid_x;
      if (r_max_y < cell_mid_y)
      {
        // cell_max_y = cell_mid_y;
        intersect_rectangle_with_cells(r_min_x, r_min_y, r_max_x, r_max_y, cell_min_x, cell_mid_x, cell_min_y, cell_mid_y, level, level_index);
      }
      else if (!(r_min_y < cell_mid_y))
      {
        // cell_min_y = cell_mid_y;
        // level_index |= 1;
        intersect_rectangle_with_cells(r_min_x, r_min_y, r_max_x, r_max_y, cell_min_x, cell_mid_x, cell_mid_y, cell_max_y, level, level_index | 2);
      }
      else
      {
        intersect_rectangle_with_cells(r_min_x, r_min_y, r_max_x, r_max_y, cell_min_x, cell_mid_x, cell_min_y, cell_mid_y, level, level_index);
        intersect_rectangle_with_cells(r_min_x, r_min_y, r_max_x, r_max_y, cell_min_x, cell_mid_x, cell_mid_y, cell_max_y, level, level_index | 2);
      }
    }
    else if (!(r_min_x < cell_mid_x))
    {
      // cell_min_x = cell_mid_x;
      // level_index |= 1;
      if (r_max_y < cell_mid_y)
      {
        // cell_max_y = cell_mid_y;
        intersect_rectangle_with_cells(r_min_x, r_min_y, r_max_x, r_max_y, cell_mid_x, cell_max_x, cell_min_y, cell_mid_y, level, level_index | 1);
      }
      else if (!(r_min_y < cell_mid_y))
      {
        // cell_min_y = cell_mid_y;
        // level_index |= 1;
        intersect_rectangle_with_cells(r_min_x, r_min_y, r_max_x, r_max_y, cell_mid_x, cell_max_x, cell_mid_y, cell_max_y, level, level_index | 3);
      }
      else
      {
        intersect_rectangle_with_cells(r_min_x, r_min_y, r_max_x, r_max_y, cell_mid_x, cell_max_x, cell_min_y, cell_mid_y, level, level_index | 1);
        intersect_rectangle_with_cells(r_min_x, r_min_y, r_max_x, r_max_y, cell_mid_x, cell_max_x, cell_mid_y, cell_max_y, level, level_index | 3);
      }
    }
    else
    {
      if (r_max_y < cell_mid_y)
      {
        // cell_max_y = cell_mid_y;
        intersect_rectangle_with_cells(r_min_x, r_min_y, r_max_x, r_max_y, cell_min_x, cell_mid_x, cell_min_y, cell_mid_y, level, level_index);
        intersect_rectangle_with_cells(r_min_x, r_min_y, r_max_x, r_max_y, cell_mid_x, cell_max_x, cell_min_y, cell_mid_y, level, level_index | 1);
      }
      else if (!(r_min_y < cell_mid_y))
      {
        // cell_min_y = cell_mid_y;
        // level_index |= 1;
        intersect_rectangle_with_cells(r_min_x, r_min_y, r_max_x, r_max_y, cell_min_x, cell_mid_x, cell_mid_y, cell_max_y, level, level_index | 2);
        intersect_rectangle_with_cells(r_min_x, r_min_y, r_max_x, r_max_y, cell_mid_x, cell_max_x, cell_mid_y, cell_max_y, level, level_index | 3);
      }
      else
      {
        intersect_rectangle_with_cells(r_min_x, r_min_y, r_max_x, r_max_y, cell_min_x, cell_mid_x, cell_min_y, cell_mid_y, level, level_index);
        intersect_rectangle_with_cells(r_min_x, r_min_y, r_max_x, r_max_y, cell_mid_x, cell_max_x, cell_min_y, cell_mid_y, level, level_index | 1);
        intersect_rectangle_with_cells(r_min_x, r_min_y, r_max_x, r_max_y, cell_min_x, cell_mid_x, cell_mid_y, cell_max_y, level, level_index | 2);
        intersect_rectangle_with_cells(r_min_x, r_min_y, r_max_x, r_max_y, cell_mid_x, cell_max_x, cell_mid_y, cell_max_y, level, level_index | 3);
      }
    }
  }
  else
  {
    ((my_cell_vector*)intersected_cells)->push_back(level_index);
  }
}

void LASquadtree::intersect_rectangle_with_cells_adaptive(const F64 r_min_x, const F64 r_min_y, const F64 r_max_x, const F64 r_max_y, const F32 cell_min_x, const F32 cell_max_x, const F32 cell_min_y, const F32 cell_max_y, U32 level, U32 level_index)
{
  volatile float cell_mid_x;
  volatile float cell_mid_y;
  U32 cell_index = get_cell_index(level_index, level);
  U32 adaptive_pos = cell_index/32;
  U32 adaptive_bit = ((U32)1) << (cell_index%32);
  if (adaptive[adaptive_pos] & adaptive_bit)
  {
    level++;
    level_index <<= 2;

    cell_mid_x = (cell_min_x + cell_max_x)/2;
    cell_mid_y = (cell_min_y + cell_max_y)/2;

    if (r_max_x < cell_mid_x)
    {
      // cell_max_x = cell_mid_x;
      if (r_max_y < cell_mid_y)
      {
        // cell_max_y = cell_mid_y;
        intersect_rectangle_with_cells_adaptive(r_min_x, r_min_y, r_max_x, r_max_y, cell_min_x, cell_mid_x, cell_min_y, cell_mid_y, level, level_index);
      }
      else if (!(r_min_y < cell_mid_y))
      {
        // cell_min_y = cell_mid_y;
        // level_index |= 1;
        intersect_rectangle_with_cells_adaptive(r_min_x, r_min_y, r_max_x, r_max_y, cell_min_x, cell_mid_x, cell_mid_y, cell_max_y, level, level_index | 2);
      }
      else
      {
        intersect_rectangle_with_cells_adaptive(r_min_x, r_min_y, r_max_x, r_max_y, cell_min_x, cell_mid_x, cell_min_y, cell_mid_y, level, level_index);
        intersect_rectangle_with_cells_adaptive(r_min_x, r_min_y, r_max_x, r_max_y, cell_min_x, cell_mid_x, cell_mid_y, cell_max_y, level, level_index | 2);
      }
    }
    else if (!(r_min_x < cell_mid_x))
    {
      // cell_min_x = cell_mid_x;
      // level_index |= 1;
      if (r_max_y < cell_mid_y)
      {
        // cell_max_y = cell_mid_y;
        intersect_rectangle_with_cells_adaptive(r_min_x, r_min_y, r_max_x, r_max_y, cell_mid_x, cell_max_x, cell_min_y, cell_mid_y, level, level_index | 1);
      }
      else if (!(r_min_y < cell_mid_y))
      {
        // cell_min_y = cell_mid_y;
        // level_index |= 1;
        intersect_rectangle_with_cells_adaptive(r_min_x, r_min_y, r_max_x, r_max_y, cell_mid_x, cell_max_x, cell_mid_y, cell_max_y, level, level_index | 3);
      }
      else
      {
        intersect_rectangle_with_cells_adaptive(r_min_x, r_min_y, r_max_x, r_max_y, cell_mid_x, cell_max_x, cell_min_y, cell_mid_y, level, level_index | 1);
        intersect_rectangle_with_cells_adaptive(r_min_x, r_min_y, r_max_x, r_max_y, cell_mid_x, cell_max_x, cell_mid_y, cell_max_y, level, level_index | 3);
      }
    }
    else
    {
      if (r_max_y < cell_mid_y)
      {
        // cell_max_y = cell_mid_y;
        intersect_rectangle_with_cells_adaptive(r_min_x, r_min_y, r_max_x, r_max_y, cell_min_x, cell_mid_x, cell_min_y, cell_mid_y, level, level_index);
        intersect_rectangle_with_cells_adaptive(r_min_x, r_min_y, r_max_x, r_max_y, cell_mid_x, cell_max_x, cell_min_y, cell_mid_y, level, level_index | 1);
      }
      else if (!(r_min_y < cell_mid_y))
      {
        // cell_min_y = cell_mid_y;
        // level_index |= 1;
        intersect_rectangle_with_cells_adaptive(r_min_x, r_min_y, r_max_x, r_max_y, cell_min_x, cell_mid_x, cell_mid_y, cell_max_y, level, level_index | 2);
        intersect_rectangle_with_cells_adaptive(r_min_x, r_min_y, r_max_x, r_max_y, cell_mid_x, cell_max_x, cell_mid_y, cell_max_y, level, level_index | 3);
      }
      else
      {
        intersect_rectangle_with_cells_adaptive(r_min_x, r_min_y, r_max_x, r_max_y, cell_min_x, cell_mid_x, cell_min_y, cell_mid_y, level, level_index);
        intersect_rectangle_with_cells_adaptive(r_min_x, r_min_y, r_max_x, r_max_y, cell_mid_x, cell_max_x, cell_min_y, cell_mid_y, level, level_index | 1);
        intersect_rectangle_with_cells_adaptive(r_min_x, r_min_y, r_max_x, r_max_y, cell_min_x, cell_mid_x, cell_mid_y, cell_max_y, level, level_index | 2);
        intersect_rectangle_with_cells_adaptive(r_min_x, r_min_y, r_max_x, r_max_y, cell_mid_x, cell_max_x, cell_mid_y, cell_max_y, level, level_index | 3);
      }
    }
  }
  else
  {
    ((my_cell_vector*)intersected_cells)->push_back(cell_index);
  }
}

void LASquadtree::intersect_tile_with_cells(const F32 ll_x, const F32 ll_y, const F32 ur_x, const F32 ur_y, const F32 cell_min_x, const F32 cell_max_x, const F32 cell_min_y, const F32 cell_max_y, U32 level, U32 level_index)
{
  volatile float cell_mid_x;
  volatile float cell_mid_y;
  if (level)
  {
    level--;
    level_index <<= 2;

    cell_mid_x = (cell_min_x + cell_max_x)/2;
    cell_mid_y = (cell_min_y + cell_max_y)/2;

    if (ur_x <= cell_mid_x)
    {
      // cell_max_x = cell_mid_x;
      if (ur_y <= cell_mid_y)
      {
        // cell_max_y = cell_mid_y;
        intersect_tile_with_cells(ll_x, ll_y, ur_x, ur_y, cell_min_x, cell_mid_x, cell_min_y, cell_mid_y, level, level_index);
      }
      else if (!(ll_y < cell_mid_y))
      {
        // cell_min_y = cell_mid_y;
        // level_index |= 1;
        intersect_tile_with_cells(ll_x, ll_y, ur_x, ur_y, cell_min_x, cell_mid_x, cell_mid_y, cell_max_y, level, level_index | 2);
      }
      else
      {
        intersect_tile_with_cells(ll_x, ll_y, ur_x, ur_y, cell_min_x, cell_mid_x, cell_min_y, cell_mid_y, level, level_index);
        intersect_tile_with_cells(ll_x, ll_y, ur_x, ur_y, cell_min_x, cell_mid_x, cell_mid_y, cell_max_y, level, level_index | 2);
      }
    }
    else if (!(ll_x < cell_mid_x))
    {
      // cell_min_x = cell_mid_x;
      // level_index |= 1;
      if (ur_y <= cell_mid_y)
      {
        // cell_max_y = cell_mid_y;
        intersect_tile_with_cells(ll_x, ll_y, ur_x, ur_y, cell_mid_x, cell_max_x, cell_min_y, cell_mid_y, level, level_index | 1);
      }
      else if (!(ll_y < cell_mid_y))
      {
        // cell_min_y = cell_mid_y;
        // level_index |= 1;
        intersect_tile_with_cells(ll_x, ll_y, ur_x, ur_y, cell_mid_x, cell_max_x, cell_mid_y, cell_max_y, level, level_index | 3);
      }
      else
      {
        intersect_tile_with_cells(ll_x, ll_y, ur_x, ur_y, cell_mid_x, cell_max_x, cell_min_y, cell_mid_y, level, level_index | 1);
        intersect_tile_with_cells(ll_x, ll_y, ur_x, ur_y, cell_mid_x, cell_max_x, cell_mid_y, cell_max_y, level, level_index | 3);
      }
    }
    else
    {
      if (ur_y <= cell_mid_y)
      {
        // cell_max_y = cell_mid_y;
        intersect_tile_with_cells(ll_x, ll_y, ur_x, ur_y, cell_min_x, cell_mid_x, cell_min_y, cell_mid_y, level, level_index);
        intersect_tile_with_cells(ll_x, ll_y, ur_x, ur_y, cell_mid_x, cell_max_x, cell_min_y, cell_mid_y, level, level_index | 1);
      }
      else if (!(ll_y < cell_mid_y))
      {
        // cell_min_y = cell_mid_y;
        // level_index |= 1;
        intersect_tile_with_cells(ll_x, ll_y, ur_x, ur_y, cell_min_x, cell_mid_x, cell_mid_y, cell_max_y, level, level_index | 2);
        intersect_tile_with_cells(ll_x, ll_y, ur_x, ur_y, cell_mid_x, cell_max_x, cell_mid_y, cell_max_y, level, level_index | 3);
      }
      else
      {
        intersect_tile_with_cells(ll_x, ll_y, ur_x, ur_y, cell_min_x, cell_mid_x, cell_min_y, cell_mid_y, level, level_index);
        intersect_tile_with_cells(ll_x, ll_y, ur_x, ur_y, cell_mid_x, cell_max_x, cell_min_y, cell_mid_y, level, level_index | 1);
        intersect_tile_with_cells(ll_x, ll_y, ur_x, ur_y, cell_min_x, cell_mid_x, cell_mid_y, cell_max_y, level, level_index | 2);
        intersect_tile_with_cells(ll_x, ll_y, ur_x, ur_y, cell_mid_x, cell_max_x, cell_mid_y, cell_max_y, level, level_index | 3);
      }
    }
  }
  else
  {
    ((my_cell_vector*)intersected_cells)->push_back(level_index);
  }
}

void LASquadtree::intersect_tile_with_cells_adaptive(const F32 ll_x, const F32 ll_y, const F32 ur_x, const F32 ur_y, const F32 cell_min_x, const F32 cell_max_x, const F32 cell_min_y, const F32 cell_max_y, U32 level, U32 level_index)
{
  volatile float cell_mid_x;
  volatile float cell_mid_y;
  U32 cell_index = get_cell_index(level_index, level);
  U32 adaptive_pos = cell_index/32;
  U32 adaptive_bit = ((U32)1) << (cell_index%32);
  if (adaptive[adaptive_pos] & adaptive_bit)
  {
    level++;
    level_index <<= 2;

    cell_mid_x = (cell_min_x + cell_max_x)/2;
    cell_mid_y = (cell_min_y + cell_max_y)/2;

    if (ur_x <= cell_mid_x)
    {
      // cell_max_x = cell_mid_x;
      if (ur_y <= cell_mid_y)
      {
        // cell_max_y = cell_mid_y;
        intersect_tile_with_cells_adaptive(ll_x, ll_y, ur_x, ur_y, cell_min_x, cell_mid_x, cell_min_y, cell_mid_y, level, level_index);
      }
      else if (!(ll_y < cell_mid_y))
      {
        // cell_min_y = cell_mid_y;
        // level_index |= 1;
        intersect_tile_with_cells_adaptive(ll_x, ll_y, ur_x, ur_y, cell_min_x, cell_mid_x, cell_mid_y, cell_max_y, level, level_index | 2);
      }
      else
      {
        intersect_tile_with_cells_adaptive(ll_x, ll_y, ur_x, ur_y, cell_min_x, cell_mid_x, cell_min_y, cell_mid_y, level, level_index);
        intersect_tile_with_cells_adaptive(ll_x, ll_y, ur_x, ur_y, cell_min_x, cell_mid_x, cell_mid_y, cell_max_y, level, level_index | 2);
      }
    }
    else if (!(ll_x < cell_mid_x))
    {
      // cell_min_x = cell_mid_x;
      // level_index |= 1;
      if (ur_y <= cell_mid_y)
      {
        // cell_max_y = cell_mid_y;
        intersect_tile_with_cells_adaptive(ll_x, ll_y, ur_x, ur_y, cell_mid_x, cell_max_x, cell_min_y, cell_mid_y, level, level_index | 1);
      }
      else if (!(ll_y < cell_mid_y))
      {
        // cell_min_y = cell_mid_y;
        // level_index |= 1;
        intersect_tile_with_cells_adaptive(ll_x, ll_y, ur_x, ur_y, cell_mid_x, cell_max_x, cell_mid_y, cell_max_y, level, level_index | 3);
      }
      else
      {
        intersect_tile_with_cells_adaptive(ll_x, ll_y, ur_x, ur_y, cell_mid_x, cell_max_x, cell_min_y, cell_mid_y, level, level_index | 1);
        intersect_tile_with_cells_adaptive(ll_x, ll_y, ur_x, ur_y, cell_mid_x, cell_max_x, cell_mid_y, cell_max_y, level, level_index | 3);
      }
    }
    else
    {
      if (ur_y <= cell_mid_y)
      {
        // cell_max_y = cell_mid_y;
        intersect_tile_with_cells_adaptive(ll_x, ll_y, ur_x, ur_y, cell_min_x, cell_mid_x, cell_min_y, cell_mid_y, level, level_index);
        intersect_tile_with_cells_adaptive(ll_x, ll_y, ur_x, ur_y, cell_mid_x, cell_max_x, cell_min_y, cell_mid_y, level, level_index | 1);
      }
      else if (!(ll_y < cell_mid_y))
      {
        // cell_min_y = cell_mid_y;
        // level_index |= 1;
        intersect_tile_with_cells_adaptive(ll_x, ll_y, ur_x, ur_y, cell_min_x, cell_mid_x, cell_mid_y, cell_max_y, level, level_index | 2);
        intersect_tile_with_cells_adaptive(ll_x, ll_y, ur_x, ur_y, cell_mid_x, cell_max_x, cell_mid_y, cell_max_y, level, level_index | 3);
      }
      else
      {
        intersect_tile_with_cells_adaptive(ll_x, ll_y, ur_x, ur_y, cell_min_x, cell_mid_x, cell_min_y, cell_mid_y, level, level_index);
        intersect_tile_with_cells_adaptive(ll_x, ll_y, ur_x, ur_y, cell_mid_x, cell_max_x, cell_min_y, cell_mid_y, level, level_index | 1);
        intersect_tile_with_cells_adaptive(ll_x, ll_y, ur_x, ur_y, cell_min_x, cell_mid_x, cell_mid_y, cell_max_y, level, level_index | 2);
        intersect_tile_with_cells_adaptive(ll_x, ll_y, ur_x, ur_y, cell_mid_x, cell_max_x, cell_mid_y, cell_max_y, level, level_index | 3);
      }
    }
  }
  else
  {
    ((my_cell_vector*)intersected_cells)->push_back(cell_index);
  }
}

void LASquadtree::intersect_circle_with_cells(const F64 center_x, const F64 center_y, const F64 radius, const F64 r_min_x, const F64 r_min_y, const F64 r_max_x, const F64 r_max_y, const F32 cell_min_x, const F32 cell_max_x, const F32 cell_min_y, const F32 cell_max_y, U32 level, U32 level_index)
{
  volatile float cell_mid_x;
  volatile float cell_mid_y;
  if (level)
  {
    level--;
    level_index <<= 2;

    cell_mid_x = (cell_min_x + cell_max_x)/2;
    cell_mid_y = (cell_min_y + cell_max_y)/2;

    if (r_max_x < cell_mid_x)
    {
      // cell_max_x = cell_mid_x;
      if (r_max_y < cell_mid_y)
      {
        // cell_max_y = cell_mid_y;
        intersect_circle_with_cells(center_x, center_y, radius, r_min_x, r_min_y, r_max_x, r_max_y, cell_min_x, cell_mid_x, cell_min_y, cell_mid_y, level, level_index);
      }
      else if (!(r_min_y < cell_mid_y))
      {
        // cell_min_y = cell_mid_y;
        // level_index |= 1;
        intersect_circle_with_cells(center_x, center_y, radius, r_min_x, r_min_y, r_max_x, r_max_y, cell_min_x, cell_mid_x, cell_mid_y, cell_max_y, level, level_index | 2);
      }
      else
      {
        intersect_circle_with_cells(center_x, center_y, radius, r_min_x, r_min_y, r_max_x, r_max_y, cell_min_x, cell_mid_x, cell_min_y, cell_mid_y, level, level_index);
        intersect_circle_with_cells(center_x, center_y, radius, r_min_x, r_min_y, r_max_x, r_max_y, cell_min_x, cell_mid_x, cell_mid_y, cell_max_y, level, level_index | 2);
      }
    }
    else if (!(r_min_x < cell_mid_x))
    {
      // cell_min_x = cell_mid_x;
      // level_index |= 1;
      if (r_max_y < cell_mid_y)
      {
        // cell_max_y = cell_mid_y;
        intersect_circle_with_cells(center_x, center_y, radius, r_min_x, r_min_y, r_max_x, r_max_y, cell_mid_x, cell_max_x, cell_min_y, cell_mid_y, level, level_index | 1);
      }
      else if (!(r_min_y < cell_mid_y))
      {
        // cell_min_y = cell_mid_y;
        // level_index |= 1;
        intersect_circle_with_cells(center_x, center_y, radius, r_min_x, r_min_y, r_max_x, r_max_y, cell_mid_x, cell_max_x, cell_mid_y, cell_max_y, level, level_index | 3);
      }
      else
      {
        intersect_circle_with_cells(center_x, center_y, radius, r_min_x, r_min_y, r_max_x, r_max_y, cell_mid_x, cell_max_x, cell_min_y, cell_mid_y, level, level_index | 1);
        intersect_circle_with_cells(center_x, center_y, radius, r_min_x, r_min_y, r_max_x, r_max_y, cell_mid_x, cell_max_x, cell_mid_y, cell_max_y, level, level_index | 3);
      }
    }
    else
    {
      if (r_max_y < cell_mid_y)
      {
        // cell_max_y = cell_mid_y;
        intersect_circle_with_cells(center_x, center_y, radius, r_min_x, r_min_y, r_max_x, r_max_y, cell_min_x, cell_mid_x, cell_min_y, cell_mid_y, level, level_index);
        intersect_circle_with_cells(center_x, center_y, radius, r_min_x, r_min_y, r_max_x, r_max_y, cell_mid_x, cell_max_x, cell_min_y, cell_mid_y, level, level_index | 1);
      }
      else if (!(r_min_y < cell_mid_y))
      {
        // cell_min_y = cell_mid_y;
        // level_index |= 1;
        intersect_circle_with_cells(center_x, center_y, radius, r_min_x, r_min_y, r_max_x, r_max_y, cell_min_x, cell_mid_x, cell_mid_y, cell_max_y, level, level_index | 2);
        intersect_circle_with_cells(center_x, center_y, radius, r_min_x, r_min_y, r_max_x, r_max_y, cell_mid_x, cell_max_x, cell_mid_y, cell_max_y, level, level_index | 3);
      }
      else
      {
        intersect_circle_with_cells(center_x, center_y, radius, r_min_x, r_min_y, r_max_x, r_max_y, cell_min_x, cell_mid_x, cell_min_y, cell_mid_y, level, level_index);
        intersect_circle_with_cells(center_x, center_y, radius, r_min_x, r_min_y, r_max_x, r_max_y, cell_mid_x, cell_max_x, cell_min_y, cell_mid_y, level, level_index | 1);
        intersect_circle_with_cells(center_x, center_y, radius, r_min_x, r_min_y, r_max_x, r_max_y, cell_min_x, cell_mid_x, cell_mid_y, cell_max_y, level, level_index | 2);
        intersect_circle_with_cells(center_x, center_y, radius, r_min_x, r_min_y, r_max_x, r_max_y, cell_mid_x, cell_max_x, cell_mid_y, cell_max_y, level, level_index | 3);
      }
    }
  }
  else
  {
    if (intersect_circle_with_rectangle(center_x, center_y, radius, cell_min_x, cell_max_x, cell_min_y, cell_max_y))
    {
      ((my_cell_vector*)intersected_cells)->push_back(level_index);
    }
  }
}

void LASquadtree::intersect_circle_with_cells_adaptive(const F64 center_x, const F64 center_y, const F64 radius, const F64 r_min_x, const F64 r_min_y, const F64 r_max_x, const F64 r_max_y, const F32 cell_min_x, const F32 cell_max_x, const F32 cell_min_y, const F32 cell_max_y, U32 level, U32 level_index)
{
  volatile float cell_mid_x;
  volatile float cell_mid_y;
  U32 cell_index = get_cell_index(level_index, level);
  U32 adaptive_pos = cell_index/32;
  U32 adaptive_bit = ((U32)1) << (cell_index%32);
  if (adaptive[adaptive_pos] & adaptive_bit)
  {
    level++;
    level_index <<= 2;

    cell_mid_x = (cell_min_x + cell_max_x)/2;
    cell_mid_y = (cell_min_y + cell_max_y)/2;

    if (r_max_x < cell_mid_x)
    {
      // cell_max_x = cell_mid_x;
      if (r_max_y < cell_mid_y)
      {
        // cell_max_y = cell_mid_y;
        intersect_circle_with_cells_adaptive(center_x, center_y, radius, r_min_x, r_min_y, r_max_x, r_max_y, cell_min_x, cell_mid_x, cell_min_y, cell_mid_y, level, level_index);
      }
      else if (!(r_min_y < cell_mid_y))
      {
        // cell_min_y = cell_mid_y;
        // level_index |= 1;
        intersect_circle_with_cells_adaptive(center_x, center_y, radius, r_min_x, r_min_y, r_max_x, r_max_y, cell_min_x, cell_mid_x, cell_mid_y, cell_max_y, level, level_index | 2);
      }
      else
      {
        intersect_circle_with_cells_adaptive(center_x, center_y, radius, r_min_x, r_min_y, r_max_x, r_max_y, cell_min_x, cell_mid_x, cell_min_y, cell_mid_y, level, level_index);
        intersect_circle_with_cells_adaptive(center_x, center_y, radius, r_min_x, r_min_y, r_max_x, r_max_y, cell_min_x, cell_mid_x, cell_mid_y, cell_max_y, level, level_index | 2);
      }
    }
    else if (!(r_min_x < cell_mid_x))
    {
      // cell_min_x = cell_mid_x;
      // level_index |= 1;
      if (r_max_y < cell_mid_y)
      {
        // cell_max_y = cell_mid_y;
        intersect_circle_with_cells_adaptive(center_x, center_y, radius, r_min_x, r_min_y, r_max_x, r_max_y, cell_mid_x, cell_max_x, cell_min_y, cell_mid_y, level, level_index | 1);
      }
      else if (!(r_min_y < cell_mid_y))
      {
        // cell_min_y = cell_mid_y;
        // level_index |= 1;
        intersect_circle_with_cells_adaptive(center_x, center_y, radius, r_min_x, r_min_y, r_max_x, r_max_y, cell_mid_x, cell_max_x, cell_mid_y, cell_max_y, level, level_index | 3);
      }
      else
      {
        intersect_circle_with_cells_adaptive(center_x, center_y, radius, r_min_x, r_min_y, r_max_x, r_max_y, cell_mid_x, cell_max_x, cell_min_y, cell_mid_y, level, level_index | 1);
        intersect_circle_with_cells_adaptive(center_x, center_y, radius, r_min_x, r_min_y, r_max_x, r_max_y, cell_mid_x, cell_max_x, cell_mid_y, cell_max_y, level, level_index | 3);
      }
    }
    else
    {
      if (r_max_y < cell_mid_y)
      {
        // cell_max_y = cell_mid_y;
        intersect_circle_with_cells_adaptive(center_x, center_y, radius, r_min_x, r_min_y, r_max_x, r_max_y, cell_min_x, cell_mid_x, cell_min_y, cell_mid_y, level, level_index);
        intersect_circle_with_cells_adaptive(center_x, center_y, radius, r_min_x, r_min_y, r_max_x, r_max_y, cell_mid_x, cell_max_x, cell_min_y, cell_mid_y, level, level_index | 1);
      }
      else if (!(r_min_y < cell_mid_y))
      {
        // cell_min_y = cell_mid_y;
        // level_index |= 1;
        intersect_circle_with_cells_adaptive(center_x, center_y, radius, r_min_x, r_min_y, r_max_x, r_max_y, cell_min_x, cell_mid_x, cell_mid_y, cell_max_y, level, level_index | 2);
        intersect_circle_with_cells_adaptive(center_x, center_y, radius, r_min_x, r_min_y, r_max_x, r_max_y, cell_mid_x, cell_max_x, cell_mid_y, cell_max_y, level, level_index | 3);
      }
      else
      {
        intersect_circle_with_cells_adaptive(center_x, center_y, radius, r_min_x, r_min_y, r_max_x, r_max_y, cell_min_x, cell_mid_x, cell_min_y, cell_mid_y, level, level_index);
        intersect_circle_with_cells_adaptive(center_x, center_y, radius, r_min_x, r_min_y, r_max_x, r_max_y, cell_mid_x, cell_max_x, cell_min_y, cell_mid_y, level, level_index | 1);
        intersect_circle_with_cells_adaptive(center_x, center_y, radius, r_min_x, r_min_y, r_max_x, r_max_y, cell_min_x, cell_mid_x, cell_mid_y, cell_max_y, level, level_index | 2);
        intersect_circle_with_cells_adaptive(center_x, center_y, radius, r_min_x, r_min_y, r_max_x, r_max_y, cell_mid_x, cell_max_x, cell_mid_y, cell_max_y, level, level_index | 3);
      }
    }
  }
  else
  {
    if (intersect_circle_with_rectangle(center_x, center_y, radius, cell_min_x, cell_max_x, cell_min_y, cell_max_y))
    {
      ((my_cell_vector*)intersected_cells)->push_back(cell_index);
    }
  }
}

BOOL LASquadtree::intersect_circle_with_rectangle(const F64 center_x, const F64 center_y, const F64 radius, const F32 r_min_x, const F32 r_max_x, const F32 r_min_y, const F32 r_max_y)
{
  F64 r_diff_x, r_diff_y;
  F64 radius_squared = radius * radius;
  if (r_max_x < center_x) // R to left of circle center
  {
    r_diff_x = center_x - r_max_x;
    if (r_max_y < center_y) // R in lower left corner
    {
      r_diff_y = center_y - r_max_y;
      return ((r_diff_x * r_diff_x + r_diff_y * r_diff_y) < radius_squared);
    }
    else if (r_min_y > center_y) // R in upper left corner
    {
      r_diff_y = -center_y + r_min_y;
      return ((r_diff_x * r_diff_x + r_diff_y * r_diff_y) < radius_squared);
    }
    else // R due West of circle
    {
      return (r_diff_x < radius);
    }
  }
  else if (r_min_x > center_x) // R to right of circle center
  {
    r_diff_x = -center_x + r_min_x;
    if (r_max_y < center_y) // R in lower right corner
    {
      r_diff_y = center_y - r_max_y;
      return ((r_diff_x * r_diff_x + r_diff_y * r_diff_y) < radius_squared);
    }
    else if (r_min_y > center_y) // R in upper right corner
    {
      r_diff_y = -center_y + r_min_y;
      return ((r_diff_x * r_diff_x + r_diff_y * r_diff_y) < radius_squared);
    }
    else // R due East of circle
    {
      return (r_diff_x < radius);
    }
  }
  else // R on circle vertical centerline
  {
    if (r_max_y < center_y) // R due South of circle
    {
      r_diff_y = center_y - r_max_y;
      return (r_diff_y < radius);
    }
    else if (r_min_y > center_y) // R due North of circle
    {
      r_diff_y = -center_y + r_min_y;
      return (r_diff_y < radius);
    }
    else // R contains circle centerpoint
    {
      return TRUE;
    }
  }
}

BOOL LASquadtree::get_intersected_cells()
{
  current_intersected_cell = 0;
  if (intersected_cells == 0)
  {
    return FALSE;
  }
  if (((my_cell_vector*)intersected_cells)->size() == 0)
  {
    return FALSE;
  }
  return TRUE;
}

BOOL LASquadtree::has_intersected_cells()
{
  if (intersected_cells == 0)
  {
    return FALSE;
  }
  if (((my_cell_vector*)intersected_cells)->size() <= current_intersected_cell)
  {
    return FALSE;
  }
  if (adaptive)
  {
    intersected_cell = ((my_cell_vector*)intersected_cells)->at(current_intersected_cell);
  }
  else
  {
    intersected_cell = level_offset[levels] + ((my_cell_vector*)intersected_cells)->at(current_intersected_cell);
  }
  current_intersected_cell++;
  return TRUE;
}

BOOL LASquadtree::setup(F64 bb_min_x, F64 bb_max_x, F64 bb_min_y, F64 bb_max_y, F32 cell_size)
{
  this->cell_size = cell_size;
  this->sub_level = 0;
  this->sub_level_index = 0;

  // enlarge bounding box to units of cells
  if (bb_min_x >= 0) min_x = cell_size*((I32)(bb_min_x/cell_size));
  else min_x = cell_size*((I32)(bb_min_x/cell_size)-1);
  if (bb_max_x >= 0) max_x = cell_size*((I32)(bb_max_x/cell_size)+1);
  else max_x = cell_size*((I32)(bb_max_x/cell_size));
  if (bb_min_y >= 0) min_y = cell_size*((I32)(bb_min_y/cell_size));
  else min_y = cell_size*((I32)(bb_min_y/cell_size)-1);
  if (bb_max_y >= 0) max_y = cell_size*((I32)(bb_max_y/cell_size)+1);
  else max_y = cell_size*((I32)(bb_max_y/cell_size));

  // how many cells minimally in each direction
  cells_x = U32_QUANTIZE((max_x - min_x)/cell_size);
  cells_y = U32_QUANTIZE((max_y - min_y)/cell_size);

  if (cells_x == 0 || cells_y == 0)
  {
    fprintf(stderr, "ERROR: cells_x %d cells_y %d\n", cells_x, cells_y);
    return FALSE;
  }

  // how many quad tree levels to get to that many cells
  U32 c = ((cells_x > cells_y) ? cells_x - 1 : cells_y - 1);
  levels = 0;
  while (c)
  {
    c = c >> 1;
    levels++;
  }

  // enlarge bounding box to quad tree size
  U32 c1, c2;
  c = (1 << levels) - cells_x;
  c1 = c/2;
  c2 = c - c1;
  min_x -= (c2 * cell_size);
  max_x += (c1 * cell_size);
  c = (1 << levels) - cells_y;
  c1 = c/2;
  c2 = c - c1;
  min_y -= (c2 * cell_size);
  max_y += (c1 * cell_size);

  return TRUE;
}

BOOL LASquadtree::subtiling_setup(F32 min_x, F32 max_x, F32 min_y, F32 max_y, U32 sub_level, U32 sub_level_index, U32 levels)
{
  this->min_x = min_x;
  this->max_x = max_x;
  this->min_y = min_y;
  this->max_y = max_y;
  F32 min[2];
  F32 max[2];
  get_cell_bounding_box(sub_level_index, sub_level, min, max);
  this->min_x = min[0];
  this->max_x = max[0];
  this->min_y = min[1];
  this->max_y = max[1];
  this->sub_level = sub_level;
  this->sub_level_index = sub_level_index;
  this->levels = levels;
  return TRUE;
}

LASquadtree::LASquadtree()
{
  U32 l;
  levels = 0;
  cell_size = 0;
  min_x = 0;
  max_x = 0;
  min_y = 0;
  max_y = 0;
  cells_x = 0;
  cells_y = 0;
  sub_level = 0;
  level_offset[0] = 0;
  for (l = 0; l < 23; l++)
  {
    level_offset[l+1] = level_offset[l] + ((1<<l)*(1<<l));
  }
  intersected_cells = 0;
  adaptive_alloc = 0;
  adaptive = 0;
}

LASquadtree::~LASquadtree()
{
  if (intersected_cells) delete ((my_cell_vector*)intersected_cells);
}
