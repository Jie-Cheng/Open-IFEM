#include "utilities.h"

namespace Utils
{
  bool Time::time_to_output() const
  {
    unsigned int delta = output_interval / delta_t;
    return (timestep >= delta && timestep % delta == 0);
  }

  bool Time::time_to_refine() const
  {
    unsigned int delta = refinement_interval / delta_t;
    return (timestep >= delta && timestep % delta == 0);
  }

  void Time::increment()
  {
    time_current += delta_t;
    ++timestep;
  }

  template <int dim, typename VectorType>
  GridInterpolator<dim, VectorType>::GridInterpolator(
    const DoFHandler<dim> &dof_handler, const Point<dim> &point)
    : dof_handler(dof_handler), point(point)
  {
    // This function throws an exception of GridTools::ExcPointNotFound if the
    // point
    // does not lie in any cell. In this case, we set the cell pointer to null.
    try
      {
        cell_point =
          GridTools::find_active_cell_around_point(mapping, dof_handler, point);
      }
    catch (GridTools::ExcPointNotFound<dim> &e)
      {
        cell_point.first = dof_handler.end();
        cell_point.second = point;
      }
  }

  template <int dim, typename VectorType>
  void GridInterpolator<dim, VectorType>::point_value(
    const VectorType &fe_function,
    Vector<typename VectorType::value_type> &value)
  {
    typedef typename VectorType::value_type Number;
    const FiniteElement<dim> &fe = dof_handler.get_fe();
    Assert(value.size() == fe.n_components(),
           ExcDimensionMismatch(value.size(), fe.n_components()));
    // If for some reason, the point is not found in any cell,
    // or it is on a cell that is not locally owned, return 0.
    if (cell_point.first == dof_handler.end() ||
        !cell_point.first->is_locally_owned())
      {
        value = 0;
        return;
      }
    Assert(GeometryInfo<dim>::distance_to_unit_cell(cell_point.second) < 1e-10,
           ExcInternalError());

    const Quadrature<dim> quadrature(
      GeometryInfo<dim>::project_to_unit_cell(cell_point.second));
    FEValues<dim> fe_values(mapping, fe, quadrature, update_values);
    fe_values.reinit(cell_point.first);
    std::vector<Vector<Number>> u_value(1, Vector<Number>(fe.n_components()));
    fe_values.get_function_values(fe_function, u_value);
    value = u_value[0];
  }

  template <int dim, typename VectorType>
  void GridInterpolator<dim, VectorType>::point_gradient(
    const VectorType &fe_function,
    std::vector<Tensor<1, dim, typename VectorType::value_type>> &gradient)
  {
    typedef typename VectorType::value_type Number;
    const FiniteElement<dim> &fe = dof_handler.get_fe();
    Assert(gradient.size() == fe.n_components(),
           ExcDimensionMismatch(gradient.size(), fe.n_components()));
    // If for some reason, the point is not found in any cell,
    // or it is on a cell that is not locally owned, return 0.
    if (cell_point.first == dof_handler.end() ||
        !cell_point.first->is_locally_owned())
      {
        for (auto &v : gradient)
          {
            v = 0;
          }
        return;
      }
    Assert(GeometryInfo<dim>::distance_to_unit_cell(cell_point.second) < 1e-10,
           ExcInternalError());

    const Quadrature<dim> quadrature(
      GeometryInfo<dim>::project_to_unit_cell(cell_point.second));
    FEValues<dim> fe_values(mapping, fe, quadrature, update_gradients);
    fe_values.reinit(cell_point.first);
    std::vector<std::vector<Tensor<1, dim, Number>>> u_gradient(
      1, std::vector<Tensor<1, dim, Number>>(fe.n_components()));
    fe_values.get_function_gradients(fe_function, u_gradient);
    gradient = u_gradient[0];
  }

  template class GridInterpolator<2, Vector<double>>;
  template class GridInterpolator<3, Vector<double>>;
  template class GridInterpolator<2, BlockVector<double>>;
  template class GridInterpolator<3, BlockVector<double>>;

