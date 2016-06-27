// automatically generated, do not modify

package mbtool.daemon.v3;

import java.nio.*;
import java.lang.*;
import java.util.*;
import com.google.flatbuffers.*;

@SuppressWarnings("unused")
public final class PathDeleteRequest extends Table {
  public static PathDeleteRequest getRootAsPathDeleteRequest(ByteBuffer _bb) { return getRootAsPathDeleteRequest(_bb, new PathDeleteRequest()); }
  public static PathDeleteRequest getRootAsPathDeleteRequest(ByteBuffer _bb, PathDeleteRequest obj) { _bb.order(ByteOrder.LITTLE_ENDIAN); return (obj.__init(_bb.getInt(_bb.position()) + _bb.position(), _bb)); }
  public PathDeleteRequest __init(int _i, ByteBuffer _bb) { bb_pos = _i; bb = _bb; return this; }

  public String path() { int o = __offset(4); return o != 0 ? __string(o + bb_pos) : null; }
  public ByteBuffer pathAsByteBuffer() { return __vector_as_bytebuffer(4, 1); }
  public short flag() { int o = __offset(6); return o != 0 ? bb.getShort(o + bb_pos) : 0; }

  public static int createPathDeleteRequest(FlatBufferBuilder builder,
      int pathOffset,
      short flag) {
    builder.startObject(2);
    PathDeleteRequest.addPath(builder, pathOffset);
    PathDeleteRequest.addFlag(builder, flag);
    return PathDeleteRequest.endPathDeleteRequest(builder);
  }

  public static void startPathDeleteRequest(FlatBufferBuilder builder) { builder.startObject(2); }
  public static void addPath(FlatBufferBuilder builder, int pathOffset) { builder.addOffset(0, pathOffset, 0); }
  public static void addFlag(FlatBufferBuilder builder, short flag) { builder.addShort(1, flag, 0); }
  public static int endPathDeleteRequest(FlatBufferBuilder builder) {
    int o = builder.endObject();
    return o;
  }
};

