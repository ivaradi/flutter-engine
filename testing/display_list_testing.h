// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TESTING_DISPLAY_LIST_TESTING_H_
#define TESTING_DISPLAY_LIST_TESTING_H_

#include <ostream>

#include "flutter/display_list/display_list.h"
#include "flutter/display_list/display_list_dispatcher.h"

namespace flutter {
namespace testing {

bool DisplayListsEQ_Verbose(const DisplayList* a, const DisplayList* b);
bool inline DisplayListsEQ_Verbose(const DisplayList& a, const DisplayList& b) {
  return DisplayListsEQ_Verbose(&a, &b);
}
bool inline DisplayListsEQ_Verbose(sk_sp<const DisplayList> a,
                                   sk_sp<const DisplayList> b) {
  return DisplayListsEQ_Verbose(a.get(), b.get());
}
bool DisplayListsNE_Verbose(const DisplayList* a, const DisplayList* b);
bool inline DisplayListsNE_Verbose(const DisplayList& a, const DisplayList& b) {
  return DisplayListsNE_Verbose(&a, &b);
}
bool inline DisplayListsNE_Verbose(sk_sp<const DisplayList> a,
                                   sk_sp<const DisplayList> b) {
  return DisplayListsNE_Verbose(a.get(), b.get());
}

extern std::ostream& operator<<(std::ostream& os,
                                const DisplayList& display_list);

class DisplayListStreamDispatcher final : public Dispatcher {
 public:
  DisplayListStreamDispatcher(std::ostream& os,
                              int cur_indent = 2,
                              int indent = 2)
      : os_(os), cur_indent_(cur_indent), indent_(indent) {}

  void setAntiAlias(bool aa) override;
  void setDither(bool dither) override;
  void setStyle(DlDrawStyle style) override;
  void setColor(DlColor color) override;
  void setStrokeWidth(SkScalar width) override;
  void setStrokeMiter(SkScalar limit) override;
  void setStrokeCap(DlStrokeCap cap) override;
  void setStrokeJoin(DlStrokeJoin join) override;
  void setColorSource(const DlColorSource* source) override;
  void setColorFilter(const DlColorFilter* filter) override;
  void setInvertColors(bool invert) override;
  void setBlendMode(DlBlendMode mode) override;
  void setBlender(sk_sp<SkBlender> blender) override;
  void setPathEffect(sk_sp<SkPathEffect> effect) override;
  void setMaskFilter(const DlMaskFilter* filter) override;
  void setImageFilter(const DlImageFilter* filter) override;

  void save() override;
  void saveLayer(const SkRect* bounds, const SaveLayerOptions options) override;
  void restore() override;

  void translate(SkScalar tx, SkScalar ty) override;
  void scale(SkScalar sx, SkScalar sy) override;
  void rotate(SkScalar degrees) override;
  void skew(SkScalar sx, SkScalar sy) override;
  // clang-format off
  void transform2DAffine(SkScalar mxx, SkScalar mxy, SkScalar mxt,
                                 SkScalar myx, SkScalar myy, SkScalar myt) override;
  void transformFullPerspective(
      SkScalar mxx, SkScalar mxy, SkScalar mxz, SkScalar mxt,
      SkScalar myx, SkScalar myy, SkScalar myz, SkScalar myt,
      SkScalar mzx, SkScalar mzy, SkScalar mzz, SkScalar mzt,
      SkScalar mwx, SkScalar mwy, SkScalar mwz, SkScalar mwt) override;
  // clang-format on
  void transformReset() override;

  void clipRect(const SkRect& rect, SkClipOp clip_op, bool is_aa) override;
  void clipRRect(const SkRRect& rrect, SkClipOp clip_op, bool is_aa) override;
  void clipPath(const SkPath& path, SkClipOp clip_op, bool is_aa) override;

  void drawColor(DlColor color, DlBlendMode mode) override;
  void drawPaint() override;
  void drawLine(const SkPoint& p0, const SkPoint& p1) override;
  void drawRect(const SkRect& rect) override;
  void drawOval(const SkRect& bounds) override;
  void drawCircle(const SkPoint& center, SkScalar radius) override;
  void drawRRect(const SkRRect& rrect) override;
  void drawDRRect(const SkRRect& outer, const SkRRect& inner) override;
  void drawPath(const SkPath& path) override;
  void drawArc(const SkRect& oval_bounds,
               SkScalar start_degrees,
               SkScalar sweep_degrees,
               bool use_center) override;
  void drawPoints(SkCanvas::PointMode mode,
                  uint32_t count,
                  const SkPoint points[]) override;
  void drawSkVertices(const sk_sp<SkVertices> vertices,
                      SkBlendMode mode) override;
  void drawVertices(const DlVertices* vertices, DlBlendMode mode) override;
  void drawImage(const sk_sp<DlImage> image,
                 const SkPoint point,
                 const SkSamplingOptions& sampling,
                 bool render_with_attributes) override;
  void drawImageRect(const sk_sp<DlImage> image,
                     const SkRect& src,
                     const SkRect& dst,
                     const SkSamplingOptions& sampling,
                     bool render_with_attributes,
                     SkCanvas::SrcRectConstraint constraint) override;
  void drawImageNine(const sk_sp<DlImage> image,
                     const SkIRect& center,
                     const SkRect& dst,
                     SkFilterMode filter,
                     bool render_with_attributes) override;
  void drawImageLattice(const sk_sp<DlImage> image,
                        const SkCanvas::Lattice& lattice,
                        const SkRect& dst,
                        SkFilterMode filter,
                        bool render_with_attributes) override;
  void drawAtlas(const sk_sp<DlImage> atlas,
                 const SkRSXform xform[],
                 const SkRect tex[],
                 const DlColor colors[],
                 int count,
                 DlBlendMode mode,
                 const SkSamplingOptions& sampling,
                 const SkRect* cull_rect,
                 bool render_with_attributes) override;
  void drawPicture(const sk_sp<SkPicture> picture,
                   const SkMatrix* matrix,
                   bool render_with_attributes) override;
  void drawDisplayList(const sk_sp<DisplayList> display_list) override;
  void drawTextBlob(const sk_sp<SkTextBlob> blob,
                    SkScalar x,
                    SkScalar y) override;
  void drawShadow(const SkPath& path,
                  const DlColor color,
                  const SkScalar elevation,
                  bool transparent_occluder,
                  SkScalar dpr) override;

 private:
  std::ostream& os_;
  int cur_indent_;
  int indent_;

  void indent() { cur_indent_ += indent_; }
  void outdent() { cur_indent_ -= indent_; }

  template <class T>
  std::ostream& out_array(std::string name, int count, const T array[]);

  std::ostream& startl();

  void out(const DlColorFilter* filter);
};

}  // namespace testing
}  // namespace flutter

#endif  // TESTING_DISPLAY_LIST_TESTING_H_
