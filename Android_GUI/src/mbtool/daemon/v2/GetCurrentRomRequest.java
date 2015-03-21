// automatically generated, do not modify

package mbtool.daemon.v2;

import java.nio.*;
import java.lang.*;
import java.util.*;
import com.google.flatbuffers.*;

public class GetCurrentRomRequest extends Table {
  public static GetCurrentRomRequest getRootAsGetCurrentRomRequest(ByteBuffer _bb) { _bb.order(ByteOrder.LITTLE_ENDIAN); return (new GetCurrentRomRequest()).__init(_bb.getInt(_bb.position()) + _bb.position(), _bb); }
  public GetCurrentRomRequest __init(int _i, ByteBuffer _bb) { bb_pos = _i; bb = _bb; return this; }


  public static void startGetCurrentRomRequest(FlatBufferBuilder builder) { builder.startObject(0); }
  public static int endGetCurrentRomRequest(FlatBufferBuilder builder) {
    int o = builder.endObject();
    return o;
  }
};

