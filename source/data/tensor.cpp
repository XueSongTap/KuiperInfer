//
// Created by fss on 22-11-12.
//

#include "data/tensor.hpp"
#include <glog/logging.h>

#include <memory>

namespace kuiper_infer {

Tensor<float>::Tensor(uint32_t channels, uint32_t rows, uint32_t cols) {
  data_ = arma::fcube(rows, cols, channels);
  if (channels == 1 && rows == 1) {
    this->raw_shapes_ = std::vector<uint32_t>{cols};
  } else if (channels == 1) {
    this->raw_shapes_ = std::vector<uint32_t>{rows, cols};
  } else {
    this->raw_shapes_ = std::vector<uint32_t>{channels, rows, cols};
  }
}

Tensor<float>::Tensor(const Tensor &tensor) {
  this->data_ = tensor.data_;
  this->raw_shapes_ = tensor.raw_shapes_;
}

Tensor<float> &Tensor<float>::operator=(const Tensor &tensor) {
  if (this != &tensor) {
    this->data_ = tensor.data_;
    this->raw_shapes_ = tensor.raw_shapes_;
  }
  return *this;
}

uint32_t Tensor<float>::rows() const {
  CHECK(!this->data_.empty());
  return this->data_.n_rows;
}

uint32_t Tensor<float>::cols() const {
  CHECK(!this->data_.empty());
  return this->data_.n_cols;
}

uint32_t Tensor<float>::channels() const {
  CHECK(!this->data_.empty());
  return this->data_.n_slices;
}

uint32_t Tensor<float>::size() const {
  CHECK(!this->data_.empty());
  return this->data_.size();
}

void Tensor<float>::set_data(const arma::fcube &data) {
  CHECK(data.n_rows == this->data_.n_rows) << data.n_rows << " != " << this->data_.n_rows;
  CHECK(data.n_cols == this->data_.n_cols) << data.n_cols << " != " << this->data_.n_cols;
  CHECK(data.n_slices == this->data_.n_slices) << data.n_slices << " != " << this->data_.n_slices;
  this->data_ = data;
}

bool Tensor<float>::empty() const {
  return this->data_.empty();
}

float Tensor<float>::index(uint32_t offset) const {
  CHECK(offset < this->data_.size());
  return this->data_.at(offset);
}

std::vector<uint32_t> Tensor<float>::shapes() const {
  CHECK(!this->data_.empty());
  return {this->channels(), this->rows(), this->cols()};
}

arma::fcube &Tensor<float>::data() {
  return this->data_;
}

const arma::fcube &Tensor<float>::data() const {
  return this->data_;
}

arma::fmat &Tensor<float>::at(uint32_t channel) {
  CHECK_LT(channel, this->channels());
  return this->data_.slice(channel);
}

const arma::fmat &Tensor<float>::at(uint32_t channel) const {
  CHECK_LT(channel, this->channels());
  return this->data_.slice(channel);
}

float Tensor<float>::at(uint32_t channel, uint32_t row, uint32_t col) const {
  CHECK_LT(row, this->rows());
  CHECK_LT(col, this->cols());
  CHECK_LT(channel, this->channels());
  return this->data_.at(row, col, channel);
}

float &Tensor<float>::at(uint32_t channel, uint32_t row, uint32_t col) {
  CHECK_LT(row, this->rows());
  CHECK_LT(col, this->cols());
  CHECK_LT(channel, this->channels());
  return this->data_.at(row, col, channel);
}

void Tensor<float>::Padding(const std::vector<uint32_t> &pads, float padding_value) {
  CHECK(!this->data_.empty());
  CHECK_EQ(pads.size(), 4);
  uint32_t pad_rows1 = pads.at(0);  // up
  uint32_t pad_rows2 = pads.at(1);  // bottom
  uint32_t pad_cols1 = pads.at(2);  // left
  uint32_t pad_cols2 = pads.at(3);  // right

  arma::fcube padded_cube;
  uint32_t channels = this->channels();
  CHECK_GT(channels, 0);

  for (uint32_t i = 0; i < channels; ++i) {
    const arma::fmat &sub_mat = this->data_.slice(i);
    CHECK(!sub_mat.empty());

    arma::fmat padded_mat(sub_mat.n_rows + pad_rows1 + pad_rows2,
                          sub_mat.n_cols + pad_cols1 + pad_cols2);

    padded_mat.fill((float) padding_value);
    padded_mat.submat(pad_rows1, pad_cols1, pad_rows1 + sub_mat.n_rows - 1,
                      pad_cols1 + sub_mat.n_cols - 1) = sub_mat;

    if (padded_cube.empty()) {
      padded_cube = arma::fcube(padded_mat.n_rows, padded_mat.n_cols, channels);
    }
    padded_cube.slice(i) = std::move(padded_mat);
  }
  CHECK(!padded_cube.empty());
  this->data_ = padded_cube;
  this->raw_shapes_ = this->shapes();
}

void Tensor<float>::Fill(float value) {
  CHECK(!this->data_.empty());
  this->data_.fill(value);
}

void Tensor<float>::Fill(const std::vector<float> &values) {
  CHECK(!this->data_.empty());
  const uint32_t total_elems = this->data_.size();
  CHECK_EQ(values.size(), total_elems);

  const uint32_t rows = this->rows();
  const uint32_t cols = this->cols();
  const uint32_t planes = rows * cols;
  const uint32_t channels = this->data_.n_slices;

  for (uint32_t i = 0; i < channels; ++i) {
    auto &channel_data = this->data_.slice(i);
    const arma::fmat channel_data_t = arma::fmat(values.data() + i * planes, this->cols(), this->rows());
    channel_data = channel_data_t.t();
  }
}

void Tensor<float>::Show() {
  for (uint32_t i = 0; i < this->channels(); ++i) {
    LOG(INFO) << "Channel: " << i;
    LOG(INFO) << "\n" << this->data_.slice(i);
  }
}

void Tensor<float>::Flatten() {
  CHECK(!this->data_.empty());
  const uint32_t size = this->data_.size();
  arma::fcube linear_cube(size, 1, 1);

  uint32_t channel = this->channels();
  uint32_t rows = this->rows();
  uint32_t cols = this->cols();
  uint32_t index = 0;

  for (uint32_t c = 0; c < channel; ++c) {
    const arma::fmat &matrix = this->data_.slice(c);

    for (uint32_t r = 0; r < rows; ++r) {
      for (uint32_t c_ = 0; c_ < cols; ++c_) {
        linear_cube.at(index, 0, 0) = matrix.at(r, c_);
        index += 1;
      }
    }
  }
  CHECK_EQ(index, size);
  this->data_ = linear_cube;
  this->raw_shapes_ = std::vector<uint32_t>{size};
}

std::shared_ptr<Tensor<float>>
Tensor<float>::Clone() {
  return std::make_shared<Tensor>(*this);
}

void Tensor<float>::Rand() {
  CHECK(!this->data_.empty());
  this->data_.randn();
}

void Tensor<float>::Ones() {
  CHECK(!this->data_.empty());
  this->data_.fill(1.);
}

std::shared_ptr<Tensor<float>>
Tensor<float>::ElementAdd(const std::shared_ptr<Tensor<float>>
                          &tensor1,
                          const std::shared_ptr<Tensor<float>> &tensor2) {
  CHECK(!tensor1->empty() && !tensor2->empty());
  CHECK(tensor1->shapes() == tensor2->shapes()) << "Tensors shape are not adapting";
  std::shared_ptr<Tensor<float>>
      output_tensor =
      std::make_shared<Tensor<float>>
          (tensor1->
              channels(), tensor1
               ->
                   rows(), tensor1
               ->
                   cols()
          );
  output_tensor->
      data_ = tensor1->data_ + tensor2->data_;
  return
      output_tensor;
}

std::shared_ptr<Tensor<float>>
Tensor<float>::ElementMultiply(const std::shared_ptr<Tensor<float>>
                               &tensor1,
                               const std::shared_ptr<Tensor<float>> &tensor2) {
  CHECK(!tensor1->empty() && !tensor2->empty());
  CHECK(tensor1->shapes() == tensor2->shapes()) << "Tensors shape are not adapting";
  std::shared_ptr<Tensor<float>>
      output_tensor =
      std::make_shared<Tensor<float>>
          (tensor1->
              channels(), tensor1
               ->
                   rows(), tensor1
               ->
                   cols()
          );
  output_tensor->
      data_ = tensor1->data_ % tensor2->data_;
  return
      output_tensor;
}

void Tensor<float>::Transform(const std::function<float(float)> &filter) {
  CHECK(!this->data_.empty());
  this->data_.transform(filter);
}

void Tensor<float>::ReRawshape(const std::vector<uint32_t> &shapes) {
  CHECK(!shapes.empty());
  const uint32_t origin_size = this->size();
  uint32_t current_size = 1;
  for (uint32_t s : shapes) {
    current_size *= s;
  }
  CHECK(shapes.size() <= 3);
  CHECK(current_size == origin_size);

  if (shapes.size() == 3) {
    this->data_.reshape(shapes.at(1), shapes.at(2), shapes.at(0));
  } else if (shapes.size() == 2) {
    this->data_.reshape(shapes.at(0), shapes.at(1), 1);
  } else {
    this->data_.reshape(shapes.at(0), 1, 1);
  }
  this->raw_shapes_ = shapes;
}

const std::vector<uint32_t> &Tensor<float>::raw_shapes() const {
  return this->raw_shapes_;
}
}