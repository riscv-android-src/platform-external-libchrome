// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/system/filtered_data_source.h"

namespace mojo {

FilteredDataSource::FilteredDataSource(
    std::unique_ptr<DataPipeProducer::DataSource> source,
    std::unique_ptr<Filter> filter)
    : source_(std::move(source)), filter_(std::move(filter)) {
  DCHECK(source_);
}

FilteredDataSource::~FilteredDataSource() {
  if (filter_)
    filter_->OnDone();
}

bool FilteredDataSource::IsValid() const {
  return source_->IsValid();
}

int64_t FilteredDataSource::GetLength() const {
  return source_->GetLength();
}

FilteredDataSource::ReadResult FilteredDataSource::Read(
    int64_t offset,
    base::span<char> buffer) {
  ReadResult result = source_->Read(offset, buffer);
  if (filter_)
    filter_->OnRead(buffer, &result);
  return result;
}

void FilteredDataSource::Abort() {
  if (filter_) {
    ReadResult result;
    result.result = MOJO_RESULT_ABORTED;
    filter_->OnRead(base::span<char>(), &result);
  }
}

}  // namespace mojo