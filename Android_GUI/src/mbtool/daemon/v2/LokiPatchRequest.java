// automatically generated, do not modify

package mbtool.daemon.v2;

import java.nio.*;
import java.lang.*;
import java.util.*;
import com.google.flatbuffers.*;

public class LokiPatchRequest extends Table {
  public static LokiPatchRequest getRootAsLokiPatchRequest(ByteBuffer _bb) { return getRootAsLokiPatchRequest(_bb, new LokiPatchRequest()); }
  public static LokiPatchRequest getRootAsLokiPatchRequest(ByteBuffer _bb, LokiPatchRequest obj) { _bb.order(ByteOrder.LITTLE_ENDIAN); return (obj.__init(_bb.getInt(_bb.position()) + _bb.position(), _bb)); }
  public LokiPatchRequest __init(int _i, ByteBuffer _bb) { bb_pos = _i; bb = _bb; return this; }


  public static void startLokiPatchRequest(FlatBufferBuilder builder) { builder.startObject(2); }
  public static int endLokiPatchRequest(FlatBufferBuilder builder) {
    int o = builder.endObject();
    return o;
  }
};

