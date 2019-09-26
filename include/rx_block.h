//
// Created by wuyua on 2019-09-24.
//

#ifndef RXSIMEX_RX_BLOCK_H
#define RXSIMEX_RX_BLOCK_H
#include <fmt/chrono.h>
#include <fmt/format.h>
#include <rx.hpp>
#include <utility>
#include "rx_param.h"
#include "rx_port.h"
#include "sample_time_t.h"
#include "utils.h"

namespace simex {
class rx_block {
 public:
  sptr_vector<port_base> input_ports{};
  sptr_vector<port_base> output_ports{};
  sptr_vector<rx_param_base> dialog_params{};
  SimStruct *S;
  bool is_started{false};

 public:
  explicit rx_block(SimStruct *S) : S(S) {}
  virtual ~rx_block() { subscriptions.unsubscribe(); }

 public:
  ///# Rx internal

  /// run_loop object for executing the delegated works in the simulink thread.
  std::unique_ptr<rxcpp::schedulers::run_loop> rl;

  /// can be used for subscribe_on and observe_on to delegate the work to main
  /// thread. When interacting with MATLAB engine the operation must be done in
  /// main thread.
  std::unique_ptr<rxcpp::observe_on_one_worker> simulink_thread;

  /// will be unsubscribed in on_terminate. More subscriptions can be added to
  /// this one to be disposed at the end of execution.
  rxcpp::composite_subscription subscriptions;

 public:
  ///# Properties

  /// Set true if the output is more than 2-D. For example, an RGB image. Can be
  /// set either in `constructor` or `on_initial_parameter_processed`
  bool allow_multi_dimension{false};
  /// Set this sample time in `constructor` or `on_initial_parameter_processed`.
  /// Currently multi-sample-time block is not implemented.
  sample_time_t sample_time;
  /// Simulink block options. Should be set in `constructor`. By default
  /// `SS_OPTION_CALL_TERMINATE_ON_EXIT` is set to ensure the resources being
  /// released. Refer to
  /// https://www.mathworks.com/help/simulink/sfg/sssetoptions.html#btikl28 for
  /// available settings.
  unsigned int options{SS_OPTION_CALL_TERMINATE_ON_EXIT};

 protected:
  /// Register a input port.
  /// \tparam T type of the data array
  /// \param port_name name of the port
  /// \param dims dimension
  /// \param direct_feed_through use direct feedthrough (read in mdlOutput
  /// instead of mdlUpdate) \return created instance
  template <typename T>
  std::shared_ptr<port<T>> register_input_port(std::string &&port_name,
                                               xt::xarray<int> &&dims,
                                               bool direct_feed_through,
                                               bool frame, bool complex) {
    auto p = std::make_shared<port<T>>();
    p->name = port_name;
    p->dims = dims;
    p->type_id = get_type_id<T>();
    p->direct_feed_through = direct_feed_through;
    p->frame = frame;
    p->complex = complex;
    p->id = input_ports.size();
    p->on_dimension_update();
    p->port_type = port_base::INPUT;
    input_ports.emplace_back(p);
    return p;
  }

  /// Register a output port.
  /// \tparam T type of the data array
  /// \param port_name name of the port
  /// \param dims dimension
  /// \return created instance
  template <typename T>
  std::shared_ptr<port<T>> register_output_port(std::string &&port_name,
                                                xt::xarray<int> &&dims,
                                                bool frame, bool complex) {
    auto p = std::make_shared<port<T>>();
    p->name = port_name;
    p->dims = dims;
    p->type_id = get_type_id<T>();
    p->frame = frame;
    p->complex = complex;
    p->id = output_ports.size();
    p->port_type = port_base::OUTPUT;
    p->on_dimension_update();
    output_ports.emplace_back(p);
    return p;
  }

  /// Register a dialog parameter. Must be called in constructor.
  /// \tparam T intended type
  /// \param name parameter name
  /// \param tunable realtime tunable
  /// \return created instance
  template <typename T>
  std::shared_ptr<rx_scalar_param<T>> register_scalar_parameter(
      std::string &&name, bool tunable) {
    auto p = std::make_shared<rx_scalar_param<T>>();
    p->name = name;
    p->tunable = tunable;
    dialog_params.emplace_back(p);
    return p;
  }

