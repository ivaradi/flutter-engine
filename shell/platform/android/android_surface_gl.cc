// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/android/android_surface_gl.h"

#include <GLES/gl.h>
#include <utility>

#include "flutter/fml/logging.h"
#include "flutter/fml/memory/ref_ptr.h"
#include "flutter/shell/platform/android/android_shell_holder.h"

namespace flutter {

namespace {
// GL renderer string prefix used by the Android emulator GLES implementation.
constexpr char kEmulatorRendererPrefix[] =
    "Android Emulator OpenGL ES Translator";
}  // anonymous namespace

AndroidSurfaceGL::AndroidSurfaceGL(
    const std::shared_ptr<AndroidContext>& android_context,
    std::shared_ptr<PlatformViewAndroidJNI> jni_facade)
    : AndroidSurface(android_context),
      native_window_(nullptr),
      onscreen_surface_(nullptr),
      offscreen_surface_(nullptr) {
  // Acquire the offscreen surface.
  offscreen_surface_ = GLContextPtr()->CreateOffscreenSurface();
  if (!offscreen_surface_->IsValid()) {
    offscreen_surface_ = nullptr;
  }
}

AndroidSurfaceGL::~AndroidSurfaceGL() = default;

void AndroidSurfaceGL::TeardownOnScreenContext() {
  // When the onscreen surface is destroyed, the context and the surface
  // instance should be deleted. Issue:
  // https://github.com/flutter/flutter/issues/64414
  GLContextPtr()->ClearCurrent();
  onscreen_surface_ = nullptr;
}

bool AndroidSurfaceGL::IsValid() const {
  return offscreen_surface_ && GLContextPtr()->IsValid();
}

std::unique_ptr<Surface> AndroidSurfaceGL::CreateGPUSurface(
    GrDirectContext* gr_context) {
  if (gr_context) {
    return std::make_unique<GPUSurfaceGL>(sk_ref_sp(gr_context), this, true);
  } else {
    sk_sp<GrDirectContext> main_skia_context =
        GLContextPtr()->GetMainSkiaContext();
    if (!main_skia_context) {
      main_skia_context = GPUSurfaceGL::MakeGLContext(this);
      GLContextPtr()->SetMainSkiaContext(main_skia_context);
    }
    return std::make_unique<GPUSurfaceGL>(main_skia_context, this, true);
  }
}

bool AndroidSurfaceGL::OnScreenSurfaceResize(const SkISize& size) {
  FML_DCHECK(IsValid());
  FML_DCHECK(onscreen_surface_);
  FML_DCHECK(native_window_);

  if (size == onscreen_surface_->GetSize()) {
    return true;
  }

  GLContextPtr()->ClearCurrent();

  // Ensure the destructor is called since it destroys the `EGLSurface` before
  // creating a new onscreen surface.
  onscreen_surface_ = nullptr;
  onscreen_surface_ = GLContextPtr()->CreateOnscreenSurface(native_window_);
  if (!onscreen_surface_->IsValid()) {
    FML_LOG(ERROR) << "Unable to create EGL window surface on resize.";
    return false;
  }
  onscreen_surface_->MakeCurrent();
  return true;
}

bool AndroidSurfaceGL::ResourceContextMakeCurrent() {
  FML_DCHECK(IsValid());
  auto status = offscreen_surface_->MakeCurrent();
  return status != AndroidEGLSurfaceMakeCurrentStatus::kFailure;
}

bool AndroidSurfaceGL::ResourceContextClearCurrent() {
  FML_DCHECK(IsValid());
  EGLBoolean result = eglMakeCurrent(eglGetCurrentDisplay(), EGL_NO_SURFACE,
                                     EGL_NO_SURFACE, EGL_NO_CONTEXT);
  return result == EGL_TRUE;
}

bool AndroidSurfaceGL::SetNativeWindow(
    fml::RefPtr<AndroidNativeWindow> window) {
  FML_DCHECK(IsValid());
  FML_DCHECK(window);
  native_window_ = window;
  // Ensure the destructor is called since it destroys the `EGLSurface` before
  // creating a new onscreen surface.
  onscreen_surface_ = nullptr;
  // Create the onscreen surface.
  onscreen_surface_ = GLContextPtr()->CreateOnscreenSurface(window);
  if (!onscreen_surface_->IsValid()) {
    return false;
  }
  return true;
}

std::unique_ptr<GLContextResult> AndroidSurfaceGL::GLContextMakeCurrent() {
  FML_DCHECK(IsValid());
  FML_DCHECK(onscreen_surface_);
  auto status = onscreen_surface_->MakeCurrent();
  auto default_context_result = std::make_unique<GLContextDefaultResult>(
      status != AndroidEGLSurfaceMakeCurrentStatus::kFailure);
  return std::move(default_context_result);
}

bool AndroidSurfaceGL::GLContextClearCurrent() {
  FML_DCHECK(IsValid());
  return GLContextPtr()->ClearCurrent();
}

SurfaceFrame::FramebufferInfo AndroidSurfaceGL::GLContextFramebufferInfo()
    const {
  FML_DCHECK(IsValid());
  SurfaceFrame::FramebufferInfo res;
  res.supports_readback = true;
  res.supports_partial_repaint = onscreen_surface_->SupportsPartialRepaint();
  res.existing_damage = onscreen_surface_->InitialDamage();
  // Some devices (Pixel2 XL) needs EGL_KHR_partial_update rect aligned to 4,
  // otherwise there are glitches
  // (https://github.com/flutter/flutter/issues/97482#)
  // Larger alignment might also be beneficial for tile base renderers.
  res.horizontal_clip_alignment = 32;
  res.vertical_clip_alignment = 32;

  return res;
}

void AndroidSurfaceGL::GLContextSetDamageRegion(
    const std::optional<SkIRect>& region) {
  FML_DCHECK(IsValid());
  onscreen_surface_->SetDamageRegion(region);
}

bool AndroidSurfaceGL::GLContextPresent(uint32_t fbo_id,
                                        const std::optional<SkIRect>& damage) {
  FML_DCHECK(IsValid());
  FML_DCHECK(onscreen_surface_);
  return onscreen_surface_->SwapBuffers(damage);
}

intptr_t AndroidSurfaceGL::GLContextFBO(GLFrameInfo frame_info) const {
  FML_DCHECK(IsValid());
  // The default window bound framebuffer on Android.
  return 0;
}

// |GPUSurfaceGLDelegate|
sk_sp<const GrGLInterface> AndroidSurfaceGL::GetGLInterface() const {
  // This is a workaround for a bug in the Android emulator EGL/GLES
  // implementation.  Some versions of the emulator will not update the
  // GL version string when the process switches to a new EGL context
  // unless the EGL context is being made current for the first time.
  // The inaccurate version string will be rejected by Skia when it
  // tries to build the GrGLInterface.  Flutter can work around this
  // by creating a new context, making it current to force an update
  // of the version, and then reverting to the previous context.
  const char* gl_renderer =
      reinterpret_cast<const char*>(glGetString(GL_RENDERER));
  if (gl_renderer && strncmp(gl_renderer, kEmulatorRendererPrefix,
                             strlen(kEmulatorRendererPrefix)) == 0) {
    EGLContext new_context = GLContextPtr()->CreateNewContext();
    if (new_context != EGL_NO_CONTEXT) {
      EGLContext old_context = eglGetCurrentContext();
      EGLDisplay display = eglGetCurrentDisplay();
      EGLSurface draw_surface = eglGetCurrentSurface(EGL_DRAW);
      EGLSurface read_surface = eglGetCurrentSurface(EGL_READ);
      [[maybe_unused]] EGLBoolean result =
          eglMakeCurrent(display, draw_surface, read_surface, new_context);
      FML_DCHECK(result == EGL_TRUE);
      result = eglMakeCurrent(display, draw_surface, read_surface, old_context);
      FML_DCHECK(result == EGL_TRUE);
      result = eglDestroyContext(display, new_context);
      FML_DCHECK(result == EGL_TRUE);
    }
  }

  return GPUSurfaceGLDelegate::GetGLInterface();
}

AndroidContextGL* AndroidSurfaceGL::GLContextPtr() const {
  return reinterpret_cast<AndroidContextGL*>(android_context_.get());
}

std::unique_ptr<Surface> AndroidSurfaceGL::CreateSnapshotSurface() {
  if (!onscreen_surface_ || !onscreen_surface_->IsValid()) {
    onscreen_surface_ = GLContextPtr()->CreatePbufferSurface();
  }
  sk_sp<GrDirectContext> main_skia_context =
      GLContextPtr()->GetMainSkiaContext();
  if (!main_skia_context) {
    main_skia_context = GPUSurfaceGL::MakeGLContext(this);
    GLContextPtr()->SetMainSkiaContext(main_skia_context);
  }

  return std::make_unique<GPUSurfaceGL>(main_skia_context, this, true);
}

}  // namespace flutter
