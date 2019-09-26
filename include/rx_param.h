//
// Created by wuyuanyi on 2019-09-24.
//

#ifndef RXSIMEX_RX_PARAM_H
#define RXSIMEX_RX_PARAM_H
#include "rx.hpp"
#include "simstruc.h"
#include "matrix.h"
#include "utils.h"
#include "xtensor/xadapt.hpp"

namespace simex {
class rx_param_base {
 public:
  std::string name;
  bool tunable;

  /// Low level callback to update parameter level.
  /// \param var
  /// \return true for accepted change, otherwise, return false to terminate simulation
  virtual bool update_parameter(const mxArray *var) = 0;
};

template<typename T>
class rx_scalar_param : public rx_param_base {
 protected:
  mxClassID class_id;
 public:
  rxcpp::subjects::behavior<T> parameter_updated{T()};
 public:
  explicit rx_scalar_param() {
      class_id = get_mx_class_id<T>();
  }

  bool update_parameter(const mxArray *var) override {
      mxClassID id = mxGetClassID(var);
      if (id != class_id) {
          return false;
      }

      // expected scalar
      auto data = static_cast<T *>(mxGetData(var));
      parameter_updated.get_subscriber().on_next(*data);

      return true;
  }

};
}
#endif //RXSIMEX_RX_PARAM_H
