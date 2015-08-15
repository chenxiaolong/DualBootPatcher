// automatically generated, do not modify

package mbtool.daemon.v2;

import java.nio.*;
import java.lang.*;
import java.util.*;
import com.google.flatbuffers.*;

public class SetKernelResponse extends Table {
  public static SetKernelResponse getRootAsSetKernelResponse(ByteBuffer _bb) { return getRootAsSetKernelResponse(_bb, new SetKernelResponse()); }
  public static SetKernelResponse getRootAsSetKernelResponse(ByteBuffer _bb, SetKernelResponse obj) { _bb.order(ByteOrder.LITTLE_ENDIAN); return (obj.__init(_bb.getInt(_bb.position()) + _bb.position(), _bb)); }
  public SetKernelResponse __init(int _i, ByteBuffer _bb) { bb_pos = _i; bb = _bb; return this; }

  public boolean success() { int o = __offset(4); return o != 0 ? 0!=bb.get(o + bb_pos) : false; }

  public static int createSetKernelResponse(FlatBufferBuilder builder,
      boolean success) {
    builder.startObject(1);
    SetKernelResponse.addSuccess(builder, success);
    return SetKernelResponse.endSetKernelResponse(builder);
  }

  public static void startSetKernelResponse(FlatBufferBuilder builder) { builder.startObject(1); }
  public static void addSuccess(FlatBufferBuilder builder, boolean success) { builder.addBoolean(0, success, false); }
  public static int endSetKernelResponse(FlatBufferBuilder builder) {
    int o = builder.endObject();
    return o;
  }
};