  // The code to create triangulation is copied from [Martin Kronbichler's code]
  // (https://github.com/kronbichler/adaflo/blob/master/tests/flow_past_cylinder.cc)
  // with very few modifications.
  // Helper function used in both 2d and 3d:
  template <int dim>
  void GridCreator<dim>::flow_around_cylinder_2d(Triangulation<2> &tria,
                                                 bool compute_in_2d)
  {
    SphericalManifold<2> boundary(Point<2>(0.5, 0.2));
    Triangulation<2> left, middle, right, tmp, tmp2;
    GridGenerator::subdivided_hyper_rectangle(
      left,
      std::vector<unsigned int>({3U, 4U}),
      Point<2>(),
      Point<2>(0.3, 0.41),
      false);
    GridGenerator::subdivided_hyper_rectangle(
      right,
      std::vector<unsigned int>({18U, 4U}),
      Point<2>(0.7, 0),
      Point<2>(2.5, 0.41),
      false);

    // Create middle part first as a hyper shell.
    GridGenerator::hyper_shell(middle, Point<2>(0.5, 0.2), 0.05, 0.2, 4, true);
    middle.set_manifold(0, boundary);
    middle.refine_global(1);

    // Then move the vertices to the points where we want them to be to create a
    // slightly asymmetric cube with a hole
    for (Triangulation<2>::cell_iterator cell = middle.begin();
         cell != middle.end();
         ++cell)
      for (unsigned int v = 0; v < GeometryInfo<2>::vertices_per_cell; ++v)
        {
          Point<2> &vertex = cell->vertex(v);
          if (std::abs(vertex[0] - 0.7) < 1e-10 &&
              std::abs(vertex[1] - 0.2) < 1e-10)
            vertex = Point<2>(0.7, 0.205);
          else if (std::abs(vertex[0] - 0.6) < 1e-10 &&
                   std::abs(vertex[1] - 0.3) < 1e-10)
            vertex = Point<2>(0.7, 0.41);
          else if (std::abs(vertex[0] - 0.6) < 1e-10 &&
                   std::abs(vertex[1] - 0.1) < 1e-10)
            vertex = Point<2>(0.7, 0);
          else if (std::abs(vertex[0] - 0.5) < 1e-10 &&
                   std::abs(vertex[1] - 0.4) < 1e-10)
            vertex = Point<2>(0.5, 0.41);
          else if (std::abs(vertex[0] - 0.5) < 1e-10 &&
                   std::abs(vertex[1] - 0.0) < 1e-10)
            vertex = Point<2>(0.5, 0.0);
          else if (std::abs(vertex[0] - 0.4) < 1e-10 &&
                   std::abs(vertex[1] - 0.3) < 1e-10)
            vertex = Point<2>(0.3, 0.41);
          else if (std::abs(vertex[0] - 0.4) < 1e-10 &&
                   std::abs(vertex[1] - 0.1) < 1e-10)
            vertex = Point<2>(0.3, 0);
          else if (std::abs(vertex[0] - 0.3) < 1e-10 &&
                   std::abs(vertex[1] - 0.2) < 1e-10)
            vertex = Point<2>(0.3, 0.205);
          else if (std::abs(vertex[0] - 0.56379) < 1e-4 &&
                   std::abs(vertex[1] - 0.13621) < 1e-4)
            vertex = Point<2>(0.59, 0.11);
          else if (std::abs(vertex[0] - 0.56379) < 1e-4 &&
                   std::abs(vertex[1] - 0.26379) < 1e-4)
            vertex = Point<2>(0.59, 0.29);
          else if (std::abs(vertex[0] - 0.43621) < 1e-4 &&
                   std::abs(vertex[1] - 0.13621) < 1e-4)
            vertex = Point<2>(0.41, 0.11);
          else if (std::abs(vertex[0] - 0.43621) < 1e-4 &&
                   std::abs(vertex[1] - 0.26379) < 1e-4)
            vertex = Point<2>(0.41, 0.29);
        }

    // Refine once to create the same level of refinement as in the
    // neighboring domains:
    middle.refine_global(1);

    // Must copy the triangulation because we cannot merge triangulations with
    // refinement:
    GridGenerator::flatten_triangulation(middle, tmp2);

    // Left domain is requred in 3d only.
    if (compute_in_2d)
      {
        GridGenerator::merge_triangulations(tmp2, right, tria);
      }
    else
      {
        GridGenerator::merge_triangulations(left, tmp2, tmp);
        GridGenerator::merge_triangulations(tmp, right, tria);
      }
  }

  // Create 2D triangulation:
  template <>
  void GridCreator<2>::flow_around_cylinder(Triangulation<2> &tria)
  {
    flow_around_cylinder_2d(tria);
    // Set the left boundary (inflow) to 0, the right boundary (outflow) to 1,
    // upper to 2, lower to 3 and the cylindrical surface to 4.
    for (Triangulation<2>::active_cell_iterator cell = tria.begin();
         cell != tria.end();
         ++cell)
      {
        for (unsigned int f = 0; f < GeometryInfo<2>::faces_per_cell; ++f)
          {
            if (cell->face(f)->at_boundary())
              {
                if (std::abs(cell->face(f)->center()[0] - 2.5) < 1e-12)
                  {
                    cell->face(f)->set_all_boundary_ids(1);
                  }
                else if (std::abs(cell->face(f)->center()[0] - 0.3) < 1e-12)
                  {
                    cell->face(f)->set_all_boundary_ids(0);
                  }
                else if (std::abs(cell->face(f)->center()[1] - 0.41) < 1e-12)
                  {
                    cell->face(f)->set_all_boundary_ids(3);
                  }
                else if (std::abs(cell->face(f)->center()[1]) < 1e-12)
                  {
                    cell->face(f)->set_all_boundary_ids(2);
                  }
                else
                  {
                    cell->face(f)->set_all_boundary_ids(4);
                  }
              }
          }
      }
  }

