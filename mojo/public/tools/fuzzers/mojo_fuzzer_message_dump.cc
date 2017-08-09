// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>

#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/task_scheduler/task_scheduler.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/public/tools/fuzzers/fuzz.mojom.h"
#include "mojo/public/tools/fuzzers/fuzz_impl.h"

/* Environment for the executable. Initializes the mojo EDK and sets up a
 * TaskScheduler, because Mojo messages must be sent and processed from
 * TaskRunners. */
struct Environment {
  Environment() : message_loop() {
    base::TaskScheduler::CreateAndStartWithDefaultParams(
        "MojoFuzzerMessageDumpProcess");
    mojo::edk::Init();
  }

  /* Message loop to send messages on. */
  base::MessageLoop message_loop;

  /* Impl to be created. Stored in environment to keep it alive after
   * DumpMessages returns. */
  std::unique_ptr<FuzzImpl> impl;
};

Environment* env = new Environment();

/* MessageReceiver which dumps raw message bytes to disk in the provided
 * directory. */
class MessageDumper : public mojo::MessageReceiver {
 public:
  explicit MessageDumper(std::string directory)
      : directory_(directory), count_(0) {}

  bool Accept(mojo::Message* message) override {
    base::FilePath path = directory_.Append(FILE_PATH_LITERAL("message_") +
                                            base::IntToString(count_++) +
                                            FILE_PATH_LITERAL(".mojomsg"));

    base::File file(path,
                    base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_WRITE);
    if (!file.IsValid()) {
      LOG(ERROR) << "Failed to create mojo message file: " << path.value();
      return false;
    }

    size_t size = message->data_num_bytes();
    const char* data = reinterpret_cast<const char*>(message->data());
    int ret = file.WriteAtCurrentPos(data, size);
    if (ret != static_cast<int>(size)) {
      LOG(ERROR) << "Failed to write " << size << " bytes.";
      return false;
    }
    return true;
  }

  base::FilePath directory_;
  int count_;
};

/* Returns a FuzzUnion with fuzz_bool initialized. */
auto GetBoolFuzzUnion() {
  fuzz::mojom::FuzzUnionPtr union_bool = fuzz::mojom::FuzzUnion::New();
  union_bool->set_fuzz_bool(true);
  return union_bool;
}

/* Returns a FuzzUnion with fuzz_struct_map initialized. Takes in a
 * FuzzDummyStructPtr to use within the fuzz_struct_map value. */
auto GetStructMapFuzzUnion(fuzz::mojom::FuzzDummyStructPtr in) {
  fuzz::mojom::FuzzUnionPtr union_struct_map = fuzz::mojom::FuzzUnion::New();
  std::unordered_map<std::string, fuzz::mojom::FuzzDummyStructPtr> struct_map;
  struct_map["fuzz"] = std::move(in);
  union_struct_map->set_fuzz_struct_map(std::move(struct_map));
  return union_struct_map;
}

/* Returns a FuzzUnion with fuzz_complex initialized. Takes in a FuzzUnionPtr
 * to use within the fuzz_complex value. */
auto GetComplexFuzzUnion(fuzz::mojom::FuzzUnionPtr in) {
  std::remove_reference<decltype(in->get_fuzz_complex())>::type complex_map;
  std::remove_reference<decltype(complex_map.value()[0])>::type outer;
  std::remove_reference<decltype(
      outer[fuzz::mojom::FuzzEnum::FUZZ_VALUE0])>::type inner;
  std::remove_reference<decltype(inner['z'])>::type center;

  center.emplace();
  center.value().push_back(std::move(in));
  inner['z'] = std::move(center);
  outer[fuzz::mojom::FuzzEnum::FUZZ_VALUE0] = std::move(inner);
  complex_map.emplace();
  complex_map.value().push_back(std::move(outer));

  fuzz::mojom::FuzzUnionPtr union_complex = fuzz::mojom::FuzzUnion::New();
  union_complex->set_fuzz_complex(std::move(complex_map));
  return union_complex;
}

