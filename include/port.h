//
// Created by wuyua on 2019-09-24.
//

#ifndef RXSIMEX_PORT_H
#define RXSIMEX_PORT_H

#include <xtensor/xarray.hpp>
#include <rx.hpp>

namespace simex {
template<typename T>
class port {
 public:
  explicit port() {}

 public:
  int id{};
  std::string name;
  xt::xarray<T> data;
  xt::xarray<int> dims;
  int type_id{};
};

template<typename T>
class output_port : public port<T> {
};

}
#endif //RXSIMEX_PORT_H
