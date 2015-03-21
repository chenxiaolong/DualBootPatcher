// automatically generated, do not modify

package mbtool.daemon.v2;

import java.nio.*;
import java.lang.*;
import java.util.*;
import com.google.flatbuffers.*;

public class GetVersionResponse extends Table {
  public static GetVersionResponse getRootAsGetVersionResponse(ByteBuffer _bb) { _bb.order(ByteOrder.LITTLE_ENDIAN); return (new GetVersionResponse()).__init(_bb.getInt(_bb.position()) + _bb.position(), _bb); }
  public GetVersionResponse __init(int _i, ByteBuffer _bb) { bb_pos = _i; bb = _bb; return this; }

  public String version() { int o = __offset(4); return o != 0 ? __string(o + bb_pos) : null; }
  public ByteBuffer versionAsByteBuffer() { return __vector_as_bytebuffer(4, 1); }

  public static int createGetVersionResponse(FlatBufferBuilder builder,
      int version) {
    builder.startObject(1);
    GetVersionResponse.addVersion(builder, version);
    return GetVersionResponse.endGetVersionResponse(builder);
  }

  public static void startGetVersionResponse(FlatBufferBuilder builder) { builder.startObject(1); }
  public static void addVersion(FlatBufferBuilder builder, int versionOffset) { builder.addOffset(0, versionOffset, 0); }
  public static int endGetVersionResponse(FlatBufferBuilder builder) {
    int o = builder.endObject();
    return o;
  }
};

