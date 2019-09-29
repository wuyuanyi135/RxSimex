//
// Created by wuyua on 2019-09-24.
//
#include "rx_simex.h"
#include "xtensor/xmath.hpp"
#include "xtensor/xadapt.hpp"
#include "simstruc.h"

const int MAX_DIM = 32;

#if defined(MATLAB_MEX_FILE)

#define MDL_CHECK_PARAMETERS
void mdlCheckParameters(SimStruct *S) {
    auto block = simex::rx_block::instance;
    assert(block);

    for (int i = 0; i < block->dialog_params.size(); i++) {
        if (!block->dialog_params[i]->update_parameter(ssGetSFcnParam(S, i))) {
            throw std::invalid_argument(std::string("failed to update param #") + std::to_string(i) + " due to type assertion failure");
        }
    }
}


extern "C" void mdlInitializeSizes(SimStruct *S) {
    auto block = simex::rx_block::get_instance(S, true);
    ssSetOptions(S, block->options);

    ssSetNumSFcnParams(S, static_cast<int_T>(block->dialog_params.size()));
    if (ssGetNumSFcnParams(S) == ssGetSFcnParamsCount(S)) {
        mdlCheckParameters(S);
        if (ssGetErrorStatus(S) != NULL) {
            throw std::invalid_argument(ssGetErrorStatus(S));
        }
    } else {
        throw std::invalid_argument("Number of parameter mismatch"); /* Parameter mismatch reported by the Simulink engine*/
    }

    for (int i = 0; i < block->dialog_params.size(); i++) {
        ssSetSFcnParamTunable(S, i, block->dialog_params[i]->tunable ? SS_PRM_TUNABLE : SS_PRM_NOT_TUNABLE);
    }

    block->on_initial_parameter_processed();


    if (block->allow_multi_dimension) {
        ssAllowSignalsWithMoreThan2D(S);
    }

    // set input ports
    ssSetNumInputPorts(S, static_cast<int_T>(block->input_ports.size()));
    for (int i = 0; i < block->input_ports.size(); ++i) {
        auto &ip = block->input_ports[i];
        ssSetInputPortFrameData(S, i, ip->frame);
        ssSetInputPortComplexSignal(S, i, ip->complex);
        ssSetInputPortDirectFeedThrough(S, i, ip->direct_feed_through);
        ssSetInputPortDataType(S, i, ip->type_id);

        // dimension
        auto size = ip->dims.size();
        if (size == 1 && ip->dims(0) == -1) {
            // dynamic
            ssSetInputPortDimensionInfo(S, i, DYNAMIC_DIMENSION);
        } else {
            DECL_AND_INIT_DIMSINFO(di);
            int_T dims[MAX_DIM];
            if (size > MAX_DIM) {
                throw std::range_error("Too many dimensions (size > MAX_DIM)");
            }

            for (int i = 0; i < size; i++) {
                assert (ip->dims(i) >= 0);
                dims[i] = ip->dims(i);
            }
            di.width = ip->get_num_elements();
            di.numDims = size;
            di.dims = dims;
            ssSetInputPortDimensionInfo(S, i, &di);
        }
    }

    ssSetNumOutputPorts(S, static_cast<int_T >(block->output_ports.size()));
    for (int i = 0; i < block->output_ports.size(); i++) {
        auto &op = block->output_ports[i];
        ssSetOutputPortFrameData(S, i, op->frame);
        ssSetOutputPortComplexSignal(S, i, op->complex);
        ssSetOutputPortDataType(S, i, op->type_id);

        auto size = op->dims.size();
        if (size == 1 && op->dims(0) == -1) {
            // dynamic
            ssSetOutputPortDimensionInfo(S, i, DYNAMIC_DIMENSION);
        } else {
            DECL_AND_INIT_DIMSINFO(di);
            auto sz = size;

            int_T dims[MAX_DIM];

            if (sz > MAX_DIM) {
                throw std::range_error("Too many dimensions (sz > MAX_DIM)");
            }

            for (int i = 0; i < sz; i++) {
                assert(op->dims(i) > 0);
                dims[i] = op->dims(i);
            }

            di.width = op->get_num_elements();
            di.numDims = sz;
            di.dims = dims;
            ssSetOutputPortDimensionInfo(S, i, &di);
        }
        ssSetNumSampleTimes(S, 1);
    }
}

