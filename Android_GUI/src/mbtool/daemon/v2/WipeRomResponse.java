// automatically generated, do not modify

package mbtool.daemon.v2;

import java.nio.*;
import java.lang.*;
import java.util.*;
import com.google.flatbuffers.*;

public class WipeRomResponse extends Table {
  public static WipeRomResponse getRootAsWipeRomResponse(ByteBuffer _bb) { return getRootAsWipeRomResponse(_bb, new WipeRomResponse()); }
  public static WipeRomResponse getRootAsWipeRomResponse(ByteBuffer _bb, WipeRomResponse obj) { _bb.order(ByteOrder.LITTLE_ENDIAN); return (obj.__init(_bb.getInt(_bb.position()) + _bb.position(), _bb)); }
  public WipeRomResponse __init(int _i, ByteBuffer _bb) { bb_pos = _i; bb = _bb; return this; }

  public short succeeded(int j) { int o = __offset(4); return o != 0 ? bb.getShort(__vector(o) + j * 2) : 0; }
  public int succeededLength() { int o = __offset(4); return o != 0 ? __vector_len(o) : 0; }
  public ByteBuffer succeededAsByteBuffer() { return __vector_as_bytebuffer(4, 2); }
  public short failed(int j) { int o = __offset(6); return o != 0 ? bb.getShort(__vector(o) + j * 2) : 0; }
  public int failedLength() { int o = __offset(6); return o != 0 ? __vector_len(o) : 0; }
  public ByteBuffer failedAsByteBuffer() { return __vector_as_bytebuffer(6, 2); }

  public static int createWipeRomResponse(FlatBufferBuilder builder,
      int succeeded,
      int failed) {
    builder.startObject(2);
    WipeRomResponse.addFailed(builder, failed);
    WipeRomResponse.addSucceeded(builder, succeeded);
    return WipeRomResponse.endWipeRomResponse(builder);
  }

  public static void startWipeRomResponse(FlatBufferBuilder builder) { builder.startObject(2); }
  public static void addSucceeded(FlatBufferBuilder builder, int succeededOffset) { builder.addOffset(0, succeededOffset, 0); }
  public static int createSucceededVector(FlatBufferBuilder builder, short[] data) { builder.startVector(2, data.length, 2); for (int i = data.length - 1; i >= 0; i--) builder.addShort(data[i]); return builder.endVector(); }
  public static void startSucceededVector(FlatBufferBuilder builder, int numElems) { builder.startVector(2, numElems, 2); }
  public static void addFailed(FlatBufferBuilder builder, int failedOffset) { builder.addOffset(1, failedOffset, 0); }
  public static int createFailedVector(FlatBufferBuilder builder, short[] data) { builder.startVector(2, data.length, 2); for (int i = data.length - 1; i >= 0; i--) builder.addShort(data[i]); return builder.endVector(); }
  public static void startFailedVector(FlatBufferBuilder builder, int numElems) { builder.startVector(2, numElems, 2); }
  public static int endWipeRomResponse(FlatBufferBuilder builder) {
    int o = builder.endObject();
    return o;
  }
};

