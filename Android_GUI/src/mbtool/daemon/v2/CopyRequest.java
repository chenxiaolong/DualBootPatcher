// automatically generated, do not modify

package mbtool.daemon.v2;

import java.nio.*;
import java.lang.*;
import java.util.*;
import com.google.flatbuffers.*;

public class CopyRequest extends Table {
  public static CopyRequest getRootAsCopyRequest(ByteBuffer _bb) { _bb.order(ByteOrder.LITTLE_ENDIAN); return (new CopyRequest()).__init(_bb.getInt(_bb.position()) + _bb.position(), _bb); }
  public CopyRequest __init(int _i, ByteBuffer _bb) { bb_pos = _i; bb = _bb; return this; }

  public String source() { int o = __offset(4); return o != 0 ? __string(o + bb_pos) : null; }
  public ByteBuffer sourceAsByteBuffer() { return __vector_as_bytebuffer(4, 1); }
  public String target() { int o = __offset(6); return o != 0 ? __string(o + bb_pos) : null; }
  public ByteBuffer targetAsByteBuffer() { return __vector_as_bytebuffer(6, 1); }

  public static int createCopyRequest(FlatBufferBuilder builder,
      int source,
      int target) {
    builder.startObject(2);
    CopyRequest.addTarget(builder, target);
    CopyRequest.addSource(builder, source);
    return CopyRequest.endCopyRequest(builder);
  }

  public static void startCopyRequest(FlatBufferBuilder builder) { builder.startObject(2); }
  public static void addSource(FlatBufferBuilder builder, int sourceOffset) { builder.addOffset(0, sourceOffset, 0); }
  public static void addTarget(FlatBufferBuilder builder, int targetOffset) { builder.addOffset(1, targetOffset, 0); }
  public static int endCopyRequest(FlatBufferBuilder builder) {
    int o = builder.endObject();
    return o;
  }
};

