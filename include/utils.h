//
// Created by wuyuanyi on 2019-09-24.
//

#ifndef RXSIMEX_UTILS_H
#define RXSIMEX_UTILS_H
#include "simstruc.h"
#define XTARRAY_MASK (1U << 31)
namespace simex {
template<typename T>
inline int get_type_id() {
    if (std::is_same<T, real64_T>::value) {
        return SS_DOUBLE;
    } else if (std::is_same<T, real32_T>::value) {
        return SS_SINGLE;
    } else if (std::is_same<T, int8_T>::value) {
        return SS_INT8;
    } else if (std::is_same<T, uint8_T>::value) {
        return SS_UINT8;
    } else if (std::is_same<T, int16_T>::value) {
        return SS_INT16;
    } else if (std::is_same<T, uint16_T>::value) {
        return SS_UINT16;
    } else if (std::is_same<T, int32_T>::value) {
        return SS_INT32;
    } else if (std::is_same<T, uint32_T>::value) {
        return SS_UINT32;
    } else if (std::is_same<T, boolean_T>::value) {
        return SS_BOOLEAN;
    } else {
        throw std::invalid_argument(std::string("invalid type provided: ") + typeid(T).name());
    }
}

template<typename T>
inline mxClassID get_mx_class_id() {
    if (std::is_same<T, real64_T>::value) {
        return mxDOUBLE_CLASS;
    } else if (std::is_same<T, real32_T>::value) {
        return mxSINGLE_CLASS;
    } else if (std::is_same<T, int8_T>::value) {
        return mxINT8_CLASS;
    } else if (std::is_same<T, uint8_T>::value) {
        return mxUINT8_CLASS;
    } else if (std::is_same<T, int16_T>::value) {
        return mxINT16_CLASS;
    } else if (std::is_same<T, uint16_T>::value) {
        return mxUINT16_CLASS;
    } else if (std::is_same<T, int32_T>::value) {
        return mxINT32_CLASS;
    } else if (std::is_same<T, uint32_T>::value) {
        return mxUINT32_CLASS;
    } else if (std::is_same<T, boolean_T>::value) {
        return mxLOGICAL_CLASS;
    } else if (std::is_same<T, std::string>::value) {
        return mxCHAR_CLASS;
    } else if (std::is_same<T, xt::xarray<real64_T> >::value) {
        return static_cast<mxClassID>(XTARRAY_MASK | mxDOUBLE_CLASS);
    } else if (std::is_same<T, xt::xarray<real32_T> >::value) {
        return static_cast<mxClassID>(XTARRAY_MASK | mxSINGLE_CLASS);
    } else if (std::is_same<T, xt::xarray<int8_T> >::value) {
        return static_cast<mxClassID>(XTARRAY_MASK | mxINT8_CLASS);
    } else if (std::is_same<T, xt::xarray<uint8_T> >::value) {
        return static_cast<mxClassID>(XTARRAY_MASK | mxUINT8_CLASS);
    } else if (std::is_same<T, xt::xarray<int16_T> >::value) {
        return static_cast<mxClassID>(XTARRAY_MASK | mxINT16_CLASS);
    } else if (std::is_same<T, xt::xarray<uint16_T> >::value) {
        return static_cast<mxClassID>(XTARRAY_MASK | mxUINT16_CLASS);
    } else if (std::is_same<T, xt::xarray<int32_T> >::value) {
        return static_cast<mxClassID>(XTARRAY_MASK | mxINT32_CLASS);
    } else if (std::is_same<T, xt::xarray<uint32_T> >::value) {
        return static_cast<mxClassID>(XTARRAY_MASK | mxUINT32_CLASS);
    } else if (std::is_same<T, xt::xarray<boolean_T> >::value) {
        return static_cast<mxClassID>(XTARRAY_MASK | mxLOGICAL_CLASS);
    } else {
        return mxUNKNOWN_CLASS;
    }
}
}

template <typename T>
using sptr_vector = std::vector<std::shared_ptr<T>>;

#endif //RXSIMEX_UTILS_H
