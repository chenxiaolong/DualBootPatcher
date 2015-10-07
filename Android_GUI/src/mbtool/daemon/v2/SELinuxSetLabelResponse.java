// automatically generated, do not modify

package mbtool.daemon.v2;

import java.nio.*;
import java.lang.*;
import java.util.*;
import com.google.flatbuffers.*;

public class SELinuxSetLabelResponse extends Table {
  public static SELinuxSetLabelResponse getRootAsSELinuxSetLabelResponse(ByteBuffer _bb) { return getRootAsSELinuxSetLabelResponse(_bb, new SELinuxSetLabelResponse()); }
  public static SELinuxSetLabelResponse getRootAsSELinuxSetLabelResponse(ByteBuffer _bb, SELinuxSetLabelResponse obj) { _bb.order(ByteOrder.LITTLE_ENDIAN); return (obj.__init(_bb.getInt(_bb.position()) + _bb.position(), _bb)); }
  public SELinuxSetLabelResponse __init(int _i, ByteBuffer _bb) { bb_pos = _i; bb = _bb; return this; }

  public boolean success() { int o = __offset(4); return o != 0 ? 0!=bb.get(o + bb_pos) : false; }
  public String errorMsg() { int o = __offset(6); return o != 0 ? __string(o + bb_pos) : null; }
  public ByteBuffer errorMsgAsByteBuffer() { return __vector_as_bytebuffer(6, 1); }

  public static int createSELinuxSetLabelResponse(FlatBufferBuilder builder,
      boolean success,
      int error_msg) {
    builder.startObject(2);
    SELinuxSetLabelResponse.addErrorMsg(builder, error_msg);
    SELinuxSetLabelResponse.addSuccess(builder, success);
    return SELinuxSetLabelResponse.endSELinuxSetLabelResponse(builder);
  }

  public static void startSELinuxSetLabelResponse(FlatBufferBuilder builder) { builder.startObject(2); }
  public static void addSuccess(FlatBufferBuilder builder, boolean success) { builder.addBoolean(0, success, false); }
  public static void addErrorMsg(FlatBufferBuilder builder, int errorMsgOffset) { builder.addOffset(1, errorMsgOffset, 0); }
  public static int endSELinuxSetLabelResponse(FlatBufferBuilder builder) {
    int o = builder.endObject();
    return o;
  }
};

