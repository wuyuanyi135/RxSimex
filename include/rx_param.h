//
// Created by wuyuanyi on 2019-09-24.
//

#ifndef RXSIMEX_RX_PARAM_H
#define RXSIMEX_RX_PARAM_H
#include "matrix.h"
#include "rx.hpp"
#include "simstruc.h"
#include "utils.h"
#include "xtensor/xadapt.hpp"

namespace simex {
class rx_param_base {
 public:
  std::string name;
  bool tunable;

  /// Low level callback to update parameter level.
  /// \param var
  /// \return true for accepted change, otherwise, return false to terminate
  /// simulation
  virtual bool update_parameter(const mxArray *var) = 0;

  virtual std::string to_string() = 0;
};

template <typename T>
class rx_scalar_param : public rx_param_base {
  static_assert(std::is_floating_point<T>::value || std::is_integral<T>::value);
 protected:
  mxClassID class_id;

 public:
  rxcpp::subjects::subject<T> parameter_updated{};
  T data;

 public:
  explicit rx_scalar_param() { class_id = get_mx_class_id<T>(); }

  bool update_parameter(const mxArray *var) override {
    mxClassID id = mxGetClassID(var);
    if (id != class_id) {
      return false;
    }

    // expected scalar
    auto val = static_cast<T *>(mxGetData(var));
    data = *val;
    parameter_updated.get_subscriber().on_next(*val);

    return true;
  }
  std::string to_string() override { return std::to_string(data); }
};

class rx_string_param : public rx_param_base {
 public:
  rxcpp::subjects::subject<std::string> parameter_updated{};
  std::string data;

 public:
  explicit rx_string_param() {}

  bool update_parameter(const mxArray *var) override {
    mxClassID id = mxGetClassID(var);
    if (id != mxCHAR_CLASS) {
      return false;
    }

    auto c_str = mxArrayToString(var);
    assert(c_str);
    data = std::string(c_str);
    parameter_updated.get_subscriber().on_next(data);

    return true;
  }
  std::string to_string() override { return data; }
};
}  // namespace simex
#endif  // RXSIMEX_RX_PARAM_H
