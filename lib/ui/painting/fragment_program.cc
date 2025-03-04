// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <iostream>

#include "flutter/lib/ui/painting/fragment_program.h"

#include "flutter/lib/ui/dart_wrapper.h"
#include "flutter/lib/ui/ui_dart_state.h"
#include "third_party/skia/include/core/SkString.h"
#include "third_party/tonic/converter/dart_converter.h"
#include "third_party/tonic/dart_args.h"
#include "third_party/tonic/dart_binding_macros.h"
#include "third_party/tonic/dart_library_natives.h"
#include "third_party/tonic/typed_data/typed_list.h"

namespace flutter {

IMPLEMENT_WRAPPERTYPEINFO(ui, FragmentProgram);

void FragmentProgram::init(std::string sksl, bool debugPrintSksl) {
  SkRuntimeEffect::Result result =
      SkRuntimeEffect::MakeForShader(SkString(sksl));
  runtime_effect_ = result.effect;

  if (runtime_effect_ == nullptr) {
    Dart_ThrowException(tonic::ToDart(
        std::string("Invalid SkSL:\n") + sksl.c_str() +
        std::string("\nSkSL Error:\n") + result.errorText.c_str()));
    return;
  }
  if (debugPrintSksl) {
    FML_DLOG(INFO) << std::string("debugPrintSksl:\n") + sksl.c_str();
  }
}

fml::RefPtr<FragmentShader> FragmentProgram::shader(Dart_Handle shader,
                                                    Dart_Handle uniforms_handle,
                                                    Dart_Handle samplers) {
  auto sampler_shaders =
      tonic::DartConverter<std::vector<ImageShader*>>::FromDart(samplers);
  tonic::Float32List uniforms(uniforms_handle);
  size_t uniform_count = uniforms.num_elements();
  size_t uniform_data_size =
      (uniform_count + 2 * sampler_shaders.size()) * sizeof(float);
  sk_sp<SkData> uniform_data = SkData::MakeUninitialized(uniform_data_size);
  // uniform_floats must only be referenced BEFORE the call to makeShader below.
  auto* uniform_floats =
      reinterpret_cast<float*>(uniform_data->writable_data());
  for (size_t i = 0; i < uniform_count; i++) {
    uniform_floats[i] = uniforms[i];
  }
  uniforms.Release();
  std::vector<sk_sp<SkShader>> sk_samplers(sampler_shaders.size());
  for (size_t i = 0; i < sampler_shaders.size(); i++) {
    SkSamplingOptions sampling;
    ImageShader* image_shader = sampler_shaders[i];
    // The default value for SkSamplingOptions is used because ImageShader
    // uses a cached value set by the user in the Dart constructor.
    // Users are instructed to make use of this in the Dart docs.
    sk_samplers[i] = image_shader->shader(sampling)->skia_object();
    uniform_floats[uniform_count + 2 * i] = image_shader->width();
    uniform_floats[uniform_count + 2 * i + 1] = image_shader->height();
  }
  auto sk_shader = runtime_effect_->makeShader(
      std::move(uniform_data), sk_samplers.data(), sk_samplers.size());
  return FragmentShader::Create(shader, std::move(sk_shader));
}

void FragmentProgram::Create(Dart_Handle wrapper) {
  auto res = fml::MakeRefCounted<FragmentProgram>();
  res->AssociateWithDartWrapper(wrapper);
}

FragmentProgram::FragmentProgram() = default;

FragmentProgram::~FragmentProgram() = default;

}  // namespace flutter
