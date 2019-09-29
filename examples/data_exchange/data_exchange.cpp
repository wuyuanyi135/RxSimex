//
// Created by wuyuanyi on 2019-09-26.
//

#include "rx_remote_block.h"
#include "rx_port.h"

class data_exchange : public simex::rx_remote_block {
 public:
  explicit data_exchange(SimStruct *s) : rx_remote_block(s) {
    in1 = register_input_port<double>("in1", {1}, false, false, false);
    in2 = register_input_port<double>("in2", {-1}, false, false, false);
    out1 = register_output_port<double>("out1", {1}, false, false);
    out2 = register_output_port<double>("out2", {3,4}, false, false);

    set_remote_accessible_port(std::dynamic_pointer_cast<simex::port_base>(in1));
    set_remote_accessible_port(std::dynamic_pointer_cast<simex::port_base>(in2));
    set_remote_accessible_port(std::dynamic_pointer_cast<simex::port_base>(out1));
    set_remote_accessible_port(std::dynamic_pointer_cast<simex::port_base>(out2));
  }
  std::shared_ptr<simex::port<double>> in1;
  std::shared_ptr<simex::port<double>> in2;
  std::shared_ptr<simex::port<double>> out1;
  std::shared_ptr<simex::port<double>> out2;
};

std::shared_ptr<simex::rx_block> simex::rx_block::create_block(SimStruct *S) {
  return std::make_shared<data_exchange>(S);
}

