#ifndef AR_CORE_EXAMPLES_C_HELLOARCLOUDANCHOR_CPP_CLOUD_ANCHOR_MANAGER_H_
#define AR_CORE_EXAMPLES_C_HELLOARCLOUDANCHOR_CPP_CLOUD_ANCHOR_MANAGER_H_

#include <mutex>  // NOLINT
#include <vector>
#include "arcore_c_api.h"

namespace cloud_anchor {

class CloudAnchorManager {
 public:
  CloudAnchorManager();

  // Called on activity's pause.  No state needs to change, but taking hold
  // of the mutex guarantees the completion of any pending work.
  void OnPause() { std::lock_guard<std::mutex> lock(mutex_); }

  // Called on the UI thread from Java when the host button is pressed.
  void OnHostButtonPress();
  // Called on the UI thread from Java when the resolve button is pressed.
  void OnResolveButtonPress();

  // Called on the UI thread when OK is pressed on the resolve fragment.
  void OnResolveDialogOkPress(int room_code);
  // Called on the UI thread when cancel is pressed on the resolve fragment.
  void OnResolveDialogCancelPress();
  // Called when a communication error, or failure to resolve a room ID happens.
  void OnFirebaseError();
  // Called when a cloud anchor ID is made available.
  void OnCloudAnchorIdMadeAvailable(const std::string& anchor_id);

  // Should be called with the updated anchors available after an
  // ArSession_update() call.
  void OnUpdate(const ArFrame* ar_frame);

  // To be called once the ArSession is successfully created.
  void SetArSession(ArSession* ar_session) { ar_session_ = ar_session; }

  // Returns an anchor which is being managed (either hosting or resolved) by
  // the CloudAnchorManager, or nullptr if no anchor is currently being managed.
  const ArAnchor* GetCloudAnchor() const;

  // Returns a cloud anchor id, if one is currently associated.
  std::string GetCloudAnchorId() const;

  // Returns true if the anchor manager is permitting an anchor to be placed.
  // Anchors cannot be placed in a non-hosting state, or if an Android is placed
  // already.
  bool AllowHostingAnchor() const;

  // Registers a local anchor as a cloud anchor. Only possible when
  // host_resolve_mode_ is in the HOSTING state (the user has selected the UX
  // option host). Returns true on success.
  ArStatus RegisterLocalAnchorAsCloudAnchor(const ArAnchor* anchor);

 private:
  // Returns true if the cloud anchor has reached a resolved state and is no
  // longer pending.
  bool AnchorInReturnableState(const ArAnchor* anchor);

  // Sets the currently active cloud anchor, possibly releasing a previous one,
  // if one was held.  It is valid to call with nullptr, in which case the
  // anchor will become unset.
  void SetTrackedCloudAnchor(ArAnchor* anchor);

  // If there is a pending anchor and it has reached a valid tracking state then
  // the current cloud anchor will be updated and this will return true.  If
  // this function returns false there either was no anchor pending or the
  // anchor is not in a tracking state.
  bool PromotePendingAnchorToCloudAnchor();

  // This pointer is owned by the application context.
  ArSession* ar_session_ = nullptr;

  // The currently tracked AR cloud anchor.
  ArAnchor* ar_cloud_anchor_ = nullptr;

  // State of the cloud anchor manager.  Valid state transitions are:
  //    NONE->HOSTING
  //    NONE->RESOLVE_DIALOG_PRESENTING
  //    RESOLVE_DIALOG_PRESENTING->RESOLVING
  //    RESOLVING->RESOLVED
  //    *->NONE
  enum class HostResolveMode {
    NONE,
    HOSTING,
    RESOLVE_DIALOG_PRESENTING,
    RESOLVING,
    RESOLVED,
  };

  // The current state of the cloud anchor manager.
  HostResolveMode host_resolve_mode_;

  // The cloud anchor, if it is exists. This anchor is owned.
  ArAnchor* pending_anchor_ = nullptr;

  mutable std::mutex mutex_;
};

}  // namespace cloud_anchor

#endif  // AR_CORE_EXAMPLES_C_HELLOARCLOUDANCHOR_CPP_CLOUD_ANCHOR_MANAGER_H_