extern "C" void mdlInitializeSampleTimes(SimStruct *S) {
    auto block = simex::rx_block::instance;
    assert(block);
    auto st = block->sample_time;
    ssSetSampleTime(S, 0, st.sample_time);
    ssSetOffsetTime(S, 0, st.offset_time);
}

#define MDL_START
extern "C" void mdlStart(SimStruct *S) {
    auto block = simex::rx_block::instance;
    assert(block);

    block->on_start();
}

extern "C" void mdlOutputs(SimStruct *S, int_T tid) {
    auto block = simex::rx_block::instance;
    assert(block);
    auto &rl = block->rl;

    // update direct feed through
    for (int i = 0; i < block->input_ports.size(); i++) {
        auto &p = block->input_ports[i];
        if (p->direct_feed_through) {
            auto input_ptr = ssGetInputPortSignalPtrs(S, i);
            block->input_ports[i]->from(input_ptr);
        }
    }

    block->on_output();

    // in case of direct feed through
    assert(rl);
    while (!rl->empty() && rl->peek().when < rl->now()) {
        rl->dispatch();
    }

    for (int i = 0; i < block->output_ports.size(); i++) {
        auto output_ptr = ssGetOutputPortSignal(S, i);
        block->output_ports[i]->to(output_ptr);

    }

}

extern "C" void mdlTerminate(SimStruct *S) {
    auto block = simex::rx_block::instance;
    if (block) {
        block->on_terminate();
    }
    simex::rx_block::destroy_instance();
}

#define MDL_UPDATE
extern "C" void mdlUpdate(SimStruct *S, int_T tid) {
    auto block = simex::rx_block::instance;
    assert(block);

    for (int i = 0; i < block->input_ports.size(); i++) {
        auto &p = block->input_ports[i];
        if (!p->direct_feed_through) {
            auto input_ptr = ssGetInputPortSignalPtrs(S, i);
            block->input_ports[i]->from(input_ptr);
        }
    }
    block->on_update();

    auto &rl = block->rl;

    assert(rl);
    while (!rl->empty() && rl->peek().when < rl->now()) {
        rl->dispatch();
    }
}

#define MDL_SET_INPUT_PORT_DIMENSION_INFO
extern "C" void mdlSetInputPortDimensionInfo(SimStruct *S, int_T port, const DimsInfo_T *dimsInfo) {
    auto block = simex::rx_block::instance;
    assert(block);
    auto &p = block->input_ports[port];
    std::vector<std::size_t> shape = { static_cast<unsigned long>(dimsInfo->numDims) };
    auto dims = xt::adapt(dimsInfo->dims, dimsInfo->numDims, xt::no_ownership(), shape);
    p->dims = dims;
    p->on_dimension_update();
    ssSetInputPortDimensionInfo(S, port, dimsInfo);
}
#define MDL_SET_OUTPUT_PORT_DIMENSION_INFO
extern "C" void mdlSetOutputPortDimensionInfo(SimStruct *S, int_T port, const DimsInfo_T *dimsInfo) {
    auto block = simex::rx_block::instance;
    assert(block);
    auto &p = block->output_ports[port];
    std::vector<std::size_t> shape = { static_cast<unsigned long>(dimsInfo->numDims) };
    auto dims = xt::adapt(dimsInfo->dims, dimsInfo->numDims, xt::no_ownership(), shape);
    p->dims = dims;
    p->on_dimension_update();
    ssSetOutputPortDimensionInfo(S, port, dimsInfo);
}

#define MDL_SET_INPUT_PORT_DATA_TYPE
extern "C" void mdlSetInputPortDataType(SimStruct *S, int_T port, DTypeId id) {
    auto block = simex::rx_block::instance;
    assert(block);
    auto &p = block->output_ports[port];
    p->type_id = id;
    ssSetOutputPortDataType(S, port, id);
}

/* for dynamic typed ports */
#define MDL_SET_OUTPUT_PORT_DATA_TYPE
extern "C" void mdlSetOutputPortDataType(SimStruct *S, int_T port, DTypeId id) {
    auto block = simex::rx_block::instance;
    assert(block);
    auto &p = block->input_ports[port];
    p->type_id = id;
    ssSetInputPortDataType(S, port, id);
}

#endif // #defined(MATLAB_MEX_FILE)

#ifdef MATLAB_MEX_FILE   /* Is this being compiled as MEX-file? */
#include "simulink.c"    /* MEX-file interface mechanism */
#else
#include "cg_sfun.h"     /* Code generation registration func */
#endif

