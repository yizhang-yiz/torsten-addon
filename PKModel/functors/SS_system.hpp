#ifndef STAN_MATH_TORSTEN_PKMODEL_FUNCTORS_SS_SYSTEM_HPP
#define STAN_MATH_TORSTEN_PKMODEL_FUNCTORS_SS_SYSTEM_HPP

#include <stan/math/rev/core.hpp>
#include <stan/math/fwd/core.hpp>
#include <vector>
#include <iostream>

/**
 * A structure to store the algebraic system
 * which gets solved when computing the steady
 * state solution.
 * 
 * In this structure, both amt and rate are fixed
 * variables.
 */
template <typename F>
struct SS_system_dd {
  F f_;
  double ii_;
  int cmt_;  // dosing compartment
  integrator_structure integrator_;

  SS_system_dd () { };

  SS_system_dd (const F& f,
                const double& ii,
                int cmt,
                const integrator_structure& integrator)
   : f_(f), ii_(ii), cmt_(cmt), integrator_(integrator) { }

  /**
   *  dd regime.
   *  dat contains the rates in each compartment followed
   *  by the adjusted amount (biovar * amt).
   */
  template <typename T0, typename T1>
  inline
  Eigen::Matrix<typename boost::math::tools::promote_args<T0, T1>::type,
                Eigen::Dynamic, 1>
  operator()(const Eigen::Matrix<T0, Eigen::Dynamic, 1>& x,
             const Eigen::Matrix<T1, Eigen::Dynamic, 1>& y,
             const std::vector<double>& dat,
             const std::vector<int>& dat_int,
             std::ostream* msgs) const {
    using stan::math::to_array_1d;
    using stan::math::to_vector;
    using std::vector;
    using stan::math::to_vector;

    typedef typename boost::math::tools::promote_args<T0, T1>::type scalar;
    typedef typename stan::return_type<T0, T1>::type T_deriv;

    double t0 = 0;
    vector<double> ts(1);
    vector<double> rate_v(dat.size() - 1, 0);
    for (size_t i = 0; i < rate_v.size(); i++) rate_v[i] = dat[i];

    vector<scalar> x0(x.size());
    for (size_t i = 0; i < x0.size(); i++) x0[i] = x(i);
    double amt = dat[dat.size() - 1];
    double rate = dat[cmt_ - 1];

    Eigen::Matrix<scalar, Eigen::Dynamic, 1> result(x.size());

    if (rate == 0) {  // bolus dose
      x0[cmt_ - 1] += amt;
      ts[0] = ii_;
      vector<scalar> pred = integrator_(f_, x0, t0, ts, to_array_1d(y),
                                        dat, dat_int)[0];

      for (int i = 0; i < result.size(); i++)
        result(i) = x(i) - pred[i];

    } else if (ii_ > 0) {  // multiple truncated infusions
      double delta = amt / rate;
      if(delta > ii_) {
        std::string msg = " but must be smaller than the interdose interval (ii): "  // NOLINT
          + boost::lexical_cast<std::string>(ii_) + "!";
        const char* msg2 = msg.c_str();
        stan::math::invalid_argument("Steady State Solution",
                                     "Infusion time (F * amt / rate)", delta,
                                     "is ", msg2);
      }

      vector<scalar> pred;
      ts[0] = delta;  // time at which infusion stops
      x0 = integrator_(f_, to_array_1d(x), t0, ts, to_array_1d(y),
                       dat, dat_int)[0];
      ts[0] = ii_ - delta;
      vector<double> rate_v(dat.size(), 0);
      pred = integrator_(f_, x0, t0, ts, to_array_1d(y), rate_v, dat_int)[0];

      // The commented out section of the code corresponds to an implementation
      // of the SS solution for the case where delta > ii_. Might be useful
      // for future releases. Also, still needs to be tested. Still need to
      // figure out what to deal with N, the number of overlapping infusions,
      // which will be a discrete parameters.
      /*
        else {
        int N = trunc(delta / ii_) + 1;  // number of overlapping rates
        ts[0] = delta - (N - 1) * ii_;  // time at which the oldest infusion dies
        vector<double> rate_v(dat.size());
        for (size_t i = 0; i < rate_v.size(); i++)
          rate_v[i] = N * dat[i];  // compute superposition of rates

        x0 = integrator_(f_, to_array_1d(x), t0, ts, to_array_1d(y),
                         rate_v, dat_int)[0];

        ts[0] = ii_ - ts[0];
        for (size_t i = 0; i < rate_v.size(); i++)
          rate_v[i] = (N - 1) * dat[i];

        pred = integrator_(f_, to_array_1d(x), t0, ts,
                           to_array_1d(y), rate_v, dat_int)[0];

      } */
      for (int i = 0; i < result.size(); i++)
        result(i) = x(i) - pred[i];

    } else {  // constant infusion
      vector<T_deriv> derivative = f_(0, to_array_1d(x), to_array_1d(y),
                                      rate_v, dat_int, 0);
      result = to_vector(derivative);
    }

    return result;
  }
};