/* Returns a populated value for FuzzStruct->fuzz_primitive_array. */
auto GetFuzzStructPrimitiveArrayValue() {
  decltype(fuzz::mojom::FuzzStruct::fuzz_primitive_array) primitive_array;
  primitive_array = {'f', 'u', 'z', 'z'};
  return primitive_array;
}

/* Returns a populated value for FuzzStruct->fuzz_primitive_map. */
auto GetFuzzStructPrimitiveMapValue() {
  decltype(fuzz::mojom::FuzzStruct::fuzz_primitive_map) primitive_map;
  primitive_map["fuzz"] = 'z';
  return primitive_map;
}

/* Returns a populated value for FuzzStruct->fuzz_array_map. */
auto GetFuzzStructArrayMapValue() {
  decltype(fuzz::mojom::FuzzStruct::fuzz_array_map) array_map;
  array_map["fuzz"] = {"fuzz1", "fuzz2"};
  return array_map;
}

/* Returns a populated value for FuzzStruct->fuzz_union_map. Takes in a
 * FuzzUnionPtr to use within the fuzz_union_map value.*/
auto GetFuzzStructUnionMapValue(fuzz::mojom::FuzzUnionPtr in) {
  decltype(fuzz::mojom::FuzzStruct::fuzz_union_map) union_map;
  union_map[fuzz::mojom::FuzzEnum::FUZZ_VALUE1] = std::move(in);
  return union_map;
}

/* Returns a populated value for FuzzStruct->fuzz_union_array. Takes in a
 * FuzzUnionPtr to use within the fuzz_union_array value.*/
auto GetFuzzStructUnionArrayValue(fuzz::mojom::FuzzUnionPtr in) {
  decltype(fuzz::mojom::FuzzStruct::fuzz_union_array) union_array;
  union_array.push_back(std::move(in));
  return union_array;
}

/* Returns a populated value for FuzzStruct->fuzz_struct_array. Takes in a
 * FuzzStructPtr to use within the fuzz_struct_array value. */
auto GetFuzzStructStructArrayValue(fuzz::mojom::FuzzStructPtr in) {
  decltype(fuzz::mojom::FuzzStruct::fuzz_struct_array) struct_array;
  struct_array.push_back(std::move(in));
  return struct_array;
}

/* Returns a populated value for FuzzStruct->fuzz_nullable_array. */
auto GetFuzzStructNullableArrayValue() {
  decltype(fuzz::mojom::FuzzStruct::fuzz_nullable_array) nullable_array;
  return nullable_array;
}

/* Returns a populated value for FuzzStruct->fuzz_complex. */
auto GetFuzzStructComplexValue() {
  decltype(fuzz::mojom::FuzzStruct::fuzz_complex) complex_map;
  std::remove_reference<decltype(complex_map.value()[0])>::type outer;
  std::remove_reference<decltype(
      outer[fuzz::mojom::FuzzEnum::FUZZ_VALUE0])>::type inner;
  std::remove_reference<decltype(inner['z'])>::type center;

  center.emplace();
  center.value().push_back(fuzz::mojom::FuzzStruct::New());
  inner['z'] = std::move(center);
  outer[fuzz::mojom::FuzzEnum::FUZZ_VALUE0] = std::move(inner);
  complex_map.emplace();
  complex_map.value().push_back(std::move(outer));
  return complex_map;
}

