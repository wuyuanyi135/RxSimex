//
// Created by wuyua on 2019-09-24.
//

#ifndef RXSIMEX_RX_PORT_H
#define RXSIMEX_RX_PORT_H

#include <xtensor/xarray.hpp>
#include <xtensor/xmath.hpp>
#include <xtensor/xadapt.hpp>
#include <xtensor/xio.hpp>
#include <rx.hpp>

namespace simex {
class rx_block;
class port_base {
 public:
  port_base() = default;
 public:
  int id{};
  std::string name;
  xt::xarray<int> dims;
  int type_id{};
  bool direct_feed_through{};
  bool frame{};
  bool complex{};

 public:
  virtual void to(void *dest, size_t size = -1) = 0;

  ///
  /// \param src input signal ptr. Note that the pointer is doubled for non-contiguous data.
  /// \param size
  virtual void from(const void *const* src, size_t size = -1) = 0;
  virtual void on_dimension_update() = 0;
  virtual size_t get_width() = 0;
  virtual size_t get_num_elements() = 0;
};

template<typename T>
class port : public port_base {
 public:
  using ArrayT = xt::xarray<T, xt::layout_type::column_major>;
  explicit port() {
      auto subscriber = rxcpp::make_subscriber<ArrayT>(
          _subscription,
          [this](ArrayT arr) {
            std::lock_guard lock(mutex);
            data = arr;
            data_updated.get_subscriber().on_next(data);
          }
      );
      data_update.get_observable().subscribe(subscriber);
  };
  virtual ~port() {
      _subscription.unsubscribe();
  }

 private:
  ArrayT data;
  std::mutex mutex;
 public:
  void to(void *dest, size_t size = -1) override {
      if (size == -1) {
          size = get_width();
      }
      std::lock_guard lock(mutex);
      memcpy(dest, data.data(), size);
  }
  void from(const void *const *src, size_t elements = -1) override {
      if (elements == -1) {
          elements = data.size();
      }
      assert(elements <= data.size());
      std::lock_guard lock(mutex);
      for (size_t i = 0; i < elements; i ++) {
          data.data_element(i) = *static_cast<const T*>(src[i]);
      }
      data_updated.get_subscriber().on_next(data);
  }
  size_t get_width() override {
      return sizeof(T) * get_num_elements();
  }
  void on_dimension_update() override {
      std::lock_guard lock(mutex);
      if (xt::any(dims <= 0)) {
          return;
      }
      data = xt::zeros<T>(dims);
  }
  size_t get_num_elements() override {
      return xt::prod(dims)(0);
  }
 public:
  rxcpp::subjects::subject<ArrayT> data_updated{};
  rxcpp::subjects::subject<ArrayT> data_update{};

 private:
  rxcpp::composite_subscription _subscription{};
};

}
#endif //RXSIMEX_RX_PORT_H
