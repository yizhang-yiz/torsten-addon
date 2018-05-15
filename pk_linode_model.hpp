#ifndef PK_LINODE_MODEL_HPP
#define PK_LINODE_MODEL_HPP

namespace refactor {

  using boost::math::tools::promote_args;
  using Eigen::Matrix;
  using Eigen::Dynamic;

  // depend on model, we can have arbitrary number of
  // parameters, e.g. biovar, k12, k10, ka. Each parameter
  // can be data or var.
  template<typename T_time, typename T_init, typename T_rate, typename T_par>
  class PKLinODEModel {
    const T_time &t0_;
    const Eigen::Matrix<T_init, 1, Eigen::Dynamic>& y0_;
    const std::vector<T_rate> &rate_;
    const Matrix<T_par, Dynamic, Dynamic> ode_; // FIXME: use reference

  public:
    // static const int ncmt = ode_; 
    using scalar_type = typename promote_args<T_time, T_rate, T_par, T_init>::type;
    using par_type = T_par;

    // constructors
    // PKLinODEModel(const T_time& t0, const Eigen::Matrix<T_init, 1, Eigen::Dynamic>& y0,
    //               const std::vector<T_rate> &rate,
    //               const Matrix<T_par, Dynamic, Dynamic> &ode) :
    template<template<typename...> class T_mp, typename... Ts>
    PKLinODEModel(const T_time& t0, const Eigen::Matrix<T_init, 1, Eigen::Dynamic>& y0,
                  const std::vector<T_rate> &rate,
                  const std::vector<T_par> & par,
                  const T_mp<Ts...> &parameter) :
      t0_(t0),
      y0_(y0),
      rate_(rate),
      ode_(parameter.get_K())
    {}

    // copy constructor
    // PKLinODEModel(const PKLinODEModel& m) :
    //   PKLinODEModel(m.t0_, m.y0_, m.rate_, m.ode_)
    // {}

    // get
    const T_time              & t0()   { return t0_; }
    const Eigen::Matrix<T_init, 1, Eigen::Dynamic>& y0() { return y0_; }
    const std::vector<T_rate> & rate() { return rate_; }
    const Eigen::Matrix<T_par, Dynamic, Dynamic> &rhs_matrix () { return ode_; }

    // can be solved by linear ode solver
  };

}

#endif