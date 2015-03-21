// automatically generated, do not modify

package mbtool.daemon.v2;

import java.nio.*;
import java.lang.*;
import java.util.*;
import com.google.flatbuffers.*;

public class SwitchRomResponse extends Table {
  public static SwitchRomResponse getRootAsSwitchRomResponse(ByteBuffer _bb) { _bb.order(ByteOrder.LITTLE_ENDIAN); return (new SwitchRomResponse()).__init(_bb.getInt(_bb.position()) + _bb.position(), _bb); }
  public SwitchRomResponse __init(int _i, ByteBuffer _bb) { bb_pos = _i; bb = _bb; return this; }

  public byte success() { int o = __offset(4); return o != 0 ? bb.get(o + bb_pos) : 0; }

  public static int createSwitchRomResponse(FlatBufferBuilder builder,
      byte success) {
    builder.startObject(1);
    SwitchRomResponse.addSuccess(builder, success);
    return SwitchRomResponse.endSwitchRomResponse(builder);
  }

  public static void startSwitchRomResponse(FlatBufferBuilder builder) { builder.startObject(1); }
  public static void addSuccess(FlatBufferBuilder builder, byte success) { builder.addByte(0, success, 0); }
  public static int endSwitchRomResponse(FlatBufferBuilder builder) {
    int o = builder.endObject();
    return o;
  }
};

