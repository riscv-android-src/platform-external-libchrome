// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/lib/fixed_buffer.h"
#include "mojo/public/cpp/bindings/lib/serialization.h"
#include "mojo/public/cpp/bindings/lib/wtf_serialization.h"
#include "mojo/public/cpp/bindings/tests/variant_test_util.h"
#include "mojo/public/interfaces/bindings/tests/test_wtf_types.mojom-blink.h"
#include "mojo/public/interfaces/bindings/tests/test_wtf_types.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace mojo {
namespace test {
namespace {

const char kHelloWorld[] = "hello world";

// Replace the "o"s in "hello world" with "o"s with acute.
const char kUTF8HelloWorld[] = "hell\xC3\xB3 w\xC3\xB3rld";

class TestWTFImpl : public TestWTF {
 public:
  explicit TestWTFImpl(TestWTFRequest request)
      : binding_(this, std::move(request)) {}

  // mojo::test::TestWTF implementation:
  void EchoString(const String& str,
                  const EchoStringCallback& callback) override {
    callback.Run(str);
  }

 private:
  Binding<TestWTF> binding_;
};

class WTFTypesTest : public testing::Test {
 public:
  WTFTypesTest() {}

 private:
  base::MessageLoop loop_;
};

}  // namespace

TEST_F(WTFTypesTest, WTFToWTFStringSerialization) {
  Array<WTF::String> strs(4);
  // strs[0] is null.
  // strs[1] is empty.
  strs[1] = "";
  strs[2] = kHelloWorld;
  strs[3] = WTF::String::fromUTF8(kUTF8HelloWorld);

  Array<WTF::String> cloned_strs = strs.Clone();

  mojo::internal::SerializationContext context;
  size_t size = GetSerializedSize_(cloned_strs, &context);

  mojo::internal::FixedBufferForTesting buf(size);
  mojo::internal::Array_Data<mojo::internal::String_Data*>* data;
  mojo::internal::ArrayValidateParams validate_params(
      0, true, new mojo::internal::ArrayValidateParams(0, false, nullptr));
  SerializeArray_(std::move(cloned_strs), &buf, &data, &validate_params,
                  &context);

  Array<WTF::String> strs2;
  Deserialize_(data, &strs2, nullptr);

  EXPECT_TRUE(strs.Equals(strs2));
}

TEST_F(WTFTypesTest, WTFToMojoStringSerialization) {
  Array<WTF::String> strs(4);
  // strs[0] is null.
  // strs[1] is empty.
  strs[1] = "";
  strs[2] = kHelloWorld;
  strs[3] = WTF::String::fromUTF8(kUTF8HelloWorld);

  Array<WTF::String> cloned_strs = strs.Clone();

  mojo::internal::SerializationContext context;
  size_t size = GetSerializedSize_(cloned_strs, &context);

  mojo::internal::FixedBufferForTesting buf(size);
  mojo::internal::Array_Data<mojo::internal::String_Data*>* data;
  mojo::internal::ArrayValidateParams validate_params(
      0, true, new mojo::internal::ArrayValidateParams(0, false, nullptr));
  SerializeArray_(std::move(cloned_strs), &buf, &data, &validate_params,
                  &context);

  Array<mojo::String> strs2;
  Deserialize_(data, &strs2, nullptr);

  ASSERT_EQ(4u, strs2.size());
  EXPECT_TRUE(strs2[0].is_null());
  EXPECT_TRUE("" == strs2[1]);
  EXPECT_TRUE(kHelloWorld == strs2[2]);
  EXPECT_TRUE(kUTF8HelloWorld == strs2[3]);
}

TEST_F(WTFTypesTest, SendString) {
  blink::TestWTFPtr ptr;
  TestWTFImpl impl(ConvertInterfaceRequest<TestWTF>(GetProxy(&ptr)));

  WTF::String strs[4];
  // strs[0] is null.
  // strs[1] is empty.
  strs[1] = "";
  strs[2] = kHelloWorld;
  strs[3] = WTF::String::fromUTF8(kUTF8HelloWorld);

  for (size_t i = 0; i < arraysize(strs); ++i) {
    base::RunLoop loop;
    // Test that a WTF::String is unchanged after the following conversion:
    //   - serialized;
    //   - deserialized as mojo::String;
    //   - serialized;
    //   - deserialized as WTF::String.
    ptr->EchoString(strs[i], [&loop, &strs, &i](const WTF::String& str) {
      EXPECT_EQ(strs[i], str);
      loop.Quit();
    });
    loop.Run();
  }
}

}  // namespace test
}  // namespace mojo
