// automatically generated, do not modify

package mbtool.daemon.v2;

import java.nio.*;
import java.lang.*;
import java.util.*;
import com.google.flatbuffers.*;

public class GetVersionRequest extends Table {
  public static GetVersionRequest getRootAsGetVersionRequest(ByteBuffer _bb) { return getRootAsGetVersionRequest(_bb, new GetVersionRequest()); }
  public static GetVersionRequest getRootAsGetVersionRequest(ByteBuffer _bb, GetVersionRequest obj) { _bb.order(ByteOrder.LITTLE_ENDIAN); return (obj.__init(_bb.getInt(_bb.position()) + _bb.position(), _bb)); }
  public GetVersionRequest __init(int _i, ByteBuffer _bb) { bb_pos = _i; bb = _bb; return this; }


  public static void startGetVersionRequest(FlatBufferBuilder builder) { builder.startObject(0); }
  public static int endGetVersionRequest(FlatBufferBuilder builder) {
    int o = builder.endObject();
    return o;
  }
};

