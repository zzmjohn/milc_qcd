/******** layout_qdp.c *********/
/* MIMD version 7 */
/* ROUTINES WHICH DETERMINE THE DISTRIBUTION OF SITES ON NODES */
/* This version uses QDP for layout */
/*
   setup_layout() does any initial setup.  When it is called the
     lattice dimensions nx,ny,nz and nt have been set.
     This routine sets the global variables "sites_on_node",
     "even_sites_on_node" and "odd_sites_on_node".
   node_number(x,y,z,t) returns the node number on which a site lives.
   node_index(x,y,z,t) returns the index of the site on the node - ie the
     site is lattice[node_index(x,y,z,t)].
   get_logical_dimensions() returns the machine dimensions
   get_logical_coordinates() returns the mesh coordinates of this node
   These routines will change as we change our minds about how to distribute
     sites among the nodes.  Hopefully the setup routines will work for any
     consistent choices. (ie node_index should return a different value for
     each site on the node.)
*/
#include "generic_includes.h"
#include <qdp.h>
#include <qmp.h>

static const int* dim_mach;

#ifdef FIX_IONODE_GEOM

static void lex_coords(int coords[], const int dim, const int size[], 
	   const size_t rank)
{
  int d;
  size_t r = rank;

  for(d = 0; d < dim; d++){
    coords[d] = r % size[d];
    r /= size[d];
  }
}

/*------------------------------------------------------------------*/
/* Convert coordinate to linear lexicographic rank (inverse of
   lex_coords) */

static size_t lex_rank(const int coords[], int dim, int size[])
{
  int d;
  size_t rank = coords[dim-1];

  for(d = dim-2; d >= 0; d--){
    rank = rank * size[d] + coords[d];
  }
  return rank;
}

#endif

void
setup_layout(void)
{
  int c[4];
  int i,n_mach;
  int d[4];

  if(mynode()==0){
    printf("LAYOUT = Hypercubes, options = ");
    printf("QDP");
    printf("\n");
  }
  c[0] = nx;
  c[1] = ny;
  c[2] = nz;
  c[3] = nt;
  QDP_set_latsize(4, c);
  QDP_create_layout();
  sites_on_node = QDP_sites_on_node;
  even_sites_on_node = QDP_subset_len(QDP_even);
  odd_sites_on_node = QDP_subset_len(QDP_odd);

  /* Report sublattice dimensions */
  n_mach = QMP_get_logical_number_of_dimensions();
  dim_mach = QMP_get_logical_dimensions();
  for(i = 0; i < 4; i++){
    /* Any extra machine dimensions are assumed to be 1 */
    if(i < n_mach)d[i] = c[i]/dim_mach[i];
    else d[i] = c[i];
  }
  if( mynode()==0)
    printf("ON EACH NODE %d x %d x %d x %d\n",d[0],d[1],d[2],d[3]);
}

int
node_number(int x, int y, int z, int t)
{
  int c[4];

  c[0] = x;
  c[1] = y;
  c[2] = z;
  c[3] = t;
  return QDP_node_number(c);
}

int
node_index(int x, int y, int z, int t)
{
  int c[4];

  c[0] = x;
  c[1] = y;
  c[2] = z;
  c[3] = t;
  return QDP_index(c);
}

size_t num_sites(int node) {
    return( sites_on_node );
}

const int *get_logical_dimensions(){
  return dim_mach;
}

const int *get_logical_coordinate(){
  return QMP_get_logical_coordinates();
}

/* Map node number and index to coordinates  */
void get_coords(int coords[], int node, int index){
  QDP_get_coords(coords, node, index);
}

/* io_node(node) maps a node to its I/O node.  The nodes are placed on
   a node lattice with dimensions nsquares.  The I/O partitions are
   hypercubes of the node lattice.  The dimensions of the hypercube are
   given by nodes_per_ionode.  The I/O node is at the origin of that
   hypercube. */

#ifdef FIX_IONODE_GEOM

/*------------------------------------------------------------------*/
/* Map any node to its I/O node */
int io_node(const int node){
  int i,j,k; 

  /* Get the machine coordinates for the specified node */
  lex_coords(io_node_coords, 4, nsquares, node);

  /* Round the node coordinates down to get the io_node coordinate */
  for(i = 0; i < 4; i++)
    io_node_coords[i] = nodes_per_ionode[i] * 
      (io_node_coords[i]/nodes_per_ionode[i]);
  
  /* Return the linearized machine coordinates of the I/O node */
  return (int)lex_rank(io_node_coords, 4, nsquares);
}

#else

/*------------------------------------------------------------------*/
/* If we don't have I/O partitions, each node does its own I/O */
int io_node(int node){
  return node;
}
#endif
