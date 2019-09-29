# Steps to create a new block

## Configure project
1. clone or add this project as submodule. 
2. `git submodule update --init --recursive` to clone the submodules
3. in your `CMakeLists.txt`, include the `CMakeLists.txt` of this project.
4. call `register_target(YOUR_S_FUN_NAME ${SOURCES} ${EXTRA_LIBRARIES})` to create target

## Write your block
Refer to `examples/`. Remember to implement `std::shared_ptr<simex::rx_block> simex::rx_block::create_block(SimStruct *S)` to tell how to create the block