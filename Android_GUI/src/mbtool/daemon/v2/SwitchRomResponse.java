// automatically generated, do not modify

package mbtool.daemon.v2;

import java.nio.*;
import java.lang.*;
import java.util.*;
import com.google.flatbuffers.*;

public class SwitchRomResponse extends Table {
  public static SwitchRomResponse getRootAsSwitchRomResponse(ByteBuffer _bb) { return getRootAsSwitchRomResponse(_bb, new SwitchRomResponse()); }
  public static SwitchRomResponse getRootAsSwitchRomResponse(ByteBuffer _bb, SwitchRomResponse obj) { _bb.order(ByteOrder.LITTLE_ENDIAN); return (obj.__init(_bb.getInt(_bb.position()) + _bb.position(), _bb)); }
  public SwitchRomResponse __init(int _i, ByteBuffer _bb) { bb_pos = _i; bb = _bb; return this; }

  public boolean success() { int o = __offset(4); return o != 0 ? 0!=bb.get(o + bb_pos) : false; }
  public short result() { int o = __offset(6); return o != 0 ? bb.getShort(o + bb_pos) : 0; }

  public static int createSwitchRomResponse(FlatBufferBuilder builder,
      boolean success,
      short result) {
    builder.startObject(2);
    SwitchRomResponse.addResult(builder, result);
    SwitchRomResponse.addSuccess(builder, success);
    return SwitchRomResponse.endSwitchRomResponse(builder);
  }

  public static void startSwitchRomResponse(FlatBufferBuilder builder) { builder.startObject(2); }
  public static void addSuccess(FlatBufferBuilder builder, boolean success) { builder.addBoolean(0, success, false); }
  public static void addResult(FlatBufferBuilder builder, short result) { builder.addShort(1, result, 0); }
  public static int endSwitchRomResponse(FlatBufferBuilder builder) {
    int o = builder.endObject();
    return o;
  }
};

