// automatically generated, do not modify

package mbtool.daemon.v2;

import java.nio.*;
import java.lang.*;
import java.util.*;
import com.google.flatbuffers.*;

public class WipeRomRequest extends Table {
  public static WipeRomRequest getRootAsWipeRomRequest(ByteBuffer _bb) { _bb.order(ByteOrder.LITTLE_ENDIAN); return (new WipeRomRequest()).__init(_bb.getInt(_bb.position()) + _bb.position(), _bb); }
  public WipeRomRequest __init(int _i, ByteBuffer _bb) { bb_pos = _i; bb = _bb; return this; }

  public String romId() { int o = __offset(4); return o != 0 ? __string(o + bb_pos) : null; }
  public ByteBuffer romIdAsByteBuffer() { return __vector_as_bytebuffer(4, 1); }
  public short targets(int j) { int o = __offset(6); return o != 0 ? bb.getShort(__vector(o) + j * 2) : 0; }
  public int targetsLength() { int o = __offset(6); return o != 0 ? __vector_len(o) : 0; }
  public ByteBuffer targetsAsByteBuffer() { return __vector_as_bytebuffer(6, 2); }

  public static int createWipeRomRequest(FlatBufferBuilder builder,
      int rom_id,
      int targets) {
    builder.startObject(2);
    WipeRomRequest.addTargets(builder, targets);
    WipeRomRequest.addRomId(builder, rom_id);
    return WipeRomRequest.endWipeRomRequest(builder);
  }

  public static void startWipeRomRequest(FlatBufferBuilder builder) { builder.startObject(2); }
  public static void addRomId(FlatBufferBuilder builder, int romIdOffset) { builder.addOffset(0, romIdOffset, 0); }
  public static void addTargets(FlatBufferBuilder builder, int targetsOffset) { builder.addOffset(1, targetsOffset, 0); }
  public static int createTargetsVector(FlatBufferBuilder builder, short[] data) { builder.startVector(2, data.length, 2); for (int i = data.length - 1; i >= 0; i--) builder.addShort(data[i]); return builder.endVector(); }
  public static void startTargetsVector(FlatBufferBuilder builder, int numElems) { builder.startVector(2, numElems, 2); }
  public static int endWipeRomRequest(FlatBufferBuilder builder) {
    int o = builder.endObject();
    return o;
  }
};

