// automatically generated, do not modify

package mbtool.daemon.v2;

import java.nio.*;
import java.lang.*;
import java.util.*;
import com.google.flatbuffers.*;

public class Request extends Table {
  public static Request getRootAsRequest(ByteBuffer _bb) { _bb.order(ByteOrder.LITTLE_ENDIAN); return (new Request()).__init(_bb.getInt(_bb.position()) + _bb.position(), _bb); }
  public Request __init(int _i, ByteBuffer _bb) { bb_pos = _i; bb = _bb; return this; }

  public short type() { int o = __offset(4); return o != 0 ? bb.getShort(o + bb_pos) : 0; }
  public GetVersionRequest getVersionRequest() { return getVersionRequest(new GetVersionRequest()); }
  public GetVersionRequest getVersionRequest(GetVersionRequest obj) { int o = __offset(6); return o != 0 ? obj.__init(__indirect(o + bb_pos), bb) : null; }
  public GetRomsListRequest getRomsListRequest() { return getRomsListRequest(new GetRomsListRequest()); }
  public GetRomsListRequest getRomsListRequest(GetRomsListRequest obj) { int o = __offset(8); return o != 0 ? obj.__init(__indirect(o + bb_pos), bb) : null; }
  public GetBuiltinRomIdsRequest getBuiltinRomIdsRequest() { return getBuiltinRomIdsRequest(new GetBuiltinRomIdsRequest()); }
  public GetBuiltinRomIdsRequest getBuiltinRomIdsRequest(GetBuiltinRomIdsRequest obj) { int o = __offset(10); return o != 0 ? obj.__init(__indirect(o + bb_pos), bb) : null; }
  public GetCurrentRomRequest getCurrentRomRequest() { return getCurrentRomRequest(new GetCurrentRomRequest()); }
  public GetCurrentRomRequest getCurrentRomRequest(GetCurrentRomRequest obj) { int o = __offset(12); return o != 0 ? obj.__init(__indirect(o + bb_pos), bb) : null; }
  public SwitchRomRequest switchRomRequest() { return switchRomRequest(new SwitchRomRequest()); }
  public SwitchRomRequest switchRomRequest(SwitchRomRequest obj) { int o = __offset(14); return o != 0 ? obj.__init(__indirect(o + bb_pos), bb) : null; }
  public SetKernelRequest setKernelRequest() { return setKernelRequest(new SetKernelRequest()); }
  public SetKernelRequest setKernelRequest(SetKernelRequest obj) { int o = __offset(16); return o != 0 ? obj.__init(__indirect(o + bb_pos), bb) : null; }
  public RebootRequest rebootRequest() { return rebootRequest(new RebootRequest()); }
  public RebootRequest rebootRequest(RebootRequest obj) { int o = __offset(18); return o != 0 ? obj.__init(__indirect(o + bb_pos), bb) : null; }
  public OpenRequest openRequest() { return openRequest(new OpenRequest()); }
  public OpenRequest openRequest(OpenRequest obj) { int o = __offset(20); return o != 0 ? obj.__init(__indirect(o + bb_pos), bb) : null; }
  public CopyRequest copyRequest() { return copyRequest(new CopyRequest()); }
  public CopyRequest copyRequest(CopyRequest obj) { int o = __offset(22); return o != 0 ? obj.__init(__indirect(o + bb_pos), bb) : null; }
  public ChmodRequest chmodRequest() { return chmodRequest(new ChmodRequest()); }
  public ChmodRequest chmodRequest(ChmodRequest obj) { int o = __offset(24); return o != 0 ? obj.__init(__indirect(o + bb_pos), bb) : null; }
  public LokiPatchRequest lokiPatchRequest() { return lokiPatchRequest(new LokiPatchRequest()); }
  public LokiPatchRequest lokiPatchRequest(LokiPatchRequest obj) { int o = __offset(26); return o != 0 ? obj.__init(__indirect(o + bb_pos), bb) : null; }
  public WipeRomRequest wipeRomRequest() { return wipeRomRequest(new WipeRomRequest()); }
  public WipeRomRequest wipeRomRequest(WipeRomRequest obj) { int o = __offset(28); return o != 0 ? obj.__init(__indirect(o + bb_pos), bb) : null; }

