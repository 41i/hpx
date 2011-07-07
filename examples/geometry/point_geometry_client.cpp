//  Copyright (c) 2007-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>

#include "point_geometry/point.hpp"
#include <boost/geometry/geometries/polygon.hpp>
#include <hpx/components/distributing_factory/distributing_factory.hpp>

namespace hpx { namespace geometry
{
    typedef boost::geometry::model::polygon<hpx::geometry::point> polygon_2d; 
}}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(boost::program_options::variables_map &vm)
{
    {

        std::size_t array_size = 4;

        namespace bg = boost::geometry;

        // create a distributing factory locally
        hpx::components::distributing_factory factory;
        factory.create(hpx::find_here());

        hpx::components::component_type block_type =
            hpx::components::get_component_type<
                hpx::geometry::point::server_component_type>();

        hpx::components::distributing_factory::result_type blocks =
            factory.create_components(block_type, array_size);

        hpx::geometry::point pt1(hpx::find_here(), 0, 0);
        hpx::geometry::point pt2(hpx::find_here(), 0, 1);
        hpx::geometry::point pt3(hpx::find_here(), 1, 1);
        hpx::geometry::point pt4(hpx::find_here(), 1, 0);

//         bg::assign_values(pt1, 1, 1);
//         bg::assign_values(pt2, 2, 2);

        double d = bg::distance(pt1, pt2);

        std::cout << "Distance: " << d << std::endl;

        hpx::geometry::polygon_2d p;
        p.outer().push_back(pt1);
        p.outer().push_back(pt2);
        p.outer().push_back(pt3);
        p.outer().push_back(pt4);
        p.outer().push_back(pt1);
        bg::correct(p);

        hpx::geometry::point pt5(hpx::find_here(), 0.5, 0.5);
        bool inside = bg::within(pt5, p);

        std::cout << "Point is " << (inside ? "inside" : "outside") << std::endl;
    }

    {
        namespace bg = boost::geometry;

        typedef bg::model::d2::point_xy<double> point_type;
        typedef bg::model::polygon<point_type> polygon_type;

        point_type pt1(0, 0);
        point_type pt2(0, 1);
        point_type pt3(1, 1);
        point_type pt4(1, 0);

        double d = bg::distance(pt1, pt2);

        std::cout << "Distance: " << d << std::endl;

        polygon_type p;
        p.outer().push_back(pt1);
        p.outer().push_back(pt2);
        p.outer().push_back(pt3);
        p.outer().push_back(pt4);
        bg::correct(p);

        point_type pt5(0.5, 0.5);
        bool inside = bg::within(pt5, p);

        std::cout << "Point is " << (inside ? "inside" : "outside") << std::endl;
    }

    hpx::finalize();
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    return hpx::init("point_geometry_client", argc, argv); // Initialize and run HPX
}
