//
// Created by fss on 22-11-13.
//

#include <gtest/gtest.h>
#include "parser/runtime_ir.hpp"
#include "tick.hpp"

TEST(test_ir, load) {
  using namespace kuiper_infer;
//  RuntimeGraph graph("./tmp/netjt.pnnx.param", "./tmp/netjt.pnnx.bin");
//  bool init = graph.Init();
//  CHECK(init == true);
}

TEST(test_ir, build) {
  using namespace kuiper_infer;
  RuntimeGraph graph("./tmp/resnet181.pnnx.param", "./tmp/resnet181.pnnx.bin");
  bool init = graph.Init();
  CHECK(init == true);
  graph.Build();
  std::vector<std::shared_ptr<Tensor>> inputs;
  std::shared_ptr<Tensor> input = std::make_shared<Tensor>(1, 28, 28);
  input->Fill(1.);
  inputs.push_back(input);
  TICK(x)
  graph.Forward(inputs);
  TOCK(x)
}