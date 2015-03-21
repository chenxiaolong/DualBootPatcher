// automatically generated, do not modify

package mbtool.daemon.v2;

import java.nio.*;
import java.lang.*;
import java.util.*;
import com.google.flatbuffers.*;

public class GetCurrentRomResponse extends Table {
  public static GetCurrentRomResponse getRootAsGetCurrentRomResponse(ByteBuffer _bb) { _bb.order(ByteOrder.LITTLE_ENDIAN); return (new GetCurrentRomResponse()).__init(_bb.getInt(_bb.position()) + _bb.position(), _bb); }
  public GetCurrentRomResponse __init(int _i, ByteBuffer _bb) { bb_pos = _i; bb = _bb; return this; }

  public String romId() { int o = __offset(4); return o != 0 ? __string(o + bb_pos) : null; }
  public ByteBuffer romIdAsByteBuffer() { return __vector_as_bytebuffer(4, 1); }

  public static int createGetCurrentRomResponse(FlatBufferBuilder builder,
      int rom_id) {
    builder.startObject(1);
    GetCurrentRomResponse.addRomId(builder, rom_id);
    return GetCurrentRomResponse.endGetCurrentRomResponse(builder);
  }

  public static void startGetCurrentRomResponse(FlatBufferBuilder builder) { builder.startObject(1); }
  public static void addRomId(FlatBufferBuilder builder, int romIdOffset) { builder.addOffset(0, romIdOffset, 0); }
  public static int endGetCurrentRomResponse(FlatBufferBuilder builder) {
    int o = builder.endObject();
    return o;
  }
};

