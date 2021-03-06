#include <H5Cpp.h>

#include "layers.hh"
#include "tensor.hh"

std::shared_ptr<nn::LayerInterface> nn::make_convolution_from_hdf5(
    size_t output_batch_size, size_t output_channels, size_t output_height,
    size_t output_width, H5::H5File parameter_file, std::string kernel_name,
    size_t stride, size_t zero_padding) {
  nn::Tensor<float, 4> output(output_batch_size, output_channels, output_height,
                              output_width);

  H5::DataSet kernel_ds = parameter_file.openDataSet(kernel_name.c_str());
  assert(kernel_ds.getSpace().getSimpleExtentNdims() == 4);

  hsize_t kernel_dims[4];
  kernel_ds.getSpace().getSimpleExtentDims(kernel_dims, nullptr);

  nn::Tensor<float, 4> kernel(kernel_dims[0], kernel_dims[1], kernel_dims[2],
                              kernel_dims[3]);
  kernel_ds.read(&kernel(0, 0, 0, 0), H5::PredType::NATIVE_FLOAT);

  auto layer = std::make_shared<nn::ConvolutionLayer>(output, kernel, stride,
                                                      zero_padding);
  return std::static_pointer_cast<nn::LayerInterface>(layer);
}

std::shared_ptr<nn::LayerInterface> nn::make_fc_from_hdf5(
    size_t output_batch_size, size_t output_channels, size_t output_height,
    size_t output_width, H5::H5File parameter_file, std::string weights_name) {
  nn::Tensor<float, 4> output(output_batch_size, output_channels, output_height,
                              output_width);

  H5::DataSet weights_ds = parameter_file.openDataSet(weights_name.c_str());
  assert(weights_ds.getSpace().getSimpleExtentNdims() == 2);

  hsize_t weights_dims[2];
  weights_ds.getSpace().getSimpleExtentDims(weights_dims, nullptr);

  nn::Tensor<float, 2> weights(weights_dims[0], weights_dims[1]);
  weights_ds.read(&weights(0, 0), H5::PredType::NATIVE_FLOAT);

  auto layer = std::make_shared<nn::FCLayer>(output, weights);
  return std::static_pointer_cast<nn::LayerInterface>(layer);
}

std::shared_ptr<nn::LayerInterface> nn::make_fc_with_bias_from_hdf5(
    size_t output_batch_size, size_t output_channels, size_t output_height,
    size_t output_width, H5::H5File parameter_file, std::string weights_name,
    std::string bias_name) {
  nn::Tensor<float, 4> output(output_batch_size, output_channels, output_height,
                              output_width);

  H5::DataSet weights_ds = parameter_file.openDataSet(weights_name.c_str());
  assert(weights_ds.getSpace().getSimpleExtentNdims() == 2);

  hsize_t weights_dims[2];
  weights_ds.getSpace().getSimpleExtentDims(weights_dims, nullptr);

  nn::Tensor<float, 2> weights(weights_dims[0], weights_dims[1]);
  weights_ds.read(&weights(0, 0), H5::PredType::NATIVE_FLOAT);

  H5::DataSet biases_ds = parameter_file.openDataSet(bias_name.c_str());
  assert(biases_ds.getSpace().getSimpleExtentNdims() == 1);

  hsize_t biases_size;
  biases_ds.getSpace().getSimpleExtentDims(&biases_size, nullptr);

  nn::Tensor<float, 1> biases(biases_size);
  biases_ds.read(&biases(0), H5::PredType::NATIVE_FLOAT);

  auto layer = std::make_shared<nn::FCWithBiasLayer>(output, weights, biases);
  return std::static_pointer_cast<nn::LayerInterface>(layer);
}

std::shared_ptr<nn::LayerInterface> nn::make_batch_norm_from_hdf5(
    size_t output_batch_size, size_t output_channels, size_t output_height,
    size_t output_width, H5::H5File parameter_file, std::string means_name,
    std::string variances_name, std::string weight_name, std::string bias_name,
    float eps)

{
  nn::Tensor<float, 4> output(output_batch_size, output_channels, output_height,
                              output_width);

  // load means
  H5::DataSet means_ds = parameter_file.openDataSet(means_name.c_str());
  assert(means_ds.getSpace().getSimpleExtentNdims() == 1);

  hsize_t means_dims[1];
  means_ds.getSpace().getSimpleExtentDims(means_dims, nullptr);

  nn::Tensor<float, 1> means(means_dims[0]);
  means_ds.read(&means(0), H5::PredType::NATIVE_FLOAT);

  // load variances
  H5::DataSet variances_ds = parameter_file.openDataSet(variances_name.c_str());
  assert(variances_ds.getSpace().getSimpleExtentNdims() == 1);

  hsize_t variances_dims[1];
  variances_ds.getSpace().getSimpleExtentDims(variances_dims, nullptr);

  nn::Tensor<float, 1> variances(variances_dims[0]);
  variances_ds.read(&variances(0), H5::PredType::NATIVE_FLOAT);

  // load weight
  H5::DataSet weight_ds = parameter_file.openDataSet(weight_name.c_str());
  assert(weight_ds.getSpace().getSimpleExtentNdims() == 1);

  hsize_t weight_dims[1];
  weight_ds.getSpace().getSimpleExtentDims(weight_dims, nullptr);

  nn::Tensor<float, 1> weight(weight_dims[0]);
  weight_ds.read(&weight(0), H5::PredType::NATIVE_FLOAT);

  // load bias
  H5::DataSet bias_ds = parameter_file.openDataSet(bias_name.c_str());
  assert(bias_ds.getSpace().getSimpleExtentNdims() == 1);

  hsize_t bias_dims[1];
  bias_ds.getSpace().getSimpleExtentDims(bias_dims, nullptr);

  nn::Tensor<float, 1> bias(bias_dims[0]);
  bias_ds.read(&bias(0), H5::PredType::NATIVE_FLOAT);

  auto layer = std::make_shared<nn::BatchNormLayer>(output, means, variances,
                                                    weight, bias, eps);
  return std::static_pointer_cast<nn::LayerInterface>(layer);
}

std::shared_ptr<nn::LayerInterface> nn::make_relu_from_hdf5(
    size_t output_batch_size, size_t output_channels, size_t output_height,
    size_t output_width) {
  nn::Tensor<float, 4> output(output_batch_size, output_channels, output_height,
                              output_width);

  auto layer = std::make_shared<nn::ReluLayer>(output);
  return std::static_pointer_cast<nn::LayerInterface>(layer);
}

std::shared_ptr<nn::LayerInterface> nn::make_pool_from_hdf5(
    size_t output_batch_size, size_t output_channels, size_t output_height,
    size_t output_width) {
  nn::Tensor<float, 4> output(output_batch_size, output_channels, output_height,
                              output_width);

  auto layer = std::make_shared<nn::PoolLayer>(output);
  return std::static_pointer_cast<nn::LayerInterface>(layer);
}
