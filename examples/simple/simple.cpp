//
// Created by wuyua on 2019-09-24.
//
#include "rx_simex.h"
#include <xtensor/xrandom.hpp>

using namespace std::chrono_literals;
class simple : public simex::rx_block {
 public:
  std::unique_ptr<rxcpp::observe_on_one_worker> background_thread;

  explicit simple(SimStruct *s) : rx_block(s) {
      param_gain = register_scalar_parameter<double>("gain", true);
  }
  void on_initial_parameter_processed() override {
      rx_block::on_initial_parameter_processed();
      sample_time.offset_time = 0;
      sample_time.sample_time = 1;

      in1 = register_input_port<double>("in1", {-1}, true, false, false); // dynamic-sized
      out1 = register_output_port<double>("out1", {5, 2}, false, false);
      out2 = register_output_port<double>("out2", {5}, false, false);
      out3 = register_output_port<double>("out3", {1}, false, false);
      log("info", "on_initial_parameter", true);
  }
  void on_start() override {
      rx_block::on_start();
      // run background thread only after started.
      background_thread = std::make_unique<rxcpp::observe_on_one_worker>(rxcpp::observe_on_new_thread());

      auto emitter = rxcpp::observable<>::interval(std::chrono::milliseconds(500));
      emitter.subscribe_on(*background_thread).subscribe(
          subscriptions,
          [this](int v) {
            log("info", "hello!!!");
          }
      );

      emitter.subscribe_on(*background_thread).subscribe(
          subscriptions,
          [this](int v) {
            out1->data_update.get_subscriber().on_next(xt::ones<double>(out1->dims) * v);
            out2->data_update.get_subscriber().on_next(xt::random::rand<double>(out2->dims));
          }
      );

      in1->data_updated.get_observable().subscribe_on(*background_thread).subscribe(
          subscriptions,
          [this](xt::xarray<double> in1_data) {
            std::stringstream sstream;
            sstream << in1_data;
            log("info", sstream.str());
              out3->data_update.get_subscriber().on_next(xt::sum<double>(in1_data) * param_gain->data);
          }
      );

      param_gain->parameter_updated.get_observable().subscribe(
          subscriptions,
          [&](double val){log("info", fmt::format("Gain updated: {}", val));}
      );
  }

  std::shared_ptr<simex::port<double>> in1;
  std::shared_ptr<simex::port<double>> out1;
  std::shared_ptr<simex::port<double>> out2;
  std::shared_ptr<simex::port<double>> out3;
  std::shared_ptr<simex::rx_scalar_param<double>> param_gain;
};

std::shared_ptr<simex::rx_block> simex::rx_block::create_block(SimStruct *S) {
    return std::make_shared<simple>(S);
}
