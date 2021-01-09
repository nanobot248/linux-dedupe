/*
   Copyright 2021 Andreas Hubert

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include <cmath>
#include <vector>

#include <errno.h>
#include <sys/ioctl.h>
#include <linux/fs.h>

#include "nan.h"

using namespace v8;
using namespace node;

const bool DEBUG_ENABLED = false;

struct to_dedup_range_result {
    bool success = false;
    
    int64_t srcFd;
    uint64_t srcOffset;
    uint64_t srcLength;
    int64_t destFd;
    uint64_t destOffset;

    std::vector<uint8_t> data = std::vector<uint8_t>((size_t) (sizeof(struct file_dedupe_range) + sizeof(struct file_dedupe_range_info)));

    struct file_dedupe_range* dedupe_range() {
      return (struct file_dedupe_range*) data.data();
    }

    const struct file_dedupe_range* dedupe_range() const {
      return (struct file_dedupe_range*) data.data();
    }

    struct file_dedupe_range_info* dedupe_range_info() {
      return (struct file_dedupe_range_info*) (data.data() + sizeof(struct file_dedupe_range));
    }

    const struct file_dedupe_range_info* dedupe_range_info() const {
      return (struct file_dedupe_range_info*) (data.data() + sizeof(struct file_dedupe_range));
    }

    uint64_t bytes_deduped() const {
      return dedupe_range_info()->bytes_deduped;
    }

    int32_t status() const {
      return dedupe_range_info()->status;
    }
};

struct to_dedup_range_result to_dedup_range(Nan::NAN_METHOD_ARGS_TYPE info) {
    const int SRC_FD = 0,
      SRC_OFF = 1,
      SRC_LEN = 2,
      DST_FD = 3,
      DST_OFF = 4;

    struct to_dedup_range_result result;

    if (info.Length() < 5) {
        Nan::ThrowTypeError("At least 5 arguments required.");
        return result;
    }

    if (info[SRC_FD]->IsNullOrUndefined() || !info[SRC_FD]->IsNumber()) {
        Nan::ThrowTypeError("Argument src_fd must be a number.");
        return result;
    }
    const double srcFdNumber = Nan::To<double>(info[SRC_FD]).ToChecked();
    const double srcFdFloor = floor(srcFdNumber);
    if (srcFdFloor != srcFdNumber) {
        Nan::ThrowTypeError("Argument src_fd must be an integer.");
        return result;
    }
    result.srcFd = (int64_t) srcFdFloor;

    if (info[SRC_OFF]->IsNullOrUndefined() || !info[SRC_OFF]->IsNumber()) {
        Nan::ThrowTypeError("Argument src_off must be a number");
        return result;
    }
    const double srcOffNumber = Nan::To<double>(info[SRC_OFF]).ToChecked();
    if (srcOffNumber < 0) {
        Nan::ThrowTypeError("Argument src_off must be a positive number.");
        return result;
    }
    const double srcOffFloor = floor(srcOffNumber);
    if (srcOffFloor != srcOffNumber) {
        Nan::ThrowTypeError("Argument src_off must be an integer.");
    }
    result.srcOffset = (uint64_t) srcOffFloor;

    if (info[SRC_LEN]->IsNullOrUndefined() || !info[SRC_LEN]->IsNumber()) {
        Nan::ThrowTypeError("Argument src_len must be a number.");
        return result;
    }
    const double srcLenNumber = Nan::To<double>(info[SRC_LEN]).ToChecked();
    if (srcLenNumber < 0) {
        Nan::ThrowTypeError("Argument src_len must be a positive number.");
        return result;
    }
    const double srcLenFloor = floor(srcLenNumber);
    if (srcLenFloor != srcLenNumber) {
        Nan::ThrowTypeError("Argument src_len must be an integer.");
        return result;
    }
    result.srcLength = (uint64_t) srcLenFloor;

    if (info[DST_FD]->IsNullOrUndefined() || !info[DST_FD]->IsNumber()) {
        Nan::ThrowTypeError("Argument dst_fd must be a number.");
        return result;
    }
    const double dstFdNumber = Nan::To<double>(info[DST_FD]).ToChecked();
    const double dstFdFloor = floor(dstFdNumber);
    if (dstFdFloor != dstFdNumber) {
        Nan::ThrowTypeError("Argument dst_fd must be an integer.");
        return result;
    }
    result.destFd = (int64_t) dstFdFloor;

    if (info[DST_OFF]->IsNullOrUndefined() || !info[DST_OFF]->IsNumber()) {
        Nan::ThrowTypeError("Argument dst_off must be a number.");
        return result;
    }
    const double dstOffNumber = Nan::To<double>(info[DST_OFF]).ToChecked();
    if (dstOffNumber < 0) {
        Nan::ThrowTypeError("Argument dst_off must be a positive number.");
        return result;
    }
    const double dstOffFloor = floor(dstOffNumber);
    if (dstOffFloor != dstOffNumber) {
        Nan::ThrowTypeError("Argument dst_off must be an integer.");
        return result;
    }
    result.destOffset = (uint64_t) dstOffFloor;

    std::vector<uint8_t> data = std::vector<uint8_t>();
    struct file_dedupe_range* range = (file_dedupe_range*) result.data.data();
    range->src_offset = result.srcOffset;
    range->src_length = result.srcLength;
    range->dest_count = 1;
    range->reserved1 = 0;
    range->reserved2 = 0;

    struct file_dedupe_range_info* rangeInfo = (file_dedupe_range_info*) (result.data.data() + sizeof(struct file_dedupe_range));
    rangeInfo->dest_fd = result.destFd;
    rangeInfo->dest_offset = result.destOffset;

    result.success = true;
    return result;
}

// params: s64 src_fd, u64 src_off, u64 src_len, s64 dst_fd, u64 dst_off
NAN_METHOD(IoctlDedupeRangeSync) {
    Nan::HandleScope scope;

    struct to_dedup_range_result args = to_dedup_range(info);

    if (args.success) {
      int res = ioctl(args.srcFd, FIDEDUPERANGE, (void*) args.data.data());
      if (res < 0) {
        return Nan::ThrowError(Nan::ErrnoException(errno, "ioctl", nullptr, nullptr));
      }
      info.GetReturnValue().Set(res);
    }
}

class IoctlAsyncDedupRangeWorker : public Nan::AsyncWorker {
  public:
    IoctlAsyncDedupRangeWorker(Nan::Callback* cb, const struct to_dedup_range_result& range) :
      AsyncWorker(cb), _range(range) {
    }

    ~IoctlAsyncDedupRangeWorker() {}
  
    virtual void Execute() {
      if (DEBUG_ENABLED) printf("executing ...\n");
      _result = ioctl(_range.srcFd, FIDEDUPERANGE, (void*) _range.data.data());
      _errno = errno;
      if (DEBUG_ENABLED) printf("executed with result: result=%x, errno=%x\n", _result, _errno);

      if (_result < 0) {
        if (DEBUG_ENABLED) printf("setting error message ...\n");
        SetErrorMessage("An ioctl error occurred.");
      }
    }

    virtual void HandleOKCallback() {
      if (DEBUG_ENABLED) printf("handling OK callback ...\n");
      Nan::HandleScope scope;
      Local<Value> args[] = {
        Nan::Null(),
        Nan::New<Number>(_result),
        Nan::New<Number>(_range.status()),
        Nan::New<Number>(_range.bytes_deduped())
      };
      callback->Call(4, args, async_resource);
      if (DEBUG_ENABLED) printf("OK callback called.\n");
    }

    virtual void HandleErrorCallback() {
      if (DEBUG_ENABLED) printf("handling ERROR callback ...\n");
      Nan::HandleScope scope;
      Local<Value> args[] = { Nan::ErrnoException(_errno, "ioctl", nullptr, nullptr) };
      callback->Call(1, args, async_resource);
      if (DEBUG_ENABLED) printf("ERROR callback called.\n");
    }

  private:
    struct to_dedup_range_result _range;
    int _result = -1;
    int _errno = -1;
};

NAN_METHOD(IoctlDedupeRangeAsync) {
    if (DEBUG_ENABLED) printf("IoctlDedupeRangeAsync starting ...\n");

    Nan::HandleScope scope;

    struct to_dedup_range_result args = to_dedup_range(info);

    if (DEBUG_ENABLED) printf(
      "IoctlDedupeRangeAsync: %li, %lu, %lu, %li, %lu\n",
      args.srcFd,
      args.srcOffset,
      args.srcLength,
      args.destFd,
      args.destOffset
    );

    if (args.success) {
      if (info.Length() < 6 || info[5]->IsNullOrUndefined() || !info[5]->IsFunction()) {
        return Nan::ThrowTypeError("Callback function is required.");
      }
      Nan::Callback* callback = new Nan::Callback(Nan::To<Function>(info[5]).ToLocalChecked());

      Nan::AsyncQueueWorker(new IoctlAsyncDedupRangeWorker(callback, args));
    }
}

void InitAll(Local<Object> exports) {
    if (DEBUG_ENABLED) printf("InitAll!\n");
    Nan::Set(exports,
             Nan::New("ioctl_dedupe_range_sync").ToLocalChecked(),
             Nan::GetFunction(Nan::New<FunctionTemplate>(IoctlDedupeRangeSync)).ToLocalChecked());
    Nan::Set(exports,
             Nan::New("ioctl_dedupe_range").ToLocalChecked(),
             Nan::GetFunction(Nan::New<FunctionTemplate>(IoctlDedupeRangeAsync)).ToLocalChecked());

}

NODE_MODULE(dedupe, InitAll)

