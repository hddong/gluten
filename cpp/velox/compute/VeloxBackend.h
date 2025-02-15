/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <folly/executors/IOThreadPoolExecutor.h>

#include "VeloxColumnarToRowConverter.h"
#include "WholeStageResultIterator.h"
#include "compute/Backend.h"
#include "operators/shuffle/SplitterBase.h"

namespace gluten {
// This class is used to convert the Substrait plan into Velox plan.
class VeloxBackend final : public Backend {
 public:
  explicit VeloxBackend(const std::unordered_map<std::string, std::string>& confMap);

  // FIXME This is not thread-safe?
  std::shared_ptr<ResultIterator> GetResultIterator(
      MemoryAllocator* allocator,
      std::string spillDir,
      std::vector<std::shared_ptr<ResultIterator>> inputs = {},
      std::unordered_map<std::string, std::string> sessionConf = {}) override;

  // Used by unit test and benchmark.
  std::shared_ptr<ResultIterator> GetResultIterator(
      MemoryAllocator* allocator,
      const std::vector<std::shared_ptr<facebook::velox::substrait::SplitInfo>>& scanInfos,
      std::unordered_map<std::string, std::string> sessionConf = {});

  arrow::Result<std::shared_ptr<ColumnarToRowConverter>> getColumnar2RowConverter(
      MemoryAllocator* allocator,
      std::shared_ptr<ColumnarBatch> cb) override;

  std::shared_ptr<SplitterBase> makeSplitter(
      const std::string& partitioning_name,
      int num_partitions,
      SplitOptions options,
      const std::string& batchType) override;

  std::shared_ptr<Metrics> GetMetrics(void* raw_iter, int64_t exportNanos) override {
    auto iter = static_cast<WholeStageResultIterator*>(raw_iter);
    return iter->GetMetrics(exportNanos);
  }

  std::shared_ptr<arrow::Schema> GetOutputSchema() override;

  std::shared_ptr<facebook::velox::memory::MemoryUsageTracker> getMemoryUsageTracker();

 private:
  void setInputPlanNode(const ::substrait::FetchRel& fetchRel);

  void setInputPlanNode(const ::substrait::ExpandRel& sExpand);

  void setInputPlanNode(const ::substrait::SortRel& sSort);

  void setInputPlanNode(const ::substrait::WindowRel& s);

  void setInputPlanNode(const ::substrait::AggregateRel& sagg);

  void setInputPlanNode(const ::substrait::ProjectRel& sproject);

  void setInputPlanNode(const ::substrait::FilterRel& sfilter);

  void setInputPlanNode(const ::substrait::JoinRel& sJoin);

  void setInputPlanNode(const ::substrait::ReadRel& sread);

  void setInputPlanNode(const ::substrait::Rel& srel);

  void setInputPlanNode(const ::substrait::RelRoot& sroot);

  void toVeloxPlan();

  std::string nextPlanNodeId();

  void cacheOutputSchema(const std::shared_ptr<const facebook::velox::core::PlanNode>& planNode);

  int planNodeId_ = 0;

  std::vector<std::shared_ptr<ResultIterator>> arrowInputIters_;

  std::shared_ptr<facebook::velox::substrait::SubstraitParser> subParser_ =
      std::make_shared<facebook::velox::substrait::SubstraitParser>();

  std::shared_ptr<facebook::velox::substrait::SubstraitVeloxPlanConverter> subVeloxPlanConverter_ =
      std::make_shared<facebook::velox::substrait::SubstraitVeloxPlanConverter>(GetDefaultWrappedVeloxMemoryPool());

  // Cache for tests/benchmark purpose.
  std::shared_ptr<const facebook::velox::core::PlanNode> veloxPlan_;
  std::shared_ptr<arrow::Schema> outputSchema_;
  std::shared_ptr<facebook::velox::memory::MemoryUsageTracker> memUsageTracker_;
};

} // namespace gluten