  /// Register a string dialog parameter. Must be called in constructor.
  /// \param name parameter name
  /// \param tunable realtime tunable
  /// \return created instance
  std::shared_ptr<rx_string_param> register_string_parameter(
      std::string &&name) {
    auto p = std::make_shared<rx_string_param>();
    p->name = name;
    p->tunable = false;
    dialog_params.emplace_back(p);
    return p;
  }

 public:
  /// #callbacks

  /// Called when the dialog parameter being processed. Useful when the number
  /// of ports or other properties are based on configurations. The block
  /// parameter should be registered in the constructor, otherwise MATLAB may
  /// crash.
  virtual void on_initial_parameter_processed() {}

  /// Called when the simulation is just started. The runtime-related operations
  /// such as thread startup should be done here rather than the constructor.
  /// Remember to call the base class `on_start` to ensure the underlying event
  /// loop being created.
  virtual void on_start() {
    is_started = true;
    // initialize event-loop and main thread scheduler
    rl = std::make_unique<rxcpp::schedulers::run_loop>();
    simulink_thread = std::make_unique<rxcpp::observe_on_one_worker>(
        rxcpp::observe_on_run_loop(*rl));

    // listening to logging channel
    logging_channel.get_observable()
        .observe_on(*simulink_thread)
        .subscribe(
            subscriptions, [](std::string str) { ssPrintf(str.c_str()); },
            [](std::exception_ptr ep) {});
  }

  /// Called when the sample point is hit. Remember to call base class's
  /// `on_update` method to dispatch queued function call in the
  /// `simulink_thread`. Note that `directFeedthrough` input ports will not be
  /// updated in this routine.
  virtual void on_update() {}

  /// Called after on_update. The `simulink_thread` is also executed in this
  /// callback to dispatch any operation that may rely on the direct feed
  /// through inputs.
  virtual void on_output() {}

  /// Called when simulation is stopped or fatal error occured. Resources may be
  /// explicitly released in this callback.
  virtual void on_terminate() { is_started = false; }

  /// Called when tunable parameter got updated. Should check is_started flag.
  virtual void on_parameter_updated() {}

  /// Called when the block is enabled in an enabled subsystem
  virtual void on_enabled() {}

  /// Called when the block is disabled in an enabled subsystem
  virtual void on_disabled() {}

  /// Called when the block is initialized or re-enabled in an enabled block
  /// with reset condition.
  /// \param first first-time initial condition
  virtual void on_initial_condition(bool first) {}

 public:
  ///# static lifetime functions

  /// Tell the framework how to create the block.
  /// \param S
  /// \return
  static std::shared_ptr<rx_block> create_block(SimStruct *S);

  /// The created block instance
  static std::shared_ptr<rx_block> instance;

  /// Get the block instance
  /// \param S
  /// \param create_new destroy and re-create a new block
  /// \return
  static std::shared_ptr<rx_block> get_instance(SimStruct *S,
                                                bool create_new = false) {
    if (create_new) {
      destroy_instance();
      instance = create_block(S);
    }
    return instance;
  }

  /// Destroy the instance
  static void destroy_instance() {
    if (instance) {
      instance.reset();
    }
  }

 public:
  ///# Logging utilities

  /// logging subject
  rxcpp::subjects::subject<std::string> logging_channel;

  /// Log message
  /// \param tag
  /// \param message
  /// \param immediate do not delegate this log to simulink_thread. Only use in
  /// the block callbacks and constructor or MATLAB will crash.
  void log(const std::string &&tag, const std::string &&message,
           bool immediate = false) {
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
  /// private error report function. Report uncaught error.
  /// \param e
  static void report_error(std::exception_ptr e) {
    try {
      rethrow_exception(std::move(e));
    } catch (const std::exception &ex) {
      ssPrintf("**Error Report**: %s \n", ex.what());
    }
  }
};  // class

}  // namespace simex

#endif  // RXSIMEX_RX_BLOCK_H
