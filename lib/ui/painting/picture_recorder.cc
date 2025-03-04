// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/lib/ui/painting/picture_recorder.h"

#include "flutter/display_list/display_list.h"
#include "flutter/lib/ui/painting/canvas.h"
#include "flutter/lib/ui/painting/picture.h"
#include "third_party/tonic/converter/dart_converter.h"
#include "third_party/tonic/dart_args.h"
#include "third_party/tonic/dart_binding_macros.h"
#include "third_party/tonic/dart_library_natives.h"

namespace flutter {

IMPLEMENT_WRAPPERTYPEINFO(ui, PictureRecorder);

void PictureRecorder::Create(Dart_Handle wrapper) {
  UIDartState::ThrowIfUIOperationsProhibited();
  auto res = fml::MakeRefCounted<PictureRecorder>();
  res->AssociateWithDartWrapper(wrapper);
}

PictureRecorder::PictureRecorder() {}

PictureRecorder::~PictureRecorder() {}

SkCanvas* PictureRecorder::BeginRecording(SkRect bounds) {
  bool enable_display_list = UIDartState::Current()->enable_display_list();
  if (enable_display_list) {
    display_list_recorder_ = sk_make_sp<DisplayListCanvasRecorder>(bounds);
    return display_list_recorder_.get();
  } else {
    return picture_recorder_.beginRecording(bounds, &rtree_factory_);
  }
}

fml::RefPtr<Picture> PictureRecorder::endRecording(Dart_Handle dart_picture) {
  if (!canvas_) {
    return nullptr;
  }

  fml::RefPtr<Picture> picture;

  if (display_list_recorder_) {
    picture = Picture::Create(
        dart_picture,
        UIDartState::CreateGPUObject(display_list_recorder_->Build()));
    display_list_recorder_ = nullptr;
  } else {
    picture = Picture::Create(
        dart_picture, UIDartState::CreateGPUObject(
                          picture_recorder_.finishRecordingAsPicture()));
  }

  canvas_->Invalidate();
  canvas_ = nullptr;
  ClearDartWrapper();
  return picture;
}

}  // namespace flutter
