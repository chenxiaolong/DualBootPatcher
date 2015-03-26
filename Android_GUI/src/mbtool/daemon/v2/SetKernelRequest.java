// automatically generated, do not modify

package mbtool.daemon.v2;

import java.nio.*;
import java.lang.*;
import java.util.*;
import com.google.flatbuffers.*;

public class SetKernelRequest extends Table {
  public static SetKernelRequest getRootAsSetKernelRequest(ByteBuffer _bb) { _bb.order(ByteOrder.LITTLE_ENDIAN); return (new SetKernelRequest()).__init(_bb.getInt(_bb.position()) + _bb.position(), _bb); }
  public SetKernelRequest __init(int _i, ByteBuffer _bb) { bb_pos = _i; bb = _bb; return this; }

  public String romId() { int o = __offset(4); return o != 0 ? __string(o + bb_pos) : null; }
  public ByteBuffer romIdAsByteBuffer() { return __vector_as_bytebuffer(4, 1); }
  public String bootBlockdev() { int o = __offset(6); return o != 0 ? __string(o + bb_pos) : null; }
  public ByteBuffer bootBlockdevAsByteBuffer() { return __vector_as_bytebuffer(6, 1); }

  public static int createSetKernelRequest(FlatBufferBuilder builder,
      int rom_id,
      int boot_blockdev) {
    builder.startObject(2);
    SetKernelRequest.addBootBlockdev(builder, boot_blockdev);
    SetKernelRequest.addRomId(builder, rom_id);
    return SetKernelRequest.endSetKernelRequest(builder);
  }

  public static void startSetKernelRequest(FlatBufferBuilder builder) { builder.startObject(2); }
  public static void addRomId(FlatBufferBuilder builder, int romIdOffset) { builder.addOffset(0, romIdOffset, 0); }
  public static void addBootBlockdev(FlatBufferBuilder builder, int bootBlockdevOffset) { builder.addOffset(1, bootBlockdevOffset, 0); }
  public static int endSetKernelRequest(FlatBufferBuilder builder) {
    int o = builder.endObject();
    return o;
  }
};

