// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_LIB_UI_WINDOW_PLATFORM_CONFIGURATION_H_
#define FLUTTER_LIB_UI_WINDOW_PLATFORM_CONFIGURATION_H_

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "flutter/fml/time/time_point.h"
#include "flutter/lib/ui/semantics/semantics_update.h"
#include "flutter/lib/ui/window/pointer_data_packet.h"
#include "flutter/lib/ui/window/viewport_metrics.h"
#include "flutter/lib/ui/window/window.h"
#include "third_party/tonic/dart_persistent_value.h"
#include "third_party/tonic/typed_data/dart_byte_data.h"

namespace flutter {
class FontCollection;
class PlatformMessage;
class Scene;

//--------------------------------------------------------------------------
/// @brief An enum for defining the different kinds of accessibility features
///        that can be enabled by the platform.
///
///         Must match the `AccessibilityFeatureFlag` enum in framework.
enum class AccessibilityFeatureFlag : int32_t {
  kAccessibleNavigation = 1 << 0,
  kInvertColors = 1 << 1,
  kDisableAnimations = 1 << 2,
  kBoldText = 1 << 3,
  kReduceMotion = 1 << 4,
  kHighContrast = 1 << 5,
  kOnOffSwitchLabels = 1 << 6,
};

//--------------------------------------------------------------------------
/// @brief A client interface that the `RuntimeController` uses to define
///        handlers for `PlatformConfiguration` requests.
///
/// @see   `PlatformConfiguration`
///
class PlatformConfigurationClient {
 public:
  //--------------------------------------------------------------------------
  /// @brief      The route or path that the embedder requested when the
  ///             application was launched.
  ///
  ///             This will be the string "`/`" if no particular route was
  ///             requested.
  ///
  virtual std::string DefaultRouteName() = 0;

  //--------------------------------------------------------------------------
  /// @brief      Requests that, at the next appropriate opportunity, a new
  ///             frame be scheduled for rendering.
  ///
  virtual void ScheduleFrame() = 0;

  //--------------------------------------------------------------------------
  /// @brief      Updates the client's rendering on the GPU with the newly
  ///             provided Scene.
  ///
  virtual void Render(Scene* scene) = 0;

  //--------------------------------------------------------------------------
  /// @brief      Receives a updated semantics tree from the Framework.
  ///
  /// @param[in] update The updated semantic tree to apply.
  ///
  virtual void UpdateSemantics(SemanticsUpdate* update) = 0;

  //--------------------------------------------------------------------------
  /// @brief      When the Flutter application has a message to send to the
  ///             underlying platform, the message needs to be forwarded to
  ///             the platform on the appropriate thread (via the platform
  ///             task runner). The PlatformConfiguration delegates this task
  ///             to the engine via this method.
  ///
  /// @see        `PlatformView::HandlePlatformMessage`
  ///
  /// @param[in]  message  The message from the Flutter application to send to
  ///                      the underlying platform.
  ///
  virtual void HandlePlatformMessage(
      std::unique_ptr<PlatformMessage> message) = 0;

  //--------------------------------------------------------------------------
  /// @brief      Returns the current collection of fonts available on the
  ///             platform.
  ///
  ///             This function reads an XML file and makes font families and
  ///             collections of them. MinikinFontForTest is used for FontFamily
  ///             creation.
  virtual FontCollection& GetFontCollection() = 0;

  //--------------------------------------------------------------------------
  /// @brief      Notifies this client of the name of the root isolate and its
  ///             port when that isolate is launched, restarted (in the
  ///             cold-restart scenario) or the application itself updates the
  ///             name of the root isolate (via `Window.setIsolateDebugName`
  ///             in `window.dart`). The name of the isolate is meaningless to
  ///             the engine but is used in instrumentation and tooling.
  ///             Currently, this information is to update the service
  ///             protocol list of available root isolates running in the VM
  ///             and their names so that the appropriate isolate can be
  ///             selected in the tools for debugging and instrumentation.
  ///
  /// @param[in]  isolate_name  The isolate name
  /// @param[in]  isolate_port  The isolate port
  ///
  virtual void UpdateIsolateDescription(const std::string isolate_name,
                                        int64_t isolate_port) = 0;

