// automatically generated by the FlatBuffers compiler, do not modify


#ifndef FLATBUFFERS_GENERATED_MBGETINSTALLEDROMS_MBTOOL_DAEMON_V3_H_
#define FLATBUFFERS_GENERATED_MBGETINSTALLEDROMS_MBTOOL_DAEMON_V3_H_

#include "flatbuffers/flatbuffers.h"

namespace mbtool {
namespace daemon {
namespace v3 {

struct MbRom;

struct MbGetInstalledRomsRequest;

struct MbGetInstalledRomsResponse;

struct MbRom FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  enum {
    VT_ID = 4,
    VT_SYSTEM_PATH = 6,
    VT_CACHE_PATH = 8,
    VT_DATA_PATH = 10,
    VT_VERSION = 12,
    VT_BUILD = 14
  };
  const flatbuffers::String *id() const {
    return GetPointer<const flatbuffers::String *>(VT_ID);
  }
  const flatbuffers::String *system_path() const {
    return GetPointer<const flatbuffers::String *>(VT_SYSTEM_PATH);
  }
  const flatbuffers::String *cache_path() const {
    return GetPointer<const flatbuffers::String *>(VT_CACHE_PATH);
  }
  const flatbuffers::String *data_path() const {
    return GetPointer<const flatbuffers::String *>(VT_DATA_PATH);
  }
  const flatbuffers::String *version() const {
    return GetPointer<const flatbuffers::String *>(VT_VERSION);
  }
  const flatbuffers::String *build() const {
    return GetPointer<const flatbuffers::String *>(VT_BUILD);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyOffset(verifier, VT_ID) &&
           verifier.Verify(id()) &&
           VerifyOffset(verifier, VT_SYSTEM_PATH) &&
           verifier.Verify(system_path()) &&
           VerifyOffset(verifier, VT_CACHE_PATH) &&
           verifier.Verify(cache_path()) &&
           VerifyOffset(verifier, VT_DATA_PATH) &&
           verifier.Verify(data_path()) &&
           VerifyOffset(verifier, VT_VERSION) &&
           verifier.Verify(version()) &&
           VerifyOffset(verifier, VT_BUILD) &&
           verifier.Verify(build()) &&
           verifier.EndTable();
  }
};

struct MbRomBuilder {
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_id(flatbuffers::Offset<flatbuffers::String> id) {
    fbb_.AddOffset(MbRom::VT_ID, id);
  }
  void add_system_path(flatbuffers::Offset<flatbuffers::String> system_path) {
    fbb_.AddOffset(MbRom::VT_SYSTEM_PATH, system_path);
  }
  void add_cache_path(flatbuffers::Offset<flatbuffers::String> cache_path) {
    fbb_.AddOffset(MbRom::VT_CACHE_PATH, cache_path);
  }
  void add_data_path(flatbuffers::Offset<flatbuffers::String> data_path) {
    fbb_.AddOffset(MbRom::VT_DATA_PATH, data_path);
  }
  void add_version(flatbuffers::Offset<flatbuffers::String> version) {
    fbb_.AddOffset(MbRom::VT_VERSION, version);
  }
  void add_build(flatbuffers::Offset<flatbuffers::String> build) {
    fbb_.AddOffset(MbRom::VT_BUILD, build);
  }
  explicit MbRomBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  MbRomBuilder &operator=(const MbRomBuilder &);
  flatbuffers::Offset<MbRom> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<MbRom>(end);
    return o;
  }
};

inline flatbuffers::Offset<MbRom> CreateMbRom(
    flatbuffers::FlatBufferBuilder &_fbb,
    flatbuffers::Offset<flatbuffers::String> id = 0,
    flatbuffers::Offset<flatbuffers::String> system_path = 0,
    flatbuffers::Offset<flatbuffers::String> cache_path = 0,
    flatbuffers::Offset<flatbuffers::String> data_path = 0,
    flatbuffers::Offset<flatbuffers::String> version = 0,
    flatbuffers::Offset<flatbuffers::String> build = 0) {
  MbRomBuilder builder_(_fbb);
  builder_.add_build(build);
  builder_.add_version(version);
  builder_.add_data_path(data_path);
  builder_.add_cache_path(cache_path);
  builder_.add_system_path(system_path);
  builder_.add_id(id);
  return builder_.Finish();
}

inline flatbuffers::Offset<MbRom> CreateMbRomDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    const char *id = nullptr,
    const char *system_path = nullptr,
    const char *cache_path = nullptr,
    const char *data_path = nullptr,
    const char *version = nullptr,
    const char *build = nullptr) {
  return mbtool::daemon::v3::CreateMbRom(
      _fbb,
      id ? _fbb.CreateString(id) : 0,
      system_path ? _fbb.CreateString(system_path) : 0,
      cache_path ? _fbb.CreateString(cache_path) : 0,
      data_path ? _fbb.CreateString(data_path) : 0,
      version ? _fbb.CreateString(version) : 0,
      build ? _fbb.CreateString(build) : 0);
}

struct MbGetInstalledRomsRequest FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           verifier.EndTable();
  }
};

struct MbGetInstalledRomsRequestBuilder {
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  explicit MbGetInstalledRomsRequestBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  MbGetInstalledRomsRequestBuilder &operator=(const MbGetInstalledRomsRequestBuilder &);
  flatbuffers::Offset<MbGetInstalledRomsRequest> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<MbGetInstalledRomsRequest>(end);
    return o;
  }
};

inline flatbuffers::Offset<MbGetInstalledRomsRequest> CreateMbGetInstalledRomsRequest(
    flatbuffers::FlatBufferBuilder &_fbb) {
  MbGetInstalledRomsRequestBuilder builder_(_fbb);
  return builder_.Finish();
}

struct MbGetInstalledRomsResponse FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  enum {
    VT_ROMS = 4
  };
  const flatbuffers::Vector<flatbuffers::Offset<MbRom>> *roms() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<MbRom>> *>(VT_ROMS);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyOffset(verifier, VT_ROMS) &&
           verifier.Verify(roms()) &&
           verifier.VerifyVectorOfTables(roms()) &&
           verifier.EndTable();
  }
};

struct MbGetInstalledRomsResponseBuilder {
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_roms(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<MbRom>>> roms) {
    fbb_.AddOffset(MbGetInstalledRomsResponse::VT_ROMS, roms);
  }
  explicit MbGetInstalledRomsResponseBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  MbGetInstalledRomsResponseBuilder &operator=(const MbGetInstalledRomsResponseBuilder &);
  flatbuffers::Offset<MbGetInstalledRomsResponse> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<MbGetInstalledRomsResponse>(end);
    return o;
  }
};

inline flatbuffers::Offset<MbGetInstalledRomsResponse> CreateMbGetInstalledRomsResponse(
    flatbuffers::FlatBufferBuilder &_fbb,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<MbRom>>> roms = 0) {
  MbGetInstalledRomsResponseBuilder builder_(_fbb);
  builder_.add_roms(roms);
  return builder_.Finish();
}

inline flatbuffers::Offset<MbGetInstalledRomsResponse> CreateMbGetInstalledRomsResponseDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    const std::vector<flatbuffers::Offset<MbRom>> *roms = nullptr) {
  return mbtool::daemon::v3::CreateMbGetInstalledRomsResponse(
      _fbb,
      roms ? _fbb.CreateVector<flatbuffers::Offset<MbRom>>(*roms) : 0);
}

}  // namespace v3
}  // namespace daemon
}  // namespace mbtool

#endif  // FLATBUFFERS_GENERATED_MBGETINSTALLEDROMS_MBTOOL_DAEMON_V3_H_
