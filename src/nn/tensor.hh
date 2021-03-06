#ifndef _NN_TENSOR_H
#define _NN_TENSOR_H

#include <Eigen/CXX11/Tensor>

#include <cassert>
#include <cstdarg>
#include <cstring>
#include <memory>
#include <string>

namespace nn {

typedef Eigen::Index Index;

template <typename T, int ndims>
class Tensor {
 private:
  const Eigen::DSizes<Eigen::Index, ndims> size_;
  const std::shared_ptr<T> data_;
  Eigen::TensorMap<Eigen::Tensor<T, ndims, Eigen::RowMajor>> tensor_;

  T* data() const { return tensor_.data(); }

 public:
  // Note: this constructor does not take ownership of the
  // input `data`. As a result, the `data` pointer is
  // wrapped in a shared_ptr with noop delete function. Take
  // care to ensure the lifetime of the `data` pointer
  // covers the usage of this tensor object.
  //
  // Recommendation: use a constructor with a shared_ptr
  // input if you want the tensor to take ownership of data.
  // You won't have to worry about the lifetime of the
  // data and/or tensor object then!
  template <typename... DimSizes>
  Tensor(T* data, const DimSizes... dims)
      : Tensor(std::shared_ptr<T>(data, [](T*) {}),
               Eigen::DSizes<Eigen::Index, ndims>(dims...)) {}

  template <typename... DimSizes>
  Tensor(std::shared_ptr<T> data, const DimSizes... dims)
      : Tensor(data, Eigen::DSizes<Eigen::Index, ndims>(dims...)) {}

  template <typename... DimSizes>
  Tensor(const DimSizes... dims)
      : Tensor(
            std::shared_ptr<T>(
                new T[Eigen::DSizes<Eigen::Index, ndims>(dims...).TotalSize()]),
            Eigen::DSizes<Eigen::Index, ndims>(dims...)) {}

  Tensor(const Eigen::DSizes<Eigen::Index, ndims> size)
      : Tensor(std::shared_ptr<T>(new T[size.TotalSize()]), size) {}

  Tensor(const Tensor<T, ndims>&& other) noexcept : Tensor(other) {}

  Tensor(const Tensor<T, ndims>& other) noexcept
      : Tensor(other.data_, other.size_) {}

  Tensor(const Eigen::Tensor<T, ndims, Eigen::RowMajor>& t)
      : Tensor(t.dimensions()) {
    std::memcpy(data_.get(), t.data(), sizeof(T) * t.size());
  }

  Tensor(std::shared_ptr<T> data, const Eigen::DSizes<Eigen::Index, ndims> size)
      : size_(size),
        data_(data),
        tensor_(Eigen::TensorMap<Eigen::Tensor<T, ndims, Eigen::RowMajor>>(
            data_.get(), size_)) {}

  ~Tensor() {}

  Tensor<T, ndims>& operator=(const Tensor<T, ndims>&& rhs) {
    // reconstruct the object because Eigen::TensorMap cannot be
    // assigned to without a bus error?...
    this->~Tensor<T, ndims>();
    new (this) Tensor<T, ndims>(rhs);

    return *this;
  }

  Tensor<T, ndims> deepcopy() const {
    Tensor<T, ndims> new_tensor(size_);
    std::memcpy(new_tensor.data(), tensor_.data(),
                sizeof(T) * size_.TotalSize());
    return new_tensor;
  }

  Eigen::Index dimension(const Eigen::Index dim) const {
    return tensor_.dimension(dim);
  }

  Eigen::Index size() const { return tensor_.size(); }

  Eigen::Index rank() const { return tensor_.rank(); }

  T maximum() const {
    return ((Eigen::Tensor<T, 0, Eigen::RowMajor>)tensor_.maximum())(0);
  }
  T minimum() const {
    return ((Eigen::Tensor<T, 0, Eigen::RowMajor>)tensor_.minimum())(0);
  }

  const decltype(tensor_)& tensor() const { return tensor_; }
  decltype(tensor_)& tensor() { return tensor_; }

  template <typename... Indices>
  inline T& operator()(const Indices... indices) {
    return tensor_(indices...);
  }

  template <typename... Indices>
  inline const T& operator()(const Indices... indices) const {
    return tensor_(indices...);
  }
};
}  // namespace nn

#endif  // _NN_TENSOR_H