  //--------------------------------------------------------------------------
  /// @brief      Notifies this client that the application has an opinion about
  ///             whether its frame timings need to be reported backed to it.
  ///             Due to the asynchronous nature of rendering in Flutter, it is
  ///             not possible for the application to determine the total time
  ///             it took to render a specific frame. While the layer-tree is
  ///             constructed on the UI thread, it needs to be rendering on the
  ///             raster thread. Dart code cannot execute on this thread. So any
  ///             instrumentation about the frame times gathered on this thread
  ///             needs to be aggregated and sent back to the UI thread for
  ///             processing in Dart.
  ///
  ///             When the application indicates that frame times need to be
  ///             reported, it collects this information till a specified number
  ///             of data points are gathered. Then this information is sent
  ///             back to Dart code via `Engine::ReportTimings`.
  ///
  ///             This option is engine counterpart of the
  ///             `Window._setNeedsReportTimings` in `window.dart`.
  ///
  /// @param[in]  needs_reporting  If reporting information should be collected
  /// and send back to Dart.
  ///
  virtual void SetNeedsReportTimings(bool value) = 0;

  //--------------------------------------------------------------------------
  /// @brief      The embedder can specify data that the isolate can request
  ///             synchronously on launch. This accessor fetches that data.
  ///
  ///             This data is persistent for the duration of the Flutter
  ///             application and is available even after isolate restarts.
  ///             Because of this lifecycle, the size of this data must be kept
  ///             to a minimum.
  ///
  ///             For asynchronous communication between the embedder and
  ///             isolate, a platform channel may be used.
  ///
  /// @return     A map of the isolate data that the framework can request upon
  ///             launch.
  ///
  virtual std::shared_ptr<const fml::Mapping> GetPersistentIsolateData() = 0;

  //--------------------------------------------------------------------------
  /// @brief      Directly invokes platform-specific APIs to compute the
  ///             locale the platform would have natively resolved to.
  ///
  /// @param[in]  supported_locale_data  The vector of strings that represents
  ///                                    the locales supported by the app.
  ///                                    Each locale consists of three
  ///                                    strings: languageCode, countryCode,
  ///                                    and scriptCode in that order.
  ///
  /// @return     A vector of 3 strings languageCode, countryCode, and
  ///             scriptCode that represents the locale selected by the
  ///             platform. Empty strings mean the value was unassigned. Empty
  ///             vector represents a null locale.
  ///
  virtual std::unique_ptr<std::vector<std::string>>
  ComputePlatformResolvedLocale(
      const std::vector<std::string>& supported_locale_data) = 0;

  //--------------------------------------------------------------------------
  /// @brief      Invoked when the Dart VM requests that a deferred library
  ///             be loaded. Notifies the engine that the deferred library
  ///             identified by the specified loading unit id should be
  ///             downloaded and loaded into the Dart VM via
  ///             `LoadDartDeferredLibrary`
  ///
  ///             Upon encountering errors or otherwise failing to load a
  ///             loading unit with the specified id, the failure should be
  ///             directly reported to dart by calling
  ///             `LoadDartDeferredLibraryFailure` to ensure the waiting dart
  ///             future completes with an error.
  ///
  /// @param[in]  loading_unit_id  The unique id of the deferred library's
  ///                              loading unit. This id is to be passed
  ///                              back into LoadDartDeferredLibrary
  ///                              in order to identify which deferred
  ///                              library to load.
  ///
  virtual void RequestDartDeferredLibrary(intptr_t loading_unit_id) = 0;

 protected:
  virtual ~PlatformConfigurationClient();
};

//----------------------------------------------------------------------------
/// @brief      A class for holding and distributing platform-level information
///             to and from the Dart code in Flutter's framework.
///
///             It handles communication between the engine and the framework,
///             and owns the main window.
///
///             It communicates with the RuntimeController through the use of a
///             PlatformConfigurationClient interface, which the
///             RuntimeController defines.
///
class PlatformConfiguration final {
 public:
  //----------------------------------------------------------------------------
  /// @brief      Creates a new PlatformConfiguration, typically created by the
  ///             RuntimeController.
  ///
  /// @param[in] client The `PlatformConfigurationClient` to be injected into
  ///                   the PlatformConfiguration. This client is used to
  ///                   forward requests to the RuntimeController.
  ///
  explicit PlatformConfiguration(PlatformConfigurationClient* client);

  // PlatformConfiguration is not copyable.
  PlatformConfiguration(const PlatformConfiguration&) = delete;
  PlatformConfiguration& operator=(const PlatformConfiguration&) = delete;

  ~PlatformConfiguration();

  //----------------------------------------------------------------------------
  /// @brief      Access to the platform configuration client (which typically
  ///             is implemented by the RuntimeController).
  ///
  /// @return     Returns the client used to construct this
  /// PlatformConfiguration.
  ///
  PlatformConfigurationClient* client() const { return client_; }