  public static int createRequest(FlatBufferBuilder builder,
      short type,
      int get_version_request,
      int get_roms_list_request,
      int get_builtin_rom_ids_request,
      int get_current_rom_request,
      int switch_rom_request,
      int set_kernel_request,
      int reboot_request,
      int open_request,
      int copy_request,
      int chmod_request,
      int loki_patch_request,
      int wipe_rom_request) {
    builder.startObject(13);
    Request.addWipeRomRequest(builder, wipe_rom_request);
    Request.addLokiPatchRequest(builder, loki_patch_request);
    Request.addChmodRequest(builder, chmod_request);
    Request.addCopyRequest(builder, copy_request);
    Request.addOpenRequest(builder, open_request);
    Request.addRebootRequest(builder, reboot_request);
    Request.addSetKernelRequest(builder, set_kernel_request);
    Request.addSwitchRomRequest(builder, switch_rom_request);
    Request.addGetCurrentRomRequest(builder, get_current_rom_request);
    Request.addGetBuiltinRomIdsRequest(builder, get_builtin_rom_ids_request);
    Request.addGetRomsListRequest(builder, get_roms_list_request);
    Request.addGetVersionRequest(builder, get_version_request);
    Request.addType(builder, type);
    return Request.endRequest(builder);
  }

  public static void startRequest(FlatBufferBuilder builder) { builder.startObject(13); }
  public static void addType(FlatBufferBuilder builder, short type) { builder.addShort(0, type, 0); }
  public static void addGetVersionRequest(FlatBufferBuilder builder, int getVersionRequestOffset) { builder.addOffset(1, getVersionRequestOffset, 0); }
  public static void addGetRomsListRequest(FlatBufferBuilder builder, int getRomsListRequestOffset) { builder.addOffset(2, getRomsListRequestOffset, 0); }
  public static void addGetBuiltinRomIdsRequest(FlatBufferBuilder builder, int getBuiltinRomIdsRequestOffset) { builder.addOffset(3, getBuiltinRomIdsRequestOffset, 0); }
  public static void addGetCurrentRomRequest(FlatBufferBuilder builder, int getCurrentRomRequestOffset) { builder.addOffset(4, getCurrentRomRequestOffset, 0); }
  public static void addSwitchRomRequest(FlatBufferBuilder builder, int switchRomRequestOffset) { builder.addOffset(5, switchRomRequestOffset, 0); }
  public static void addSetKernelRequest(FlatBufferBuilder builder, int setKernelRequestOffset) { builder.addOffset(6, setKernelRequestOffset, 0); }
  public static void addRebootRequest(FlatBufferBuilder builder, int rebootRequestOffset) { builder.addOffset(7, rebootRequestOffset, 0); }
  public static void addOpenRequest(FlatBufferBuilder builder, int openRequestOffset) { builder.addOffset(8, openRequestOffset, 0); }
  public static void addCopyRequest(FlatBufferBuilder builder, int copyRequestOffset) { builder.addOffset(9, copyRequestOffset, 0); }
  public static void addChmodRequest(FlatBufferBuilder builder, int chmodRequestOffset) { builder.addOffset(10, chmodRequestOffset, 0); }
  public static void addLokiPatchRequest(FlatBufferBuilder builder, int lokiPatchRequestOffset) { builder.addOffset(11, lokiPatchRequestOffset, 0); }
  public static void addWipeRomRequest(FlatBufferBuilder builder, int wipeRomRequestOffset) { builder.addOffset(12, wipeRomRequestOffset, 0); }
  public static int endRequest(FlatBufferBuilder builder) {
    int o = builder.endObject();
    return o;
  }
  public static void finishRequestBuffer(FlatBufferBuilder builder, int offset) { builder.finish(offset); }
};

