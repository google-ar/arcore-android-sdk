#include "cloud_anchor_manager.h"

#include "arcore_c_api.h"
#include "util.h"

namespace cloud_anchor {

CloudAnchorManager::CloudAnchorManager()
    : host_resolve_mode_(HostResolveMode::NONE) {}

bool CloudAnchorManager::AllowHostingAnchor() const {
  std::lock_guard<std::mutex> lock(mutex_);
  if (ar_cloud_anchor_ != nullptr) {
    return false;
  }
  if (pending_anchor_ != nullptr) {
    return false;
  }

  return (host_resolve_mode_ == HostResolveMode::HOSTING);
}

ArStatus CloudAnchorManager::RegisterLocalAnchorAsCloudAnchor(
    const ArAnchor* anchor) {
  std::lock_guard<std::mutex> lock(mutex_);
  ArAnchor* out_anchor;
  ArStatus out_status =
      ArSession_hostAndAcquireNewCloudAnchor(ar_session_, anchor, &out_anchor);
  if (out_status == AR_SUCCESS) {
    pending_anchor_ = out_anchor;
  } else {
    LOGE("Failed to create cloud anchor.");
  }
  return out_status;
}

bool CloudAnchorManager::AnchorInReturnableState(const ArAnchor* anchor) {
  ArCloudAnchorState out_state;
  ArAnchor_getCloudAnchorState(ar_session_, anchor, &out_state);

  switch (out_state) {
    case ArCloudAnchorState::AR_CLOUD_ANCHOR_STATE_NONE:
    case ArCloudAnchorState::AR_CLOUD_ANCHOR_STATE_TASK_IN_PROGRESS:
      return false;
    default:
      return true;
  }
}

void CloudAnchorManager::SetTrackedCloudAnchor(ArAnchor* anchor) {
  // Note:  This function is private and it is assumed the write lock is held.
  if (ar_cloud_anchor_ != nullptr) {
    ArAnchor_release(ar_cloud_anchor_);
  }
  ar_cloud_anchor_ = anchor;

  pending_anchor_ = nullptr;
}

bool CloudAnchorManager::PromotePendingAnchorToCloudAnchor() {
  // Note:  This function is private and it is assumed the write lock is held.
  if (!pending_anchor_) {
    return false;
  }
  if (AnchorInReturnableState(pending_anchor_)) {
    SetTrackedCloudAnchor(pending_anchor_);
    return true;
  }
  return false;
}

void CloudAnchorManager::OnHostButtonPress() {
  std::lock_guard<std::mutex> lock(mutex_);
  CHECK(host_resolve_mode_ == HostResolveMode::NONE ||
        host_resolve_mode_ == HostResolveMode::HOSTING);
  switch (host_resolve_mode_) {
    case HostResolveMode::NONE:
      host_resolve_mode_ = HostResolveMode::HOSTING;
      util::DisplayMessageOnLowerSnackbar(
          "Now in Hosting mode.  Press Cancel to exit.");
      util::SetHostAndResolveButtonVisibility(
          util::HostResolveVisibilityEnum::ONLY_HOST);
      util::UpdateFirebaseRoomCode(true, 0);
      break;
    case HostResolveMode::HOSTING:
      // When the host button is pressed in hosting state, it cancels.
      SetTrackedCloudAnchor(nullptr);
      host_resolve_mode_ = HostResolveMode::NONE,
      util::SetHostAndResolveButtonVisibility(
          util::HostResolveVisibilityEnum::ALL);
      util::UpdateFirebaseRoomCode(false, 0);
      break;
    default:
      break;
  }
}

void CloudAnchorManager::OnResolveButtonPress() {
  std::lock_guard<std::mutex> lock(mutex_);
  CHECK(host_resolve_mode_ == HostResolveMode::NONE ||
        host_resolve_mode_ == HostResolveMode::RESOLVING ||
        host_resolve_mode_ == HostResolveMode::RESOLVED);
  util::SetHostAndResolveButtonVisibility(
      util::HostResolveVisibilityEnum::ONLY_RESOLVE);

  switch (host_resolve_mode_) {
    case HostResolveMode::NONE:
      host_resolve_mode_ = HostResolveMode::RESOLVE_DIALOG_PRESENTING;
      util::ShowResolveDialog();
      break;
    case HostResolveMode::RESOLVING:
    case HostResolveMode::RESOLVED:
      // When the resolve button is pressed in a resolving state, it cancels.
      host_resolve_mode_ = HostResolveMode::NONE,
      util::SetHostAndResolveButtonVisibility(
          util::HostResolveVisibilityEnum::ALL);
      SetTrackedCloudAnchor(nullptr);
      util::UpdateFirebaseRoomCode(false, 0);
      break;
    default:
      break;
  }
}

void CloudAnchorManager::OnResolveDialogOkPress(int room_code) {
  std::lock_guard<std::mutex> lock(mutex_);
  CHECK(host_resolve_mode_ == HostResolveMode::RESOLVE_DIALOG_PRESENTING);
  util::DisplayMessageOnLowerSnackbar(
      "Now in Resolving mode. Press Cancel to exit.");
  util::SetHostAndResolveButtonVisibility(
      util::HostResolveVisibilityEnum::ONLY_RESOLVE);
  util::UpdateFirebaseRoomCode(false, room_code);
  host_resolve_mode_ = HostResolveMode::RESOLVING;
}

void CloudAnchorManager::OnResolveDialogCancelPress() {
  std::lock_guard<std::mutex> lock(mutex_);
  CHECK(host_resolve_mode_ == HostResolveMode::RESOLVE_DIALOG_PRESENTING);
  util::SetHostAndResolveButtonVisibility(util::HostResolveVisibilityEnum::ALL);
  SetTrackedCloudAnchor(nullptr);
  util::UpdateFirebaseRoomCode(false, 0);
  host_resolve_mode_ = HostResolveMode::NONE;
}

void CloudAnchorManager::OnFirebaseError() {
  std::lock_guard<std::mutex> lock(mutex_);
  CHECK(ar_session_);
  util::SetHostAndResolveButtonVisibility(util::HostResolveVisibilityEnum::ALL);
  util::DisplayMessageOnLowerSnackbar("Failed to resolve room ID.");
  SetTrackedCloudAnchor(nullptr);
  util::UpdateFirebaseRoomCode(false, 0);
  host_resolve_mode_ = HostResolveMode::NONE;
}

void CloudAnchorManager::OnCloudAnchorIdMadeAvailable(
    const std::string& anchor_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  CHECK(ar_session_);
  ArAnchor* out_anchor;
  ArStatus ar_status = ArSession_resolveAndAcquireNewCloudAnchor(
      ar_session_, anchor_id.c_str(), &out_anchor);
  CHECK(ar_status == AR_SUCCESS);
  SetTrackedCloudAnchor(out_anchor);
}

void CloudAnchorManager::OnUpdate(const ArFrame* ar_frame) {
  std::lock_guard<std::mutex> lock(mutex_);
  CHECK(ar_session_);
  CHECK(ar_frame);

  if (PromotePendingAnchorToCloudAnchor()) {
    util::MaybeUpdateFirebase(GetCloudAnchorId());
  }
}

const ArAnchor* CloudAnchorManager::GetCloudAnchor() const {
  std::lock_guard<std::mutex> lock(mutex_);
  if (ar_cloud_anchor_) return ar_cloud_anchor_;
  if (pending_anchor_) return pending_anchor_;
  return nullptr;
}

std::string CloudAnchorManager::GetCloudAnchorId() const {
  if (!ar_cloud_anchor_) return std::string();

  char* out_cloud_anchor_id;
  ArAnchor_acquireCloudAnchorId(ar_session_, ar_cloud_anchor_,
                                &out_cloud_anchor_id);
  std::string ret_val(out_cloud_anchor_id);
  ArString_release(out_cloud_anchor_id);

  return ret_val;
}

}  // namespace cloud_anchor
