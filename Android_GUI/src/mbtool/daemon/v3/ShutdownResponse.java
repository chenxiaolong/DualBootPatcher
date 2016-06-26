// automatically generated, do not modify

package mbtool.daemon.v3;

import java.nio.*;
import java.lang.*;
import java.util.*;
import com.google.flatbuffers.*;

@SuppressWarnings("unused")
public final class ShutdownResponse extends Table {
  public static ShutdownResponse getRootAsShutdownResponse(ByteBuffer _bb) { return getRootAsShutdownResponse(_bb, new ShutdownResponse()); }
  public static ShutdownResponse getRootAsShutdownResponse(ByteBuffer _bb, ShutdownResponse obj) { _bb.order(ByteOrder.LITTLE_ENDIAN); return (obj.__init(_bb.getInt(_bb.position()) + _bb.position(), _bb)); }
  public ShutdownResponse __init(int _i, ByteBuffer _bb) { bb_pos = _i; bb = _bb; return this; }

  public boolean success() { int o = __offset(4); return o != 0 ? 0!=bb.get(o + bb_pos) : false; }

  public static int createShutdownResponse(FlatBufferBuilder builder,
      boolean success) {
    builder.startObject(1);
    ShutdownResponse.addSuccess(builder, success);
    return ShutdownResponse.endShutdownResponse(builder);
  }

  public static void startShutdownResponse(FlatBufferBuilder builder) { builder.startObject(1); }
  public static void addSuccess(FlatBufferBuilder builder, boolean success) { builder.addBoolean(0, success, false); }
  public static int endShutdownResponse(FlatBufferBuilder builder) {
    int o = builder.endObject();
    return o;
  }
};

