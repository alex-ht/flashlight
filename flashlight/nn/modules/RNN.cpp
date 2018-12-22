/**
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <stdexcept>

#include "RNN.h"

#include <flashlight/autograd/Functions.h>
#include <flashlight/nn/Init.h>
#include <flashlight/nn/Utils.h>

namespace fl {

RNN::RNN(
    int input_size,
    int hidden_size,
    int num_layers,
    RnnMode mode,
    bool bidirectional /* false */,
    float drop_prob /* = 0.0 */)
    : inputSize_(input_size),
      hiddenSize_(hidden_size),
      numLayers_(num_layers),
      mode_(mode),
      bidirectional_(bidirectional),
      dropProb_(drop_prob) {
  initialize();
}

void RNN::initialize() {
  dim_t n_params = detail::getNumRnnParams(
      inputSize_, hiddenSize_, numLayers_, mode_, bidirectional_);

  double stdv = std::sqrt(1.0 / (double)hiddenSize_);
  auto w = uniform({n_params}, -stdv, stdv, f32, true);
  params_ = {w};
}

std::vector<Variable> RNN::forward(const std::vector<Variable>& inputs) {
  if (inputs.size() < 1 || inputs.size() > 3) {
    throw std::invalid_argument("Invalid inputs size");
  }

  const auto& input = inputs[0];
  const auto& hiddenState = inputs.size() >= 2 ? inputs[1] : Variable();
  const auto& cellState = inputs.size() == 3 ? inputs[2] : Variable();

  float dropProb = train_ ? dropProb_ : 0.0;
  auto rnnRes =
      rnn(input,
          hiddenState,
          cellState,
          params_[0],
          hiddenSize_,
          numLayers_,
          mode_,
          bidirectional_,
          dropProb);

  std::vector<Variable> output(1, std::get<0>(rnnRes));
  if (inputs.size() >= 2) {
    output.push_back(std::get<1>(rnnRes));
  }
  if (inputs.size() == 3) {
    output.push_back(std::get<2>(rnnRes));
  }
  return output;
}

Variable RNN::forward(const Variable& input) {
  return forward(std::vector<Variable>{input}).front();
}

std::tuple<Variable, Variable> RNN::forward(
    const Variable& input,
    const Variable& hidden_state) {
  auto res = forward(std::vector<Variable>{input, hidden_state});
  return std::make_tuple(res[0], res[1]);
}

std::tuple<Variable, Variable, Variable> RNN::forward(
    const Variable& input,
    const Variable& hidden_state,
    const Variable& cell_state) {
  auto res = forward(std::vector<Variable>{input, hidden_state, cell_state});
  return std::make_tuple(res[0], res[1], res[2]);
}

std::string RNN::prettyString() const {
  std::ostringstream ss;
  switch (mode_) {
    case RnnMode::RELU:
      ss << "RNN (relu)";
      break;
    case RnnMode::TANH:
      ss << "RNN (tanh)";
      break;
    case RnnMode::LSTM:
      ss << "LSTM";
      break;
    case RnnMode::GRU:
      ss << "GRU";
      break;
    default:
      break;
  }
  int output_size = bidirectional_ ? 2 * hiddenSize_ : hiddenSize_;
  ss << " (" << inputSize_ << "->" << output_size << ")";
  if (numLayers_ > 1) {
    ss << " (" << numLayers_ << "-layer)";
  }
  if (bidirectional_) {
    ss << " (bidirectional)";
  }
  if (dropProb_ > 0) {
    ss << " (dropout=" << dropProb_ << ")";
  }
  return ss.str();
}

} // namespace fl
