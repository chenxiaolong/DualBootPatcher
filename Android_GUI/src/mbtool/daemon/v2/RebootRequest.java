// automatically generated, do not modify

package mbtool.daemon.v2;

import java.nio.*;
import java.lang.*;
import java.util.*;
import com.google.flatbuffers.*;

public class RebootRequest extends Table {
  public static RebootRequest getRootAsRebootRequest(ByteBuffer _bb) { _bb.order(ByteOrder.LITTLE_ENDIAN); return (new RebootRequest()).__init(_bb.getInt(_bb.position()) + _bb.position(), _bb); }
  public RebootRequest __init(int _i, ByteBuffer _bb) { bb_pos = _i; bb = _bb; return this; }

  public String arg() { int o = __offset(4); return o != 0 ? __string(o + bb_pos) : null; }
  public ByteBuffer argAsByteBuffer() { return __vector_as_bytebuffer(4, 1); }

  public static int createRebootRequest(FlatBufferBuilder builder,
      int arg) {
    builder.startObject(1);
    RebootRequest.addArg(builder, arg);
    return RebootRequest.endRebootRequest(builder);
  }

  public static void startRebootRequest(FlatBufferBuilder builder) { builder.startObject(1); }
  public static void addArg(FlatBufferBuilder builder, int argOffset) { builder.addOffset(0, argOffset, 0); }
  public static int endRebootRequest(FlatBufferBuilder builder) {
    int o = builder.endObject();
    return o;
  }
};

