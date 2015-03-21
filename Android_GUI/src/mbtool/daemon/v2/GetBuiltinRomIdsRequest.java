// automatically generated, do not modify

package mbtool.daemon.v2;

import java.nio.*;
import java.lang.*;
import java.util.*;
import com.google.flatbuffers.*;

public class GetBuiltinRomIdsRequest extends Table {
  public static GetBuiltinRomIdsRequest getRootAsGetBuiltinRomIdsRequest(ByteBuffer _bb) { _bb.order(ByteOrder.LITTLE_ENDIAN); return (new GetBuiltinRomIdsRequest()).__init(_bb.getInt(_bb.position()) + _bb.position(), _bb); }
  public GetBuiltinRomIdsRequest __init(int _i, ByteBuffer _bb) { bb_pos = _i; bb = _bb; return this; }


  public static void startGetBuiltinRomIdsRequest(FlatBufferBuilder builder) { builder.startObject(0); }
  public static int endGetBuiltinRomIdsRequest(FlatBufferBuilder builder) {
    int o = builder.endObject();
    return o;
  }
};

