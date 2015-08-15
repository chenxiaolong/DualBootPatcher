// automatically generated, do not modify

package mbtool.daemon.v2;

import java.nio.*;
import java.lang.*;
import java.util.*;
import com.google.flatbuffers.*;

public class GetBuiltinRomIdsResponse extends Table {
  public static GetBuiltinRomIdsResponse getRootAsGetBuiltinRomIdsResponse(ByteBuffer _bb) { return getRootAsGetBuiltinRomIdsResponse(_bb, new GetBuiltinRomIdsResponse()); }
  public static GetBuiltinRomIdsResponse getRootAsGetBuiltinRomIdsResponse(ByteBuffer _bb, GetBuiltinRomIdsResponse obj) { _bb.order(ByteOrder.LITTLE_ENDIAN); return (obj.__init(_bb.getInt(_bb.position()) + _bb.position(), _bb)); }
  public GetBuiltinRomIdsResponse __init(int _i, ByteBuffer _bb) { bb_pos = _i; bb = _bb; return this; }

  public String romIds(int j) { int o = __offset(4); return o != 0 ? __string(__vector(o) + j * 4) : null; }
  public int romIdsLength() { int o = __offset(4); return o != 0 ? __vector_len(o) : 0; }

  public static int createGetBuiltinRomIdsResponse(FlatBufferBuilder builder,
      int rom_ids) {
    builder.startObject(1);
    GetBuiltinRomIdsResponse.addRomIds(builder, rom_ids);
    return GetBuiltinRomIdsResponse.endGetBuiltinRomIdsResponse(builder);
  }

  public static void startGetBuiltinRomIdsResponse(FlatBufferBuilder builder) { builder.startObject(1); }
  public static void addRomIds(FlatBufferBuilder builder, int romIdsOffset) { builder.addOffset(0, romIdsOffset, 0); }
  public static int createRomIdsVector(FlatBufferBuilder builder, int[] data) { builder.startVector(4, data.length, 4); for (int i = data.length - 1; i >= 0; i--) builder.addOffset(data[i]); return builder.endVector(); }
  public static void startRomIdsVector(FlatBufferBuilder builder, int numElems) { builder.startVector(4, numElems, 4); }
  public static int endGetBuiltinRomIdsResponse(FlatBufferBuilder builder) {
    int o = builder.endObject();
    return o;
  }
};

