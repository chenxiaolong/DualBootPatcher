// automatically generated, do not modify

package mbtool.daemon.v2;

import java.nio.*;
import java.lang.*;
import java.util.*;
import com.google.flatbuffers.*;

public class Response extends Table {
  public static Response getRootAsResponse(ByteBuffer _bb) { return getRootAsResponse(_bb, new Response()); }
  public static Response getRootAsResponse(ByteBuffer _bb, Response obj) { _bb.order(ByteOrder.LITTLE_ENDIAN); return (obj.__init(_bb.getInt(_bb.position()) + _bb.position(), _bb)); }
  public Response __init(int _i, ByteBuffer _bb) { bb_pos = _i; bb = _bb; return this; }

  public short type() { int o = __offset(4); return o != 0 ? bb.getShort(o + bb_pos) : 0; }
  public GetVersionResponse getVersionResponse() { return getVersionResponse(new GetVersionResponse()); }
  public GetVersionResponse getVersionResponse(GetVersionResponse obj) { int o = __offset(6); return o != 0 ? obj.__init(__indirect(o + bb_pos), bb) : null; }
  public GetRomsListResponse getRomsListResponse() { return getRomsListResponse(new GetRomsListResponse()); }
  public GetRomsListResponse getRomsListResponse(GetRomsListResponse obj) { int o = __offset(8); return o != 0 ? obj.__init(__indirect(o + bb_pos), bb) : null; }
  public GetBuiltinRomIdsResponse getBuiltinRomIdsResponse() { return getBuiltinRomIdsResponse(new GetBuiltinRomIdsResponse()); }
  public GetBuiltinRomIdsResponse getBuiltinRomIdsResponse(GetBuiltinRomIdsResponse obj) { int o = __offset(10); return o != 0 ? obj.__init(__indirect(o + bb_pos), bb) : null; }
  public GetCurrentRomResponse getCurrentRomResponse() { return getCurrentRomResponse(new GetCurrentRomResponse()); }
  public GetCurrentRomResponse getCurrentRomResponse(GetCurrentRomResponse obj) { int o = __offset(12); return o != 0 ? obj.__init(__indirect(o + bb_pos), bb) : null; }
  public SwitchRomResponse switchRomResponse() { return switchRomResponse(new SwitchRomResponse()); }
  public SwitchRomResponse switchRomResponse(SwitchRomResponse obj) { int o = __offset(14); return o != 0 ? obj.__init(__indirect(o + bb_pos), bb) : null; }
  public SetKernelResponse setKernelResponse() { return setKernelResponse(new SetKernelResponse()); }
  public SetKernelResponse setKernelResponse(SetKernelResponse obj) { int o = __offset(16); return o != 0 ? obj.__init(__indirect(o + bb_pos), bb) : null; }
  public RebootResponse rebootResponse() { return rebootResponse(new RebootResponse()); }
  public RebootResponse rebootResponse(RebootResponse obj) { int o = __offset(18); return o != 0 ? obj.__init(__indirect(o + bb_pos), bb) : null; }
  public OpenResponse openResponse() { return openResponse(new OpenResponse()); }
  public OpenResponse openResponse(OpenResponse obj) { int o = __offset(20); return o != 0 ? obj.__init(__indirect(o + bb_pos), bb) : null; }
  public CopyResponse copyResponse() { return copyResponse(new CopyResponse()); }
  public CopyResponse copyResponse(CopyResponse obj) { int o = __offset(22); return o != 0 ? obj.__init(__indirect(o + bb_pos), bb) : null; }
  public ChmodResponse chmodResponse() { return chmodResponse(new ChmodResponse()); }
  public ChmodResponse chmodResponse(ChmodResponse obj) { int o = __offset(24); return o != 0 ? obj.__init(__indirect(o + bb_pos), bb) : null; }
  public WipeRomResponse wipeRomResponse() { return wipeRomResponse(new WipeRomResponse()); }
  public WipeRomResponse wipeRomResponse(WipeRomResponse obj) { int o = __offset(28); return o != 0 ? obj.__init(__indirect(o + bb_pos), bb) : null; }

  public static int createResponse(FlatBufferBuilder builder,
      short type,
      int get_version_response,
      int get_roms_list_response,
      int get_builtin_rom_ids_response,
      int get_current_rom_response,
      int switch_rom_response,
      int set_kernel_response,
      int reboot_response,
      int open_response,
      int copy_response,
      int chmod_response,
      int wipe_rom_response) {
    builder.startObject(13);
    Response.addWipeRomResponse(builder, wipe_rom_response);
    Response.addChmodResponse(builder, chmod_response);
    Response.addCopyResponse(builder, copy_response);
    Response.addOpenResponse(builder, open_response);
    Response.addRebootResponse(builder, reboot_response);
    Response.addSetKernelResponse(builder, set_kernel_response);
    Response.addSwitchRomResponse(builder, switch_rom_response);
    Response.addGetCurrentRomResponse(builder, get_current_rom_response);
    Response.addGetBuiltinRomIdsResponse(builder, get_builtin_rom_ids_response);
    Response.addGetRomsListResponse(builder, get_roms_list_response);
    Response.addGetVersionResponse(builder, get_version_response);
    Response.addType(builder, type);
    return Response.endResponse(builder);
  }

  public static void startResponse(FlatBufferBuilder builder) { builder.startObject(13); }
  public static void addType(FlatBufferBuilder builder, short type) { builder.addShort(0, type, 0); }
  public static void addGetVersionResponse(FlatBufferBuilder builder, int getVersionResponseOffset) { builder.addOffset(1, getVersionResponseOffset, 0); }
  public static void addGetRomsListResponse(FlatBufferBuilder builder, int getRomsListResponseOffset) { builder.addOffset(2, getRomsListResponseOffset, 0); }
  public static void addGetBuiltinRomIdsResponse(FlatBufferBuilder builder, int getBuiltinRomIdsResponseOffset) { builder.addOffset(3, getBuiltinRomIdsResponseOffset, 0); }
  public static void addGetCurrentRomResponse(FlatBufferBuilder builder, int getCurrentRomResponseOffset) { builder.addOffset(4, getCurrentRomResponseOffset, 0); }
  public static void addSwitchRomResponse(FlatBufferBuilder builder, int switchRomResponseOffset) { builder.addOffset(5, switchRomResponseOffset, 0); }
  public static void addSetKernelResponse(FlatBufferBuilder builder, int setKernelResponseOffset) { builder.addOffset(6, setKernelResponseOffset, 0); }
  public static void addRebootResponse(FlatBufferBuilder builder, int rebootResponseOffset) { builder.addOffset(7, rebootResponseOffset, 0); }
  public static void addOpenResponse(FlatBufferBuilder builder, int openResponseOffset) { builder.addOffset(8, openResponseOffset, 0); }
  public static void addCopyResponse(FlatBufferBuilder builder, int copyResponseOffset) { builder.addOffset(9, copyResponseOffset, 0); }
  public static void addChmodResponse(FlatBufferBuilder builder, int chmodResponseOffset) { builder.addOffset(10, chmodResponseOffset, 0); }
  public static void addWipeRomResponse(FlatBufferBuilder builder, int wipeRomResponseOffset) { builder.addOffset(12, wipeRomResponseOffset, 0); }
  public static int endResponse(FlatBufferBuilder builder) {
    int o = builder.endObject();
    return o;
  }
  public static void finishResponseBuffer(FlatBufferBuilder builder, int offset) { builder.finish(offset); }
};

