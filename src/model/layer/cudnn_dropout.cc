/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifdef USE_CUDNN
// cudnn dropout is added in cudnn 5
#if CUDNN_MAJOR_VERSION >= 5
#include "./cudnn_dropout.h"
#include "./cudnn_utils.h"
#include "singa/utils/logging.h"
namespace singa {
CudnnDropout::~CudnnDropout() {
  if (drop_desc_ != nullptr)
    CUDNN_CHECK(cudnnDestroyDropoutDescriptor(drop_desc_));
  if (x_desc_ != nullptr) CUDNN_CHECK(cudnnDestroyTensorDescriptor(x_desc_));
  if (y_desc_ != nullptr) CUDNN_CHECK(cudnnDestroyTensorDescriptor(y_desc_));
}

void CudnnDropout::InitCudnn(int size, DataType dtype, Context* ctx) {
  CHECK(!has_init_cudnn_);
  CUDNN_CHECK(cudnnCreateTensorDescriptor(&x_desc_));
  CUDNN_CHECK(cudnnCreateTensorDescriptor(&y_desc_));
  CUDNN_CHECK(cudnnCreateDropoutDescriptor(&drop_desc_));

  CUDNN_CHECK(cudnnSetTensor4dDescriptor(
      x_desc_, CUDNN_TENSOR_NCHW, GetCudnnDataType(dtype), 1, 1, 1, size));
  CUDNN_CHECK(cudnnSetTensor4dDescriptor(
      y_desc_, CUDNN_TENSOR_NCHW, GetCudnnDataType(dtype), 1, 1, 1, size));

  cudnnDropoutGetStatesSize(ctx->cudnn_handle, &state_size_);
  cudnnDropoutGetReserveSpaceSize(x_desc_, &reserve_size_);
  cudnnSetDropoutDescriptor(drop_desc_, ctx->cudnn_handle, 1 - dropout_ratio_,
                            state_.blob()->mutable_data(), state_size_,
                            ctx->seed);
  has_init_cudnn_ = true;
}

const Tensor CudnnDropout::Forward(int flag, const Tensor& input) {
  if (flag & kTrain) {
    auto size = input.Size();
    DataType dtype = input.data_type();
    if (!has_init_cudnn_) {
      input.device()->Exec(
          [size, dtype, this](Context* ctx) {
            this->InitCudnn(size, dtype, ctx);
          },
          {}, {this->state_.blob()});
      mask_.ResetLike(input);
      // TODO(wangwei) update for async running,
      // where reserve_size_ may not available
      CHECK_EQ(reserve_size_, mask_.MemSize());
    }
    Tensor output;
    output.ResetLike(input);
    output.device()->Exec(
        [input, output, this](Context* ctx) {
          Blob *inblob = input.blob(), *outblob = output.blob(),
               *mblob = mask_.blob();
          cudnnDropoutForward(ctx->cudnn_handle, this->drop_desc_,
                              this->x_desc_, inblob->data(), this->y_desc_,
                              outblob->mutable_data(), mblob,
                              this->reserve_size_);
        },
        {input.blob()}, {output.blob(), mask_.blob()});
    return output;
  } else {
    return input;
  }
}

const std::pair<Tensor, vector<Tensor>> CudnnDropout::Backward(
    int flag, const Tensor& grad) {
  vector<Tensor> param_grad;
  Tensor dx;
  if (flag & kTrain) {
    dx.ResetLike(grad);
    dx.device()->Exec(
        [dx, grad, this](Context* ctx) {
          Blob *dyblob = grad.blob(), *dxblob = dx.blob(),
               *mblob = this->mask_.blob();
          cudnnDropoutBackward(ctx->cudnn_handle, this->drop_desc_,
                               this->y_desc_, dyblob->data(), this->x_desc_,
                               dxblob->mutable_data(), mblob->mutable_data(),
                               this->reserve_size_);
        },
        {grad.blob(), mask_.blob()}, {dx.blob()});
  } else {
    LOG(ERROR) << "Do not call backward for evaluation phase";
  }
  return std::make_pair(dx, param_grad);
}
}  // namespace singa
#endif  // CUDNN_VERSION_MAJOR>=5
#endif  // USE_CUDNN