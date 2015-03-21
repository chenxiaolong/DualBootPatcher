// automatically generated, do not modify

package mbtool.daemon.v2;

import java.nio.*;
import java.lang.*;
import java.util.*;
import com.google.flatbuffers.*;

public class SetKernelResponse extends Table {
  public static SetKernelResponse getRootAsSetKernelResponse(ByteBuffer _bb) { _bb.order(ByteOrder.LITTLE_ENDIAN); return (new SetKernelResponse()).__init(_bb.getInt(_bb.position()) + _bb.position(), _bb); }
  public SetKernelResponse __init(int _i, ByteBuffer _bb) { bb_pos = _i; bb = _bb; return this; }

  public byte success() { int o = __offset(4); return o != 0 ? bb.get(o + bb_pos) : 0; }

  public static int createSetKernelResponse(FlatBufferBuilder builder,
      byte success) {
    builder.startObject(1);
    SetKernelResponse.addSuccess(builder, success);
    return SetKernelResponse.endSetKernelResponse(builder);
  }

  public static void startSetKernelResponse(FlatBufferBuilder builder) { builder.startObject(1); }
  public static void addSuccess(FlatBufferBuilder builder, byte success) { builder.addByte(0, success, 0); }
  public static int endSetKernelResponse(FlatBufferBuilder builder) {
    int o = builder.endObject();
    return o;
  }
};

