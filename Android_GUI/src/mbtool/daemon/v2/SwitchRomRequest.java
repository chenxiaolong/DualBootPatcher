// automatically generated, do not modify

package mbtool.daemon.v2;

import java.nio.*;
import java.lang.*;
import java.util.*;
import com.google.flatbuffers.*;

public class SwitchRomRequest extends Table {
  public static SwitchRomRequest getRootAsSwitchRomRequest(ByteBuffer _bb) { _bb.order(ByteOrder.LITTLE_ENDIAN); return (new SwitchRomRequest()).__init(_bb.getInt(_bb.position()) + _bb.position(), _bb); }
  public SwitchRomRequest __init(int _i, ByteBuffer _bb) { bb_pos = _i; bb = _bb; return this; }

  public String romId() { int o = __offset(4); return o != 0 ? __string(o + bb_pos) : null; }
  public ByteBuffer romIdAsByteBuffer() { return __vector_as_bytebuffer(4, 1); }
  public String bootBlockdev() { int o = __offset(6); return o != 0 ? __string(o + bb_pos) : null; }
  public ByteBuffer bootBlockdevAsByteBuffer() { return __vector_as_bytebuffer(6, 1); }

  public static int createSwitchRomRequest(FlatBufferBuilder builder,
      int rom_id,
      int boot_blockdev) {
    builder.startObject(2);
    SwitchRomRequest.addBootBlockdev(builder, boot_blockdev);
    SwitchRomRequest.addRomId(builder, rom_id);
    return SwitchRomRequest.endSwitchRomRequest(builder);
  }

  public static void startSwitchRomRequest(FlatBufferBuilder builder) { builder.startObject(2); }
  public static void addRomId(FlatBufferBuilder builder, int romIdOffset) { builder.addOffset(0, romIdOffset, 0); }
  public static void addBootBlockdev(FlatBufferBuilder builder, int bootBlockdevOffset) { builder.addOffset(1, bootBlockdevOffset, 0); }
  public static int endSwitchRomRequest(FlatBufferBuilder builder) {
    int o = builder.endObject();
    return o;
  }
};