/**
 * A structure to store the algebraic system
 * which gets solved when computing the steady
 * state solution.
 *
 * In this structure, amt is a random variable
 * and rate a fixed variable (vd regime).
 */
template <typename F>
struct SS_system_vd {
  F f_;
  double ii_;
  int cmt_;  // dosing compartment
  integrator_structure integrator_;

  SS_system_vd () { };

  SS_system_vd (const F& f,
                const double& ii,
                int cmt,
                const integrator_structure& integrator)
    : f_(f), ii_(ii), cmt_(cmt), integrator_(integrator) { }

 /**
  *  Case where the modified amt is a random variable. This
  *  will usually happen because biovar is a parameter, making 
  *  amt a transformed parameter.
  *  The last element of y is contains amt.
  *  dat stores the rate.
  */
  template <typename T0, typename T1>
  inline
    Eigen::Matrix<typename boost::math::tools::promote_args<T0, T1>::type,
                  Eigen::Dynamic, 1>
  operator()(const Eigen::Matrix<T0, Eigen::Dynamic, 1>& x,
             const Eigen::Matrix<T1, Eigen::Dynamic, 1>& y,
             const std::vector<double>& dat,
             const std::vector<int>& dat_int,
             std::ostream* msgs) const {
    using stan::math::to_array_1d;
    using stan::math::to_vector;
    using std::vector;
    using stan::math::to_vector;
    using stan::math::invalid_argument;

    typedef typename boost::math::tools::promote_args<T0, T1>::type scalar;
    typedef typename stan::return_type<T0, T1>::type T_deriv;

    double t0 = 0;
    vector<double> ts(1);
    vector<double> rate_v(dat.size(), 0);
    for (size_t i = 0; i < rate_v.size(); i++) rate_v[i] = dat[i];

    vector<scalar> x0(x.size());
    for (size_t i = 0; i < x0.size(); i++) x0[i] = x(i);
    scalar amt = y(y.size() - 1);
    double rate = dat[cmt_ - 1];

    Eigen::Matrix<scalar, Eigen::Dynamic, 1> result(x.size());
    std::vector<scalar> parms(y.size() - 1);
    for (size_t i = 0; i < parms.size(); i++) parms[i] = y(i);

    if (rate == 0) {  // bolus dose
      x0[cmt_ - 1] += amt;
      ts[0] = ii_;
      vector<scalar> pred = integrator_(f_, x0, t0, ts, parms,
                                        dat, dat_int)[0];

      for (int i = 0; i < result.size(); i++)
        result(i) = x(i) - pred[i];

    } else if (ii_ > 0) {  // multiple truncated infusions
      // FIX ME: can actually work out a solution.
      invalid_argument("Steady State Solution",
                       "Current version does not handle case of a",
                       "", " multple truncated infusion solution",
                       "(i.e ii > 0 and rate > 0) when F * amt is a parameter");  // NOLINT

    } else {  // constant infusion
      vector<T_deriv> derivative = f_(0, to_array_1d(x), parms,
                                      dat, dat_int, 0);
      result = to_vector(derivative);
    }

    return result;
  }
};

#endif