  // Create 3D triangulation:
  template <>
  void GridCreator<3>::flow_around_cylinder(Triangulation<3> &tria)
  {
    Triangulation<2> tria_2d;
    flow_around_cylinder_2d(tria_2d, false);
    GridGenerator::extrude_triangulation(tria_2d, 5, 0.41, tria);
    // Set boundaries in x direction to 0 and 1; y direction to 2 and 3;
    // z direction to 4 and 5; the cylindrical surface 6.
    for (Triangulation<3>::active_cell_iterator cell = tria.begin();
         cell != tria.end();
         ++cell)
      {
        for (unsigned int f = 0; f < GeometryInfo<3>::faces_per_cell; ++f)
          {
            if (cell->face(f)->at_boundary())
              {
                if (std::abs(cell->face(f)->center()[0] - 2.5) < 1e-12)
                  {
                    cell->face(f)->set_all_boundary_ids(1);
                  }
                else if (std::abs(cell->face(f)->center()[0]) < 1e-12)
                  {
                    cell->face(f)->set_all_boundary_ids(0);
                  }
                else if (std::abs(cell->face(f)->center()[1] - 0.41) < 1e-12)
                  {
                    cell->face(f)->set_all_boundary_ids(3);
                  }
                else if (std::abs(cell->face(f)->center()[1]) < 1e-12)
                  {
                    cell->face(f)->set_all_boundary_ids(2);
                  }
                else if (std::abs(cell->face(f)->center()[2] - 0.41) < 1e-12)
                  {
                    cell->face(f)->set_all_boundary_ids(5);
                  }
                else if (std::abs(cell->face(f)->center()[2]) < 1e-12)
                  {
                    cell->face(f)->set_all_boundary_ids(4);
                  }
                else
                  {
                    cell->face(f)->set_all_boundary_ids(6);
                  }
              }
          }
      }
  }

  template <int dim>
  void GridCreator<dim>::sphere(Triangulation<dim> &tria,
                                const Point<dim> &center,
                                double radius)
  {
    GridGenerator::hyper_ball(tria, center, radius);
    static const SphericalManifold<dim> boundary(center);
    tria.set_all_manifold_ids_on_boundary(0);
    tria.set_manifold(0, boundary);
    const double core_radius = radius / 5.0, inner_radius = radius / 3.0;

    for (auto cell = tria.begin_active(); cell != tria.end(); ++cell)
      {
        if (center.distance(cell->center()) < 1e-5 * radius)
          {
            for (unsigned int v = 0; v < GeometryInfo<dim>::vertices_per_cell;
                 ++v)
              {
                double scale = core_radius / center.distance(cell->vertex(v));
                cell->vertex(v) =
                  scale * cell->vertex(v) + (1.0 - scale) * center;
              }
          }
      }

    for (auto cell = tria.begin_active(); cell != tria.end(); ++cell)
      {
        if (center.distance(cell->center()) >= 1e-5 * radius)
          {
            cell->set_refine_flag();
          }
      }
    tria.execute_coarsening_and_refinement();

    for (auto cell = tria.begin_active(); cell != tria.end(); ++cell)
      {
        for (unsigned int v = 0; v < GeometryInfo<dim>::vertices_per_cell; ++v)
          {
            const double dist = center.distance(cell->vertex(v));
            if (dist > core_radius * 1.0001 && dist < 0.9999 * radius)
              {
                double scale = inner_radius / dist;
                cell->vertex(v) =
                  scale * cell->vertex(v) + (1.0 - scale) * center;
              }
          }
      }

    for (auto cell = tria.begin_active(); cell != tria.end(); ++cell)
      {
        bool is_in_inner_circle = false;
        for (unsigned int v = 0; v < GeometryInfo<dim>::vertices_per_cell; ++v)
          {
            if (center.distance(cell->vertex(v)) < inner_radius)
              {
                is_in_inner_circle = true;
                break;
              }
          }
        if (!is_in_inner_circle)
          {
            cell->set_all_manifold_ids(0);
          }
      }
  }

  template class GridCreator<2>;
  template class GridCreator<3>;
}
