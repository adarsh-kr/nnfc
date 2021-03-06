#include <H5Cpp.h>

#include <math.h>

#include <iostream>
#include <stdexcept>

#include "tensor.hh"
#include "activation.hh"
#include "convolution.hh"
#include "normalization.hh"
#include "fullyconnected.hh"
#include "pool.hh"

const double tolerance = 1e-6;

int main(int argc, char* argv[]){

    if(argc != 2){
        std::cerr << std::string("usage: ") + std::string(argv[1])  + "<input h5file>";
        return -1;
    }
    
    H5::H5File test_file(argv[1], H5F_ACC_RDONLY);

    H5::DataSet input = test_file.openDataSet("input");
    H5::DataSet output = test_file.openDataSet("output");

    size_t input_ndims = input.getSpace().getSimpleExtentNdims();
    size_t output_ndims = output.getSpace().getSimpleExtentNdims();
    
    hsize_t input_dims[4];
    hsize_t output_dims[2];
    input.getSpace().getSimpleExtentDims(input_dims, NULL);
    output.getSpace().getSimpleExtentDims(output_dims, NULL);

    size_t input_size = input_dims[0] * input_dims[1] * input_dims[2] * input_dims[3];
    size_t output_size = output_dims[0] * output_dims[1];
    
    assert(input_ndims == 4);
    assert(output_ndims == 2);

    std::unique_ptr<float> input_data(new float[input_size]);
    std::unique_ptr<float> output_data(new float[output_size]);
    std::unique_ptr<float> output_data_correct(new float[output_size]);

    input.read(input_data.get(), H5::PredType::NATIVE_FLOAT);
    output.read(output_data_correct.get(), H5::PredType::NATIVE_FLOAT);
    
    nn::Tensor<float, 4> input_blob{input_data.get(), input_dims[0], input_dims[1], input_dims[2], input_dims[3]};
    nn::Tensor<float, 4> output_blob{output_data.get(), output_dims[0], output_dims[1], 1, 1};
    nn::Tensor<float, 4> output_blob_correct{output_data_correct.get(), output_dims[0], output_dims[1], 1, 1};

    // process the input and check its output after going through Cnn

    // layer1 convoluation ///////////////////////////////////////////
    
    // load conv kernel
    H5::DataSet kernel = test_file.openDataSet("conv1.weight");
    size_t kernel_ndims = kernel.getSpace().getSimpleExtentNdims();
    assert(kernel_ndims == 4);
    hsize_t kernel_dims[4];
    kernel.getSpace().getSimpleExtentDims(kernel_dims, NULL);
    size_t kernel_size = kernel_dims[0] * kernel_dims[1] * kernel_dims[2] * kernel_dims[3];
    std::unique_ptr<float> kernel_data(new float[kernel_size]);
    //kernel.read(kernel_data.get(), H5::PredType::NATIVE_FLOAT);

    nn::Tensor<float, 4> kernel_blob{kernel_data.get(), kernel_dims[0], kernel_dims[1], kernel_dims[2], kernel_dims[3]};
    
    // load the stride and padding paramters of the conv
    //H5::DataSet stride = test_file.openDataSet("conv1.stride");
    //H5::DataSet zero_padding = test_file.openDataSet("conv1.zero_padding");

    uint64_t stride_ = 1;
    //stride.read(&stride_, H5::PredType::NATIVE_UINT64);
    uint64_t zero_padding_ = 1;
    //zero_padding.read(&zero_padding_, H5::PredType::NATIVE_UINT64);

    // create output 
    std::unique_ptr<float> conv1_output_data(new float[input_dims[0] * kernel_dims[0] * input_dims[2] * input_dims[3]]);
    nn::Tensor<float, 4> conv1_output_blob{conv1_output_data.get(), input_dims[0], kernel_dims[0], input_dims[2], input_dims[3]};
    
    nn::conv2d(input_blob, kernel_blob, conv1_output_blob, stride_, zero_padding_);

    // layer1 batch normalization ////////////////////////////////////

    H5::DataSet means = test_file.openDataSet("bn1.running_mean");
    H5::DataSet variances = test_file.openDataSet("bn1.running_var");
    H5::DataSet weight = test_file.openDataSet("bn1.weight");
    H5::DataSet bn_bias = test_file.openDataSet("bn1.bias");
    //H5::DataSet epsilon = test_file.openDataSet("bn1.eps");
    
    size_t bn_size = conv1_output_blob.dimension(1);

    std::unique_ptr<float> means_data(new float[bn_size]);
    std::unique_ptr<float> variances_data(new float[bn_size]);
    std::unique_ptr<float> weight_data(new float[bn_size]);
    std::unique_ptr<float> bn_bias_data(new float[bn_size]);
    
    means.read(means_data.get(), H5::PredType::NATIVE_FLOAT);
    variances.read(variances_data.get(), H5::PredType::NATIVE_FLOAT);
    weight.read(weight_data.get(), H5::PredType::NATIVE_FLOAT);
    bn_bias.read(bn_bias_data.get(), H5::PredType::NATIVE_FLOAT);

    float epsilon_val = 0.00001;
    //epsilon.read(&epsilon_val, H5::PredType::NATIVE_FLOAT);

    nn::Tensor<float, 1> means_blob{means_data.get(), bn_size};
    nn::Tensor<float, 1> variances_blob{variances_data.get(), bn_size};
    nn::Tensor<float, 1> weight_blob{weight_data.get(), bn_size};
    nn::Tensor<float, 1> bn_bias_blob{bn_bias_data.get(), bn_size};

    std::unique_ptr<float> bn_output_data(new float[input_dims[0] * 64 * input_dims[2] * input_dims[3]]);
    nn::Tensor<float, 4> bn_output_blob{bn_output_data.get(), input_dims[0], 64, input_dims[2], input_dims[3]};
    
    nn::batch_norm(conv1_output_blob, means_blob, variances_blob, weight_blob, bn_bias_blob, bn_output_blob, epsilon_val);

    // relu //////////////////////////////////////////////////////////
    
    std::unique_ptr<float> relu_output_data(new float[input_dims[0] * 64 * input_dims[2] * input_dims[3]]);
    nn::Tensor<float, 4> relu_output_blob{bn_output_data.get(), input_dims[0], 64, input_dims[2], input_dims[3]};
    
    nn::relu(bn_output_blob, relu_output_blob);
    
    // average pooling layer /////////////////////////////////////////

    std::unique_ptr<float> ap_output_data(new float[1 * 64 * 1 * 1]);
    nn::Tensor<float, 4> ap_output_blob{ap_output_data.get(), 1, 64, 1, 1};

    nn::average_pooling(relu_output_blob, ap_output_blob);

    // fc layer //////////////////////////////////////////////////////
    H5::DataSet weights = test_file.openDataSet("linear.weight");
    size_t weights_size = 64 * 10;
    std::unique_ptr<float> weights_data(new float[weights_size]);
    weights.read(weights_data.get(), H5::PredType::NATIVE_FLOAT);

    nn::Tensor<float, 2> weights_blob{weights_data.get(), 10, 64};
    
    nn::fully_connected(ap_output_blob, weights_blob, output_blob);
    
    // check output blob
    for(nn::Index n = 0; n < output_blob_correct.dimension(0); n++) {
        for(nn::Index c = 0; c < output_blob_correct.dimension(1); c++) {
            for(nn::Index h = 0; h < output_blob_correct.dimension(2); h++) {
                for(nn::Index w = 0; w < output_blob_correct.dimension(3); w++) {

                    float correct_value = output_blob_correct(n,c,h,w);
                    float computed_value =  output_blob(n,c,h,w);
                    const float error = correct_value - computed_value;
                    const float squared_error = error*error;

                    std::cout << " expected:" << correct_value << " but computed:" << computed_value << "\n"; 
                    
                    if( squared_error > tolerance or std::isnan(computed_value)){
                        std::cout << " expected:" << correct_value << " but computed:" << computed_value << "\n"; 
                        return -1;
                    }
                    
                }
            }
        }
    }
    
    return 0;
    
}
