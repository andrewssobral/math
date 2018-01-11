/*
 * Copyright Nick Thompson, 2017
 * Use, modification and distribution are subject to the
 * Boost Software License, Version 1.0. (See accompanying file
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#define BOOST_TEST_MODULE naive_monte_carlo_test
#include <cmath>
#include <ostream>
#include <boost/type_index.hpp>
#include <boost/test/included/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/math/constants/constants.hpp>
#include <boost/math/quadrature/naive_monte_carlo.hpp>

using std::abs;
using std::vector;
using std::pair;
using boost::math::constants::pi;
using boost::math::quadrature::naive_monte_carlo;


template<class Real>
void test_pi()
{
    std::cout << "Testing pi is calculated correctly using Monte-Carlo on type " << boost::typeindex::type_id<Real>().pretty_name() << "\n";
    auto g = [](std::vector<Real> const & x)->Real
    {
        Real r = x[0]*x[0]+x[1]*x[1];
        if (r <= 1)
        {
            return 4;
        }
        return 0;
    };

    std::vector<std::pair<Real, Real>> bounds{{0, 1}, {0, 1}};
    naive_monte_carlo<Real, decltype(g)> mc(g, bounds, (Real) 0.0005);

    auto task = mc.integrate();
    Real pi_estimated = task.get();
    if (abs(pi_estimated - pi<Real>())/pi<Real>() > 0.005)
    {
        std::cout << "Error in estimation of pi too high, function calls: " << mc.calls() << "\n";
        BOOST_CHECK_CLOSE_FRACTION(pi_estimated, pi<Real>(), 0.005);
    }

}

template<class Real>
void test_constant()
{
    std::cout << "Testing constants are integrated correctly using Monte-Carlo on type " << boost::typeindex::type_id<Real>().pretty_name() << "\n";
    auto g = [](std::vector<Real> const & x)->Real
    {
      return 1;
    };

    std::vector<std::pair<Real, Real>> bounds{{0, 1}, {0, 1}};
    naive_monte_carlo<Real, decltype(g)> mc(g, bounds, (Real) 0.0001);

    auto task = mc.integrate();
    Real one = task.get();
    BOOST_CHECK_CLOSE_FRACTION(one, 1, 0.001);
    BOOST_CHECK_SMALL(mc.current_error_estimate(), std::numeric_limits<Real>::epsilon());
    BOOST_CHECK(mc.calls() > 1000);
}

template<class Real>
void test_nan()
{
    std::cout << "Testing that a reasonable action is performed by the Monte-Carlo integrator when singularities are hit on type " << boost::typeindex::type_id<Real>().pretty_name() << "\n";
    auto g = [](std::vector<Real> const & x)->Real
    {
      return (Real) 1/ (Real) 0;
    };

    std::vector<std::pair<Real, Real>> bounds{{0, 1}, {0, 1}};
    naive_monte_carlo<Real, decltype(g)> mc(g, bounds, (Real) 0.0001);

    auto task = mc.integrate();
    Real result = task.get();
    // I think this is reasonable, but should it throw an exception?
    BOOST_CHECK(std::isnan(result));
}

template<class Real>
void test_exception_from_integrand()
{
    std::cout << "Testing that a reasonable action is performed by the Monte-Carlo integrator when the integrand throws an exception on type " << boost::typeindex::type_id<Real>().pretty_name() << "\n";
    auto g = [](std::vector<Real> const & x)->Real
    {
        if (x[0] > 0.5 && x[0] < 0.5001)
        {
            throw std::domain_error("You have done something wrong.\n");
        }
        return (Real) 1;
    };

    std::vector<std::pair<Real, Real>> bounds{{0, 1}, {0, 1}};
    naive_monte_carlo<Real, decltype(g)> mc(g, bounds, (Real) 0.0001);

    auto task = mc.integrate();
    bool caught_exception = false;
    try
    {
      Real result = task.get();
      // Get rid of unused variable warning:
      std::ostream cnull(0);
      cnull << result;
    }
    catch(std::exception const & e)
    {
        caught_exception = true;
    }
    BOOST_CHECK(caught_exception);
}


template<class Real>
void test_cancel_and_restart()
{
    std::cout << "Testing that cancellation and restarting works on naive Monte-Carlo integration on type " << boost::typeindex::type_id<Real>().pretty_name() << "\n";
    Real exact = 1.3932039296856768591842462603255;
    constexpr const Real A = 1.0 / (pi<Real>() * pi<Real>() * pi<Real>());
    auto g = [&](std::vector<Real> const & x)->Real
    {
        return A / (1.0 - cos(x[0])*cos(x[1])*cos(x[2]));
    };
    vector<pair<Real, Real>> bounds{{0, pi<Real>()}, {0, pi<Real>()}, {0, pi<Real>()}};
    naive_monte_carlo<Real, decltype(g)> mc(g, bounds, (Real) 0.05);

    auto task = mc.integrate();
    mc.cancel();
    double y = task.get();
    // Super low tolerance; because it got canceled so fast:
    BOOST_CHECK_CLOSE_FRACTION(y, exact, 1.0);

    mc.update_target_error((Real) 0.01);
    task = mc.integrate();
    y = task.get();
    BOOST_CHECK_CLOSE_FRACTION(y, exact, 0.1);
}

template<class Real>
void test_variance()
{
    std::cout << "Testing that variance computed by naive Monte-Carlo integration converges to integral formula on type " << boost::typeindex::type_id<Real>().pretty_name() << "\n";
    Real exact_variance = (Real) 1/(Real) 12;
    auto g = [&](std::vector<Real> const & x)->Real
    {
        return x[0];
    };
    vector<pair<Real, Real>> bounds{{0, 1}};
    naive_monte_carlo<Real, decltype(g)> mc(g, bounds, (Real) 0.001);

    auto task = mc.integrate();
    Real y = task.get();
    BOOST_CHECK_CLOSE_FRACTION(y, 0.5, 0.01);
    BOOST_CHECK_CLOSE_FRACTION(mc.variance(), exact_variance, 0.05);
}

template<class Real, size_t dimension>
void test_product()
{
    std::cout << "Testing that product functions are integrated correctly by naive Monte-Carlo on type " << boost::typeindex::type_id<Real>().pretty_name() << "\n";
    auto g = [&](std::vector<Real> const & x)->Real
    {
        double y = 1;
        for (size_t i = 0; i < x.size(); ++i)
        {
            y *= 2*x[i];
        }
        return y;
    };

    vector<pair<Real, Real>> bounds(dimension);
    for (size_t i = 0; i < dimension; ++i)
    {
        bounds[i] = std::make_pair<Real, Real>(0, 1);
    }
    naive_monte_carlo<Real, decltype(g)> mc(g, bounds, (Real) 0.001);

    auto task = mc.integrate();
    Real y = task.get();
    BOOST_CHECK_CLOSE_FRACTION(y, 1, 0.01);
    using std::pow;
    Real exact_variance = pow(4.0/3.0, dimension) - 1;
    BOOST_CHECK_CLOSE_FRACTION(mc.variance(), exact_variance, 0.05);
}

template<class Real>
void test_upper_bound_infinite()
{
    std::cout << "Testing that infinite upper bounds are integrated correctly by naive Monte-Carlo on type " << boost::typeindex::type_id<Real>().pretty_name() << "\n";
    auto g = [](std::vector<Real> const & x)->Real
    {
        return 1.0/(x[0]*x[0] + 1.0);
    };

    vector<pair<Real, Real>> bounds(1);
    for (size_t i = 0; i < bounds.size(); ++i)
    {
        bounds[i] = std::make_pair<Real, Real>(0, std::numeric_limits<Real>::infinity());
    }
    naive_monte_carlo<Real, decltype(g)> mc(g, bounds, (Real) 0.001);

    auto task = mc.integrate();
    Real y = task.get();
    BOOST_CHECK_CLOSE_FRACTION(y, M_PI/2, 0.01);
}

template<class Real>
void test_lower_bound_infinite()
{
    std::cout << "Testing that infinite lower bounds are integrated correctly by naive Monte-Carlo on type " << boost::typeindex::type_id<Real>().pretty_name() << "\n";
    auto g = [](std::vector<Real> const & x)->Real
    {
        return 1.0/(x[0]*x[0] + 1.0);
    };

    vector<pair<Real, Real>> bounds(1);
    for (size_t i = 0; i < bounds.size(); ++i)
    {
        bounds[i] = std::make_pair<Real, Real>(-std::numeric_limits<Real>::infinity(), 0);
    }
    naive_monte_carlo<Real, decltype(g)> mc(g, bounds, (Real) 0.001);

    auto task = mc.integrate();
    Real y = task.get();
    BOOST_CHECK_CLOSE_FRACTION(y, M_PI/2, 0.01);
}

template<class Real>
void test_double_infinite()
{
    std::cout << "Testing that double infinite bounds are integrated correctly by naive Monte-Carlo on type " << boost::typeindex::type_id<Real>().pretty_name() << "\n";
    auto g = [](std::vector<Real> const & x)->Real
    {
        return 1.0/(x[0]*x[0] + 1.0);
    };

    vector<pair<Real, Real>> bounds(1);
    for (size_t i = 0; i < bounds.size(); ++i)
    {
        bounds[i] = std::make_pair<Real, Real>(-std::numeric_limits<Real>::infinity(), std::numeric_limits<Real>::infinity());
    }
    naive_monte_carlo<Real, decltype(g)> mc(g, bounds, (Real) 0.001);

    auto task = mc.integrate();
    Real y = task.get();
    BOOST_CHECK_CLOSE_FRACTION(y, M_PI, 0.01);
}


BOOST_AUTO_TEST_CASE(naive_monte_carlo_test)
{
    test_nan<float>();
    test_pi<float>();
    test_pi<double>();
    test_pi<long double>();
    test_constant<float>();
    test_constant<double>();
    test_constant<long double>();
    test_cancel_and_restart<float>();
    test_exception_from_integrand<float>();
    test_variance<float>();
    test_variance<double>();
    test_product<float, 1>();
    test_product<float, 2>();
    test_product<float, 3>();
    test_product<float, 4>();
    test_product<float, 5>();
    test_product<float, 6>();
    test_product<double, 1>();
    test_product<double, 2>();
    test_product<double, 3>();
    test_product<double, 4>();
    test_upper_bound_infinite<float>();
    test_upper_bound_infinite<double>();
    test_lower_bound_infinite<float>();
    test_lower_bound_infinite<double>();
    test_double_infinite<float>();
    test_double_infinite<double>();
}