/* Returns a FuzzStruct with its fields populated. */
fuzz::mojom::FuzzStructPtr GetPopulatedFuzzStruct() {
  /* Make some populated Unions. */
  auto union_bool = GetBoolFuzzUnion();
  auto union_struct_map =
      GetStructMapFuzzUnion(fuzz::mojom::FuzzDummyStruct::New());
  auto union_complex = GetComplexFuzzUnion(std::move(union_bool));

  /* Prepare the nontrivial fields for the struct. */
  auto fuzz_primitive_array = GetFuzzStructPrimitiveArrayValue();
  auto fuzz_primitive_map = GetFuzzStructPrimitiveMapValue();
  auto fuzz_array_map = GetFuzzStructArrayMapValue();
  auto fuzz_union_map = GetFuzzStructUnionMapValue(std::move(union_struct_map));
  auto fuzz_union_array =
      GetFuzzStructUnionArrayValue(std::move(union_complex));
  auto fuzz_struct_array =
      GetFuzzStructStructArrayValue(fuzz::mojom::FuzzStruct::New());
  auto fuzz_nullable_array = GetFuzzStructNullableArrayValue();
  auto fuzz_complex = GetFuzzStructComplexValue();

  /* Make a populated struct and return it. */
  return fuzz::mojom::FuzzStruct::New(
      true,                            /* fuzz_bool */
      -1,                              /* fuzz_int8 */
      1,                               /* fuzz_uint8 */
      -(1 << 8),                       /* fuzz_int16 */
      1 << 8,                          /* fuzz_uint16 */
      -(1 << 16),                      /* fuzz_int32 */
      1 << 16,                         /* fuzz_uint32 */
      -((int64_t)1 << 32),             /* fuzz_int64 */
      (uint64_t)1 << 32,               /* fuzz_uint64 */
      1.0,                             /* fuzz_float */
      1.0,                             /* fuzz_double */
      "fuzz",                          /* fuzz_string */
      std::move(fuzz_primitive_array), /* fuzz_primitive_array */
      std::move(fuzz_primitive_map),   /* fuzz_primitive_map */
      std::move(fuzz_array_map),       /* fuzz_array_map */
      std::move(fuzz_union_map),       /* fuzz_union_map */
      std::move(fuzz_union_array),     /* fuzz_union_array */
      std::move(fuzz_struct_array),    /* fuzz_struct_array */
      std::move(fuzz_nullable_array),  /* fuzz_nullable_array */
      std::move(fuzz_complex));        /* fuzz_complex */
}

/* Callback used for messages with responses. Does nothing. */
void FuzzCallback() {}

/* Invokes each method in the FuzzInterface and dumps the messages to the
 * supplied directory. */
void DumpMessages(std::string output_directory) {
  fuzz::mojom::FuzzInterfacePtr fuzz;

  /* Create the impl and add a MessageDumper to the filter chain. */
  env->impl = base::MakeUnique<FuzzImpl>(MakeRequest(&fuzz));
  env->impl->binding_.AddFilter(
      base::MakeUnique<MessageDumper>(output_directory));

  /* Call methods in various ways to generate interesting messages. */
  fuzz->FuzzBasic();
  fuzz->FuzzBasicResp(base::Bind(FuzzCallback));
  fuzz->FuzzBasicSyncResp();
  fuzz->FuzzArgs(fuzz::mojom::FuzzStruct::New(),
                 fuzz::mojom::FuzzStructPtr(nullptr));
  fuzz->FuzzArgs(fuzz::mojom::FuzzStruct::New(), GetPopulatedFuzzStruct());
  fuzz->FuzzArgsResp(fuzz::mojom::FuzzStruct::New(), GetPopulatedFuzzStruct(),
                     base::Bind(FuzzCallback));
  fuzz->FuzzArgsResp(fuzz::mojom::FuzzStruct::New(), GetPopulatedFuzzStruct(),
                     base::Bind(FuzzCallback));
  fuzz->FuzzArgsSyncResp(fuzz::mojom::FuzzStruct::New(),
                         GetPopulatedFuzzStruct(), base::Bind(FuzzCallback));
  fuzz->FuzzArgsSyncResp(fuzz::mojom::FuzzStruct::New(),
                         GetPopulatedFuzzStruct(), base::Bind(FuzzCallback));
}

int main(int argc, char** argv) {
  if (argc < 2) {
    printf("Usage: %s [output_directory]\n", argv[0]);
    exit(1);
  }
  std::string output_directory(argv[1]);

  /* Dump the messages from a MessageLoop, and wait for it to finish. */
  env->message_loop.task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&DumpMessages, output_directory));
  base::RunLoop().RunUntilIdle();

  return 0;
}
