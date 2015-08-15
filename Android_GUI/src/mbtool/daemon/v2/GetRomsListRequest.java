// automatically generated, do not modify

package mbtool.daemon.v2;

import java.nio.*;
import java.lang.*;
import java.util.*;
import com.google.flatbuffers.*;

public class GetRomsListRequest extends Table {
  public static GetRomsListRequest getRootAsGetRomsListRequest(ByteBuffer _bb) { return getRootAsGetRomsListRequest(_bb, new GetRomsListRequest()); }
  public static GetRomsListRequest getRootAsGetRomsListRequest(ByteBuffer _bb, GetRomsListRequest obj) { _bb.order(ByteOrder.LITTLE_ENDIAN); return (obj.__init(_bb.getInt(_bb.position()) + _bb.position(), _bb)); }
  public GetRomsListRequest __init(int _i, ByteBuffer _bb) { bb_pos = _i; bb = _bb; return this; }


  public static void startGetRomsListRequest(FlatBufferBuilder builder) { builder.startObject(0); }
  public static int endGetRomsListRequest(FlatBufferBuilder builder) {
    int o = builder.endObject();
    return o;
  }
};

