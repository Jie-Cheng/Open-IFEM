#ifndef FSI_H
#define FSI_H

#include <deal.II/base/table_indices.h>
#include <deal.II/physics/elasticity/standard_tensors.h>

#include "linearElasticSolver.h"
#include "navierstokes.h"
#include "utilities.h"

using namespace dealii;

extern template class Fluid::NavierStokes<2>;
extern template class Fluid::NavierStokes<3>;
extern template class Solid::LinearElasticSolver<2>;
extern template class Solid::LinearElasticSolver<3>;
extern template class Utils::GridInterpolator<2, Vector<double>>;
extern template class Utils::GridInterpolator<3, Vector<double>>;
extern template class Utils::GridInterpolator<2, BlockVector<double>>;
extern template class Utils::GridInterpolator<3, BlockVector<double>>;

template <int dim>
class FSI
{
public:
  FSI(Fluid::NavierStokes<dim> &,
      Solid::LinearElasticSolver<dim> &,
      const Parameters::AllParameters &);
  void run();

private:
  /// Check if a point is inside a mesh.
  bool point_in_mesh(const DoFHandler<dim> &, const Point<dim> &);

  /*! \brief Update the indicator field of the fluid solver.
   *
   *  Although the indicator field is defined at quadrature points in order to
   *  cache the fsi force, the quadrature points in the same cell are updated
   *  as a whole: they are either all 1 or all 0. The criteria is that whether
   *  all of the vertices are in solid mesh (because later on Dirichlet BCs
   *  obtained from the solid will be applied).
   */
  void update_indicator();

  /// Move solid triangulation either forward or backward using displacements,
  void move_solid_mesh(bool);

  /*! \brief Compute the fluid traction on solid boundaries.
   *
   *  The implementation is straight-forward: loop over the faces on the
   *  solid Neumann boundary, find the quadrature points and normals,
   *  then interpolate the fluid pressure and symmetric gradient of velocity at
   *  those points, based on which the fluid traction is calculated.
   */
  void find_solid_bc();

  /*! \brief Compute the Dirichlet BCs on the artificial fluid using solid
   * velocity,
   *         as well as the fsi stress and acceleration terms at the artificial
   * fluid
   *         quadrature points.
   *
   *  The Dirichlet BCs are obtained by interpolating solid velocity to the
   * fluid
   *  vertices and the FSI force is defined as:
   *  \f$F^{\text{FSI}} = \frac{Dv^f_i}{Dt}) - \sigma^f_{ij,j})\f$.
   *  In practice, we avoid directly evaluating stress divergence, so the stress
   *  itself and the acceleration are separately cached onto the fluid
   * quadrature
   *  points to be used by the fluid solver.
   */
  void find_fluid_bc();

  Fluid::NavierStokes<dim> &fluid_solver;
  Solid::LinearElasticSolver<dim> &solid_solver;
  Parameters::AllParameters parameters;
  Utils::Time time;
};

#endif
