// automatically generated, do not modify

package mbtool.daemon.v3;

import java.nio.*;
import java.lang.*;
import java.util.*;
import com.google.flatbuffers.*;

@SuppressWarnings("unused")
public final class PathMkdirRequest extends Table {
  public static PathMkdirRequest getRootAsPathMkdirRequest(ByteBuffer _bb) { return getRootAsPathMkdirRequest(_bb, new PathMkdirRequest()); }
  public static PathMkdirRequest getRootAsPathMkdirRequest(ByteBuffer _bb, PathMkdirRequest obj) { _bb.order(ByteOrder.LITTLE_ENDIAN); return (obj.__init(_bb.getInt(_bb.position()) + _bb.position(), _bb)); }
  public PathMkdirRequest __init(int _i, ByteBuffer _bb) { bb_pos = _i; bb = _bb; return this; }

  public String path() { int o = __offset(4); return o != 0 ? __string(o + bb_pos) : null; }
  public ByteBuffer pathAsByteBuffer() { return __vector_as_bytebuffer(4, 1); }
  public long mode() { int o = __offset(6); return o != 0 ? (long)bb.getInt(o + bb_pos) & 0xFFFFFFFFL : 0; }
  public boolean recursive() { int o = __offset(8); return o != 0 ? 0!=bb.get(o + bb_pos) : false; }

  public static int createPathMkdirRequest(FlatBufferBuilder builder,
      int pathOffset,
      long mode,
      boolean recursive) {
    builder.startObject(3);
    PathMkdirRequest.addMode(builder, mode);
    PathMkdirRequest.addPath(builder, pathOffset);
    PathMkdirRequest.addRecursive(builder, recursive);
    return PathMkdirRequest.endPathMkdirRequest(builder);
  }

  public static void startPathMkdirRequest(FlatBufferBuilder builder) { builder.startObject(3); }
  public static void addPath(FlatBufferBuilder builder, int pathOffset) { builder.addOffset(0, pathOffset, 0); }
  public static void addMode(FlatBufferBuilder builder, long mode) { builder.addInt(1, (int)mode, 0); }
  public static void addRecursive(FlatBufferBuilder builder, boolean recursive) { builder.addBoolean(2, recursive, false); }
  public static int endPathMkdirRequest(FlatBufferBuilder builder) {
    int o = builder.endObject();
    return o;
  }
};

