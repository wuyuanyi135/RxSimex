//
// Created by wuyua on 2019-09-24.
//


#ifndef RXSIMEX_RX_BLOCK_H
#define RXSIMEX_RX_BLOCK_H
#include <fmt/format.h>
#include <fmt/chrono.h>
#include <rx.hpp>
#include <utility>
#include "rx_port.h"
#include "rx_param.h"
#include "sample_time_t.h"
#include "utils.h"

namespace simex {
class rx_block {
 public:
  sptr_vector<port_base> input_ports{};
  sptr_vector<port_base> output_ports{};
  sptr_vector<rx_param_base> dialog_params{};
  SimStruct *S;
 public:
  explicit rx_block(SimStruct *S) : S(S) {
      // listening to logging channel
      logging_channel.get_observable().observe_on(simulink_thread).subscribe(
          subscriptions,
          [](std::string str) {
            ssPrintf(str.c_str());
          },
          [](std::exception_ptr ep) {}
      );
  }
  virtual ~rx_block() {
      subscriptions.unsubscribe();
  }

 public:
  // internal variables
  // main loop scheduler
  rxcpp::schedulers::run_loop rl;
  rxcpp::observe_on_one_worker simulink_thread{rxcpp::observe_on_run_loop(rl)};
  rxcpp::composite_subscription subscriptions;
 public:
  // properties
  bool allow_multi_dimension{false};
  sample_time_t sample_time;
  unsigned int options{SS_OPTION_CALL_TERMINATE_ON_EXIT};
 protected:
  /// Register a input port
  /// \tparam T type of the data array
  /// \param port_name name of the port
  /// \param dims dimension
  /// \param direct_feed_through use direct feedthrough (read in mdlOutput instead of mdlUpdate)
  /// \return created instance
  template<typename T>
  std::shared_ptr<port<T>> register_input_port(std::string &&port_name,
                                               xt::xarray<int> &&dims,
                                               bool direct_feed_through,
                                               bool frame,
                                               bool complex) {
      auto p = std::make_shared<port<T>>();
      p->name = port_name;
      p->dims = dims;
      p->type_id = get_type_id<T>();
      p->direct_feed_through = direct_feed_through;
      p->frame = frame;
      p->complex = complex;
      p->id = input_ports.size();
      p->on_dimension_update();
      input_ports.emplace_back(p);
      return p;
  }

  /// Register a output port
  /// \tparam T type of the data array
  /// \param port_name name of the port
  /// \param dims dimension
  /// \return created instance
  template<typename T>
  std::shared_ptr<port<T>> register_output_port(std::string &&port_name,
                                                xt::xarray<int> &&dims,
                                                bool frame,
                                                bool complex) {
      auto p = std::make_shared<port<T>>();
      p->name = port_name;
      p->dims = dims;
      p->type_id = get_type_id<T>();
      p->frame = frame;
      p->complex = complex;
      p->id = output_ports.size();
      p->on_dimension_update();
      output_ports.emplace_back(p);
      return p;
  }

  /// register a dialog parameter
  /// \tparam T intended type
  /// \param name parameter name
  /// \param tunable realtime tunable
  /// \return created instance
  template<typename T>
  std::shared_ptr<rx_scalar_param<T>> register_scalar_parameter(std::string &&name, bool tunable) {
      auto p = std::make_shared<rx_scalar_param<T>>();
      p->name = name;
      p->tunable = tunable;
      dialog_params.emplace_back(p);
      return p;
  }

 public:
  // callbacks
  virtual void on_initial_parameter_processed() {}
  virtual void on_start() {}
  virtual void on_update() {}
  virtual void on_output() {}
  virtual void on_terminate() {}
 public:

  static std::shared_ptr<rx_block> create_block(SimStruct *S);
  static std::shared_ptr<rx_block> instance;
  static std::shared_ptr<rx_block> get_instance(SimStruct *S, bool create_new = false) {
      if (create_new) {
          instance = create_block(S);
      }
      return instance;
  }
  static void destory_instance() {
      if (instance) {
          instance.reset();
      }
  }
 public:
  // logging
  rxcpp::subjects::subject<std::string> logging_channel;
  void log(const std::string &&tag, const std::string &&message, bool immediate = false) {
#if defined(NDEBUG)
#else
      std::time_t t = std::time(nullptr);
      std::string now = fmt::format("{:%Y-%m-%d %H:%M:%S}", *std::localtime(&t));
      auto text = fmt::format("[{}] @{}->{}: {}\n", tag, now, S->path, message);

      if (immediate) {
          ssPrintf(text.c_str());
      } else {
          logging_channel.get_subscriber().on_next(text);
      }
#endif
  }


 private:
  static void report_error(std::exception_ptr e) {
      try { rethrow_exception(std::move(e)); }
      catch (const std::exception &ex) {
          ssPrintf("**Error Report**: %s \n", ex.what());
      }
  }
};  // class

} // namespace

#endif //RXSIMEX_RX_BLOCK_H
