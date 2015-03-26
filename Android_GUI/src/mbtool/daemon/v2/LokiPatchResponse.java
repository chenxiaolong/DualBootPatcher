// automatically generated, do not modify

package mbtool.daemon.v2;

import java.nio.*;
import java.lang.*;
import java.util.*;
import com.google.flatbuffers.*;

public class LokiPatchResponse extends Table {
  public static LokiPatchResponse getRootAsLokiPatchResponse(ByteBuffer _bb) { _bb.order(ByteOrder.LITTLE_ENDIAN); return (new LokiPatchResponse()).__init(_bb.getInt(_bb.position()) + _bb.position(), _bb); }
  public LokiPatchResponse __init(int _i, ByteBuffer _bb) { bb_pos = _i; bb = _bb; return this; }


  public static void startLokiPatchResponse(FlatBufferBuilder builder) { builder.startObject(1); }
  public static int endLokiPatchResponse(FlatBufferBuilder builder) {
    int o = builder.endObject();
    return o;
  }
};

