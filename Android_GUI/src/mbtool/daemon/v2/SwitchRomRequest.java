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
  public String blockdevBaseDirs(int j) { int o = __offset(8); return o != 0 ? __string(__vector(o) + j * 4) : null; }
  public int blockdevBaseDirsLength() { int o = __offset(8); return o != 0 ? __vector_len(o) : 0; }
  public ByteBuffer blockdevBaseDirsAsByteBuffer() { return __vector_as_bytebuffer(8, 4); }

  public static int createSwitchRomRequest(FlatBufferBuilder builder,
      int rom_id,
      int boot_blockdev,
      int blockdev_base_dirs) {
    builder.startObject(3);
    SwitchRomRequest.addBlockdevBaseDirs(builder, blockdev_base_dirs);
    SwitchRomRequest.addBootBlockdev(builder, boot_blockdev);
    SwitchRomRequest.addRomId(builder, rom_id);
    return SwitchRomRequest.endSwitchRomRequest(builder);
  }

  public static void startSwitchRomRequest(FlatBufferBuilder builder) { builder.startObject(3); }
  public static void addRomId(FlatBufferBuilder builder, int romIdOffset) { builder.addOffset(0, romIdOffset, 0); }
  public static void addBootBlockdev(FlatBufferBuilder builder, int bootBlockdevOffset) { builder.addOffset(1, bootBlockdevOffset, 0); }
  public static void addBlockdevBaseDirs(FlatBufferBuilder builder, int blockdevBaseDirsOffset) { builder.addOffset(2, blockdevBaseDirsOffset, 0); }
  public static int createBlockdevBaseDirsVector(FlatBufferBuilder builder, int[] data) { builder.startVector(4, data.length, 4); for (int i = data.length - 1; i >= 0; i--) builder.addOffset(data[i]); return builder.endVector(); }
  public static void startBlockdevBaseDirsVector(FlatBufferBuilder builder, int numElems) { builder.startVector(4, numElems, 4); }
  public static int endSwitchRomRequest(FlatBufferBuilder builder) {
    int o = builder.endObject();
    return o;
  }
};

