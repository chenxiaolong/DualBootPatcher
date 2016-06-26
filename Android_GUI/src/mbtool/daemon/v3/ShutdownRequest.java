// automatically generated, do not modify

package mbtool.daemon.v3;

import java.nio.*;
import java.lang.*;
import java.util.*;
import com.google.flatbuffers.*;

@SuppressWarnings("unused")
public final class ShutdownRequest extends Table {
  public static ShutdownRequest getRootAsShutdownRequest(ByteBuffer _bb) { return getRootAsShutdownRequest(_bb, new ShutdownRequest()); }
  public static ShutdownRequest getRootAsShutdownRequest(ByteBuffer _bb, ShutdownRequest obj) { _bb.order(ByteOrder.LITTLE_ENDIAN); return (obj.__init(_bb.getInt(_bb.position()) + _bb.position(), _bb)); }
  public ShutdownRequest __init(int _i, ByteBuffer _bb) { bb_pos = _i; bb = _bb; return this; }

  public short type() { int o = __offset(4); return o != 0 ? bb.getShort(o + bb_pos) : 0; }

  public static int createShutdownRequest(FlatBufferBuilder builder,
      short type) {
    builder.startObject(1);
    ShutdownRequest.addType(builder, type);
    return ShutdownRequest.endShutdownRequest(builder);
  }

  public static void startShutdownRequest(FlatBufferBuilder builder) { builder.startObject(1); }
  public static void addType(FlatBufferBuilder builder, short type) { builder.addShort(0, type, 0); }
  public static int endShutdownRequest(FlatBufferBuilder builder) {
    int o = builder.endObject();
    return o;
  }
};

