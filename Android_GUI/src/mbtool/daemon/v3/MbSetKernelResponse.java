// automatically generated, do not modify

package mbtool.daemon.v3;

import java.nio.*;
import java.lang.*;
import java.util.*;
import com.google.flatbuffers.*;

@SuppressWarnings("unused")
public final class MbSetKernelResponse extends Table {
  public static MbSetKernelResponse getRootAsMbSetKernelResponse(ByteBuffer _bb) { return getRootAsMbSetKernelResponse(_bb, new MbSetKernelResponse()); }
  public static MbSetKernelResponse getRootAsMbSetKernelResponse(ByteBuffer _bb, MbSetKernelResponse obj) { _bb.order(ByteOrder.LITTLE_ENDIAN); return (obj.__init(_bb.getInt(_bb.position()) + _bb.position(), _bb)); }
  public MbSetKernelResponse __init(int _i, ByteBuffer _bb) { bb_pos = _i; bb = _bb; return this; }

  public boolean success() { int o = __offset(4); return o != 0 ? 0!=bb.get(o + bb_pos) : false; }

  public static int createMbSetKernelResponse(FlatBufferBuilder builder,
      boolean success) {
    builder.startObject(1);
    MbSetKernelResponse.addSuccess(builder, success);
    return MbSetKernelResponse.endMbSetKernelResponse(builder);
  }

  public static void startMbSetKernelResponse(FlatBufferBuilder builder) { builder.startObject(1); }
  public static void addSuccess(FlatBufferBuilder builder, boolean success) { builder.addBoolean(0, success, false); }
  public static int endMbSetKernelResponse(FlatBufferBuilder builder) {
    int o = builder.endObject();
    return o;
  }
};