  //----------------------------------------------------------------------------
  /// @brief      Called by the RuntimeController once it has created the root
  ///             isolate, so that the PlatformController can get a handle to
  ///             the 'dart:ui' library.
  ///
  ///             It uses the handle to call the hooks in hooks.dart.
  ///
  void DidCreateIsolate();

  //----------------------------------------------------------------------------
  /// @brief      Update the specified locale data in the framework.
  ///
  /// @deprecated The persistent isolate data must be used for this purpose
  ///             instead.
  ///
  /// @param[in]  locale_data  The locale data. This should consist of groups of
  ///             4 strings, each group representing a single locale.
  ///
  void UpdateLocales(const std::vector<std::string>& locales);

  //----------------------------------------------------------------------------
  /// @brief      Update the user settings data in the framework.
  ///
  /// @deprecated The persistent isolate data must be used for this purpose
  ///             instead.
  ///
  /// @param[in]  data  The user settings data.
  ///
  void UpdateUserSettingsData(const std::string& data);

  //----------------------------------------------------------------------------
  /// @brief      Updates the lifecycle state data in the framework.
  ///
  /// @deprecated The persistent isolate data must be used for this purpose
  ///             instead.
  ///
  /// @param[in]  data  The lifecycle state data.
  ///
  void UpdateLifecycleState(const std::string& data);

  //----------------------------------------------------------------------------
  /// @brief      Notifies the PlatformConfiguration that the embedder has
  ///             expressed an opinion about whether the accessibility tree
  ///             should be generated or not. This call originates in the
  ///             platform view and is forwarded to the PlatformConfiguration
  ///             here by the engine.
  ///
  /// @param[in]  enabled  Whether the accessibility tree is enabled or
  ///                      disabled.
  ///
  void UpdateSemanticsEnabled(bool enabled);

  //----------------------------------------------------------------------------
  /// @brief      Forward the preference of accessibility features that must be
  ///             enabled in the semantics tree to the framwork.
  ///
  /// @param[in]  flags  The accessibility features that must be generated in
  ///             the semantics tree.
  ///
  void UpdateAccessibilityFeatures(int32_t flags);

  //----------------------------------------------------------------------------
  /// @brief      Notifies the PlatformConfiguration that the client has sent
  ///             it a message. This call originates in the platform view and
  ///             has been forwarded through the engine to here.
  ///
  /// @param[in]  message  The message sent from the embedder to the Dart
  ///                      application.
  ///
  void DispatchPlatformMessage(std::unique_ptr<PlatformMessage> message);

  //----------------------------------------------------------------------------
  /// @brief      Notifies the framework that the embedder encountered an
  ///             accessibility related action on the specified node. This call
  ///             originates on the platform view and has been forwarded to the
  ///             platform configuration here by the engine.
  ///
  /// @param[in]  id      The identifier of the accessibility node.
  /// @param[in]  action  The accessibility related action performed on the
  ///                     node of the specified ID.
  /// @param[in]  args    Optional data that applies to the specified action.
  ///
  void DispatchSemanticsAction(int32_t id,
                               SemanticsAction action,
                               fml::MallocMapping args);

  //----------------------------------------------------------------------------
  /// @brief      Notifies the framework that it is time to begin working on a
  ///             new frame previously scheduled via a call to
  ///             `PlatformConfigurationClient::ScheduleFrame`. This call
  ///             originates in the animator.
  ///
  ///             The frame time given as the argument indicates the point at
  ///             which the current frame interval began. It is very slightly
  ///             (because of scheduling overhead) in the past. If a new layer
  ///             tree is not produced and given to the raster task runner
  ///             within one frame interval from this point, the Flutter
  ///             application will jank.
  ///
  ///             This method calls the `::_beginFrame` method in `hooks.dart`.
  ///
  /// @param[in]  frame_time  The point at which the current frame interval
  ///                         began. May be used by animation interpolators,
  ///                         physics simulations, etc..
  ///
  /// @param[in]  frame_number The frame number recorded by the animator. Used
  ///                          by the framework to associate frame specific
  ///                          debug information with frame timings and timeline
  ///                          events.
  ///
  void BeginFrame(fml::TimePoint frame_time, uint64_t frame_number);

