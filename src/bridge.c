//
// Created by wuyua on 2019-09-24.
//
#include "simstruc.h"

#if defined(MATLAB_MEX_FILE)

static void mdlInitializeSizes(SimStruct *S) {
}

static void mdlInitializeSampleTimes(SimStruct *S) {
}

#define MDL_START
static void mdlStart(SimStruct *S) {
}

static void mdlOutputs(SimStruct *S, int_T tid) {
}

static void mdlTerminate(SimStruct *S) {
}

#define MDL_UPDATE
static void mdlUpdate(SimStruct *S, int_T tid) {
}

#define MDL_SET_INPUT_PORT_DIMENSION_INFO
static void mdlSetInputPortDimensionInfo(SimStruct *S, int_T port, const DimsInfo_T *dimsInfo) {
}
#define MDL_SET_OUTPUT_PORT_DIMENSION_INFO
static void mdlSetOutputPortDimensionInfo(SimStruct *S, int_T port, const DimsInfo_T *dimsInfo) {
}

#define MDL_SET_INPUT_PORT_DATA_TYPE
static void mdlSetInputPortDataType(SimStruct *S, int_T port, DTypeId id) {
}

/* for dynamic typed ports */
#define MDL_SET_OUTPUT_PORT_DATA_TYPE
static void mdlSetOutputPortDataType(SimStruct *S, int_T port, DTypeId id) {
}

#define MDL_CHECK_PARAMETERS
void mdlCheckParameters(SimStruct *S) {
}
#endif // #defined(MATLAB_MEX_FILE)

#ifdef MATLAB_MEX_FILE   /* Is this being compiled as MEX-file? */
#include "simulink.c"    /* MEX-file interface mechanism */
#else
#include "cg_sfun.h"     /* Code generation registration func */
#endif

