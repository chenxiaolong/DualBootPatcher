// automatically generated, do not modify

package mbtool.daemon.v2;

import java.nio.*;
import java.lang.*;
import java.util.*;
import com.google.flatbuffers.*;

public class ChmodRequest extends Table {
  public static ChmodRequest getRootAsChmodRequest(ByteBuffer _bb) { _bb.order(ByteOrder.LITTLE_ENDIAN); return (new ChmodRequest()).__init(_bb.getInt(_bb.position()) + _bb.position(), _bb); }
  public ChmodRequest __init(int _i, ByteBuffer _bb) { bb_pos = _i; bb = _bb; return this; }

  public String path() { int o = __offset(4); return o != 0 ? __string(o + bb_pos) : null; }
  public ByteBuffer pathAsByteBuffer() { return __vector_as_bytebuffer(4, 1); }
  public int mode() { int o = __offset(6); return o != 0 ? bb.getInt(o + bb_pos) : 0; }

  public static int createChmodRequest(FlatBufferBuilder builder,
      int path,
      int mode) {
    builder.startObject(2);
    ChmodRequest.addMode(builder, mode);
    ChmodRequest.addPath(builder, path);
    return ChmodRequest.endChmodRequest(builder);
  }

  public static void startChmodRequest(FlatBufferBuilder builder) { builder.startObject(2); }
  public static void addPath(FlatBufferBuilder builder, int pathOffset) { builder.addOffset(0, pathOffset, 0); }
  public static void addMode(FlatBufferBuilder builder, int mode) { builder.addInt(1, mode, 0); }
  public static int endChmodRequest(FlatBufferBuilder builder) {
    int o = builder.endObject();
    return o;
  }
};