  //----------------------------------------------------------------------------
  /// @brief      Dart code cannot fully measure the time it takes for a
  ///             specific frame to be rendered. This is because Dart code only
  ///             runs on the UI task runner. That is only a small part of the
  ///             overall frame workload. The raster task runner frame workload
  ///             is executed on a thread where Dart code cannot run (and hence
  ///             instrument). Besides, due to the pipelined nature of rendering
  ///             in Flutter, there may be multiple frame workloads being
  ///             processed at any given time. However, for non-Timeline based
  ///             profiling, it is useful for trace collection and processing to
  ///             happen in Dart. To do this, the raster task runner frame
  ///             workloads need to be instrumented separately. After a set
  ///             number of these profiles have been gathered, they need to be
  ///             reported back to Dart code. The engine reports this extra
  ///             instrumentation information back to the framework by invoking
  ///             this method at predefined intervals.
  ///
  /// @see        `FrameTiming`
  ///
  /// @param[in]  timings  Collection of `FrameTiming::kStatisticsCount` * 'n'
  ///                      values for `n` frames whose timings have not been
  ///                      reported yet. Many of the values are timestamps, but
  ///                      a collection of integers is reported here for easier
  ///                      conversions to Dart objects. The timestamps are
  ///                      measured against the system monotonic clock measured
  ///                      in microseconds.
  ///
  void ReportTimings(std::vector<int64_t> timings);

  //----------------------------------------------------------------------------
  /// @brief      Retrieves the Window with the given ID managed by the
  ///             `PlatformConfiguration`.
  ///
  /// @param[in] window_id The id of the window to find and return.
  ///
  /// @return     a pointer to the Window.
  ///
  Window* get_window(int window_id) { return windows_[window_id].get(); }

  //----------------------------------------------------------------------------
  /// @brief      Responds to a previous platform message to the engine from the
  ///             framework.
  ///
  /// @param[in] response_id The unique id that identifies the original platform
  ///                        message to respond to.
  /// @param[in] data        The data to send back in the response.
  ///
  void CompletePlatformMessageResponse(int response_id,
                                       std::vector<uint8_t> data);

  //----------------------------------------------------------------------------
  /// @brief      Responds to a previous platform message to the engine from the
  ///             framework with an empty response.
  ///
  /// @param[in] response_id The unique id that identifies the original platform
  ///                        message to respond to.
  ///
  void CompletePlatformMessageEmptyResponse(int response_id);

  Dart_Handle on_error() { return on_error_.Get(); }

 private:
  PlatformConfigurationClient* client_;
  tonic::DartPersistentValue on_error_;
  tonic::DartPersistentValue update_locales_;
  tonic::DartPersistentValue update_user_settings_data_;
  tonic::DartPersistentValue update_lifecycle_state_;
  tonic::DartPersistentValue update_semantics_enabled_;
  tonic::DartPersistentValue update_accessibility_features_;
  tonic::DartPersistentValue dispatch_platform_message_;
  tonic::DartPersistentValue dispatch_semantics_action_;
  tonic::DartPersistentValue begin_frame_;
  tonic::DartPersistentValue draw_frame_;
  tonic::DartPersistentValue report_timings_;

  std::unordered_map<int64_t, std::unique_ptr<Window>> windows_;

  // ID starts at 1 because an ID of 0 indicates that no response is expected.
  int next_response_id_ = 1;
  std::unordered_map<int, fml::RefPtr<PlatformMessageResponse>>
      pending_responses_;
};

//----------------------------------------------------------------------------
// API exposed as FFI calls in Dart.
//
// These are probably not supposed to be called directly, and should instead
// be called through their sibling API in `PlatformConfiguration` or
// `PlatformConfigurationClient`.
//
// These are intentionally undocumented. Refer instead to the sibling methods
// above.
//----------------------------------------------------------------------------
class PlatformConfigurationNativeApi {
 public:
  static std::string DefaultRouteName();

  static void ScheduleFrame();

  static void Render(Scene* scene);

  static void UpdateSemantics(SemanticsUpdate* update);

  static void SetNeedsReportTimings(bool value);

  static Dart_Handle GetPersistentIsolateData();

  static Dart_Handle ComputePlatformResolvedLocale(
      Dart_Handle supportedLocalesHandle);

  static void SetIsolateDebugName(const std::string name);

  static Dart_Handle SendPlatformMessage(const std::string& name,
                                         Dart_Handle callback,
                                         Dart_Handle data_handle);

  static void RespondToPlatformMessage(int response_id,
                                       const tonic::DartByteData& data);
};

}  // namespace flutter

#endif  // FLUTTER_LIB_UI_WINDOW_PLATFORM_CONFIGURATION_H_
