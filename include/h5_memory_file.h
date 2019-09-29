//
// Created by wuyuanyi on 2019-09-26.
//

#ifndef RXSIMEX_H5_MEMORY_FILE_H
#define RXSIMEX_H5_MEMORY_FILE_H
#ifdef ENABLE_HDF5
=======
>>>>>>> b1c3bee... switch dev env
#include <highfive/H5PropertyList.hpp>
#include <hdf5.h>

namespace simex {
class h5_memory_access : public HighFive::FileAccessProps {
 public:
  h5_memory_access() : HighFive::FileAccessProps() {
      _initializeIfNeeded();
      auto result = H5Pset_fapl_core(getId(), 1 << 22, false);
      assert(result >= 0 && "H5Pset_fapl_core failed");
  }
};

inline void h5_get_buffer(hid_t id, std::vector<uint8_t> &buf) {
    ssize_t sz = H5Fget_file_image(id, nullptr, 0);
    buf.resize(sz);
    H5Fget_file_image(id, buf.data(), sz);
}
}
<<<<<<< HEAD
#endif
#endif //RXSIMEX_H5_MEMORY_FILE_H
